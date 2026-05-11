#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include "../Pitch/PitchDetector.h"
#include "../VST/PluginHost.h"
#include "Recorder.h"

namespace sp
{
    class Engine : public juce::AudioIODeviceCallback,
                   private juce::ChangeListener
    {
    public:
        Engine();
        ~Engine() override;

        juce::AudioDeviceManager& getDeviceManager()       { return deviceManager; }
        PitchDetector&            getPitchDetector()       { return pitchDetector; }
        PluginHost&               getVocalChain()          { return vocalChain; }

        // Instrumental playback. Thread-safe entry; underlying swap happens on the message thread.
        bool loadInstrumental (const juce::File& file);
        void unloadInstrumental();
        bool hasInstrumental() const noexcept;
        void play();
        void stop();
        bool isPlaying() const noexcept;
        double getPlaybackPositionSeconds() const noexcept;
        double getInstrumentalLengthSeconds() const noexcept;
        void   setPlaybackPositionSeconds (double seconds);

        // Wall clock that advances as long as the audio device is running.
        // Pitch samples are timestamped against this clock, so use it for the playhead
        // to keep the live pitch trace anchored at "now".
        double getEngineTimeSeconds() const noexcept;

        // Mixing gains [0..1]. Atomic, OK to set from UI.
        void  setVocalMonitorGain (float g) noexcept { vocalGain.store (g); }
        void  setInstrumentalGain (float g) noexcept { instrumentalGain.store (g); }
        float getVocalMonitorGain() const noexcept   { return vocalGain.load(); }
        float getInstrumentalGain() const noexcept   { return instrumentalGain.load(); }

        // Which active input channel to use as the mono mic. Index is into the audio
        // callback's compacted inputChannelData array (0-based across *enabled* channels).
        void setInputChannelIndex (int idx) noexcept { selectedInputChannel.store (idx); }
        int  getInputChannelIndex() const noexcept   { return selectedInputChannel.load(); }

        // Status surfaced to UI.
        float  getInputLevel() const noexcept       { return inputLevel.load(); }
        double getCurrentSampleRate() const noexcept { return currentSampleRate.load(); }
        int    getCurrentBufferSize() const noexcept { return currentBufferSize.load(); }
        double getRoundTripLatencyMs() const noexcept;

        // Recording (writes vocal_dry_<ts>.wav into folder).
        void startRecording (const juce::File& folder);
        void stopRecording();
        bool isRecording() const noexcept { return recorder.isActive(); }

        // juce::AudioIODeviceCallback
        void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                               int numInputChannels,
                                               float* const* outputChannelData,
                                               int numOutputChannels,
                                               int numSamples,
                                               const juce::AudioIODeviceCallbackContext& ctx) override;
        void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
        void audioDeviceStopped() override;

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override;

        static juce::File getDeviceSettingsFile();
        void loadPersistedDeviceState();
        void savePersistedDeviceState();

        juce::String lastWrittenDeviceXml;

        juce::AudioDeviceManager deviceManager;
        juce::AudioFormatManager formatManager;

        // Transport for the instrumental.
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        juce::AudioTransportSource transport;
        juce::TimeSliceThread       transportThread { "spTransport" };

        PitchDetector pitchDetector;
        PluginHost    vocalChain;
        Recorder      recorder;

        // Stereo scratch buffer for the vocal bus (mic duplicated -> chain -> monitor).
        // Sized in audioDeviceAboutToStart so processBlock never allocates.
        juce::AudioBuffer<float> vocalScratch;

        std::atomic<float>  vocalGain        { 0.0f };  // start muted to avoid feedback
        std::atomic<float>  instrumentalGain { 1.0f };
        std::atomic<float>  inputLevel       { 0.0f };
        std::atomic<double> currentSampleRate { 0.0 };
        std::atomic<int>    currentBufferSize { 0 };
        std::atomic<int>    inputLatencySamples  { 0 };
        std::atomic<int>    outputLatencySamples { 0 };
        std::atomic<int>    selectedInputChannel { 0 };
        std::atomic<std::uint64_t> samplesProcessed { 0 };

        juce::CriticalSection transportLock;  // guards readerSource swap (audio thread holds briefly during swap-in)

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
    };
}
