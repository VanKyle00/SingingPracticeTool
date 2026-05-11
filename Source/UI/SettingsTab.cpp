#include "SettingsTab.h"
#include "../Audio/Engine.h"

namespace sp
{
    SettingsTab::SettingsTab (Engine& engine)
        : device (engine)
    {
        addAndMakeVisible (device);
    }

    SettingsTab::~SettingsTab() = default;

    void SettingsTab::resized()
    {
        device.setBounds (getLocalBounds());
    }
}
