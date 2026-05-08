#pragma once

#include <JuceHeader.h>

class BifrostAudioProcessor;

class AdsrEnvelopeView final : public juce::Component
{
public:
    explicit AdsrEnvelopeView(BifrostAudioProcessor& processor);
    void paint(juce::Graphics& g) override;

private:
    BifrostAudioProcessor& processor;
};
