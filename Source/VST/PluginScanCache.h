#pragma once

#include <JuceHeader.h>

namespace sp
{
    // Persists scan results so we don't re-scan all VST3s on every launch.
    class PluginScanCache
    {
    public:
        PluginScanCache();

        bool load();
        bool save();

        const juce::KnownPluginList& getList() const { return list; }
        juce::KnownPluginList&       getList()       { return list; }

        juce::File getCacheFile() const;

    private:
        juce::KnownPluginList list;
    };
}
