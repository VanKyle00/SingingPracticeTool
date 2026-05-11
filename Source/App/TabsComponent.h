#pragma once

#include <JuceHeader.h>

namespace sp
{
    class Engine;
    class Backend;
    class PracticeTab;
    class StemTab;
    class VocalToMidiTab;
    class SettingsTab;

    class TabsComponent : public juce::TabbedComponent
    {
    public:
        TabsComponent (Engine& engine, Backend& backend);
        ~TabsComponent() override;

    private:
        Engine&  engine;
        Backend& backend;
        std::unique_ptr<PracticeTab>     practice;
        std::unique_ptr<StemTab>         stem;
        std::unique_ptr<VocalToMidiTab>  v2m;
        std::unique_ptr<SettingsTab>     settings;
    };
}
