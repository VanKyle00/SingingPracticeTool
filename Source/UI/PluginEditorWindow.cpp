#include "PluginEditorWindow.h"

namespace sp
{
    PluginEditorWindow::PluginEditorWindow (juce::AudioPluginInstance& plugin, const juce::String& title)
        : juce::DocumentWindow (title,
                                juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour (juce::ResizableWindow::backgroundColourId),
                                juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);

        editor.reset (plugin.createEditorIfNeeded());
        if (editor != nullptr)
        {
            setContentNonOwned (editor.get(), true);  // size to editor
        }
        else
        {
            auto* generic = new juce::GenericAudioProcessorEditor (plugin);
            generic->setSize (400, 300);
            editor.reset (generic);
            setContentNonOwned (editor.get(), true);
        }

        setResizable (editor->isResizable(), false);
        setVisible (true);
        toFront (true);
    }

    PluginEditorWindow::~PluginEditorWindow()
    {
        clearContentComponent();
        editor.reset();
    }

    void PluginEditorWindow::closeButtonPressed()
    {
        if (onClose) onClose();  // owner will delete us
    }
}
