#pragma once

#include <JuceHeader.h>

class BifrostAudioProcessor;

class WaveformView final : public juce::Component
{
public:
    explicit WaveformView(BifrostAudioProcessor& processor);
    void paint(juce::Graphics& g) override;

private:
    BifrostAudioProcessor& processor;
};
