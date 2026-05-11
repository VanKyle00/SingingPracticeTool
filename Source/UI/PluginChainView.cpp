#include "PluginChainView.h"
#include "PluginEditorWindow.h"
#include "../VST/PluginHost.h"

namespace sp
{
    struct PluginChainView::SlotButton : public juce::TextButton
    {
        PluginChainView& owner;
        int slotIndex;

        SlotButton (PluginChainView& o, int idx, const juce::String& name, bool bypassed)
            : juce::TextButton (name), owner (o), slotIndex (idx)
        {
            setTooltip ("Click: open editor.  Right-click: bypass / remove.");
            setClickingTogglesState (false);
            setColour (juce::TextButton::buttonColourId,
                       bypassed ? juce::Colour::fromRGB (60, 60, 65)
                                : juce::Colour::fromRGB (60, 90, 140));
            setColour (juce::TextButton::textColourOffId,
                       bypassed ? juce::Colours::grey : juce::Colours::white);
        }

        void clicked (const juce::ModifierKeys& mods) override
        {
            if (mods.isPopupMenu()) owner.showSlotMenu (slotIndex);
            else                    owner.openEditorFor (slotIndex);
        }
    };

    PluginChainView::PluginChainView (PluginHost& h) : host (h)
    {
        addAndMakeVisible (addButton);
        addButton.onClick = [this] { openAddDialog(); };

        host.onChainChanged = [this] { rebuild(); };
        rebuild();
    }

    PluginChainView::~PluginChainView()
    {
        host.onChainChanged = nullptr;
        editors.clear();
    }

    void PluginChainView::rebuild()
    {
        slotButtons.clear();

        const int n = host.getNumSlots();
        for (int i = 0; i < n; ++i)
        {
            auto btn = std::make_unique<SlotButton> (*this, i,
                                                     host.getSlotName (i),
                                                     host.isBypassed (i));
            addAndMakeVisible (*btn);
            slotButtons.push_back (std::move (btn));
        }
        resized();
        repaint();
    }

    void PluginChainView::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour::fromRGB (24, 26, 30));
    }

    void PluginChainView::resized()
    {
        auto r = getLocalBounds().reduced (4);
        const int slotW = 130;
        const int gap   = 4;

        for (auto& b : slotButtons)
        {
            b->setBounds (r.removeFromLeft (slotW));
            r.removeFromLeft (gap);
        }
        addButton.setBounds (r.removeFromLeft (140));
    }

    void PluginChainView::openAddDialog()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Pick a VST3 plugin",
            juce::File ("C:/Program Files/Common Files/VST3"),
            "*.vst3");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::canSelectDirectories,
            [this] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f == juce::File()) return;

                host.addPluginFromFileAsync (f,
                    [this] (PluginHost::Slot* slot, const juce::String& err)
                    {
                        if (slot == nullptr)
                            juce::AlertWindow::showAsync (
                                juce::MessageBoxOptions()
                                    .withIconType (juce::MessageBoxIconType::WarningIcon)
                                    .withTitle ("Plugin load failed")
                                    .withMessage (err)
                                    .withButton ("OK"),
                                nullptr);
                        // onChainChanged callback handles rebuild on success.
                    });
            });
    }

    void PluginChainView::openEditorFor (int index)
    {
        auto* inst = host.getInstance (index);
        if (inst == nullptr) return;

        auto win = std::make_unique<PluginEditorWindow> (*inst, host.getSlotName (index));
        auto* raw = win.get();
        raw->onClose = [this, raw]
        {
            for (auto it = editors.begin(); it != editors.end(); ++it)
                if (it->get() == raw) { editors.erase (it); return; }
        };
        editors.push_back (std::move (win));
    }

    void PluginChainView::showSlotMenu (int index)
    {
        juce::PopupMenu m;
        const bool bypassed = host.isBypassed (index);
        m.addItem (1, bypassed ? "Un-bypass" : "Bypass");
        m.addItem (2, "Remove");

        m.showMenuAsync (juce::PopupMenu::Options(),
            [this, index, bypassed] (int result)
            {
                if (result == 1) { host.setBypass (index, ! bypassed); rebuild(); }
                else if (result == 2) host.removePlugin (index);  // triggers rebuild via callback
            });
    }
}
