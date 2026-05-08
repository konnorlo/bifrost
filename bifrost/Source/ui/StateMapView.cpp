#include "ui/StateMapView.h"
#include "PluginProcessor.h"
#include "ui/BifrostLookAndFeel.h"

StateMapView::StateMapView(BifrostAudioProcessor& p) : processor(p) {}

void StateMapView::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(4.0f);
    BifrostLookAndFeel::fillPanel(g, r, 5.0f);

    auto starArea = r.reduced(12.0f);
    auto fract = [](float value)
    {
        return value - std::floor(value);
    };

    for (int i = 0; i < 150; ++i)
    {
        const float xNorm = fract(std::sin(static_cast<float>(i) * 12.9898f) * 43758.5453f);
        const float yNorm = fract(std::sin(static_cast<float>(i) * 78.233f + 1.7f) * 24634.6345f);
        const float glow = 0.08f + 0.22f * fract(std::sin(static_cast<float>(i) * 37.719f) * 19417.123f);
        const float size = 0.7f + 1.7f * fract(std::sin(static_cast<float>(i) * 9.31f) * 991.17f);
        g.setColour((i % 9 == 0 ? BifrostLookAndFeel::accent() : BifrostLookAndFeel::text()).withAlpha(glow));
        g.fillEllipse(starArea.getX() + starArea.getWidth() * xNorm,
                      starArea.getY() + starArea.getHeight() * yNorm,
                      size,
                      size);
    }

    struct GhostNode
    {
        const char* name;
        float x;
        float y;
    };

    const GhostNode ghostNodes[] {
        { "Body", -0.54f, -0.10f },
        { "Air", -0.18f, -0.42f },
        { "Metal", 0.25f, -0.30f },
        { "Motion", 0.52f, 0.02f },
        { "Transient", 0.25f, 0.38f },
        { "Noise", -0.24f, 0.36f },
        { "Sustain", -0.55f, 0.20f },
    };
    constexpr int numGhostNodes = static_cast<int>(sizeof(ghostNodes) / sizeof(ghostNodes[0]));

    auto ghostPoint = [&r](const GhostNode& node)
    {
        const auto centre = r.getCentre();
        const float radius = std::min(r.getWidth(), r.getHeight()) * 0.34f;
        return juce::Point<float>(centre.x + node.x * radius, centre.y + node.y * radius);
    };

    for (int i = 0; i < numGhostNodes; ++i)
    {
        const auto a = ghostPoint(ghostNodes[i]);
        const auto b = ghostPoint(ghostNodes[(i + 1) % numGhostNodes]);
        g.setColour(BifrostLookAndFeel::accent().withAlpha(0.08f));
        g.drawLine({ a, b }, 1.0f);
    }

    for (const auto& node : ghostNodes)
    {
        const auto p = ghostPoint(node);
        g.setColour(BifrostLookAndFeel::accent().withAlpha(0.10f));
        g.fillEllipse(p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
        g.setColour(BifrostLookAndFeel::accent().withAlpha(0.28f));
        g.drawEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f, 1.2f);
        g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.34f));
        g.setFont(BifrostLookAndFeel::uiFont(10.5f, true));
        g.drawFittedText(node.name,
                         juce::Rectangle<int>(static_cast<int>(p.x + 8.0f), static_cast<int>(p.y - 8.0f), 62, 16),
                         juce::Justification::centredLeft,
                         1);
    }

    g.setColour(BifrostLookAndFeel::text().withAlpha(0.11f));
    g.setFont(BifrostLookAndFeel::uiFont(16.0f, true).withExtraKerningFactor(0.12f));
    g.drawFittedText("TIMBRE / STATE MAP", r.toNearestInt().reduced(12), juce::Justification::centred, 1);

    auto model = processor.getActiveModel();
    const auto center = r.getCentre();
    const float radius = std::min(r.getWidth(), r.getHeight()) * 0.36f;

    if (!model || model->states.stateCount == 0)
        return;

    for (int from = 0; from < model->states.stateCount; ++from)
    {
        const auto& a = model->states.states[static_cast<size_t>(from)];
        const juce::Point<float> start(center.x + a.x * radius, center.y + a.y * radius);
        for (int to = 0; to < model->states.stateCount; ++to)
        {
            if (from == to)
                continue;

            const float weight = model->states.transition(from, to);
            if (weight < 0.08f)
                continue;

            const auto& b = model->states.states[static_cast<size_t>(to)];
            const juce::Point<float> end(center.x + b.x * radius, center.y + b.y * radius);
            g.setColour(BifrostLookAndFeel::accent().withAlpha(std::clamp(weight * 0.30f, 0.045f, 0.22f)));
            g.drawLine(juce::Line<float>(start, end), 1.0f + weight * 2.0f);
        }
    }

    for (int i = 0; i < model->states.stateCount; ++i)
    {
        const auto& s = model->states.states[static_cast<size_t>(i)];
        const float x = center.x + s.x * radius;
        const float y = center.y + s.y * radius;
        const float size = 8.0f + 18.0f * s.usage;
        if (i == selectedState)
        {
            g.setColour(BifrostLookAndFeel::accent().withAlpha(0.16f));
            g.fillEllipse(x - size * 1.25f, y - size * 1.25f, size * 2.5f, size * 2.5f);
            g.setColour(BifrostLookAndFeel::accent().withAlpha(0.55f));
            g.drawEllipse(x - size, y - size, size * 2.0f, size * 2.0f, 1.2f);
        }

        g.setColour(BifrostLookAndFeel::accent().withAlpha(0.24f));
        g.fillEllipse(x - size * 0.72f, y - size * 0.72f, size * 1.44f, size * 1.44f);
        g.setColour(juce::Colour::fromFloatRGBA(0.18f + s.brightness * 0.35f, 0.60f + s.noisiness * 0.20f, 1.0f, 0.92f));
        g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
        g.setColour(BifrostLookAndFeel::background().withAlpha(0.72f));
        g.fillEllipse(x - size * 0.22f, y - size * 0.22f, size * 0.44f, size * 0.44f);

    }

    const auto voices = processor.getVoiceActivities();
    for (const auto& voice : voices)
    {
        if (!voice.active || model->states.stateCount <= 0)
            continue;

        const float scaled = voice.modelTimeNormalized * static_cast<float>(std::max(1, model->states.stateCount - 1));
        const int i0 = std::clamp(static_cast<int>(std::floor(scaled)), 0, model->states.stateCount - 1);
        const int i1 = std::clamp(i0 + 1, 0, model->states.stateCount - 1);
        const float frac = scaled - static_cast<float>(i0);
        const auto& a = model->states.states[static_cast<size_t>(i0)];
        const auto& b = model->states.states[static_cast<size_t>(i1)];
        const float nx = a.x + (b.x - a.x) * frac;
        const float ny = a.y + (b.y - a.y) * frac;
        const float x = center.x + nx * radius;
        const float y = center.y + ny * radius;
        const float hue = std::fmod(static_cast<float>(std::max(0, voice.midiNote)) / 24.0f, 1.0f);
        const float size = 6.0f + 10.0f * std::clamp(voice.level * 8.0f, 0.0f, 1.0f);

        g.setColour(juce::Colour::fromHSV(hue, 0.55f, 1.0f, 0.95f));
        g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
        g.setColour(juce::Colours::black.withAlpha(0.38f));
        g.drawEllipse(x - size * 0.5f, y - size * 0.5f, size, size, 1.0f);
    }
}

void StateMapView::mouseDown(const juce::MouseEvent& event)
{
    auto model = processor.getActiveModel();
    if (!model || model->states.stateCount <= 0)
        return;

    auto r = getLocalBounds().toFloat().reduced(4.0f);
    const auto center = r.getCentre();
    const float radius = std::min(r.getWidth(), r.getHeight()) * 0.36f;
    float bestDistance = std::numeric_limits<float>::max();
    int bestState = -1;

    for (int i = 0; i < model->states.stateCount; ++i)
    {
        const auto& state = model->states.states[static_cast<size_t>(i)];
        const juce::Point<float> point(center.x + state.x * radius, center.y + state.y * radius);
        const float distance = point.getDistanceFrom(event.position);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestState = i;
        }
    }

    if (bestDistance <= 36.0f)
    {
        selectedState = bestState;
        repaint();
    }
}
