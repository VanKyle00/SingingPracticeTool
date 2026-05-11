#pragma once

#include <JuceHeader.h>

namespace sp
{
    // App-wide LookAndFeel. Dark theme with a calm teal accent — set in Main.cpp
    // via juce::LookAndFeel::setDefaultLookAndFeel.
    class ModernLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        ModernLookAndFeel();
        ~ModernLookAndFeel() override = default;

        // Palette — also exposed so paint() overrides in tabs can use them
        // without duplicating literals.
        static constexpr juce::uint32 colBackground     = 0xFF16181D;
        static constexpr juce::uint32 colSurface        = 0xFF1F232B;
        static constexpr juce::uint32 colSurfaceRaised  = 0xFF2A2F3A;
        static constexpr juce::uint32 colBorder         = 0xFF353A45;
        static constexpr juce::uint32 colInputBg        = 0xFF1A1D24;
        static constexpr juce::uint32 colTextPrimary    = 0xFFE8EAEE;
        static constexpr juce::uint32 colTextSecondary  = 0xFFA0A8B6;
        static constexpr juce::uint32 colTextMuted      = 0xFF6A7384;
        static constexpr juce::uint32 colAccent         = 0xFF59C7CC;
        static constexpr juce::uint32 colAccentHover    = 0xFF6BD6DA;
        static constexpr juce::uint32 colAccentDim      = 0xFF3F8B8F;
        static constexpr juce::uint32 colSuccess        = 0xFF6FCF97;
        static constexpr juce::uint32 colError          = 0xFFEB5757;

        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;

        void drawButtonBackground (juce::Graphics&, juce::Button&,
                                   const juce::Colour& bg,
                                   bool isHighlighted, bool isDown) override;

        void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                               bool isHighlighted, bool isDown) override;

        void drawTickBox (juce::Graphics&, juce::Component&,
                          float x, float y, float w, float h,
                          bool ticked, bool isEnabled,
                          bool isHighlighted, bool isDown) override;

        void drawComboBox (juce::Graphics&, int width, int height,
                           bool isDown, int buttonX, int buttonY,
                           int buttonW, int buttonH,
                           juce::ComboBox&) override;

        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;

        void drawProgressBar (juce::Graphics&, juce::ProgressBar&,
                              int width, int height,
                              double progress, const juce::String& textToShow) override;

        void drawTabButton (juce::TabBarButton&, juce::Graphics&,
                            bool isHighlighted, bool isDown) override;
        void drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&,
                                           int width, int height) override;
        int  getTabButtonBestWidth (juce::TabBarButton&, int tabDepth) override;

        void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
        void drawTextEditorOutline    (juce::Graphics&, int w, int h, juce::TextEditor&) override;

        void drawScrollbar (juce::Graphics&, juce::ScrollBar&,
                            int x, int y, int width, int height,
                            bool isScrollbarVertical,
                            int thumbStartPosition, int thumbSize,
                            bool isMouseOver, bool isMouseDown) override;

    private:
        void installColourIds();
    };
}
