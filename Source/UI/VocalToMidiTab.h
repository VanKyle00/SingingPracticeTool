#pragma once

#include <JuceHeader.h>
#include "../Sidecar/Backend.h"

namespace sp
{
    // Vocal -> MIDI tab: audio file -> basic-pitch -> .mid in chosen output dir.
    class VocalToMidiTab : public juce::Component,
                           public juce::FileDragAndDropTarget
    {
    public:
        explicit VocalToMidiTab (Backend& backend);
        ~VocalToMidiTab() override;

        void paint (juce::Graphics& g) override;
        void resized() override;

        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void filesDropped (const juce::StringArray& files, int, int) override;

    private:
        Backend& backend;

        juce::TextEditor inputField;
        juce::TextButton browseButton    { "Browse..." };
        juce::TextButton outputBrowseBtn { "Output..." };
        juce::Label      outputLabel;

        juce::Label    onsetLabel    { {}, "Onset threshold" };
        juce::Slider   onsetSlider;
        juce::Label    frameLabel    { {}, "Frame threshold" };
        juce::Slider   frameSlider;
        juce::Label    minLenLabel   { {}, "Min note length (ms)" };
        juce::Slider   minLenSlider;

        juce::TextButton runButton    { "Transcribe" };
        juce::TextButton cancelButton { "Cancel" };
        juce::TextButton revealButton { "Reveal MIDI" };

        juce::ProgressBar progressBar { progress };
        juce::Label       statusLabel;
        juce::TextEditor  logView;

        double     progress = 0.0;
        int        currentJobId = -1;
        juce::File outputDir;
        juce::File lastMidiOutput;
        std::unique_ptr<juce::FileChooser> fileChooser;

        void doBrowseInput();
        void doBrowseOutput();
        void doRun();
        void doCancel();
        void doReveal();

        void onProgress (float frac, const juce::String& text);
        void onComplete (bool ok, const juce::var& resultOrError);

        void appendLog (const juce::String& s);
        void setRunning (bool running);
    };
}
