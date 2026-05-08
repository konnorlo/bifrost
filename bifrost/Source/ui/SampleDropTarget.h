#pragma once

#include <JuceHeader.h>

class BifrostAudioProcessor;

class SampleDropTarget final : public juce::Component,
                               public juce::FileDragAndDropTarget
{
public:
    explicit SampleDropTarget(BifrostAudioProcessor& processor);

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void paint(juce::Graphics& g) override;

private:
    BifrostAudioProcessor& processor;
    bool isSupported(const juce::File& file) const;
};
