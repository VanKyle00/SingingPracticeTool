#pragma once

#include <JuceHeader.h>
#include "../Sidecar/Backend.h"

namespace sp
{
    // Stem-extraction tab: file or YouTube URL -> Demucs -> WAV stems.
    class StemTab : public juce::Component,
                    public juce::FileDragAndDropTarget
    {
    public:
        explicit StemTab (Backend& backend);
        ~StemTab() override;

        void paint (juce::Graphics& g) override;
        void resized() override;

        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void filesDropped (const juce::StringArray& files, int, int) override;

    private:
        Backend& backend;

        juce::TextEditor   inputField;      // file path or YouTube URL
        juce::TextButton   browseButton     { "Browse..." };
        juce::TextButton   outputBrowseBtn  { "Output..." };
        juce::Label        outputLabel;

        juce::ComboBox     modelCombo;
        juce::ComboBox     deviceCombo;
        juce::ToggleButton vocalsOnlyToggle { "Vocals only (faster)" };

        juce::TextButton   runButton        { "Separate" };
        juce::TextButton   cancelButton     { "Cancel" };
        juce::TextButton   openInPracticeBtn{ "Open instrumental in Practice tab" };

        juce::ProgressBar  progressBar      { progress };
        juce::Label        statusLabel;
        juce::TextEditor   logView;

        double progress = 0.0;
        int    currentJobId = -1;
        juce::File outputDir;
        juce::File lastInstrumentalOutput;
        std::unique_ptr<juce::FileChooser> fileChooser;

        void doBrowseInput();
        void doBrowseOutput();
        void doRun();
        void doCancel();
        void doOpenInPractice();

        void onProgress (float frac, const juce::String& text);
        void onComplete (bool ok, const juce::var& resultOrError);

        void appendLog (const juce::String& s);
        void setRunning (bool running);
        bool inputLooksLikeUrl() const;
        juce::File inputAsFile() const;
    };
}
