#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "model/TimbreModel.h"
#include "synth/Voice.h"
#include "synth/VoiceRenderParameters.h"

struct VoiceActivity
{
    bool active = false;
    int midiNote = -1;
    float modelTimeNormalized = 0.0f;
    float level = 0.0f;
};

class VoiceManager
{
public:
    static constexpr int maxVoices = 12;

    VoiceManager();
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void render(juce::AudioBuffer<float>& buffer,
                juce::MidiBuffer& midi,
                std::shared_ptr<const TimbreModel> model,
                const VoiceRenderParameters& params);
    std::array<VoiceActivity, maxVoices> getVoiceActivities() const noexcept;

private:
    std::array<Voice, maxVoices> voices;
    std::array<std::atomic<int>, maxVoices> voiceActive;
    std::array<std::atomic<int>, maxVoices> voiceNote;
    std::array<std::atomic<float>, maxVoices> voiceTimeNormalized;
    std::array<std::atomic<float>, maxVoices> voiceLevel;
    double fs = 44100.0;
    int blockSize = 512;
    uint32_t noteSerial = 1;

    Voice& chooseVoice(int voiceLimit);
    void noteOff(int note, int voiceLimit);
    int countActiveVoices(int voiceLimit) const noexcept;
    void renderVoices(juce::AudioBuffer<float>& buffer,
                      int startSample,
                      int numSamples,
                      int voiceLimit,
                      const VoiceRenderParameters& params);
    void publishVoiceActivity(int index) noexcept;
};
