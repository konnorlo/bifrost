#pragma once

#include <JuceHeader.h>
#include <vector>
#include "model/TimbreModel.h"
#include "synth/VoiceRenderParameters.h"

class NoiseBank
{
public:
    void prepare(double sampleRate, int maxBands);
    void reset();
    float renderSample(const TimbreModel& model, float modelTimeSeconds, const VoiceRenderParameters& params);

private:
    juce::Random rng;
    double fs = 44100.0;
    float last = 0.0f;
    std::vector<juce::dsp::StateVariableTPTFilter<float>> filters;
    std::vector<float> lastCenterHz;
};
