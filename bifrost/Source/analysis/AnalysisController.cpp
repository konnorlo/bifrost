#include "analysis/AnalysisController.h"
#include "analysis/AnalysisJob.h"
#include "PluginProcessor.h"

class AnalysisThread final : public juce::Thread
{
public:
    AnalysisThread(AnalysisController& ownerRef,
                   BifrostAudioProcessor& procRef,
                   AudioFileLoader& loaderRef,
                   juce::File fileToAnalyze,
                   std::optional<float> rootOverride)
        : juce::Thread("Bifrost Analysis"),
          owner(ownerRef),
          processor(procRef),
          loader(loaderRef),
          file(std::move(fileToAnalyze)),
          rootOverrideHz(rootOverride) {}

    void run() override
    {
        owner.setStatus("Loading", 0.08f);
        auto loaded = loader.load(file);

        if (threadShouldExit())
            return;

        if (!loaded.ok())
        {
            owner.setErrorStatus(file, loaded.error);
            return;
        }

        owner.setLoadedAudioStatus(loaded);

        if (threadShouldExit())
            return;

        owner.setStatus("Analyzing", 0.55f);

        AnalysisJob job;
        auto model = job.run(loaded,
                             rootOverrideHz,
                             [this](juce::String stage, float progress)
                             {
                                 owner.setStatus(std::move(stage), progress);
                             },
                             [this]
                             {
                                 return threadShouldExit();
                             });

        if (threadShouldExit())
            return;

        if (model)
        {
            processor.installModel(model);
            owner.setReadyStatus(*model);
        }
        else
        {
            owner.setErrorStatus(file, "Analysis did not produce a model");
        }
    }

private:
    AnalysisController& owner;
    BifrostAudioProcessor& processor;
    AudioFileLoader& loader;
    juce::File file;
    std::optional<float> rootOverrideHz;
};

AnalysisController::AnalysisController(BifrostAudioProcessor& p) : processor(p) {}
AnalysisController::~AnalysisController() { cancel(); }

void AnalysisController::analyzeFileAsync(const juce::File& file)
{
    cancel();
    {
        const juce::ScopedLock lock(statusLock);
        status = {};
        status.lastFile = file;
        status.stage = "Queued";
        status.progress = 0.0f;
        status.fileName = file.getFileName();
        if (auto rootOverrideHz = processor.getRootOverrideHz())
            status.rootOverrideHz = *rootOverrideHz;
    }
    worker = std::make_unique<AnalysisThread>(*this, processor, loader, file, processor.getRootOverrideHz());
    worker->startThread();
}

void AnalysisController::reanalyzeLastFile()
{
    juce::File file;
    {
        const juce::ScopedLock lock(statusLock);
        file = status.lastFile;
    }
    if (file.existsAsFile()) analyzeFileAsync(file);
}

void AnalysisController::cancel()
{
    if (worker)
    {
        worker->signalThreadShouldExit();
        worker->stopThread(1500);
        worker.reset();
    }
}

void AnalysisController::rejectFile(const juce::File& file, juce::String error)
{
    cancel();
    setErrorStatus(file, std::move(error));
}

AnalysisStatus AnalysisController::getStatus() const
{
    const juce::ScopedLock lock(statusLock);
    return status;
}

void AnalysisController::setStatus(juce::String stage, float progress, juce::String error)
{
    const juce::ScopedLock lock(statusLock);
    status.stage = std::move(stage);
    status.progress = progress;
    status.lastError = std::move(error);
}

void AnalysisController::setLoadedAudioStatus(const LoadedAudioFile& loaded)
{
    const juce::ScopedLock lock(statusLock);
    status.stage = "Decoded";
    status.progress = 0.35f;
    status.lastError = {};
    status.lastFile = loaded.file;
    status.fileName = loaded.file.getFileName();
    status.decoderBackend = loaded.decoderBackend;
    status.sourceHash = loaded.hash;
    status.sampleRate = loaded.sampleRate;
    status.durationSeconds = loaded.importedDurationSeconds;
    status.originalDurationSeconds = loaded.originalDurationSeconds;
    status.sourceChannels = loaded.originalNumChannels;
    status.wasTruncated = loaded.wasTruncated;
    status.hasLoadedAudio = true;
    status.hasModel = false;
}

void AnalysisController::setReadyStatus(const TimbreModel& model)
{
    const juce::ScopedLock lock(statusLock);
    status.stage = "Ready";
    status.progress = 1.0f;
    status.lastError = {};
    status.durationSeconds = model.durationSeconds;
    status.sampleRate = model.analysisSampleRate;
    status.sourceHash = model.sourceHash;
    status.detectedRootHz = model.detectedRootHz;
    status.pitchConfidence = model.pitchConfidence;
    status.harmonicEnergyExplained = model.harmonics.energyExplained;
    status.tempoBpm = model.tempoBpm;
    status.tempoConfidence = model.tempoConfidence;
    status.qualityBadge = makeQualityBadge(model);
    status.hasLoadedAudio = true;
    status.hasModel = true;
}

void AnalysisController::setErrorStatus(const juce::File& file, juce::String error)
{
    const juce::ScopedLock lock(statusLock);
    status = {};
    status.stage = "Error";
    status.progress = 0.0f;
    status.lastError = std::move(error);
    status.lastFile = file;
    status.fileName = file.getFileName();
}

juce::String AnalysisController::makeQualityBadge(const TimbreModel& model)
{
    if (model.pitchConfidence >= 0.78f && model.harmonics.energyExplained >= 0.12f)
        return model.usedMlEmbedding ? "Excellent tonal source + ML" : "Excellent tonal source";
    if (model.pitchConfidence >= 0.55f && model.harmonics.energyExplained >= 0.05f)
        return "Good synth source";
    if (model.pitchConfidence < 0.35f)
        return "Pitch uncertain, using assist layers";

    return "Noisy source, harmonizer may be unstable";
}
