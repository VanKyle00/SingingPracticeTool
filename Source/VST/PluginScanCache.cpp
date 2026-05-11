#include "PluginScanCache.h"

namespace sp
{
    PluginScanCache::PluginScanCache() = default;

    juce::File PluginScanCache::getCacheFile() const
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("SingingPracticeTool")
                   .getChildFile ("plugin_scan.xml");
    }

    bool PluginScanCache::load()
    {
        const auto f = getCacheFile();
        if (! f.existsAsFile()) return false;

        if (auto xml = juce::XmlDocument::parse (f))
        {
            list.recreateFromXml (*xml);
            return true;
        }
        return false;
    }

    bool PluginScanCache::save()
    {
        const auto f = getCacheFile();
        f.getParentDirectory().createDirectory();
        auto xml = list.createXml();
        return xml != nullptr && xml->writeTo (f);
    }
}
