#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

namespace sp
{
    // Background-thread WAV recorder. start()/stop() are message-thread; processInput() is audio-thread.
    class Recorder
    {
    public:
        Recorder();
        ~Recorder();

        // Begin writing to `file`. Returns false on failure (still safe to call stop()).
        bool start (const juce::File& file, double sampleRate, int numChannels);
        void stop();

        bool isActive() const noexcept { return active.load(); }

        // Audio thread. Pushes mono input into the writer FIFO. No-op if not recording.
        void processInput (const float* mono, int numSamples) noexcept;

    private:
        juce::TimeSliceThread                            writerThread { "spRecorder" };
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
        std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
        std::atomic<bool>                                active { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Recorder)
    };
}
