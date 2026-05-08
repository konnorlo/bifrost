#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include "audio/AudioFileLoader.h"
#include "model/TimbreModel.h"

class BifrostAudioProcessor;
class AnalysisThread;

struct AnalysisStatus
{
    juce::String stage = "Idle";
    float progress = 0.0f;
    juce::String lastError;
    juce::File lastFile;
    juce::String fileName;
    juce::String decoderBackend;
    juce::String sourceHash;
    juce::String qualityBadge;
    double sampleRate = 0.0;
    double durationSeconds = 0.0;
    double originalDurationSeconds = 0.0;
    float rootOverrideHz = 0.0f;
    float detectedRootHz = 0.0f;
    float pitchConfidence = 0.0f;
    float harmonicEnergyExplained = 0.0f;
    float tempoBpm = 0.0f;
    float tempoConfidence = 0.0f;
    int sourceChannels = 0;
    bool wasTruncated = false;
    bool hasLoadedAudio = false;
    bool hasModel = false;
};

class AnalysisController
{
public:
    explicit AnalysisController(BifrostAudioProcessor& processor);
    ~AnalysisController();

    void analyzeFileAsync(const juce::File& file);
    void reanalyzeLastFile();
    void cancel();
    void rejectFile(const juce::File& file, juce::String error);

    AnalysisStatus getStatus() const;

private:
    friend class AnalysisThread;

    BifrostAudioProcessor& processor;
    AudioFileLoader loader;
    mutable juce::CriticalSection statusLock;
    AnalysisStatus status;
    std::unique_ptr<juce::Thread> worker;

    void setStatus(juce::String stage, float progress, juce::String error = {});
    void setLoadedAudioStatus(const LoadedAudioFile& loaded);
    void setReadyStatus(const TimbreModel& model);
    void setErrorStatus(const juce::File& file, juce::String error);
    static juce::String makeQualityBadge(const TimbreModel& model);
};
