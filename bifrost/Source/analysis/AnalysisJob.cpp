#include "analysis/AnalysisJob.h"
#include "analysis/STFT.h"
#include "analysis/PitchTracker.h"
#include "analysis/HarmonicExtractor.h"
#include "analysis/NoiseExtractor.h"
#include "analysis/ResonatorExtractor.h"
#include "analysis/TempoAnalyzer.h"
#include "analysis/TimbreStateModel.h"
#include "ml/TinyTimbreEncoder.h"

std::shared_ptr<const TimbreModel> AnalysisJob::run(const LoadedAudioFile& loaded,
                                                    std::optional<float> rootOverrideHz,
                                                    ProgressCallback progress,
                                                    ShouldCancelCallback shouldCancel)
{
    auto model = std::make_shared<TimbreModel>();
    model->analysisSampleRate = loaded.sampleRate;
    model->sourcePath = loaded.file.getFullPathName();
    model->sourceHash = loaded.hash;
    model->durationSeconds = static_cast<float>(loaded.importedDurationSeconds);
    model->controlRateHz = 200.0f;

    auto mono = loaded.monoMid.getNumSamples() > 0 ? loaded.monoMid : makeMono(loaded.audio);

    if (progress) progress("Pitch", 0.45f);
    if (cancelled(shouldCancel)) return {};

    PitchTracker pitchTracker;
    auto pitch = pitchTracker.estimate(mono, loaded.sampleRate, model->controlRateHz, rootOverrideHz);
    model->pitchHz = std::move(pitch.pitchHz);
    model->pitchConfidenceCurve = std::move(pitch.confidenceCurve);
    model->detectedRootHz = pitch.rootHz;
    model->pitchConfidence = pitch.confidence;

    if (progress) progress("STFT", 0.58f);
    if (cancelled(shouldCancel)) return {};

    STFT stft;
    auto spectral = stft.analyzeMagnitude(mono, loaded.sampleRate, 4096, 1024);
    auto spectralFeatures = stft.computeSpectralFeatures(spectral, model->controlRateHz);

    TinyTimbreEncoder encoder;
    auto melPatch = encoder.makeLogMelPatch(spectral);
    if (!melPatch.empty())
    {
        model->timbreEmbedding = encoder.encodeLogMelPatch(melPatch.values.data(), melPatch.melBins, melPatch.frames);
        model->usedMlEmbedding = encoder.isAvailable();
        model->mlBackend = encoder.getBackendName();
    }

    if (progress) progress("Harmonics", 0.72f);
    if (cancelled(shouldCancel)) return {};

    HarmonicExtractor harmonicExtractor;
    model->harmonics = harmonicExtractor.extract(spectral, model->pitchHz, loaded.sampleRate, model->controlRateHz, 48);
    model->loudness = stft.computeRmsCurve(mono, loaded.sampleRate, model->controlRateHz);
    model->centroid = std::move(spectralFeatures.centroidHz);
    model->spectralFlux = std::move(spectralFeatures.spectralFlux);
    model->spectralFlatness = std::move(spectralFeatures.spectralFlatness);
    model->waveformPreview = stft.makeWaveformPreview(mono, loaded.sampleRate, 512);

    TempoAnalyzer tempoAnalyzer;
    const auto tempo = tempoAnalyzer.estimate(model->spectralFlux);
    model->tempoBpm = tempo.bpm;
    model->tempoConfidence = tempo.confidence;

    NoiseExtractor noiseExtractor;
    model->noise = noiseExtractor.extractPlaceholder(model->durationSeconds, model->controlRateHz, 8);

    ResonatorExtractor resonatorExtractor;
    model->resonators = resonatorExtractor.extractPlaceholder(model->durationSeconds, model->controlRateHz, 4);

    if (progress) progress("States", 0.88f);
    if (cancelled(shouldCancel)) return {};

    TimbreStateModel stateModel;
    model->states = stateModel.fitPlaceholder(*model, 6);

    return model;
}

juce::AudioBuffer<float> AnalysisJob::makeMono(const juce::AudioBuffer<float>& input)
{
    juce::AudioBuffer<float> mono(1, input.getNumSamples());
    mono.clear();
    for (int ch = 0; ch < input.getNumChannels(); ++ch)
        mono.addFrom(0, 0, input, ch, 0, input.getNumSamples(), 1.0f / static_cast<float>(input.getNumChannels()));
    return mono;
}

bool AnalysisJob::cancelled(const ShouldCancelCallback& shouldCancel)
{
    return shouldCancel && shouldCancel();
}
