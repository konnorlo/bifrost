#include "ui/BifrostLookAndFeel.h"
#include "BinaryData.h"

namespace
{
juce::Typeface::Ptr rajdhaniRegular()
{
    static auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::RajdhaniRegular_ttf, BinaryData::RajdhaniRegular_ttfSize);
    return face;
}

juce::Typeface::Ptr rajdhaniSemiBold()
{
    static auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::RajdhaniSemiBold_ttf, BinaryData::RajdhaniSemiBold_ttfSize);
    return face;
}

juce::Typeface::Ptr orbitronMedium()
{
    static auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::OrbitronMedium_ttf, BinaryData::OrbitronMedium_ttfSize);
    return face;
}

void drawSoftShadow(juce::Graphics& g, juce::Rectangle<float> area, float corner, float alpha)
{
    g.setColour(juce::Colours::black.withAlpha(alpha));
    g.fillRoundedRectangle(area.translated(0.0f, 2.0f), corner);
}

void drawKnobTicks(juce::Graphics& g,
                   juce::Point<float> centre,
                   float radius,
                   float startAngle,
                   float endAngle,
                   int ticks,
                   juce::Colour colour)
{
    g.setColour(colour);
    for (int i = 0; i <= ticks; ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(ticks);
        const float a = startAngle + p * (endAngle - startAngle) - juce::MathConstants<float>::halfPi;
        const float inner = radius * (i % 5 == 0 ? 0.77f : 0.82f);
        const float outer = radius * 0.89f;
        g.drawLine({ centre.x + std::cos(a) * inner,
                     centre.y + std::sin(a) * inner,
                     centre.x + std::cos(a) * outer,
                     centre.y + std::sin(a) * outer },
                   i % 5 == 0 ? 1.1f : 0.75f);
    }
}
}

BifrostLookAndFeel::BifrostLookAndFeel()
{
    setColour(juce::Label::textColourId, text());
    setColour(juce::TextButton::buttonColourId, panelRaised());
    setColour(juce::TextButton::buttonOnColourId, cyanFill());
    setColour(juce::TextButton::textColourOffId, text());
    setColour(juce::TextButton::textColourOnId, text());
    setColour(juce::ComboBox::backgroundColourId, panelRaised());
    setColour(juce::ComboBox::outlineColourId, panelEdge());
    setColour(juce::ComboBox::textColourId, text());
    setColour(juce::ComboBox::arrowColourId, accent());
    setColour(juce::PopupMenu::backgroundColourId, panel());
    setColour(juce::PopupMenu::textColourId, text());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, cyanFill());
    setColour(juce::PopupMenu::highlightedTextColourId, text());
    setColour(juce::Slider::textBoxTextColourId, accent());
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::thumbColourId, accent());
    setColour(juce::Slider::trackColourId, accent());
}

void BifrostLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                          int x,
                                          int y,
                                          int width,
                                          int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(2.0f);
    const float size = std::min(bounds.getWidth(), bounds.getHeight());
    auto knob = bounds.withSizeKeepingCentre(size, size);
    const float centreX = knob.getCentreX();
    const float centreY = knob.getCentreY();
    const float radius = size * 0.48f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    drawSoftShadow(g, knob.reduced(size * 0.04f), radius, 0.34f);

    const auto centre = knob.getCentre();
    juce::ColourGradient outerGradient(juce::Colour(0xff303944), centreX, knob.getY(),
                                       juce::Colour(0xff05080c), centreX, knob.getBottom(), false);
    g.setGradientFill(outerGradient);
    g.fillEllipse(knob.reduced(size * 0.04f));
    g.setColour(juce::Colours::black.withAlpha(0.70f));
    g.drawEllipse(knob.reduced(size * 0.04f), 1.0f);

    drawKnobTicks(g, centre, radius, rotaryStartAngle, rotaryEndAngle, 34, accent().withAlpha(slider.isEnabled() ? 0.34f : 0.10f));

    auto cap = knob.reduced(size * 0.22f);
    juce::ColourGradient capGradient(juce::Colour(0xff293039), cap.getCentreX(), cap.getY(),
                                     juce::Colour(0xff12171d), cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill(capGradient);
    g.fillEllipse(cap);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawEllipse(cap, 1.0f);

    juce::Path baseArc;
    baseArc.addCentredArc(centreX, centreY, radius * 0.78f, radius * 0.78f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff5b6774).withAlpha(slider.isEnabled() ? 0.48f : 0.18f));
    g.strokePath(baseArc, juce::PathStrokeType(std::max(1.2f, size * 0.025f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius * 0.78f, radius * 0.78f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(accent().withAlpha(slider.isEnabled() ? 0.96f : 0.25f));
    g.strokePath(valueArc, juce::PathStrokeType(std::max(2.0f, size * 0.045f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float pointerLength = radius * 0.50f;
    const float pointerThickness = std::max(2.0f, size * 0.045f);
    juce::Line<float> pointer({ centreX, centreY },
                              { centreX + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLength,
                                centreY + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLength });
    g.setColour((slider.isEnabled() ? accent() : mutedText()).withAlpha(0.95f));
    g.drawLine(pointer, pointerThickness);
    g.setColour(juce::Colours::white.withAlpha(slider.isEnabled() ? 0.34f : 0.12f));
    g.drawEllipse(cap.reduced(1.0f), 1.0f);
}

void BifrostLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                          int x,
                                          int y,
                                          int width,
                                          int height,
                                          float sliderPos,
                                          float minSliderPos,
                                          float maxSliderPos,
                                          const juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos);

    auto area = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(2.0f);
    if (style == juce::Slider::LinearVertical)
    {
        const float trackW = std::min(18.0f, area.getWidth() * 0.35f);
        auto track = area.withWidth(trackW).withX(area.getCentreX() - trackW * 0.5f).reduced(0.0f, 4.0f);
        drawSoftShadow(g, track.expanded(2.0f, 1.0f), 4.0f, 0.35f);
        juce::ColourGradient trackGradient(juce::Colour(0xff1b242c), track.getX(), track.getY(),
                                           juce::Colour(0xff05080c), track.getX(), track.getBottom(), false);
        g.setGradientFill(trackGradient);
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(panelEdge().withAlpha(0.88f));
        g.drawRoundedRectangle(track, 4.0f, 1.0f);

        g.setColour(mutedText().withAlpha(0.35f));
        for (int i = 1; i < 8; ++i)
        {
            const float yTick = juce::jmap(static_cast<float>(i), 0.0f, 8.0f, track.getY() + 9.0f, track.getBottom() - 9.0f);
            g.drawLine(track.getCentreX() - 3.5f, yTick, track.getCentreX() + 3.5f, yTick, 0.8f);
        }

        const float fillTop = std::clamp(sliderPos, track.getY(), track.getBottom());
        auto fill = track.withTop(fillTop).reduced(trackW * 0.33f, 7.0f);
        g.setColour(accent().withAlpha(slider.isEnabled() ? 0.95f : 0.22f));
        g.fillRoundedRectangle(fill, 3.0f);

        const float thumbW = std::min(42.0f, area.getWidth() - 4.0f);
        auto thumb = juce::Rectangle<float>(area.getCentreX() - thumbW * 0.5f, sliderPos - 9.0f, thumbW, 18.0f);
        drawSoftShadow(g, thumb, 3.0f, 0.35f);
        g.setColour(juce::Colour(0xff1d2933));
        g.fillRoundedRectangle(thumb, 3.0f);
        g.setColour(accent().withAlpha(0.92f));
        g.drawRoundedRectangle(thumb, 3.0f, 1.2f);
        g.setColour(text().withAlpha(0.74f));
        g.fillRoundedRectangle(thumb.reduced(thumbW * 0.24f, 7.4f), 1.0f);
        return;
    }

    auto track = area.withHeight(8.0f).withCentre(area.getCentre());
    fillPanel(g, track, 4.0f);
    g.setColour(accent().withAlpha(0.86f));
    g.fillRoundedRectangle(track.withRight(sliderPos), 4.0f);
    g.setColour(text().withAlpha(0.86f));
    g.fillEllipse(sliderPos - 6.0f, track.getCentreY() - 6.0f, 12.0f, 12.0f);
}

void BifrostLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const float corner = 4.0f;
    drawSoftShadow(g, r, corner, 0.22f);
    g.setColour(shouldDrawButtonAsDown ? cyanFill() : panelRaised());
    g.fillRoundedRectangle(r, corner);
    g.setColour((shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown ? accent() : panelEdge()).withAlpha(0.95f));
    g.drawRoundedRectangle(r, corner, shouldDrawButtonAsHighlighted ? 1.4f : 1.0f);
}

void BifrostLookAndFeel::drawButtonText(juce::Graphics& g,
                                        juce::TextButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    g.setFont(uiFont(13.0f, true));
    g.setColour((shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown ? accent() : text()).withAlpha(button.isEnabled() ? 0.95f : 0.32f));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(10, 0), juce::Justification::centred, 1);
}

void BifrostLookAndFeel::drawComboBox(juce::Graphics& g,
                                      int width,
                                      int height,
                                      bool isButtonDown,
                                      int,
                                      int,
                                      int,
                                      int,
                                      juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);
    fillPanel(g, r, 4.0f);
    g.setColour((isButtonDown ? accent() : panelEdge()).withAlpha(0.95f));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    juce::Path arrow;
    const float cx = r.getRight() - 18.0f;
    const float cy = r.getCentreY();
    arrow.startNewSubPath(cx - 5.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 4.0f);
    arrow.lineTo(cx + 5.0f, cy - 2.0f);
    g.setColour(accent().withAlpha(0.9f));
    g.strokePath(arrow, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void BifrostLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(12, 1, box.getWidth() - 34, box.getHeight() - 2);
    label.setFont(uiFont(13.0f, true));
    label.setJustificationType(juce::Justification::centredLeft);
}

juce::Font BifrostLookAndFeel::getLabelFont(juce::Label&)
{
    return uiFont(12.5f, true);
}

juce::Font BifrostLookAndFeel::uiFont(float height, bool semiBold)
{
    auto typeface = semiBold ? rajdhaniSemiBold() : rajdhaniRegular();
    if (typeface != nullptr)
        return juce::Font(juce::FontOptions(typeface).withHeight(height));

    return juce::Font(juce::FontOptions("Rajdhani", height, semiBold ? juce::Font::bold : juce::Font::plain));
}

juce::Font BifrostLookAndFeel::valueFont(float height)
{
    return uiFont(height, true);
}

juce::Font BifrostLookAndFeel::logoFont(float height)
{
    if (auto typeface = orbitronMedium())
        return juce::Font(juce::FontOptions(typeface).withHeight(height));

    return juce::Font(juce::FontOptions("Orbitron", height, juce::Font::plain));
}

void BifrostLookAndFeel::fillPanel(juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    juce::ColourGradient gradient(panelRaised().withAlpha(panelBoxTopOpacity), r.getX(), r.getY(),
                                  panel().withAlpha(panelBoxBottomOpacity), r.getX(), r.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(r, corner);
    g.setColour(panelEdge().withAlpha(panelBoxBorderOpacity));
    g.drawRoundedRectangle(r, corner, 1.0f);
}

void BifrostLookAndFeel::drawFineGrid(juce::Graphics& g, juce::Rectangle<float> r, float major, float minor)
{
    g.setColour(juce::Colours::white.withAlpha(0.030f));
    for (float x = r.getX(); x <= r.getRight(); x += minor)
        g.drawVerticalLine(static_cast<int>(std::round(x)), r.getY(), r.getBottom());
    for (float y = r.getY(); y <= r.getBottom(); y += minor)
        g.drawHorizontalLine(static_cast<int>(std::round(y)), r.getX(), r.getRight());

    g.setColour(juce::Colours::white.withAlpha(0.055f));
    for (float x = r.getX(); x <= r.getRight(); x += major)
        g.drawVerticalLine(static_cast<int>(std::round(x)), r.getY(), r.getBottom());
    for (float y = r.getY(); y <= r.getBottom(); y += major)
        g.drawHorizontalLine(static_cast<int>(std::round(y)), r.getX(), r.getRight());
}

void BifrostLookAndFeel::drawBifrostGlyph(juce::Graphics& g, juce::Rectangle<float> r, float alpha)
{
    const auto c = r.getCentre();
    const float w = r.getWidth();
    const float h = r.getHeight();
    const float stroke = std::max(1.1f, std::min(w, h) * 0.075f);
    const float towerTop = r.getY() + h * 0.14f;
    const float towerBottom = r.getBottom() - h * 0.16f;
    const float leftX = r.getX() + w * 0.27f;
    const float rightX = r.getRight() - w * 0.27f;

    g.setColour(text().withAlpha(alpha));
    juce::Path bridge;
    bridge.startNewSubPath(leftX, towerBottom);
    bridge.lineTo(leftX, towerTop);
    bridge.lineTo(c.x, r.getY() + h * 0.04f);
    bridge.lineTo(rightX, towerTop);
    bridge.lineTo(rightX, towerBottom);
    g.strokePath(bridge, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path arch;
    arch.startNewSubPath(leftX - w * 0.17f, r.getY() + h * 0.72f);
    arch.cubicTo(r.getX() + w * 0.02f, r.getY() + h * 0.68f,
                 r.getX() + w * 0.04f, r.getY() + h * 0.42f,
                 leftX - w * 0.02f, r.getY() + h * 0.34f);
    g.strokePath(arch, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto diamond = [&g](juce::Point<float> centre, float size)
    {
        juce::Path d;
        d.startNewSubPath(centre.x, centre.y - size);
        d.lineTo(centre.x + size, centre.y);
        d.lineTo(centre.x, centre.y + size);
        d.lineTo(centre.x - size, centre.y);
        d.closeSubPath();
        g.strokePath(d, juce::PathStrokeType(std::max(1.0f, size * 0.18f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    diamond(c.withY(r.getY() + h * 0.18f), std::min(w, h) * 0.12f);
    diamond(c.withY(r.getY() + h * 0.50f), std::min(w, h) * 0.15f);

    juce::ignoreUnused(towerBottom);
}
