#include "Recorder.h"

namespace sp
{
    Recorder::Recorder()
    {
        writerThread.startThread();
    }

    Recorder::~Recorder()
    {
        stop();
        writerThread.stopThread (500);
    }

    bool Recorder::start (const juce::File& file, double sampleRate, int numChannels)
    {
        stop();
        if (sampleRate <= 0.0 || numChannels <= 0) return false;

        file.deleteFile();
        auto stream = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream());
        if (stream == nullptr) return false;

        juce::WavAudioFormat fmt;
        std::unique_ptr<juce::AudioFormatWriter> w (
            fmt.createWriterFor (stream.get(), sampleRate, (unsigned int) numChannels, 24, {}, 0));
        if (w == nullptr) return false;
        stream.release();  // writer owns it now

        threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (
            w.release(), writerThread, 32768);

        activeWriter.store (threadedWriter.get(), std::memory_order_release);
        active.store (true);
        return true;
    }

    void Recorder::stop()
    {
        active.store (false);
        activeWriter.store (nullptr, std::memory_order_release);
        threadedWriter.reset();  // flushes any queued blocks
    }

    void Recorder::processInput (const float* mono, int numSamples) noexcept
    {
        auto* w = activeWriter.load (std::memory_order_acquire);
        if (w == nullptr || mono == nullptr) return;

        const float* channels[1] = { mono };
        w->write (channels, numSamples);
    }
}
