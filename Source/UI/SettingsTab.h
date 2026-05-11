#pragma once

#include <JuceHeader.h>
#include "../Audio/DeviceSettings.h"

namespace sp
{
    class Engine;

    class SettingsTab : public juce::Component
    {
    public:
        explicit SettingsTab (Engine& engine);
        ~SettingsTab() override;

        void resized() override;

    private:
        DeviceSettingsComponent device;
    };
}
