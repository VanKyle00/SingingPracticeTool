#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <functional>
#include "PluginScanCache.h"

namespace sp
{
    // Ordered VST3 chain on the vocal bus. Audio-thread safe: chain edits acquire
    // a CriticalSection that the audio callback tries to take non-blockingly. If
    // the audio thread can't take it (rare, only during an edit), it passes audio
    // through dry that block — never blocks.
    class PluginHost
    {
    public:
        struct Slot
        {
            std::unique_ptr<juce::AudioPluginInstance> instance;
            juce::PluginDescription                    description;
            std::atomic<bool>                          bypassed { false };
        };

        PluginHost();
        ~PluginHost();

        juce::AudioPluginFormatManager& getFormatManager() { return formats; }
        PluginScanCache&                getCache()         { return cache; }

        // Async plugin creation. onDone runs on the message thread.
        // result == nullptr if creation failed; errorMsg has details.
        void addPluginFromFileAsync (const juce::File& pluginFile,
                                     std::function<void (Slot* result, const juce::String& errorMsg)> onDone);

        void removePlugin (int index);
        void moveSlot (int from, int to);
        void setBypass (int index, bool bypass);
        bool isBypassed (int index) const;
        int  getNumSlots() const;

        // Access for UI (message-thread).
        juce::String getSlotName (int index) const;
        juce::AudioPluginInstance* getInstance (int index);

        // Lifecycle (message-thread).
        void prepareToPlay (double sampleRate, int blockSize);
        void releaseResources();

        // Audio thread. Buffer is stereo. Processes in chain order; bypassed slots skipped.
        void processBlock (juce::AudioBuffer<float>& buffer) noexcept;

        // For UI to know when chain changes (slot added/removed/reordered).
        std::function<void()> onChainChanged;

    private:
        juce::AudioPluginFormatManager formats;
        PluginScanCache                cache;

        // Slots are owned via unique_ptr. The audio callback walks the vector under
        // a try-lock; the message thread takes the lock fully when mutating.
        std::vector<std::unique_ptr<Slot>> slots;
        mutable juce::CriticalSection      chainLock;

        double  currentSampleRate = 0.0;
        int     currentBlockSize  = 0;
        juce::MidiBuffer emptyMidi;

        void prepareSlot (Slot& s);
        void notifyChanged();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginHost)
    };
}
