#include "StemTab.h"
#include "ModernLookAndFeel.h"

namespace sp
{
    StemTab::StemTab (Backend& b) : backend (b)
    {
        addAndMakeVisible (inputField);
        inputField.setTextToShowWhenEmpty ("Drop a file, paste a path, or paste a YouTube URL",
                                           juce::Colours::darkgrey);
        inputField.setMultiLine (false);
        inputField.setReturnKeyStartsNewLine (false);

        // Strip YouTube tracking/playlist params (&list=, &t=, &pp=, etc.) so
        // yt-dlp doesn't grab the whole playlist or start mid-track.
        inputField.onTextChange = [this]
        {
            const auto t = inputField.getText();
            if (! (t.containsIgnoreCase ("youtube.com") || t.containsIgnoreCase ("youtu.be")))
                return;
            const auto amp = t.indexOfChar ('&');
            if (amp < 0) return;
            inputField.setText (t.substring (0, amp), juce::dontSendNotification);
        };

        addAndMakeVisible (browseButton);
        browseButton.onClick = [this] { doBrowseInput(); };

        outputDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                        .getChildFile ("SingingPracticeTool/Stems");
        outputDir.createDirectory();

        addAndMakeVisible (outputBrowseBtn);
        outputBrowseBtn.onClick = [this] { doBrowseOutput(); };
        addAndMakeVisible (outputLabel);
        outputLabel.setText ("Output: " + outputDir.getFullPathName(), juce::dontSendNotification);
        outputLabel.setColour (juce::Label::textColourId, juce::Colour (ModernLookAndFeel::colTextSecondary));

        addAndMakeVisible (modelCombo);
        modelCombo.addItem ("htdemucs (default, fast)",       1);
        modelCombo.addItem ("htdemucs_ft (higher quality)",   2);
        modelCombo.addItem ("mdx_extra (alternative)",        3);
        modelCombo.setSelectedId (1, juce::dontSendNotification);

        addAndMakeVisible (deviceCombo);
        deviceCombo.addItem ("Auto (CUDA if available)", 1);
        deviceCombo.addItem ("CUDA (GPU)",               2);
        deviceCombo.addItem ("CPU",                      3);
        deviceCombo.setSelectedId (1, juce::dontSendNotification);

        addAndMakeVisible (vocalsOnlyToggle);

        addAndMakeVisible (runButton);
        runButton.onClick = [this] { doRun(); };
        addAndMakeVisible (cancelButton);
        cancelButton.onClick = [this] { doCancel(); };
        cancelButton.setEnabled (false);

        addAndMakeVisible (openInPracticeBtn);
        openInPracticeBtn.setEnabled (false);
        openInPracticeBtn.onClick = [this] { doOpenInPractice(); };

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

        backend.onStatus     = [this] (const juce::String& s) { appendLog ("[status] " + s); };
        backend.onStderrLine = [this] (const juce::String& s) { appendLog (s); };
        backend.onExit       = [this] (int code)
        {
            appendLog ("[exit] code " + juce::String (code));
            setRunning (false);
        };
    }

    StemTab::~StemTab()
    {
        if (currentJobId >= 0) backend.cancel (currentJobId);
        backend.onStatus     = nullptr;
        backend.onStderrLine = nullptr;
        backend.onExit       = nullptr;
    }

