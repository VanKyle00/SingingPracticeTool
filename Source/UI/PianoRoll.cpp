#include "PianoRoll.h"
#include <cmath>

namespace sp
{
    PianoRoll::PianoRoll() = default;
    PianoRoll::~PianoRoll() = default;

    void PianoRoll::setPitchRange (int lo, int hi)
    {
        lowMidi  = juce::jmin (lo, hi);
        highMidi = juce::jmax (lo, hi);
        repaint();
    }

    void PianoRoll::setWindowSeconds (double past, double future)
    {
        pastSec   = juce::jmax (0.1, past);
        futureSec = juce::jmax (0.1, future);
        repaint();
    }

    void PianoRoll::setPlayheadSeconds (double t)
    {
        playheadSec = t;

        // Keep ~60s of history so the user can scroll back through it.
        const double cutoff = t - juce::jmax (pastSec + 0.5, 60.0);
        while (! history.empty() && history.front().timeSeconds < cutoff)
            history.pop_front();

        repaint();
    }

    void PianoRoll::appendPitchSample (const PitchSample& s)
    {
        history.push_back (s);
        if (history.size() > 8192)
            history.pop_front();
    }

    void PianoRoll::clearPitchHistory()
    {
        history.clear();
        repaint();
    }

    void PianoRoll::setMidiNotes (std::vector<MidiNote> notes)
    {
        midiNotes = std::move (notes);
        repaint();
    }

    void PianoRoll::clearMidi()
    {
        midiNotes.clear();
        repaint();
    }

    void PianoRoll::setMidiOffsetSeconds (double sec)
    {
        if (midiOffsetSec != sec) { midiOffsetSec = sec; repaint(); }
    }

    void PianoRoll::resetView()
    {
        lowMidi  = kDefaultLow;
        highMidi = kDefaultHigh;
        viewTimeOffsetSec = 0.0;
        repaint();
    }

    void PianoRoll::mouseDoubleClick (const juce::MouseEvent&)
    {
        resetView();
    }

    void PianoRoll::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        const float dy = wheel.deltaY;
        if (dy == 0.0f) return;

