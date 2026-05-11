#include "ModernLookAndFeel.h"

namespace sp
{
    static juce::Font systemFont (float h, int style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), h, style));
    }

    ModernLookAndFeel::ModernLookAndFeel()
    {
        installColourIds();
    }

    void ModernLookAndFeel::installColourIds()
    {
        using Cb = juce::ResizableWindow;
        using Doc = juce::DocumentWindow;
        using L = juce::Label;
        using Tb = juce::TextButton;
        using Cbx = juce::ComboBox;
        using Sl = juce::Slider;
        using Te = juce::TextEditor;
        using Pm = juce::PopupMenu;
        using Sb = juce::ScrollBar;
        using Tbb = juce::TabbedButtonBar;
        using Pb = juce::ProgressBar;
        using Tg = juce::ToggleButton;

        setColour (Cb::backgroundColourId,            juce::Colour (colBackground));
        setColour (Doc::backgroundColourId,           juce::Colour (colBackground));

        setColour (L::textColourId,                   juce::Colour (colTextPrimary));
        setColour (L::backgroundColourId,             juce::Colours::transparentBlack);

        setColour (Tb::buttonColourId,                juce::Colour (colSurfaceRaised));
        setColour (Tb::buttonOnColourId,              juce::Colour (colAccentDim));
        setColour (Tb::textColourOnId,                juce::Colour (colTextPrimary));
        setColour (Tb::textColourOffId,               juce::Colour (colTextPrimary));

        setColour (Cbx::backgroundColourId,           juce::Colour (colInputBg));
        setColour (Cbx::outlineColourId,              juce::Colour (colBorder));
        setColour (Cbx::textColourId,                 juce::Colour (colTextPrimary));
        setColour (Cbx::arrowColourId,                juce::Colour (colTextSecondary));
        setColour (Cbx::buttonColourId,               juce::Colours::transparentBlack);
        setColour (Cbx::focusedOutlineColourId,       juce::Colour (colAccent));

        setColour (Sl::backgroundColourId,            juce::Colours::transparentBlack);
        setColour (Sl::trackColourId,                 juce::Colour (colBorder));
        setColour (Sl::thumbColourId,                 juce::Colour (colAccent));
        setColour (Sl::textBoxBackgroundColourId,     juce::Colour (colInputBg));
        setColour (Sl::textBoxTextColourId,           juce::Colour (colTextPrimary));
        setColour (Sl::textBoxOutlineColourId,        juce::Colours::transparentBlack);
        setColour (Sl::rotarySliderFillColourId,      juce::Colour (colAccent));
        setColour (Sl::rotarySliderOutlineColourId,   juce::Colour (colBorder));

        setColour (Te::backgroundColourId,            juce::Colour (colInputBg));
        setColour (Te::textColourId,                  juce::Colour (colTextPrimary));
        setColour (Te::highlightColourId,             juce::Colour (colAccent).withAlpha (0.30f));
        setColour (Te::highlightedTextColourId,       juce::Colour (colTextPrimary));
        setColour (Te::outlineColourId,               juce::Colour (colBorder));
        setColour (Te::focusedOutlineColourId,        juce::Colour (colAccent));
        setColour (Te::shadowColourId,                juce::Colours::transparentBlack);

        setColour (Pm::backgroundColourId,            juce::Colour (colSurfaceRaised));
        setColour (Pm::textColourId,                  juce::Colour (colTextPrimary));
        setColour (Pm::headerTextColourId,            juce::Colour (colTextSecondary));
        setColour (Pm::highlightedBackgroundColourId, juce::Colour (colAccent).withAlpha (0.20f));
        setColour (Pm::highlightedTextColourId,       juce::Colour (colTextPrimary));

        setColour (Sb::backgroundColourId,            juce::Colours::transparentBlack);
        setColour (Sb::thumbColourId,                 juce::Colour (colBorder));
        setColour (Sb::trackColourId,                 juce::Colours::transparentBlack);

        setColour (Tbb::tabOutlineColourId,           juce::Colours::transparentBlack);
        setColour (Tbb::tabTextColourId,              juce::Colour (colTextSecondary));
        setColour (Tbb::frontOutlineColourId,         juce::Colours::transparentBlack);
        setColour (Tbb::frontTextColourId,            juce::Colour (colTextPrimary));

        setColour (Pb::backgroundColourId,            juce::Colour (colInputBg));
        setColour (Pb::foregroundColourId,            juce::Colour (colAccent));

        setColour (Tg::textColourId,                  juce::Colour (colTextPrimary));
        setColour (Tg::tickColourId,                  juce::Colour (colAccent));
        setColour (Tg::tickDisabledColourId,          juce::Colour (colTextMuted));
    }

    juce::Font ModernLookAndFeel::getTextButtonFont (juce::TextButton&, int h)
    {
        return systemFont (juce::jlimit (12.0f, 14.0f, h * 0.45f));
    }

    juce::Font ModernLookAndFeel::getLabelFont (juce::Label& l)
    {
        return systemFont (juce::jmax (12.0f, (float) l.getFont().getHeight()));
    }

    juce::Font ModernLookAndFeel::getComboBoxFont (juce::ComboBox&) { return systemFont (13.0f); }
    juce::Font ModernLookAndFeel::getPopupMenuFont()                { return systemFont (13.0f); }

    void ModernLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                  const juce::Colour& bg,
                                                  bool isHighlighted, bool isDown)
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const float radius = 6.0f;

        auto base = bg;
        if (! b.isEnabled())          base = base.darker (0.4f);
        else if (isDown)              base = base.brighter (0.05f);
        else if (isHighlighted)       base = base.brighter (0.12f);

        g.setColour (base);
        g.fillRoundedRectangle (r, radius);

        g.setColour (juce::Colour (colBorder).withAlpha (b.isEnabled() ? 1.0f : 0.4f));
        g.drawRoundedRectangle (r, radius, 1.0f);

        // Subtle accent underline on hover for a "tactile" feel.
        if (isHighlighted && b.isEnabled() && ! isDown)
        {
            g.setColour (juce::Colour (colAccent).withAlpha (0.35f));
            g.drawRoundedRectangle (r.reduced (1.0f), radius - 1.0f, 1.0f);
        }
    }

    void ModernLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                              bool isHighlighted, bool isDown)
    {
        const float boxSize = 16.0f;
        const float yCenter = b.getHeight() * 0.5f;
        const juce::Rectangle<float> box (4.0f, yCenter - boxSize * 0.5f, boxSize, boxSize);

        drawTickBox (g, b, box.getX(), box.getY(), box.getWidth(), box.getHeight(),
                     b.getToggleState(), b.isEnabled(), isHighlighted, isDown);

        g.setColour (b.findColour (juce::ToggleButton::textColourId).withAlpha (b.isEnabled() ? 1.0f : 0.5f));
        g.setFont (systemFont (13.0f));
        g.drawText (b.getButtonText(),
                    (int) (box.getRight() + 8.0f), 0, b.getWidth(), b.getHeight(),
                    juce::Justification::centredLeft);
    }

    void ModernLookAndFeel::drawTickBox (juce::Graphics& g, juce::Component&,
                                         float x, float y, float w, float h,
                                         bool ticked, bool isEnabled,
                                         bool isHighlighted, bool /*isDown*/)
    {
        const juce::Rectangle<float> box (x, y, w, h);
        const float r = 4.0f;

        g.setColour (juce::Colour (colInputBg));
        g.fillRoundedRectangle (box, r);

        auto outline = ticked ? juce::Colour (colAccent) : juce::Colour (colBorder);
        if (! isEnabled) outline = outline.withAlpha (0.4f);
        else if (isHighlighted && ! ticked) outline = outline.brighter (0.3f);
        g.setColour (outline);
        g.drawRoundedRectangle (box.reduced (0.5f), r, 1.2f);

        if (ticked)
        {
            g.setColour (juce::Colour (colAccent).withAlpha (isEnabled ? 1.0f : 0.5f));
            const juce::Rectangle<float> fill = box.reduced (3.0f);
            g.fillRoundedRectangle (fill, r - 2.0f);
        }
    }

    void ModernLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                          bool /*isDown*/, int /*buttonX*/, int /*buttonY*/,
                                          int /*buttonW*/, int /*buttonH*/,
                                          juce::ComboBox& box)
    {
        const auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        const float radius = 6.0f;

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, radius);

        auto outline = box.hasKeyboardFocus (true)
                         ? box.findColour (juce::ComboBox::focusedOutlineColourId)
                         : box.findColour (juce::ComboBox::outlineColourId);
        g.setColour (outline);
        g.drawRoundedRectangle (r, radius, 1.0f);

        // Chevron
        juce::Path chev;
        const float cx = width - 14.0f;
        const float cy = height * 0.5f;
        chev.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chev.lineTo          (cx,        cy + 3.0f);
        chev.lineTo          (cx + 4.0f, cy - 2.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (chev, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void ModernLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                              float sliderPos, float /*minPos*/, float /*maxPos*/,
                                              juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearVertical
            && style != juce::Slider::LinearBar && style != juce::Slider::LinearBarVertical)
        {
            // Fallback to base for rotary etc.
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos,
                                                    (float) slider.valueToProportionOfLength (slider.getMinimum()),
                                                    (float) slider.valueToProportionOfLength (slider.getMaximum()),
                                                    style, slider);
            return;
        }

        const bool horiz = (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar);
        const float trackThickness = 4.0f;
        const float thumbRadius    = 7.0f;

        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float cy = bounds.getCentreY();
        const float cx = bounds.getCentreX();

        juce::Rectangle<float> trackFull, trackFilled;
        if (horiz)
        {
            trackFull = juce::Rectangle<float> (bounds.getX(), cy - trackThickness * 0.5f,
                                                bounds.getWidth(), trackThickness);
            trackFilled = trackFull.withRight (sliderPos);
        }
        else
        {
            trackFull = juce::Rectangle<float> (cx - trackThickness * 0.5f, bounds.getY(),
                                                trackThickness, bounds.getHeight());
            trackFilled = trackFull.withTop (sliderPos);
        }

        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRoundedRectangle (trackFull, trackThickness * 0.5f);

        g.setColour (juce::Colour (colAccent).withAlpha (slider.isEnabled() ? 1.0f : 0.4f));
        g.fillRoundedRectangle (trackFilled, trackThickness * 0.5f);

        // Thumb
        const auto thumbCentre = horiz
            ? juce::Point<float> (sliderPos, cy)
            : juce::Point<float> (cx, sliderPos);

        const auto thumbCol = juce::Colour (colTextPrimary).withAlpha (slider.isEnabled() ? 1.0f : 0.5f);
        g.setColour (thumbCol);
        g.fillEllipse (thumbCentre.x - thumbRadius, thumbCentre.y - thumbRadius,
                       thumbRadius * 2.0f, thumbRadius * 2.0f);

        g.setColour (juce::Colour (colAccent));
        g.drawEllipse (thumbCentre.x - thumbRadius, thumbCentre.y - thumbRadius,
                       thumbRadius * 2.0f, thumbRadius * 2.0f, 1.5f);
    }

    void ModernLookAndFeel::drawProgressBar (juce::Graphics& g, juce::ProgressBar& bar,
                                             int width, int height,
                                             double progress, const juce::String& textToShow)
    {
        const auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height);
        const float radius = juce::jmin (6.0f, r.getHeight() * 0.5f);

        g.setColour (bar.findColour (juce::ProgressBar::backgroundColourId));
        g.fillRoundedRectangle (r, radius);

        if (progress >= 0.0 && progress <= 1.0)
        {
            const float fillW = (float) (r.getWidth() * progress);
            if (fillW > 1.0f)
            {
                g.setColour (bar.findColour (juce::ProgressBar::foregroundColourId));
                g.fillRoundedRectangle (r.withWidth (fillW), radius);
            }
        }
        else
        {
            // Indeterminate — animated stripes
            juce::Path stripes;
            const float t = (float) (juce::Time::getMillisecondCounter() * 0.0005);
            const float stripeW = 14.0f;
            for (float ox = -stripeW; ox < r.getWidth(); ox += stripeW * 2.0f)
            {
                const float shift = std::fmod (t * stripeW * 4.0f, stripeW * 2.0f);
                stripes.addRectangle (ox + shift, 0.0f, stripeW, r.getHeight());
            }
            g.saveState();
            g.reduceClipRegion (r.toNearestInt());
            g.setColour (bar.findColour (juce::ProgressBar::foregroundColourId).withAlpha (0.6f));
            g.fillPath (stripes);
            g.restoreState();
        }

        if (textToShow.isNotEmpty())
        {
            g.setColour (juce::Colour (colTextPrimary));
            g.setFont (systemFont (11.0f));
            g.drawText (textToShow, r.toNearestInt(), juce::Justification::centred);
        }
    }

    int ModernLookAndFeel::getTabButtonBestWidth (juce::TabBarButton& b, int /*tabDepth*/)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (systemFont (13.0f), b.getButtonText(), 0.0f, 0.0f);
        return (int) std::ceil (ga.getBoundingBox (0, -1, true).getWidth()) + 32;
    }

    void ModernLookAndFeel::drawTabButton (juce::TabBarButton& b, juce::Graphics& g,
                                           bool isHighlighted, bool /*isDown*/)
    {
        auto r = b.getLocalBounds();
        const bool active = b.isFrontTab();

        auto textCol = active ? juce::Colour (colTextPrimary)
                              : juce::Colour (colTextSecondary);
        if (isHighlighted && ! active) textCol = juce::Colour (colTextPrimary);
        if (! b.isEnabled())            textCol = textCol.withAlpha (0.4f);

        g.setColour (textCol);
        g.setFont (systemFont (13.0f, active ? juce::Font::bold : juce::Font::plain));
        g.drawText (b.getButtonText(), r, juce::Justification::centred);

        if (active)
        {
            g.setColour (juce::Colour (colAccent));
            const float thickness = 2.0f;
            g.fillRect ((float) r.getX() + 8.0f, (float) r.getBottom() - thickness,
                        (float) r.getWidth() - 16.0f, thickness);
        }
    }

    void ModernLookAndFeel::drawTabAreaBehindFrontButton (juce::TabbedButtonBar& bar, juce::Graphics& g,
                                                          int width, int height)
    {
        // Bottom hairline that runs across the whole tab strip.
        g.setColour (juce::Colour (colBorder));
        g.fillRect (0, height - 1, width, 1);
        juce::ignoreUnused (bar);
    }

    void ModernLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& te)
    {
        const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        const float radius = 6.0f;
        g.setColour (te.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (r, radius);
    }

    void ModernLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& te)
    {
        if (te.isReadOnly()) return;
        const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        const float radius = 6.0f;
        const auto outline = te.hasKeyboardFocus (true)
            ? te.findColour (juce::TextEditor::focusedOutlineColourId)
            : te.findColour (juce::TextEditor::outlineColourId);
        g.setColour (outline);
        g.drawRoundedRectangle (r, radius, 1.0f);
    }

    void ModernLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar& bar,
                                           int x, int y, int width, int height,
                                           bool isScrollbarVertical,
                                           int thumbStartPosition, int thumbSize,
                                           bool isMouseOver, bool /*isMouseDown*/)
    {
        const auto baseRect = isScrollbarVertical
            ? juce::Rectangle<float> ((float) x + width * 0.25f, (float) thumbStartPosition,
                                       width * 0.5f, (float) thumbSize)
            : juce::Rectangle<float> ((float) thumbStartPosition, (float) y + height * 0.25f,
                                       (float) thumbSize, height * 0.5f);

        auto col = bar.findColour (juce::ScrollBar::thumbColourId);
        if (isMouseOver) col = col.brighter (0.3f);
        g.setColour (col);
        g.fillRoundedRectangle (baseRect, juce::jmin (baseRect.getWidth(), baseRect.getHeight()) * 0.5f);
    }
}
