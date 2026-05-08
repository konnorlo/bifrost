#pragma once

#include <JuceHeader.h>
#include <vector>

#include "model/Curve.h"

struct MagnitudeSpectrogram
{
    int fftSize = 0;
    int hopSize = 0;
    double sampleRate = 0.0;
    int sourceSampleCount = 0;
    int frameCount = 0;
    int binCount = 0;
    std::vector<float> magnitude;

    float get(int frame, int bin) const noexcept
    {
        return magnitude[static_cast<size_t>(frame * binCount + bin)];
    }

    bool isValid() const noexcept { return fftSize > 0 && hopSize > 0 && sampleRate > 0.0 && frameCount > 0 && binCount > 0; }
    float getFrameTimeSeconds(int frame) const noexcept { return static_cast<float>(frame * hopSize / sampleRate); }
    float getBinFrequencyHz(int bin) const noexcept { return static_cast<float>(bin * sampleRate / fftSize); }
    float durationSeconds() const noexcept { return sampleRate > 0.0 ? static_cast<float>(sourceSampleCount / sampleRate) : 0.0f; }
};

struct WaveformPreview
{
    double sampleRate = 0.0;
    float durationSeconds = 0.0f;
    int samplesPerPoint = 0;
    std::vector<float> minimum;
    std::vector<float> maximum;
    std::vector<float> rms;

    int pointCount() const noexcept { return static_cast<int>(rms.size()); }
};

struct SpectralFeatureCurves
{
    Curve centroidHz;
    Curve spectralFlux;
    Curve spectralFlatness;
};

class STFT
{
public:
    MagnitudeSpectrogram analyzeMagnitude(const juce::AudioBuffer<float>& mono,
                                          double sampleRate,
                                          int fftSize,
                                          int hopSize) const;

    WaveformPreview makeWaveformPreview(const juce::AudioBuffer<float>& mono,
                                        double sampleRate,
                                        int maxPoints) const;

    Curve computeRmsCurve(const juce::AudioBuffer<float>& mono,
                          double sampleRate,
                          float controlRateHz) const;

    SpectralFeatureCurves computeSpectralFeatures(const MagnitudeSpectrogram& spec,
                                                  float controlRateHz) const;

private:
    static bool isPowerOfTwo(int value) noexcept;
    static int getControlFrameCount(float durationSeconds, float controlRateHz) noexcept;
};
