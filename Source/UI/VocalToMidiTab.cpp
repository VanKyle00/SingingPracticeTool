#include "VocalToMidiTab.h"
#include "ModernLookAndFeel.h"

namespace sp
{
    VocalToMidiTab::VocalToMidiTab (Backend& b) : backend (b)
    {
        addAndMakeVisible (inputField);
        inputField.setTextToShowWhenEmpty ("Drop a vocal WAV/MP3/FLAC, or paste a path",
                                           juce::Colours::darkgrey);
        inputField.setMultiLine (false);
        inputField.setReturnKeyStartsNewLine (false);

        addAndMakeVisible (browseButton);
        browseButton.onClick = [this] { doBrowseInput(); };

        outputDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                        .getChildFile ("SingingPracticeTool/MIDI");
        outputDir.createDirectory();

        addAndMakeVisible (outputBrowseBtn);
        outputBrowseBtn.onClick = [this] { doBrowseOutput(); };
        addAndMakeVisible (outputLabel);
        outputLabel.setText ("Output: " + outputDir.getFullPathName(), juce::dontSendNotification);
        outputLabel.setColour (juce::Label::textColourId, juce::Colour (ModernLookAndFeel::colTextSecondary));

        // Threshold sliders — defaults match basic-pitch's library defaults.
        auto setupSlider = [this] (juce::Slider& s, juce::Label& l, double minV, double maxV,
                                   double step, double value)
        {
            addAndMakeVisible (l);
            l.setColour (juce::Label::textColourId, juce::Colour (ModernLookAndFeel::colTextSecondary));
            addAndMakeVisible (s);
            s.setRange (minV, maxV, step);
            s.setValue (value, juce::dontSendNotification);
            s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 18);
            s.setSliderStyle (juce::Slider::LinearHorizontal);
        };
        setupSlider (onsetSlider,  onsetLabel,  0.05, 0.95, 0.01,   0.50);
        setupSlider (frameSlider,  frameLabel,  0.05, 0.95, 0.01,   0.30);
        setupSlider (minLenSlider, minLenLabel, 30.0, 500.0, 1.0, 127.7);

        addAndMakeVisible (runButton);
        runButton.onClick = [this] { doRun(); };
        addAndMakeVisible (cancelButton);
        cancelButton.onClick = [this] { doCancel(); };
        cancelButton.setEnabled (false);

        addAndMakeVisible (revealButton);
        revealButton.setEnabled (false);
        revealButton.onClick = [this] { doReveal(); };

