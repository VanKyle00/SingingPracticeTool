#pragma once

#include <JuceHeader.h>

namespace sp
{
    // Floating window that owns the plugin's GUI editor. Closing the window destroys it.
    class PluginEditorWindow : public juce::DocumentWindow
    {
    public:
        PluginEditorWindow (juce::AudioPluginInstance& plugin, const juce::String& title);
        ~PluginEditorWindow() override;

        void closeButtonPressed() override;

        // Invoked when the window closes (so the owner can drop its pointer).
        std::function<void()> onClose;

    private:
        std::unique_ptr<juce::AudioProcessorEditor> editor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditorWindow)
    };
}