    void StemTab::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (ModernLookAndFeel::colSurface));
    }

    void StemTab::resized()
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

        r.removeFromTop (6);
        auto optsRow = r.removeFromTop (28);
        modelCombo.setBounds (optsRow.removeFromLeft (260));
        optsRow.removeFromLeft (12);
        deviceCombo.setBounds (optsRow.removeFromLeft (200));
        optsRow.removeFromLeft (12);
        vocalsOnlyToggle.setBounds (optsRow.removeFromLeft (220));

        r.removeFromTop (6);
        auto actionRow = r.removeFromTop (32);
        runButton   .setBounds (actionRow.removeFromLeft (100));
        actionRow.removeFromLeft (6);
        cancelButton.setBounds (actionRow.removeFromLeft (90));
        actionRow.removeFromLeft (16);
        openInPracticeBtn.setBounds (actionRow.removeFromLeft (260));

        r.removeFromTop (8);
        progressBar.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        statusLabel.setBounds (r.removeFromTop (20));

        r.removeFromTop (8);
        logView.setBounds (r);
    }

    bool StemTab::isInterestedInFileDrag (const juce::StringArray& files)
    {
        return ! files.isEmpty();
    }

    void StemTab::filesDropped (const juce::StringArray& files, int, int)
    {
        if (files.isEmpty()) return;
        inputField.setText (files[0], juce::dontSendNotification);
    }

    bool StemTab::inputLooksLikeUrl() const
    {
        const auto t = inputField.getText().trim();
        return t.startsWithIgnoreCase ("http://")
            || t.startsWithIgnoreCase ("https://")
            || t.startsWithIgnoreCase ("www.")
            || t.containsIgnoreCase ("youtube.com")
            || t.containsIgnoreCase ("youtu.be");
    }

    juce::File StemTab::inputAsFile() const
    {
        return juce::File (inputField.getText().trim().unquoted());
    }

    void StemTab::doBrowseInput()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Pick an audio file to separate", juce::File(),
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

    void StemTab::doBrowseOutput()
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

    void StemTab::doRun()
    {
        if (currentJobId >= 0) return;
        appendLog ("\n=== Run ===");

        const juce::String modelName = [&]
        {
            switch (modelCombo.getSelectedId())
            {
                case 2:  return "htdemucs_ft";
                case 3:  return "mdx_extra";
                default: return "htdemucs";
            }
        }();

        const juce::String device = [&]
        {
            switch (deviceCombo.getSelectedId())
            {
                case 2:  return "cuda";
                case 3:  return "cpu";
                default: return "auto";
            }
        }();

        const bool vocalsOnly = vocalsOnlyToggle.getToggleState();
        lastInstrumentalOutput = juce::File();
        openInPracticeBtn.setEnabled (false);

        Backend::JobHandle h;
        h.onProgress = [this] (float frac, const juce::String& text) { onProgress (frac, text); };
        h.onComplete = [this] (bool ok, const juce::var& r)          { onComplete (ok, r); };

        if (inputLooksLikeUrl())
        {
            statusLabel.setText ("downloading...", juce::dontSendNotification);
            progress = 0.0;

            const juce::String url = inputField.getText().trim();

            // Two-phase: download to outputDir, then separate the resulting WAV.
            Backend::JobHandle dlHandlers;
            dlHandlers.onProgress = [this] (float f, const juce::String& t) { onProgress (f, t); };
            dlHandlers.onComplete = [this, modelName, vocalsOnly, device, h] (bool ok, const juce::var& res) mutable
            {
                if (! ok)
                {
                    appendLog ("download failed: " + juce::JSON::toString (res));
                    setRunning (false);
                    return;
                }
                const juce::String outPath = res["output_path"].toString();
                appendLog ("downloaded: " + outPath);
                statusLabel.setText ("separating...", juce::dontSendNotification);
                progress = 0.0;
                currentJobId = backend.separateStems (juce::File (outPath), outputDir,
                                                     modelName, vocalsOnly, device, h);
            };
            currentJobId = backend.youtubeDownload (url, outputDir, "wav", dlHandlers);
            setRunning (true);
            return;
        }

        const auto in = inputAsFile();
        if (! in.existsAsFile())
        {
            statusLabel.setText ("file not found: " + in.getFullPathName(), juce::dontSendNotification);
            return;
        }

        statusLabel.setText ("separating...", juce::dontSendNotification);
        progress = 0.0;
        currentJobId = backend.separateStems (in, outputDir, modelName, vocalsOnly, device, h);
        setRunning (true);
    }

    void StemTab::doCancel()
    {
        if (currentJobId < 0) return;
        backend.cancel (currentJobId);
        appendLog ("[cancel] requested for id " + juce::String (currentJobId));
    }

    void StemTab::doOpenInPractice()
    {
        // Practice tab isn't directly accessible from here without a callback wire.
        // For now, surface the path so the user can drop it manually; route can be
        // added later via a Backend-style mediator.
        if (lastInstrumentalOutput.existsAsFile())
            lastInstrumentalOutput.revealToUser();
    }

    void StemTab::onProgress (float frac, const juce::String& text)
    {
        progress = juce::jlimit (0.0, 1.0, (double) frac);
        statusLabel.setText (text + "  (" + juce::String ((int) (progress * 100)) + "%)",
                             juce::dontSendNotification);
    }

    void StemTab::onComplete (bool ok, const juce::var& result)
    {
        currentJobId = -1;
        setRunning (false);

        if (! ok)
        {
            statusLabel.setText ("failed: " + juce::JSON::toString (result), juce::dontSendNotification);
            return;
        }

        progress = 1.0;
        statusLabel.setText ("done", juce::dontSendNotification);
        appendLog ("[done] " + juce::JSON::toString (result));

        // Pick the instrumental for "open in practice".
        if (result.isObject() && result["stems"].isObject())
        {
            const auto stems = result["stems"];
            for (const auto& key : { "no_vocals", "accompaniment", "other" })
            {
                const auto path = stems[juce::Identifier (key)].toString();
                if (path.isNotEmpty()) { lastInstrumentalOutput = juce::File (path); break; }
            }
            openInPracticeBtn.setEnabled (lastInstrumentalOutput.existsAsFile());
        }
    }

    void StemTab::appendLog (const juce::String& s)
    {
        if (s.isEmpty()) return;
        logView.moveCaretToEnd();
        logView.insertTextAtCaret (s + "\n");
    }

    void StemTab::setRunning (bool running)
    {
        runButton.setEnabled (! running);
        cancelButton.setEnabled (running);
        inputField.setEnabled (! running);
        browseButton.setEnabled (! running);
        outputBrowseBtn.setEnabled (! running);
        modelCombo.setEnabled (! running);
        deviceCombo.setEnabled (! running);
        vocalsOnlyToggle.setEnabled (! running);
    }
}
