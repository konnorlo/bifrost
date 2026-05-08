#pragma once

#include <JuceHeader.h>
#include <vector>
#include "model/TimbreModel.h"
#include "synth/VoiceRenderParameters.h"

class AdditiveOscBank
{
public:
    void prepare(double sampleRate, int maxHarmonics);
    void reset();
    void setBaseFrequency(float midiHz);
    float renderSample(const TimbreModel& model,
                       float modelTimeSeconds,
                       const VoiceRenderParameters& params,
                       float phaseOffsetCycles = 0.0f,
                       float alternateModelTimeSeconds = -1.0f,
                       float alternateMix = 0.0f);
    float renderStaticSample(const std::vector<float>& harmonicAmplitudes,
                             float loudness,
                             const VoiceRenderParameters& params,
                             float phaseOffsetCycles = 0.0f);

private:
    double fs = 44100.0;
    float baseFrequencyHz = 0.0f;
    std::vector<float> sine;
    std::vector<float> cosine;
    std::vector<float> rotatorSine;
    std::vector<float> rotatorCosine;
};
