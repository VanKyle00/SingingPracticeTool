#pragma once

#include <JuceHeader.h>

namespace sp
{
    class Engine;

    // Thin wrapper around juce::AudioDeviceSelectorComponent so the Settings tab can drop it in.
    class DeviceSettingsComponent : public juce::Component
    {
    public:
        explicit DeviceSettingsComponent (Engine& engine);
        ~DeviceSettingsComponent() override;

        void resized() override;

    private:
        std::unique_ptr<juce::AudioDeviceSelectorComponent> selector;
    };
}
