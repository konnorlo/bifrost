#pragma once

#include <JuceHeader.h>

class BifrostLookAndFeel final : public juce::LookAndFeel_V4 {
public:
  BifrostLookAndFeel();

  void drawRotarySlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &) override;

  void drawLinearSlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider &) override;

  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

  void drawButtonText(juce::Graphics &, juce::TextButton &,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

  void drawComboBox(juce::Graphics &, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &) override;

  void positionComboBoxText(juce::ComboBox &, juce::Label &) override;
  juce::Font getLabelFont(juce::Label &) override;

  static juce::Colour background() noexcept { return juce::Colour(0xff060b10); }
  static juce::Colour panel() noexcept { return juce::Colour(0xff0b1219); }
  static juce::Colour panelRaised() noexcept {
    return juce::Colour(0xff101923);
  }
  static juce::Colour panelEdge() noexcept { return juce::Colour(0xff21313d); }
  static juce::Colour text() noexcept { return juce::Colour(0xffe6eef7); }
  static juce::Colour mutedText() noexcept { return juce::Colour(0xff8fa0ad); }
  static juce::Colour accent() noexcept { return juce::Colour(0xff2cbcff); }
  static juce::Colour accentSoft() noexcept { return juce::Colour(0xff126a95); }
  static juce::Colour cyanFill() noexcept { return juce::Colour(0xff0d3144); }

  // Manual UI opacity controls. Edit these values, rebuild, and rescan the
  // plugin.
  static constexpr float backgroundImageOpacity = 0.30f;
  static constexpr float outerBoxFillOpacity = 0.46f;
  static constexpr float outerBoxBorderOpacity = 0.36f;
  static constexpr float panelBoxTopOpacity = 0.30f;
  static constexpr float panelBoxBottomOpacity = 0.18f;
  static constexpr float panelBoxBorderOpacity = 0.54f;

  static juce::Font uiFont(float height, bool semiBold = false);
  static juce::Font valueFont(float height);
  static juce::Font logoFont(float height);
  static void fillPanel(juce::Graphics &, juce::Rectangle<float>,
                        float corner = 5.0f);
  static void drawFineGrid(juce::Graphics &, juce::Rectangle<float>,
                           float major = 48.0f, float minor = 12.0f);
  static void drawBifrostGlyph(juce::Graphics &, juce::Rectangle<float>,
                               float alpha = 1.0f);

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BifrostLookAndFeel)
};
