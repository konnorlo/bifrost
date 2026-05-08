#pragma once

#include <JuceHeader.h>
#include <vector>
#include "analysis/STFT.h"

struct MelPatch
{
    int melBins = 0;
    int frames = 0;
    std::vector<float> values;

    bool empty() const noexcept { return values.empty(); }
};

class TinyTimbreEncoder
{
public:
    TinyTimbreEncoder();

    bool isAvailable() const noexcept;
    juce::String getBackendName() const;
    juce::File getModelFile() const;

    MelPatch makeLogMelPatch(const MagnitudeSpectrogram& spec, int melBins = 64, int frames = 16) const;
    std::vector<float> encodeLogMelPatch(const float* data, int melBins, int frames) const;

private:
    juce::File modelFile;

    static juce::File discoverModelFile();
    static float hzToMel(float hz) noexcept;
    static float melToHz(float mel) noexcept;
};
