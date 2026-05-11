#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include "PitchRingBuffer.h"

namespace sp
{
    // YIN-style monophonic pitch detector. RT-safe after prepare().
    // Designed so the implementation can be swapped for aubio without touching callers.
    class PitchDetector
    {
    public:
        PitchDetector();

        // Call from prepareToPlay / audioDeviceAboutToStart.
        void prepare (double sampleRate, int analysisWindow = 1024);

        // Audio thread. Accumulates samples and emits one PitchSample per analysis window.
        void process (const float* mono, int numSamples) noexcept;

        // UI thread. Drains pending pitch samples.
        bool readNext (PitchSample& out) noexcept { return ring.pop (out); }

        // Tuning reference in Hz (default 440). Used by callers that convert to cents.
        void  setReferenceHz (float hz) noexcept { referenceHz.store (hz); }
        float getReferenceHz() const noexcept    { return referenceHz.load(); }

        double getSampleRate() const noexcept    { return sampleRate; }

    private:
        double sampleRate    = 48000.0;
        int    analysisSize  = 1024;
        int    tauMin        = 0;
        int    tauMax        = 0;
        std::uint64_t samplesProcessed = 0;

        std::vector<float> buffer;
        int                writePos = 0;

        std::vector<float> diff;          // YIN difference function workspace
        std::vector<float> cumNormDiff;   // cumulative mean normalized

        PitchRingBuffer<256> ring;
        std::atomic<float>   referenceHz { 440.0f };

        void analyse() noexcept;
    };
}
