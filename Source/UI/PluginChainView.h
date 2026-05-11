#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace sp
{
    class PluginHost;
    class PluginEditorWindow;

    // Horizontal strip showing the vocal-bus plugin chain:
    //   [Comp] [EQ] [Reverb] [+ Add Plugin]
    // - Left-click a slot -> open editor window
    // - Right-click a slot -> menu (bypass / remove)
    // - Bypassed slot renders dimmed.
    class PluginChainView : public juce::Component
    {
    public:
        explicit PluginChainView (PluginHost& host);
        ~PluginChainView() override;

        void rebuild();           // call when the chain changes
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        struct SlotButton;

        PluginHost& host;
        juce::TextButton addButton { "+ Add Plugin..." };
        std::vector<std::unique_ptr<SlotButton>>           slotButtons;
        std::vector<std::unique_ptr<PluginEditorWindow>>   editors;
        std::unique_ptr<juce::FileChooser>                 fileChooser;

        void openAddDialog();
        void openEditorFor (int index);
        void showSlotMenu (int index);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginChainView)
    };
}
