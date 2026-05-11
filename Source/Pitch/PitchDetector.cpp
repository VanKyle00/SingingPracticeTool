#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

namespace sp
{
    PitchDetector::PitchDetector() = default;

    void PitchDetector::prepare (double sr, int window)
    {
        sampleRate   = sr;
        analysisSize = window;

        // Vocal-range bounds: ~70 Hz to ~1500 Hz covers everything we need.
        tauMin = std::max (2, (int) std::floor (sampleRate / 1500.0));
        tauMax = std::min (analysisSize / 2, (int) std::ceil (sampleRate / 70.0));

        buffer.assign      ((std::size_t) analysisSize, 0.0f);
        diff.assign        ((std::size_t) (tauMax + 1), 0.0f);
        cumNormDiff.assign ((std::size_t) (tauMax + 1), 0.0f);
        writePos = 0;
        samplesProcessed = 0;
    }

    void PitchDetector::process (const float* mono, int n) noexcept
    {
        if (mono == nullptr || analysisSize <= 0) return;

        for (int i = 0; i < n; ++i)
        {
            buffer[(std::size_t) writePos] = mono[i];
            writePos = (writePos + 1) % analysisSize;
            ++samplesProcessed;

            if (writePos == 0)
                analyse();
        }
    }

    void PitchDetector::analyse() noexcept
    {
        // Rearrange ring into a linear analysis frame.
        // writePos is 0 right now, so buffer[] is already in order.
        const float* x = buffer.data();
        const int    N = analysisSize;

        // Energy gate: silent input would otherwise make YIN pick the first tau
        // (cumNormDiff is ~0 everywhere) and emit a confident phantom pitch.
        float energy = 0.0f;
        for (int i = 0; i < N; ++i) energy += x[i] * x[i];
        energy /= (float) N;
        constexpr float gateRms = 0.002f;  // ~-54 dBFS
        if (energy < gateRms * gateRms)
        {
            PitchSample s;
            s.timeSeconds = (double) samplesProcessed / sampleRate;
            ring.push (s);
            return;
        }

        // Step 1: difference function d(tau) = sum_j (x[j] - x[j+tau])^2
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            float sum = 0.0f;
            const int limit = N - tau;
            for (int j = 0; j < limit; ++j)
            {
                const float d = x[j] - x[j + tau];
                sum += d * d;
            }
            diff[(std::size_t) tau] = sum;
        }

        // Step 2: cumulative mean normalized difference d'(tau)
        cumNormDiff[(std::size_t) tauMin] = 1.0f;
        float running = 0.0f;
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            running += diff[(std::size_t) tau];
            if (running <= 0.0f)
                cumNormDiff[(std::size_t) tau] = 1.0f;
            else
                cumNormDiff[(std::size_t) tau] = diff[(std::size_t) tau] * (float) (tau - tauMin + 1) / running;
        }

        // Step 3: find first tau below threshold with local minimum
        constexpr float threshold = 0.15f;
        int chosenTau = -1;
        for (int tau = tauMin + 1; tau < tauMax; ++tau)
        {
            if (cumNormDiff[(std::size_t) tau] < threshold)
            {
                while (tau + 1 < tauMax
                       && cumNormDiff[(std::size_t) (tau + 1)] < cumNormDiff[(std::size_t) tau])
                {
                    ++tau;
                }
                chosenTau = tau;
                break;
            }
        }

        PitchSample s;
        s.timeSeconds = (double) samplesProcessed / sampleRate;

        if (chosenTau < 0)
        {
            s.frequencyHz = 0.0f;
            s.confidence  = 0.0f;
        }
        else
        {
            // Parabolic interpolation around chosenTau for sub-sample precision.
            float betterTau = (float) chosenTau;
            if (chosenTau > tauMin && chosenTau < tauMax)
            {
                const float s0 = cumNormDiff[(std::size_t) (chosenTau - 1)];
                const float s1 = cumNormDiff[(std::size_t) chosenTau];
                const float s2 = cumNormDiff[(std::size_t) (chosenTau + 1)];
                const float denom = (s0 + s2 - 2.0f * s1);
                if (std::abs (denom) > 1.0e-9f)
                    betterTau = (float) chosenTau + 0.5f * (s0 - s2) / denom;
            }
            s.frequencyHz = (float) (sampleRate / (double) betterTau);
            s.confidence  = juce::jlimit (0.0f, 1.0f, 1.0f - cumNormDiff[(std::size_t) chosenTau]);
        }

        ring.push (s);
    }
}
