#pragma once

#include <JuceHeader.h>

class BifrostAudioProcessor;

class StateMapView final : public juce::Component
{
public:
    explicit StateMapView(BifrostAudioProcessor& processor);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    BifrostAudioProcessor& processor;
    int selectedState = -1;
};
