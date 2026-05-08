#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "model/Curve.h"
#include "model/StateGraph.h"
#include "analysis/STFT.h"

struct HarmonicModel
{
    int harmonicCount = 0;
    float energyExplained = 0.0f;
    std::vector<Curve> harmonicAmplitudes;
};

struct NoiseModel
{
    int bandCount = 0;
    std::vector<float> bandCenterHz;
    std::vector<Curve> bandAmplitudes;
};

struct ResonatorModel
{
    int resonatorCount = 0;
    std::vector<float> frequencyHz;
    std::vector<float> q;
    std::vector<Curve> gains;
};

struct TransientModel
{
    float attackEndSeconds = 0.05f;
    Curve attackEnergy;
};

struct TimbreModel
{
    int modelFormatVersion = 1;
    double analysisSampleRate = 44100.0;
    float controlRateHz = 200.0f;
    float durationSeconds = 0.0f;
    float detectedRootHz = 440.0f;
    float pitchConfidence = 0.0f;
    float tempoBpm = 0.0f;
    float tempoConfidence = 0.0f;
    juce::String sourcePath;
    juce::String sourceHash;
    bool usedMlEmbedding = false;
    juce::String mlBackend;
    std::vector<float> timbreEmbedding;

    Curve loudness;
    Curve centroid;
    Curve spectralFlux;
    Curve spectralFlatness;
    WaveformPreview waveformPreview;
    Curve pitchHz;
    Curve pitchConfidenceCurve;
    HarmonicModel harmonics;
    NoiseModel noise;
    ResonatorModel resonators;
    TransientModel transient;
    StateGraph states;

    bool isUsable() const noexcept { return durationSeconds > 0.0f && harmonics.harmonicCount > 0; }
};

using TimbreModelPtr = std::shared_ptr<const TimbreModel>;
