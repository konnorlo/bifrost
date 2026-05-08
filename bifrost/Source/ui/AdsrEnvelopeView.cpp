#include "ui/AdsrEnvelopeView.h"
#include "PluginProcessor.h"
#include "ui/BifrostLookAndFeel.h"

AdsrEnvelopeView::AdsrEnvelopeView(BifrostAudioProcessor& p) : processor(p) {}

void AdsrEnvelopeView::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(4.0f);
    BifrostLookAndFeel::fillPanel(g, r, 5.0f);

    auto& params = processor.parameters();
    const float attack = params.getRawParameterValue("attack")->load();
    const float decay = params.getRawParameterValue("decay")->load();
    const float sustain = params.getRawParameterValue("sustain")->load();
    const float release = params.getRawParameterValue("release")->load();
    const float curve = params.getRawParameterValue("adsr_curve")->load();
    const float loopStart = std::clamp(params.getRawParameterValue("loop_start")->load(), 0.0f, 0.98f);
    const float loopEnd = std::clamp(params.getRawParameterValue("loop_end")->load(), loopStart + 0.01f, 1.0f);

    const float total = std::max(0.01f, attack + decay + release + 0.35f);
    auto graph = r.reduced(16.0f, 16.0f);
    graph.removeFromTop(14.0f);

    BifrostLookAndFeel::drawFineGrid(g, graph, 48.0f, 12.0f);

    auto pointFor = [&graph, total](float timeSeconds, float level)
    {
        return juce::Point<float>(graph.getX() + graph.getWidth() * std::clamp(timeSeconds / total, 0.0f, 1.0f),
                                  graph.getBottom() - graph.getHeight() * std::clamp(level, 0.0f, 1.0f));
    };

    auto shaped = [curve](float value)
    {
        const float power = juce::jmap(std::clamp(curve, 0.0f, 1.0f), 2.5f, 0.45f);
        return std::pow(std::clamp(value, 0.0f, 1.0f), power);
    };

    const float hold = 0.35f;
    const float t0 = 0.0f;
    const float t1 = attack;
    const float t2 = attack + decay;
    const float t3 = t2 + hold;
    const float t4 = t3 + release;

    juce::Path fill;
    fill.startNewSubPath(pointFor(t0, 0.0f));
    for (int i = 1; i <= 14; ++i)
    {
        const float p = static_cast<float>(i) / 14.0f;
        fill.lineTo(pointFor(t1 * p, shaped(p)));
    }
    for (int i = 1; i <= 14; ++i)
    {
        const float p = static_cast<float>(i) / 14.0f;
        fill.lineTo(pointFor(t1 + decay * p, 1.0f + (sustain - 1.0f) * shaped(p)));
    }
    fill.lineTo(pointFor(t3, sustain));
    for (int i = 1; i <= 14; ++i)
    {
        const float p = static_cast<float>(i) / 14.0f;
        fill.lineTo(pointFor(t3 + release * p, sustain * (1.0f - shaped(p))));
    }
    fill.lineTo(pointFor(t4, 0.0f).withY(graph.getBottom()));
    fill.closeSubPath();

    g.setColour(BifrostLookAndFeel::accentSoft().withAlpha(0.22f));
    g.fillPath(fill);

    juce::Path line;
    line.startNewSubPath(pointFor(t0, 0.0f));
    for (int i = 1; i <= 14; ++i)
    {
        const float p = static_cast<float>(i) / 14.0f;
        line.lineTo(pointFor(t1 * p, shaped(p)));
    }
    for (int i = 1; i <= 14; ++i)
    {
        const float p = static_cast<float>(i) / 14.0f;
        line.lineTo(pointFor(t1 + decay * p, 1.0f + (sustain - 1.0f) * shaped(p)));
    }
    line.lineTo(pointFor(t3, sustain));
    for (int i = 1; i <= 14; ++i)
    {
        const float p = static_cast<float>(i) / 14.0f;
        line.lineTo(pointFor(t3 + release * p, sustain * (1.0f - shaped(p))));
    }

    g.setColour(BifrostLookAndFeel::accent());
    g.strokePath(line, juce::PathStrokeType(2.0f));

    const auto points = { pointFor(t0, 0.0f), pointFor(t1, 1.0f), pointFor(t2, sustain), pointFor(t3, sustain), pointFor(t4, 0.0f) };
    for (const auto& point : points)
    {
        g.setColour(BifrostLookAndFeel::accent().withAlpha(0.20f));
        g.fillEllipse(point.x - 6.0f, point.y - 6.0f, 12.0f, 12.0f);
        g.setColour(BifrostLookAndFeel::accent());
        g.drawEllipse(point.x - 4.0f, point.y - 4.0f, 8.0f, 8.0f, 1.5f);
    }

    const float loopX0 = graph.getX() + graph.getWidth() * std::clamp(loopStart, 0.0f, 1.0f);
    const float loopX1 = graph.getX() + graph.getWidth() * std::clamp(loopEnd, loopStart, 1.0f);
    g.setColour(BifrostLookAndFeel::accent().withAlpha(0.18f));
    g.fillRect(juce::Rectangle<float>(loopX0, graph.getY(), loopX1 - loopX0, graph.getHeight()));
    g.setColour(BifrostLookAndFeel::accent().withAlpha(0.78f));
    g.drawVerticalLine(static_cast<int>(std::round(loopX0)), graph.getY(), graph.getBottom());
    g.drawVerticalLine(static_cast<int>(std::round(loopX1)), graph.getY(), graph.getBottom());

    g.setFont(BifrostLookAndFeel::uiFont(11.5f, true));
    g.setColour(BifrostLookAndFeel::text().withAlpha(0.76f));
    g.drawFittedText("ADSR", r.toNearestInt().reduced(10), juce::Justification::topLeft, 1);
    g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.76f));
    g.drawFittedText("Loop " + juce::String(loopStart, 2) + " - " + juce::String(loopEnd, 2),
                     r.toNearestInt().reduced(10),
                     juce::Justification::topRight,
                     1);
}
