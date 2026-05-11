#pragma once

#include <JuceHeader.h>
#include <functional>

namespace sp
{
    // Horizontal timeline strip showing playback position over the full song
    // duration. Click / drag / wheel emits seek requests; the owner is
    // responsible for actually moving the transport and feeding back the new
    // position via setPosition() so the visual stays in sync.
    class TransportScrubber : public juce::Component
    {
    public:
        TransportScrubber();
        ~TransportScrubber() override = default;

        // Called every time the user requests a seek (click, drag, wheel).
        // Argument is seconds, already clamped to [0, duration].
        std::function<void (double seconds)> onSeek;

        void setDurationSeconds (double seconds);   // 0 disables interaction
        void setPositionSeconds (double seconds);   // updates display

        void paint (juce::Graphics&) override;
        void mouseDown  (const juce::MouseEvent&) override;
        void mouseDrag  (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    private:
        double duration = 0.0;
        double position = 0.0;

        juce::Rectangle<float> railBounds() const;
        double xToSeconds (float x) const;
        float  secondsToX (double s) const;

        void seekTo (double seconds);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportScrubber)
    };
}
