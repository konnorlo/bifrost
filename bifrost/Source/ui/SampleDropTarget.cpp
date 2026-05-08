#include "ui/SampleDropTarget.h"
#include "PluginProcessor.h"
#include "ui/BifrostLookAndFeel.h"

SampleDropTarget::SampleDropTarget(BifrostAudioProcessor& p) : processor(p) {}

bool SampleDropTarget::isInterestedInFileDrag(const juce::StringArray& files)
{
    return !files.isEmpty();
}

void SampleDropTarget::filesDropped(const juce::StringArray& files, int, int)
{
    juce::File firstFile;
    bool accepted = false;

    for (const auto& path : files)
    {
        juce::File f(path);
        if (firstFile == juce::File())
            firstFile = f;

        if (isSupported(f))
        {
            processor.analysis().analyzeFileAsync(f);
            accepted = true;
            break;
        }
    }

    if (!accepted && firstFile != juce::File())
    {
        processor.analysis().rejectFile(firstFile, "Unsupported file type. Supported: " + AudioFileLoader::supportedExtensionsDescription());
    }
}

void SampleDropTarget::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(4.0f);
    BifrostLookAndFeel::fillPanel(g, r, 5.0f);

    auto status = processor.analysis().getStatus();
    auto textArea = getLocalBounds().reduced(18);
    textArea.removeFromBottom(34);

    auto header = textArea.removeFromTop(20);
    g.setColour(BifrostLookAndFeel::text().withAlpha(0.78f));
    g.setFont(BifrostLookAndFeel::uiFont(12.0f, true).withExtraKerningFactor(0.06f));
    g.drawFittedText("SAMPLE", header, juce::Justification::centredLeft, 1);

    auto dropBox = textArea.removeFromTop(40).reduced(0, 3).toFloat();
    g.setColour(BifrostLookAndFeel::background().withAlpha(0.18f));
    g.fillRoundedRectangle(dropBox, 4.0f);
    g.setColour(BifrostLookAndFeel::panelEdge().withAlpha(0.45f));
    g.drawRoundedRectangle(dropBox, 4.0f, 1.0f);
    g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.82f));
    g.setFont(BifrostLookAndFeel::uiFont(13.0f, true));
    g.drawFittedText("Drop audio here or click browse", dropBox.toNearestInt().reduced(10), juce::Justification::centred, 1);

    g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.58f));
    g.setFont(BifrostLookAndFeel::uiFont(10.5f));
    g.drawFittedText(AudioFileLoader::supportedExtensionsDescription(), textArea.removeFromTop(16), juce::Justification::centredLeft, 1);

    g.setFont(BifrostLookAndFeel::uiFont(13.0f));
    if (status.lastError.isNotEmpty())
    {
        g.setColour(juce::Colour(0xffff8b8b));
        g.drawFittedText(status.lastError, textArea, juce::Justification::centred, 3);
    }
    else if (status.hasLoadedAudio || status.hasModel)
    {
        g.setColour(BifrostLookAndFeel::text().withAlpha(0.88f));
        g.setFont(BifrostLookAndFeel::uiFont(14.0f, true));
        g.drawFittedText(status.fileName, textArea.removeFromTop(18), juce::Justification::centredLeft, 1);

        g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.82f));
        g.setFont(BifrostLookAndFeel::uiFont(12.0f));
        juce::String details = juce::String(status.durationSeconds, 2) + " s"
            + "  /  " + juce::String(status.sampleRate, 0) + " Hz"
            + "  /  " + juce::String(status.sourceChannels) + " ch";
        if (status.wasTruncated)
            details += "  /  first 60 s";
        g.drawFittedText(details, textArea.removeFromTop(16), juce::Justification::centredLeft, 1);

        if (status.hasModel)
        {
            juce::String rootLine = "root " + juce::String(status.detectedRootHz, 1) + " Hz";
            if (status.rootOverrideHz >= 20.0f)
                rootLine += " override";
            rootLine += "  /  pitch " + juce::String(status.pitchConfidence, 2)
                + "  /  harm " + juce::String(status.harmonicEnergyExplained, 2);
            g.drawFittedText(rootLine, textArea.removeFromTop(16), juce::Justification::centredLeft, 1);
            if (status.tempoBpm >= 20.0f)
            {
                const juce::String tempoLine = "tempo " + juce::String(status.tempoBpm, 1) + " BPM"
                    + "  /  conf " + juce::String(status.tempoConfidence, 2);
                g.drawFittedText(tempoLine, textArea.removeFromTop(16), juce::Justification::centredLeft, 1);
            }
            if (status.qualityBadge.isNotEmpty())
            {
                g.setColour(BifrostLookAndFeel::accent().withAlpha(0.90f));
                g.drawFittedText(status.qualityBadge, textArea.removeFromTop(16), juce::Justification::centredLeft, 1);
                g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.82f));
            }
        }
    }
    else
    {
        g.setColour(BifrostLookAndFeel::mutedText().withAlpha(0.72f));
        g.drawFittedText(status.stage, textArea.removeFromTop(18), juce::Justification::centredLeft, 1);
    }

    auto bar = getLocalBounds().reduced(18).removeFromBottom(42).withHeight(8).toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.fillRoundedRectangle(bar, 4.0f);
    g.setColour(status.lastError.isNotEmpty() ? juce::Colour(0xffff8b8b) : BifrostLookAndFeel::accent());
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * juce::jlimit(0.0f, 1.0f, status.progress)), 4.0f);
}

bool SampleDropTarget::isSupported(const juce::File& file) const
{
    return AudioFileLoader::isSupportedExtension(file);
}
