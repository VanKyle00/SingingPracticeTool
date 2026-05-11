#include "DeviceSettings.h"
#include "Engine.h"

namespace sp
{
    DeviceSettingsComponent::DeviceSettingsComponent (Engine& engine)
    {
        selector = std::make_unique<juce::AudioDeviceSelectorComponent> (
            engine.getDeviceManager(),
            1, 32,   // min/max input  channels (allow multi-channel interfaces)
            0, 2,    // min/max output channels
            false,   // show MIDI input
            false,   // show MIDI output channel selector
            false,   // showChannelsAsStereoPairs — false lists individual mono channels
            false);  // hide advanced
        addAndMakeVisible (*selector);
    }

    DeviceSettingsComponent::~DeviceSettingsComponent() = default;

    void DeviceSettingsComponent::resized()
    {
        selector->setBounds (getLocalBounds().reduced (12));
    }
}
