#include "TransportScrubber.h"
#include "ModernLookAndFeel.h"
#include <cmath>

namespace sp
{
    TransportScrubber::TransportScrubber()
    {
        setMouseCursor (juce::MouseCursor::IBeamCursor);
    }

    void TransportScrubber::setDurationSeconds (double s)
    {
        const double clamped = juce::jmax (0.0, s);
        if (duration != clamped)
        {
            duration = clamped;
            if (position > duration) position = duration;
            repaint();
        }
    }

    void TransportScrubber::setPositionSeconds (double s)
    {
        const double clamped = juce::jlimit (0.0, duration, s);
        if (position != clamped)
        {
            position = clamped;
            repaint();
        }
    }

    juce::Rectangle<float> TransportScrubber::railBounds() const
    {
        // Track sits centred vertically with a small inset so the thumb can extend
        // past the rail without clipping. Time labels render below the rail.
        const auto r = getLocalBounds().toFloat();
        const float pad = 6.0f;
        const float labelArea = 14.0f;
        return juce::Rectangle<float> (r.getX() + pad,
                                       r.getY() + (r.getHeight() - labelArea) * 0.5f - 4.0f,
                                       r.getWidth() - pad * 2.0f,
                                       8.0f);
    }

    double TransportScrubber::xToSeconds (float x) const
    {
        if (duration <= 0.0) return 0.0;
        const auto rail = railBounds();
        const float t = juce::jlimit (0.0f, 1.0f, (x - rail.getX()) / juce::jmax (1.0f, rail.getWidth()));
        return t * duration;
    }

    float TransportScrubber::secondsToX (double s) const
    {
        const auto rail = railBounds();
        if (duration <= 0.0) return rail.getX();
        const double t = juce::jlimit (0.0, 1.0, s / duration);
        return rail.getX() + (float) (t * rail.getWidth());
    }

    void TransportScrubber::seekTo (double seconds)
    {
        const double clamped = juce::jlimit (0.0, duration, seconds);
        setPositionSeconds (clamped);
        if (onSeek) onSeek (clamped);
    }

    static juce::String formatTime (double s)
    {
        if (s < 0.0 || ! std::isfinite (s)) return "--:--";
        const int total = (int) std::floor (s);
        const int mm = total / 60;
        const int ss = total % 60;
        return juce::String::formatted ("%d:%02d", mm, ss);
    }

    void TransportScrubber::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (ModernLookAndFeel::colSurface));

        const auto rail = railBounds();
        const float radius = rail.getHeight() * 0.5f;
        const bool  enabled = duration > 0.0;

        // Rail
        g.setColour (juce::Colour (ModernLookAndFeel::colInputBg));
        g.fillRoundedRectangle (rail, radius);
        g.setColour (juce::Colour (ModernLookAndFeel::colBorder));
        g.drawRoundedRectangle (rail.reduced (0.5f), radius, 1.0f);

        if (! enabled)
        {
            g.setColour (juce::Colour (ModernLookAndFeel::colTextMuted));
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText ("(no instrumental loaded)", getLocalBounds(),
                        juce::Justification::centred);
            return;
        }

        // Filled portion
        const float playheadX = secondsToX (position);
        g.setColour (juce::Colour (ModernLookAndFeel::colAccent));
        g.fillRoundedRectangle (rail.withRight (playheadX), radius);

        // Time labels (left = current, right = duration)
        g.setColour (juce::Colour (ModernLookAndFeel::colTextSecondary));
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        const auto labelRow = juce::Rectangle<int> ((int) rail.getX(),
                                                     (int) rail.getBottom() + 2,
                                                     (int) rail.getWidth(), 12);
        g.drawText (formatTime (position),  labelRow, juce::Justification::topLeft);
        g.drawText (formatTime (duration),  labelRow, juce::Justification::topRight);

        // Thumb (drawn last so it overlays everything).
        const float thumbR = 7.0f;
        const float thumbY = rail.getCentreY();
        g.setColour (juce::Colour (ModernLookAndFeel::colTextPrimary));
        g.fillEllipse (playheadX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
        g.setColour (juce::Colour (ModernLookAndFeel::colAccent));
        g.drawEllipse (playheadX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f, 1.5f);
    }

    void TransportScrubber::mouseDown (const juce::MouseEvent& e)
    {
        if (duration <= 0.0) return;
        seekTo (xToSeconds ((float) e.x));
    }

    void TransportScrubber::mouseDrag (const juce::MouseEvent& e)
    {
        if (duration <= 0.0) return;
        seekTo (xToSeconds ((float) e.x));
    }

    void TransportScrubber::mouseWheelMove (const juce::MouseEvent& e,
                                            const juce::MouseWheelDetails& wheel)
    {
        if (duration <= 0.0) return;
        // Use vertical wheel delta (or horizontal if a horizontal wheel is present).
        const float dy = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
        if (dy == 0.0f) return;

        // Shift = coarser (5 s/notch); plain = 1 s/notch.
        const double step = e.mods.isShiftDown() ? 5.0 : 1.0;
        seekTo (position - dy * step);
    }
}
