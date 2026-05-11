#include "PracticeTab.h"
#include "../Audio/Engine.h"
#include "PluginChainView.h"
#include "ModernLookAndFeel.h"

namespace sp
{
    PracticeTab::PracticeTab (Engine& e)
        : engine (e)
    {
        addAndMakeVisible (pianoRoll);
        pianoRoll.setPitchRange (36, 84);
        pianoRoll.setWindowSeconds (1.0, 4.0);

        addAndMakeVisible (scrubber);
        scrubber.onSeek = [this] (double seconds) { engine.setPlaybackPositionSeconds (seconds); };

        for (auto* b : { &playButton, &stopButton, &loadAudioBtn, &loadMidiBtn, &recordButton })
            addAndMakeVisible (*b);

        playButton.onClick   = [this]
        {
            if (! engine.hasInstrumental())
            {
                statusLabel.setText ("Load an instrumental first (button or drag a file).",
                                     juce::dontSendNotification);
                return;
            }
            engine.play();
        };
        stopButton.onClick   = [this] { engine.stop(); };
        loadAudioBtn.onClick = [this] { loadAudioDialog(); };
        loadMidiBtn.onClick  = [this] { loadMidiDialog(); };
        recordButton.onClick = [this] { toggleRecord(); };

        for (auto* s : { &vocalGainSlider, &instGainSlider })
        {
            addAndMakeVisible (*s);
            s->setSliderStyle (juce::Slider::LinearHorizontal);
            s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 18);
            s->setRange (0.0, 1.0, 0.01);
        }
        vocalGainSlider.setValue (engine.getVocalMonitorGain(), juce::dontSendNotification);
        instGainSlider .setValue (engine.getInstrumentalGain(), juce::dontSendNotification);
        vocalGainSlider.onValueChange = [this] { engine.setVocalMonitorGain ((float) vocalGainSlider.getValue()); };
        instGainSlider .onValueChange = [this] { engine.setInstrumentalGain ((float) instGainSlider.getValue()); };

        addAndMakeVisible (vocalGainLabel);
        addAndMakeVisible (instGainLabel);
        vocalGainLabel.setJustificationType (juce::Justification::centredRight);
        instGainLabel .setJustificationType (juce::Justification::centredRight);

        addAndMakeVisible (inputChannelLabel);
        addAndMakeVisible (inputChannelCombo);
        inputChannelLabel.setJustificationType (juce::Justification::centredRight);
        inputChannelCombo.onChange = [this]
        {
            const int idx = inputChannelCombo.getSelectedItemIndex();
            if (idx >= 0) engine.setInputChannelIndex (idx);
        };

        engine.getDeviceManager().addChangeListener (this);
        refreshInputChannelOptions();

        chainView = std::make_unique<PluginChainView> (engine.getVocalChain());
        addAndMakeVisible (*chainView);

