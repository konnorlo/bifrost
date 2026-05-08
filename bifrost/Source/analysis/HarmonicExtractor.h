#pragma once

#include <JuceHeader.h>
#include "analysis/STFT.h"
#include "model/TimbreModel.h"

class HarmonicExtractor
{
public:
    HarmonicModel extract(const MagnitudeSpectrogram& spec,
                          const Curve& pitchHz,
                          double sampleRate,
                          float controlRateHz,
                          int harmonicCount) const;

    Curve extractLoudness(const juce::AudioBuffer<float>& mono, double sampleRate, float controlRateHz) const;
    Curve extractCentroid(const MagnitudeSpectrogram& spec, double sampleRate, float controlRateHz) const;
};
