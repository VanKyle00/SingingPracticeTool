#pragma once

#include <JuceHeader.h>
#include "../Audio/Engine.h"
#include "../Sidecar/Backend.h"

namespace sp
{
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name);
        ~MainWindow() override;

        void closeButtonPressed() override;

    private:
        Engine  engine;
        Backend backend;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };
}
