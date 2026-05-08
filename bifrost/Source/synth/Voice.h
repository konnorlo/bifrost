#pragma once

#include <JuceHeader.h>
#include <array>
#include "model/TimbreModel.h"
#include "synth/VoiceRenderParameters.h"
#include "synth/AdditiveOscBank.h"
#include "synth/NoiseBank.h"
#include "synth/ResonatorBank.h"

class Voice
{
public:
    void prepare(double sampleRate, int blockSize, int maxHarmonics, int maxResonators);
    void start(int midiNote, float velocity, std::shared_ptr<const TimbreModel> model, uint32_t noteSerial);
    void stop();
    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return releasing; }
    int getMidiNote() const noexcept { return note; }
    float getAgeSeconds() const noexcept { return ageSeconds; }
    float getModelTimeNormalized() const noexcept;
    float getLastPeak() const noexcept { return lastPeak; }
    void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, const VoiceRenderParameters& params);

private:
    struct LayerRuntime
    {
        VoiceRenderParameters params;
        float phaseOffset = 0.0f;
        float leftGain = 1.0f;
        float rightGain = 1.0f;
        float movingColour = 1.0f;
    };

    bool active = false;
    bool releasing = false;
    int note = -1;
    float velocity = 0.0f;
    float midiHz = 440.0f;
    double fs = 44100.0;
    float ageSeconds = 0.0f;
    float currentModelTimeSeconds = 0.0f;
    float lastPeak = 0.0f;
    float envelopeLevel = 0.0f;
    float releaseStartLevel = 0.0f;
    float releaseAgeSeconds = 0.0f;
    float randomUnit = 0.0f;
    float randomStartNormalized = 0.0f;
    float lastBaseFrequencyHz[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
    int cachedHarmonicCount = 0;
    int cachedMaxHarmonics = 0;
    float cachedModelTimeSeconds = -1.0f;
    float cachedLoudness = 0.0f;
    std::vector<float> cachedHarmonicAmplitudes;
    std::shared_ptr<const TimbreModel> model;
    std::array<AdditiveOscBank, 4> additiveLayers;
    NoiseBank noise;
    ResonatorBank resonators;

    static float midiNoteToHz(int midiNote) noexcept;
    static float randomUnitFromSerial(uint32_t noteSerial, int midiNote, float velocity) noexcept;
    static float randomUnitFromLayer(float noteRandom, int layer, int salt) noexcept;
    void beginRelease() noexcept;
    float computeEnvelope(const VoiceRenderParameters& params) noexcept;
    static float shapeEnvelopeProgress(float progress, float curve) noexcept;
    float getEffectiveTimeStretch(const VoiceRenderParameters& params) const noexcept;
    int getEffectiveUnisonVoices(const VoiceRenderParameters& params) const noexcept;
    void updateBaseFrequencies(const VoiceRenderParameters& params);
    void updateStaticSustainCache(const VoiceRenderParameters& params);
    float getSustainModelTime(float durationSeconds, const VoiceRenderParameters& params) const noexcept;
    float getLayerModelTime(float stretchedAgeSeconds, float durationSeconds, const VoiceRenderParameters& params, int layer, int layerCount) const noexcept;
    float getLoopScanTime(float stretchedAgeSeconds, float durationSeconds, const VoiceRenderParameters& params, int layer, int layerCount, float phaseOffset) const noexcept;
    float getLayerPhaseOffset(int layer, int layerCount, const VoiceRenderParameters& params) const noexcept;
    VoiceRenderParameters getLayerParameters(int layer, int layerCount, const VoiceRenderParameters& params) const noexcept;
    float getLayerPan(int layer, int layerCount, const VoiceRenderParameters& params) const noexcept;
    float getModelTime(float stretchedAgeSeconds, float durationSeconds, const VoiceRenderParameters& params) const noexcept;
};
