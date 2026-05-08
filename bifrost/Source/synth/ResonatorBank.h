#pragma once

#include <JuceHeader.h>
#include <vector>
#include "model/TimbreModel.h"
#include "synth/VoiceRenderParameters.h"

class ResonatorBank
{
public:
    void prepare(double sampleRate, int maxResonators);
    void reset();
    float process(float input, const TimbreModel& model, float modelTimeSeconds, const VoiceRenderParameters& params);

private:
    double fs = 44100.0;
    std::vector<juce::dsp::StateVariableTPTFilter<float>> filters;
};
