#include "ui/WaveformView.h"
#include "PluginProcessor.h"
#include "ui/BifrostLookAndFeel.h"

WaveformView::WaveformView(BifrostAudioProcessor& p) : processor(p) {}

void WaveformView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    BifrostLookAndFeel::fillPanel(g, bounds, 5.0f);

    auto model = processor.getActiveModel();
    if (!model || model->waveformPreview.pointCount() <= 0)
    {
        auto center = bounds.reduced(12.0f);
        BifrostLookAndFeel::drawFineGrid(g, center, 44.0f, 11.0f);
        g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.64f));
        g.setFont(BifrostLookAndFeel::uiFont(12.0f, true).withExtraKerningFactor(0.06f));
        g.drawFittedText("WAVEFORM", center.toNearestInt(), juce::Justification::centred, 1);
        return;
    }

    auto header = bounds.reduced(12.0f, 8.0f).removeFromTop(16.0f);
    g.setFont(BifrostLookAndFeel::uiFont(10.5f, true).withExtraKerningFactor(0.04f));
    g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.86f));
    const auto status = processor.analysis().getStatus();
    g.drawFittedText(juce::String(status.sampleRate, 0) + " Hz", header.toNearestInt(), juce::Justification::centredLeft, 1);
    g.drawFittedText(model->sourcePath.isNotEmpty() ? juce::File(model->sourcePath).getFileName() : "Analyzed Source",
                     header.toNearestInt(),
                     juce::Justification::centred,
                     1);
    g.drawFittedText(juce::String(model->durationSeconds, 2) + " s", header.toNearestInt(), juce::Justification::centredRight, 1);

    auto waveArea = bounds.reduced(12.0f, 20.0f).withTop(header.getBottom() + 4.0f);
    const auto& preview = model->waveformPreview;
    const int points = preview.pointCount();
    const float centerY = waveArea.getCentreY();
    const float halfHeight = waveArea.getHeight() * 0.43f;

    BifrostLookAndFeel::drawFineGrid(g, waveArea, 48.0f, 12.0f);

    auto xForIndex = [&waveArea, points](int index)
    {
        return waveArea.getX() + waveArea.getWidth() * static_cast<float>(index) / static_cast<float>(std::max(1, points - 1));
    };

    auto xForTime = [&waveArea, duration = std::max(0.001f, model->durationSeconds)](float seconds)
    {
        return waveArea.getX() + waveArea.getWidth() * std::clamp(seconds / duration, 0.0f, 1.0f);
    };

    auto& params = processor.parameters();
    const float loopStartNorm = params.getRawParameterValue("loop_start")->load();
    const float loopEndNorm = params.getRawParameterValue("loop_end")->load();
    const float sustainStart = model->durationSeconds * std::clamp(loopStartNorm, 0.0f, 0.98f);
    const float sustainEnd = model->durationSeconds * std::clamp(loopEndNorm, loopStartNorm + 0.01f, 1.0f);
    const float attackEnd = std::clamp(model->transient.attackEndSeconds, 0.0f, model->durationSeconds);

    auto attackRect = waveArea.withRight(xForTime(attackEnd));
    auto sustainRect = waveArea.withLeft(xForTime(sustainStart)).withRight(xForTime(sustainEnd));

    g.setColour(BifrostLookAndFeel::accent().withAlpha(0.10f));
    g.fillRect(attackRect);
    g.setColour(BifrostLookAndFeel::accentSoft().withAlpha(0.28f));
    g.fillRect(sustainRect);

    for (float marker : { attackEnd, sustainStart, sustainEnd })
    {
        const float x = xForTime(marker);
        g.setColour(BifrostLookAndFeel::accent().withAlpha(0.52f));
        g.drawVerticalLine(static_cast<int>(std::round(x)), waveArea.getY(), waveArea.getBottom());
    }

    juce::Path body;
    for (int i = 0; i < points; ++i)
    {
        const float x = xForIndex(i);
        const float y = centerY - std::clamp(preview.rms[static_cast<size_t>(i)], 0.0f, 1.0f) * halfHeight;
        if (i == 0) body.startNewSubPath(x, y);
        else body.lineTo(x, y);
    }
    for (int i = points - 1; i >= 0; --i)
    {
        const float x = xForIndex(i);
        const float y = centerY + std::clamp(preview.rms[static_cast<size_t>(i)], 0.0f, 1.0f) * halfHeight;
        body.lineTo(x, y);
    }
    body.closeSubPath();

    g.setColour(BifrostLookAndFeel::accentSoft().withAlpha(0.33f));
    g.fillPath(body);

    juce::Path minimum;
    juce::Path maximum;
    for (int i = 0; i < points; ++i)
    {
        const auto index = static_cast<size_t>(i);
        const float x = xForIndex(i);
        const float yMin = centerY - std::clamp(preview.maximum[index], -1.0f, 1.0f) * halfHeight;
        const float yMax = centerY - std::clamp(preview.minimum[index], -1.0f, 1.0f) * halfHeight;
        if (i == 0)
        {
            maximum.startNewSubPath(x, yMin);
            minimum.startNewSubPath(x, yMax);
        }
        else
        {
            maximum.lineTo(x, yMin);
            minimum.lineTo(x, yMax);
        }
    }

    g.setColour(juce::Colour(0xffd9e8ff).withAlpha(0.82f));
    g.strokePath(maximum, juce::PathStrokeType(1.2f));
    g.strokePath(minimum, juce::PathStrokeType(1.2f));

    g.setFont(BifrostLookAndFeel::uiFont(10.5f, true));
    g.setColour(BifrostLookAndFeel::text().withAlpha(0.72f));
    if (sustainRect.getWidth() > 54.0f)
        g.drawFittedText("Loop", sustainRect.toNearestInt().reduced(4), juce::Justification::centred, 1);
}