        addAndMakeVisible (statusLabel);
        statusLabel.setJustificationType (juce::Justification::centredLeft);
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (ModernLookAndFeel::colTextSecondary));

        recordingFolder = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                              .getChildFile ("SingingPracticeTool");
        recordingFolder.createDirectory();

        startTimerHz (60);
    }

    PracticeTab::~PracticeTab()
    {
        engine.getDeviceManager().removeChangeListener (this);
        stopTimer();
    }

    void PracticeTab::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (ModernLookAndFeel::colSurface));
    }

    void PracticeTab::resized()
    {
        auto r = getLocalBounds().reduced (8);

        auto top = r.removeFromTop (32);
        loadAudioBtn .setBounds (top.removeFromLeft (150));    top.removeFromLeft (4);
        loadMidiBtn  .setBounds (top.removeFromLeft (170));    top.removeFromLeft (12);
        playButton   .setBounds (top.removeFromLeft (60));     top.removeFromLeft (4);
        stopButton   .setBounds (top.removeFromLeft (60));     top.removeFromLeft (12);
        recordButton .setBounds (top.removeFromLeft (80));

        r.removeFromTop (6);

        auto mixers = r.removeFromTop (28);
        vocalGainLabel  .setBounds (mixers.removeFromLeft (110));
        vocalGainSlider .setBounds (mixers.removeFromLeft (280));
        mixers.removeFromLeft (16);
        instGainLabel   .setBounds (mixers.removeFromLeft (110));
        instGainSlider  .setBounds (mixers.removeFromLeft (280));
        mixers.removeFromLeft (16);
        inputChannelLabel.setBounds (mixers.removeFromLeft (80));
        inputChannelCombo.setBounds (mixers.removeFromLeft (240));

        r.removeFromTop (6);
        if (chainView != nullptr)
            chainView->setBounds (r.removeFromTop (34));

        r.removeFromTop (4);
        scrubber.setBounds (r.removeFromTop (32));

        r.removeFromTop (4);
        auto status = r.removeFromBottom (22);
        statusLabel.setBounds (status);

        pianoRoll.setBounds (r.reduced (0, 4));
    }

    bool PracticeTab::isInterestedInFileDrag (const juce::StringArray& files)
    {
        for (const auto& f : files)
            if (f.endsWithIgnoreCase (".wav") || f.endsWithIgnoreCase (".aif")
             || f.endsWithIgnoreCase (".aiff") || f.endsWithIgnoreCase (".flac")
             || f.endsWithIgnoreCase (".ogg")  || f.endsWithIgnoreCase (".mid")
             || f.endsWithIgnoreCase (".midi"))
                return true;
        return false;
    }

    void PracticeTab::filesDropped (const juce::StringArray& files, int, int)
    {
        for (const auto& path : files)
        {
            juce::File f (path);
            if (f.hasFileExtension ("mid;midi"))      loadMidiFile (f);
            else                                       loadAudioFile (f);
        }
    }

    void PracticeTab::loadAudioDialog()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load instrumental", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f.existsAsFile()) loadAudioFile (f);
            });
    }

    void PracticeTab::loadMidiDialog()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load reference MIDI", juce::File(), "*.mid;*.midi");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f.existsAsFile()) loadMidiFile (f);
            });
    }

    void PracticeTab::loadAudioFile (const juce::File& f)
    {
        if (! engine.loadInstrumental (f))
        {
            statusLabel.setText ("Failed to load: " + f.getFileName(), juce::dontSendNotification);
            return;
        }
        pianoRoll.clearPitchHistory();
        statusLabel.setText ("Loaded " + f.getFileName(), juce::dontSendNotification);
    }

    void PracticeTab::loadMidiFile (const juce::File& f)
    {
        juce::FileInputStream stream (f);
        if (! stream.openedOk()) return;

        juce::MidiFile midi;
        if (! midi.readFrom (stream))
        {
            statusLabel.setText ("Failed to parse MIDI: " + f.getFileName(), juce::dontSendNotification);
            return;
        }
        midi.convertTimestampTicksToSeconds();

        std::vector<PianoRoll::MidiNote> notes;
        for (int t = 0; t < midi.getNumTracks(); ++t)
        {
            const auto* track = midi.getTrack (t);
            if (track == nullptr) continue;

            for (int i = 0; i < track->getNumEvents(); ++i)
            {
                const auto* ev = track->getEventPointer (i);
                if (ev->message.isNoteOn())
                {
                    PianoRoll::MidiNote n;
                    n.startSec   = ev->message.getTimeStamp();
                    n.noteNumber = ev->message.getNoteNumber();
                    n.track      = t;
                    if (ev->noteOffObject != nullptr)
                        n.endSec = ev->noteOffObject->message.getTimeStamp();
                    else
                        n.endSec = n.startSec + 0.25;
                    notes.push_back (n);
                }
            }
        }
        pianoRoll.setMidiNotes (std::move (notes));
        statusLabel.setText ("MIDI loaded: " + f.getFileName(), juce::dontSendNotification);
    }

    void PracticeTab::toggleRecord()
    {
        if (isRecordingNow)
        {
            engine.stopRecording();
            isRecordingNow = false;
            recordButton.setButtonText ("Record");
        }
        else
        {
            engine.startRecording (recordingFolder);
            isRecordingNow = true;
            recordButton.setButtonText ("Stop Rec");
        }
    }

    void PracticeTab::timerCallback()
    {
        // Drain pitch ring buffer into piano roll history.
        PitchSample s;
        while (engine.getPitchDetector().readNext (s))
            pianoRoll.appendPitchSample (s);

        // Playhead runs on the engine wall clock so live pitch always lands at "now".
        const double engineTime = engine.getEngineTimeSeconds();
        pianoRoll.setPlayheadSeconds (engineTime);

        // MIDI is in transport-relative time; shift it so it tracks the transport
        // position within the engine timeline.
        pianoRoll.setMidiOffsetSeconds (engineTime - engine.getPlaybackPositionSeconds());

        scrubber.setDurationSeconds (engine.getInstrumentalLengthSeconds());
        scrubber.setPositionSeconds (engine.getPlaybackPositionSeconds());

        updateStatus();
    }

    void PracticeTab::changeListenerCallback (juce::ChangeBroadcaster*)
    {
        refreshInputChannelOptions();
    }

    void PracticeTab::refreshInputChannelOptions()
    {
        auto* device = engine.getDeviceManager().getCurrentAudioDevice();
        if (device == nullptr)
        {
            inputChannelCombo.clear (juce::dontSendNotification);
            inputChannelCombo.addItem ("(no device)", 1);
            inputChannelCombo.setSelectedItemIndex (0, juce::dontSendNotification);
            return;
        }

        const auto names  = device->getInputChannelNames();
        const auto active = device->getActiveInputChannels();

        const int prevIdx = juce::jmax (0, inputChannelCombo.getSelectedItemIndex());

        inputChannelCombo.clear (juce::dontSendNotification);
        int itemId = 1;
        for (int ch = 0; ch < names.size(); ++ch)
        {
            if (! active[ch]) continue;
            const auto label = "In " + juce::String (ch + 1)
                             + (names[ch].isNotEmpty() ? " \xE2\x80\x94 " + names[ch] : juce::String());
            inputChannelCombo.addItem (label, itemId++);
        }

        const int count = itemId - 1;
        if (count <= 0)
        {
            inputChannelCombo.addItem ("(no input channels)", 1);
            inputChannelCombo.setSelectedItemIndex (0, juce::dontSendNotification);
            engine.setInputChannelIndex (0);
            return;
        }

        const int chosen = juce::jlimit (0, count - 1, prevIdx);
        inputChannelCombo.setSelectedItemIndex (chosen, juce::sendNotificationSync);
    }

    void PracticeTab::updateStatus()
    {
        const double sr = engine.getCurrentSampleRate();
        const int    bs = engine.getCurrentBufferSize();
        const double lat = engine.getRoundTripLatencyMs();

        juce::String s;
        s << "SR " << juce::String (sr, 0) << " Hz"
          << "   Buf " << bs
          << "   RTT " << juce::String (lat, 1) << " ms"
          << "   In " << juce::String (juce::Decibels::gainToDecibels (engine.getInputLevel(), -80.0f), 1) << " dB"
          << "   Pos " << juce::String (engine.getPlaybackPositionSeconds(), 2) << " s"
          << " / " << juce::String (engine.getInstrumentalLengthSeconds(), 2) << " s";

        if (isRecordingNow) s << "   \xE2\x97\x8F REC";

        statusLabel.setText (s, juce::dontSendNotification);
    }
}