        addAndMakeVisible (progressBar);
        addAndMakeVisible (statusLabel);
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (ModernLookAndFeel::colTextSecondary));

        addAndMakeVisible (logView);
        logView.setMultiLine (true);
        logView.setReadOnly (true);
        logView.setScrollbarsShown (true);
        logView.setCaretVisible (false);
        logView.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                                        12.0f, juce::Font::plain)));
        logView.setColour (juce::TextEditor::backgroundColourId, juce::Colour (ModernLookAndFeel::colBackground));
        logView.setColour (juce::TextEditor::textColourId, juce::Colour (ModernLookAndFeel::colTextSecondary));
    }

    VocalToMidiTab::~VocalToMidiTab()
    {
        if (currentJobId >= 0) backend.cancel (currentJobId);
    }

    void VocalToMidiTab::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (ModernLookAndFeel::colSurface));
    }

    void VocalToMidiTab::resized()
    {
        auto r = getLocalBounds().reduced (12);

        auto inputRow = r.removeFromTop (28);
        browseButton.setBounds (inputRow.removeFromRight (96));
        inputRow.removeFromRight (4);
        inputField.setBounds (inputRow);

        r.removeFromTop (4);
        auto outRow = r.removeFromTop (24);
        outputBrowseBtn.setBounds (outRow.removeFromLeft (96));
        outRow.removeFromLeft (8);
        outputLabel.setBounds (outRow);

        // Three slider rows: label on the left, slider+textbox stretches.
        auto sliderRow = [&] (juce::Label& l, juce::Slider& s)
        {
            r.removeFromTop (6);
            auto row = r.removeFromTop (22);
            l.setBounds (row.removeFromLeft (160));
            s.setBounds (row);
        };
        sliderRow (onsetLabel,  onsetSlider);
        sliderRow (frameLabel,  frameSlider);
        sliderRow (minLenLabel, minLenSlider);

        r.removeFromTop (8);
        auto actionRow = r.removeFromTop (32);
        runButton   .setBounds (actionRow.removeFromLeft (110));
        actionRow.removeFromLeft (6);
        cancelButton.setBounds (actionRow.removeFromLeft (90));
        actionRow.removeFromLeft (16);
        revealButton.setBounds (actionRow.removeFromLeft (160));

        r.removeFromTop (8);
        progressBar.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        statusLabel.setBounds (r.removeFromTop (20));

        r.removeFromTop (8);
        logView.setBounds (r);
    }

    bool VocalToMidiTab::isInterestedInFileDrag (const juce::StringArray& files)
    {
        return ! files.isEmpty();
    }

    void VocalToMidiTab::filesDropped (const juce::StringArray& files, int, int)
    {
        if (files.isEmpty()) return;
        inputField.setText (files[0], juce::dontSendNotification);
    }

    void VocalToMidiTab::doBrowseInput()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Pick a vocal audio file", juce::File(),
            "*.wav;*.mp3;*.flac;*.ogg;*.m4a;*.aif;*.aiff");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f != juce::File())
                    inputField.setText (f.getFullPathName(), juce::dontSendNotification);
            });
    }

    void VocalToMidiTab::doBrowseOutput()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Choose output folder", outputDir, juce::String());
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f.isDirectory())
                {
                    outputDir = f;
                    outputLabel.setText ("Output: " + outputDir.getFullPathName(),
                                         juce::dontSendNotification);
                }
            });
    }

    void VocalToMidiTab::doRun()
    {
        if (currentJobId >= 0) return;

        const auto in = juce::File (inputField.getText().trim().unquoted());
        if (! in.existsAsFile())
        {
            statusLabel.setText ("file not found: " + in.getFullPathName(), juce::dontSendNotification);
            return;
        }

        appendLog ("\n=== Transcribe ===");
        appendLog ("input: " + in.getFullPathName());

        lastMidiOutput = juce::File();
        revealButton.setEnabled (false);

        Backend::JobHandle h;
        h.onProgress = [this] (float f, const juce::String& t) { onProgress (f, t); };
        h.onComplete = [this] (bool ok, const juce::var& r)    { onComplete (ok, r); };

        progress = 0.0;
        statusLabel.setText ("starting...", juce::dontSendNotification);
        currentJobId = backend.vocalToMidi (in, outputDir,
                                           (float) onsetSlider.getValue(),
                                           (float) frameSlider.getValue(),
                                           (float) minLenSlider.getValue(),
                                           h);
        setRunning (true);
    }

    void VocalToMidiTab::doCancel()
    {
        if (currentJobId < 0) return;
        backend.cancel (currentJobId);
        appendLog ("[cancel] requested for id " + juce::String (currentJobId));
    }

    void VocalToMidiTab::doReveal()
    {
        if (lastMidiOutput.existsAsFile())
            lastMidiOutput.revealToUser();
    }

    void VocalToMidiTab::onProgress (float frac, const juce::String& text)
    {
        progress = juce::jlimit (0.0, 1.0, (double) frac);
        statusLabel.setText (text + "  (" + juce::String ((int) (progress * 100)) + "%)",
                             juce::dontSendNotification);
    }

    void VocalToMidiTab::onComplete (bool ok, const juce::var& result)
    {
        currentJobId = -1;
        setRunning (false);

        if (! ok)
        {
            statusLabel.setText ("failed: " + juce::JSON::toString (result), juce::dontSendNotification);
            appendLog ("[error] " + juce::JSON::toString (result));
            return;
        }

        progress = 1.0;

        const auto outPath = result["output"].toString();
        const int  noteCount = (int) result["note_count"];
        const auto dur     = (double) result["duration_s"];

        lastMidiOutput = juce::File (outPath);
        revealButton.setEnabled (lastMidiOutput.existsAsFile());

        statusLabel.setText ("done — " + juce::String (noteCount) + " notes, "
                             + juce::String (dur, 1) + "s", juce::dontSendNotification);
        appendLog ("[done] " + outPath);
    }

    void VocalToMidiTab::appendLog (const juce::String& s)
    {
        if (s.isEmpty()) return;
        logView.moveCaretToEnd();
        logView.insertTextAtCaret (s + "\n");
    }

    void VocalToMidiTab::setRunning (bool running)
    {
        runButton.setEnabled (! running);
        cancelButton.setEnabled (running);
        inputField.setEnabled (! running);
        browseButton.setEnabled (! running);
        outputBrowseBtn.setEnabled (! running);
        onsetSlider.setEnabled (! running);
        frameSlider.setEnabled (! running);
        minLenSlider.setEnabled (! running);
    }
}
