#pragma once

#include <JuceHeader.h>
#include <memory>
#include "PianoRoll.h"
#include "TransportScrubber.h"

namespace sp
{
    class Engine;
    class PluginChainView;

    class PracticeTab : public juce::Component,
                        public juce::Timer,
                        public juce::ChangeListener,
                        public juce::FileDragAndDropTarget
    {
    public:
        explicit PracticeTab (Engine& engine);
        ~PracticeTab() override;

        void resized() override;
        void paint (juce::Graphics& g) override;

        // FileDragAndDropTarget
        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void filesDropped (const juce::StringArray& files, int x, int y) override;

        // Timer
        void timerCallback() override;

        // ChangeListener (AudioDeviceManager device-changed events)
        void changeListenerCallback (juce::ChangeBroadcaster*) override;

    private:
        Engine& engine;
        PianoRoll pianoRoll;
        TransportScrubber scrubber;

        juce::TextButton playButton    { "Play" };
        juce::TextButton stopButton    { "Stop" };
        juce::TextButton loadAudioBtn  { "Load Instrumental..." };
        juce::TextButton loadMidiBtn   { "Load Reference MIDI..." };
        juce::TextButton recordButton  { "Record" };

        juce::Slider vocalGainSlider;
        juce::Slider instGainSlider;
        juce::Label  vocalGainLabel    { {}, "Vocal Monitor" };
        juce::Label  instGainLabel     { {}, "Instrumental" };

        juce::ComboBox inputChannelCombo;
        juce::Label    inputChannelLabel { {}, "Mic Input" };

        std::unique_ptr<PluginChainView> chainView;

        juce::Label  statusLabel;

        std::unique_ptr<juce::FileChooser> fileChooser;
        juce::File recordingFolder;
        bool isRecordingNow = false;

        void loadAudioDialog();
        void loadMidiDialog();
        void loadMidiFile (const juce::File& f);
        void loadAudioFile (const juce::File& f);
        void toggleRecord();
        void updateStatus();
        void refreshInputChannelOptions();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PracticeTab)
    };
}
