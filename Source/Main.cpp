#include <JuceHeader.h>
#include "App/MainWindow.h"
#include "UI/ModernLookAndFeel.h"

class SingingPracticeApp : public juce::JUCEApplication
{
public:
    SingingPracticeApp() = default;

    const juce::String getApplicationName() override       { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override    { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        laf = std::make_unique<sp::ModernLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (laf.get());

        mainWindow = std::make_unique<sp::MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        laf = nullptr;
    }

    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<sp::ModernLookAndFeel> laf;
    std::unique_ptr<sp::MainWindow>        mainWindow;
};

START_JUCE_APPLICATION (SingingPracticeApp)
