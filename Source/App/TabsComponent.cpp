#include "TabsComponent.h"
#include "../UI/PracticeTab.h"
#include "../UI/StemTab.h"
#include "../UI/VocalToMidiTab.h"
#include "../UI/SettingsTab.h"
#include "../UI/ModernLookAndFeel.h"

namespace sp
{
    TabsComponent::TabsComponent (Engine& e, Backend& b)
        : juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop),
          engine (e), backend (b)
    {
        const auto bg = juce::Colour (ModernLookAndFeel::colSurface);

        setTabBarDepth (38);
        setOutline (0);
        setIndent (0);

        practice = std::make_unique<PracticeTab> (engine);
        stem     = std::make_unique<StemTab> (backend);
        v2m      = std::make_unique<VocalToMidiTab> (backend);
        settings = std::make_unique<SettingsTab> (engine);

        addTab ("Practice",      bg, practice.get(), false);
        addTab ("Stem Extract",  bg, stem.get(),     false);
        addTab ("Vocal \xE2\x86\x92 MIDI", bg, v2m.get(), false);
        addTab ("Settings",      bg, settings.get(), false);
    }

    TabsComponent::~TabsComponent()
    {
        clearTabs();
    }
}
