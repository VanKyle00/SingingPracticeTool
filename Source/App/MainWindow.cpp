#include "MainWindow.h"
#include "TabsComponent.h"

namespace sp
{
    MainWindow::MainWindow (juce::String name)
        : juce::DocumentWindow (name,
                                juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour (juce::ResizableWindow::backgroundColourId),
                                juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);

        setContentOwned (new TabsComponent (engine, backend), false);

        setResizable (true, true);
        centreWithSize (1200, 800);
        setVisible (true);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::closeButtonPressed()
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
}
