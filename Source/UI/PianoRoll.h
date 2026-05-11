#pragma once

#include <JuceHeader.h>
#include <deque>
#include <vector>
#include "../Pitch/PitchRingBuffer.h"

namespace sp
{
    // Scrolling piano-roll style component shared by Practice and Vocal->MIDI tabs.
    // Vertical: pitch (MIDI note). Horizontal: time (seconds), with a fixed playhead.
    class PianoRoll : public juce::Component
    {
    public:
        struct MidiNote { double startSec; double endSec; int noteNumber; int track = 0; };

        PianoRoll();
        ~PianoRoll() override;

        // Display config.
        void setPitchRange (int lowMidi, int highMidi);
        void setWindowSeconds (double pastSec, double futureSec);

        // Live data.
        void setPlayheadSeconds (double t);
        void appendPitchSample (const PitchSample& s);
        void clearPitchHistory();

        // MIDI overlay.
        void setMidiNotes (std::vector<MidiNote> notes);
        void clearMidi();

        // Shift all MIDI notes by this many seconds when drawing. Lets the caller
        // anchor MIDI to a transport position while the playhead runs on a different clock.
        void setMidiOffsetSeconds (double sec);

        // Resets pitch range and view offset to defaults.
        void resetView();

        void paint (juce::Graphics& g) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

    private:
        static constexpr int    kDefaultLow  = 36;   // C2
        static constexpr int    kDefaultHigh = 84;   // C6

        int    lowMidi  = kDefaultLow;
        int    highMidi = kDefaultHigh;
        double pastSec   = 1.0;
        double futureSec = 4.0;
        double playheadSec = 0.0;
        double viewTimeOffsetSec = 0.0;  // positive = scrolled into the past

        double midiOffsetSec = 0.0;

        std::deque<PitchSample> history;  // sorted by time, bounded
        std::vector<MidiNote>   midiNotes;

        float midiToY (double midi, juce::Rectangle<float> r) const;
        float timeToX (double t, juce::Rectangle<float> r) const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRoll)
    };
}