        if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        {
            // Zoom pitch range around its centre.
            const int span   = juce::jmax (1, highMidi - lowMidi);
            const int centre = (highMidi + lowMidi) / 2;
            const int delta  = (int) std::round (dy * 6.0f);
            const int newSpan = juce::jlimit (12, 96, span - delta);
            int newLow  = centre - newSpan / 2;
            int newHigh = newLow + newSpan;
            if (newLow < 0)   { newLow = 0; newHigh = newSpan; }
            if (newHigh > 127){ newHigh = 127; newLow = 127 - newSpan; }
            lowMidi  = newLow;
            highMidi = newHigh;
        }
        else if (e.mods.isShiftDown())
        {
            // Scrub time: positive deltaY -> look further into the past.
            const double span = pastSec + futureSec;
            viewTimeOffsetSec = juce::jmax (0.0, viewTimeOffsetSec + dy * span * 0.5);
        }
        else
        {
            // Scroll pitch range up/down.
            const int span = juce::jmax (1, highMidi - lowMidi);
            const int step = (int) std::round (dy * juce::jmax (2, span / 4));
            const int newLow = juce::jlimit (0, 127 - span, lowMidi + step);
            lowMidi  = newLow;
            highMidi = newLow + span;
        }
        repaint();
    }

    float PianoRoll::midiToY (double midi, juce::Rectangle<float> r) const
    {
        const double range = juce::jmax (1, highMidi - lowMidi);
        const double norm  = (midi - lowMidi) / range;
        return r.getBottom() - (float) (norm * r.getHeight());
    }

    float PianoRoll::timeToX (double t, juce::Rectangle<float> r) const
    {
        const double totalSpan = pastSec + futureSec;
        const double windowStart = playheadSec - pastSec - viewTimeOffsetSec;
        const double norm = (t - windowStart) / totalSpan;
        return r.getX() + (float) (norm * r.getWidth());
    }

    void PianoRoll::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.fillAll (juce::Colours::black);

        // Pitch grid: highlight C keys, faint lines on each semitone.
        const float noteHeight = bounds.getHeight() / juce::jmax (1, highMidi - lowMidi);
        for (int n = lowMidi; n <= highMidi; ++n)
        {
            const float y = midiToY ((double) n, bounds);
            const bool  isC = (n % 12) == 0;
            const bool  isBlack = juce::MidiMessage::isMidiNoteBlack (n);

            if (isBlack)
            {
                g.setColour (juce::Colour::fromRGB (18, 18, 22));
                g.fillRect (bounds.getX(), y - noteHeight, bounds.getWidth(), noteHeight);
            }

            g.setColour (isC ? juce::Colour::fromRGB (60, 60, 80)
                             : juce::Colour::fromRGB (30, 30, 35));
            g.drawHorizontalLine ((int) y, bounds.getX(), bounds.getRight());

            if (isC)
            {
                g.setColour (juce::Colours::grey);
                g.setFont (juce::Font (juce::FontOptions (10.0f)));
                g.drawText ("C" + juce::String ((n / 12) - 1),
                            (int) bounds.getX() + 2, (int) (y - noteHeight),
                            24, (int) noteHeight, juce::Justification::centredLeft);
            }
        }

        // Time grid: vertical line every second.
        const double windowStart = playheadSec - pastSec - viewTimeOffsetSec;
        const double windowEnd   = playheadSec + futureSec - viewTimeOffsetSec;
        const int    firstSec = (int) std::floor (windowStart);
        const int    lastSec  = (int) std::ceil  (windowEnd);
        g.setColour (juce::Colour::fromRGB (35, 35, 45));
        for (int s = firstSec; s <= lastSec; ++s)
        {
            const float x = timeToX ((double) s, bounds);
            g.drawVerticalLine ((int) x, bounds.getY(), bounds.getBottom());
        }

        // MIDI notes (shifted by midiOffsetSec so they align with the transport).
        if (! midiNotes.empty())
        {
            g.setColour (juce::Colour::fromRGBA (90, 140, 220, 140));
            for (const auto& n : midiNotes)
            {
                const double a = n.startSec + midiOffsetSec;
                const double b = n.endSec   + midiOffsetSec;
                if (b < windowStart || a > windowEnd) continue;
                if (n.noteNumber < lowMidi || n.noteNumber > highMidi) continue;

                const float x1 = timeToX (a, bounds);
                const float x2 = timeToX (b, bounds);
                const float y  = midiToY ((double) n.noteNumber, bounds);
                g.fillRect (x1, y - noteHeight, juce::jmax (2.0f, x2 - x1), noteHeight);
            }
        }

        // Pitch trace.
        if (! history.empty())
        {
            juce::Path path;
            bool started = false;
            for (const auto& s : history)
            {
                if (s.timeSeconds < windowStart || s.timeSeconds > windowEnd) continue;
                if (s.frequencyHz <= 0.0f || s.confidence < 0.5f) { started = false; continue; }

                const double midi = 69.0 + 12.0 * std::log2 ((double) s.frequencyHz / 440.0);
                if (midi < lowMidi - 1 || midi > highMidi + 1) { started = false; continue; }

                const float x = timeToX (s.timeSeconds, bounds);
                const float y = midiToY (midi, bounds);

                if (! started) { path.startNewSubPath (x, y); started = true; }
                else            { path.lineTo (x, y); }
            }
            g.setColour (juce::Colours::limegreen);
            g.strokePath (path, juce::PathStrokeType (2.0f));
        }

        // Playhead.
        const float phX = timeToX (playheadSec, bounds);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawVerticalLine ((int) phX, bounds.getY(), bounds.getBottom());
    }
}
