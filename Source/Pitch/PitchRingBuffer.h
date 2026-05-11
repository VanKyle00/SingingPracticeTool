#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

namespace sp
{
    struct PitchSample
    {
        double  timeSeconds  = 0.0;
        float   frequencyHz  = 0.0f;
        float   confidence   = 0.0f;
    };

    // Single-producer (audio thread) / single-consumer (UI thread) ring buffer.
    template <std::size_t Capacity>
    class PitchRingBuffer
    {
    public:
        static_assert ((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");

        // Audio thread. Never blocks. Drops on overrun.
        bool push (const PitchSample& s) noexcept
        {
            const auto w = writeIdx.load (std::memory_order_relaxed);
            const auto r = readIdx.load  (std::memory_order_acquire);
            if (w - r >= Capacity) return false;
            data[w & mask] = s;
            writeIdx.store (w + 1, std::memory_order_release);
            return true;
        }

        // UI thread. Returns false if empty.
        bool pop (PitchSample& out) noexcept
        {
            const auto r = readIdx.load  (std::memory_order_relaxed);
            const auto w = writeIdx.load (std::memory_order_acquire);
            if (r == w) return false;
            out = data[r & mask];
            readIdx.store (r + 1, std::memory_order_release);
            return true;
        }

    private:
        static constexpr std::size_t mask = Capacity - 1;
        std::array<PitchSample, Capacity> data {};
        std::atomic<std::uint64_t> writeIdx { 0 };
        std::atomic<std::uint64_t> readIdx  { 0 };
    };
}
