#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <optional>

#include "analysis/AnalysisController.h"
#include "model/TimbreModel.h"
#include "synth/VoiceManager.h"

class BifrostAudioProcessor final : public juce::AudioProcessor
{
public:
    BifrostAudioProcessor();
    ~BifrostAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& parameters() { return apvts; }
    const juce::AudioProcessorValueTreeState& parameters() const { return apvts; }

    AnalysisController& analysis() { return analysisController; }
    std::shared_ptr<const TimbreModel> getActiveModel() const;
    std::array<VoiceActivity, VoiceManager::maxVoices> getVoiceActivities() const noexcept;
    void installModel(std::shared_ptr<const TimbreModel> model);
    std::optional<float> getRootOverrideHz() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static void applySoftLimiter(juce::AudioBuffer<float>& buffer) noexcept;
    void cacheParameterPointers();

    juce::AudioProcessorValueTreeState apvts;
    VoiceManager voiceManager;
    AnalysisController analysisController;

    std::shared_ptr<const TimbreModel> activeModel;
    std::array<std::shared_ptr<const TimbreModel>, 8> retiredModels;
    size_t retiredModelIndex = 0;

    std::atomic<float>* bodyParam = nullptr;
    std::atomic<float>* airParam = nullptr;
    std::atomic<float>* metalParam = nullptr;
    std::atomic<float>* brightnessParam = nullptr;
    std::atomic<float>* motionParam = nullptr;
    std::atomic<float>* inertiaParam = nullptr;
    std::atomic<float>* mutationParam = nullptr;
    std::atomic<float>* transientParam = nullptr;
    std::atomic<float>* formantLockParam = nullptr;
    std::atomic<float>* timeStretchParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* adsrCurveParam = nullptr;
    std::atomic<float>* velocitySensitivityParam = nullptr;
    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* stereoWidthParam = nullptr;
    std::atomic<float>* stereoReconstructionParam = nullptr;
    std::atomic<float>* phaseOffsetParam = nullptr;
    std::atomic<float>* phaseRandomParam = nullptr;
    std::atomic<float>* timeSyncParam = nullptr;
    std::atomic<float>* rootRandomParam = nullptr;
    std::atomic<float>* randomDirectionParam = nullptr;
    std::atomic<float>* loopStartParam = nullptr;
    std::atomic<float>* loopEndParam = nullptr;
    std::atomic<float>* unisonVoicesParam = nullptr;
    std::atomic<float>* unisonDetuneParam = nullptr;
    std::atomic<float>* rootOverrideHzParam = nullptr;
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* qualityParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BifrostAudioProcessor)
};
