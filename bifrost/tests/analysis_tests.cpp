#include <JuceHeader.h>

#include "BuildInfo.h"
#include "analysis/STFT.h"
#include "analysis/TempoAnalyzer.h"
#include "analysis/PitchTracker.h"
#include "analysis/HarmonicExtractor.h"
#include "analysis/AnalysisJob.h"
#include "analysis/TimbreStateModel.h"
#include "ml/TinyTimbreEncoder.h"
#include "model/Curve.h"
#include "model/PresetSerializer.h"
#include "synth/NoiseBank.h"
#include "synth/ResonatorBank.h"
#include "synth/VoiceManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace
{
constexpr double sampleRate = 48000.0;

VoiceRenderParameters makeRenderParams();

bool expect(bool condition, const std::string& message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

bool expectNear(float actual, float expected, float tolerance, const std::string& message)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::cerr << "FAIL: " << message << " actual=" << actual << " expected=" << expected << " tolerance=" << tolerance << '\n';
        return false;
    }

    return true;
}

juce::AudioBuffer<float> makeSine(float frequencyHz, float amplitude, double durationSeconds)
{
    const int samples = static_cast<int>(std::round(sampleRate * durationSeconds));
    juce::AudioBuffer<float> buffer(1, samples);
    auto* x = buffer.getWritePointer(0);

    for (int i = 0; i < samples; ++i)
    {
        const float phase = juce::MathConstants<float>::twoPi * frequencyHz * static_cast<float>(i / sampleRate);
        x[i] = amplitude * std::sin(phase);
    }

    return buffer;
}

juce::AudioBuffer<float> makeSineSweep(float startHz, float endHz, float amplitude, double durationSeconds)
{
    const int samples = static_cast<int>(std::round(sampleRate * durationSeconds));
    juce::AudioBuffer<float> buffer(1, samples);
    auto* x = buffer.getWritePointer(0);

    float phase = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        const float progress = samples > 1 ? static_cast<float>(i) / static_cast<float>(samples - 1) : 0.0f;
        const float frequencyHz = startHz * std::pow(endHz / startHz, progress);
        phase += juce::MathConstants<float>::twoPi * frequencyHz / static_cast<float>(sampleRate);
        x[i] = amplitude * std::sin(phase);
    }

    return buffer;
}

float averageCurve(const Curve& curve, size_t skip = 0)
{
    if (curve.values.size() <= skip)
        return 0.0f;

    double sum = 0.0;
    for (size_t i = skip; i < curve.values.size(); ++i)
        sum += curve.values[i];

    return static_cast<float>(sum / static_cast<double>(curve.values.size() - skip));
}

bool curveIsFinite(const Curve& curve)
{
    return std::all_of(curve.values.begin(), curve.values.end(), [](float value) { return std::isfinite(value); });
}

LoadedAudioFile makeLoadedAudio(juce::AudioBuffer<float> mono, double sourceSampleRate);

bool testCurveInterpolation()
{
    Curve curve;
    curve.sampleRateHz = 1.0f;
    curve.values = { 0.0f, 10.0f };

    bool ok = true;
    ok &= expectNear(curve.sample(0.0f), 0.0f, 1.0e-6f, "Curve samples first point");
    ok &= expectNear(curve.sample(0.5f), 5.0f, 1.0e-6f, "Curve linearly interpolates");
    ok &= expectNear(curve.sample(2.0f), 10.0f, 1.0e-6f, "Curve clamps past end");
    return ok;
}

bool testShortBufferStft()
{
    juce::AudioBuffer<float> buffer(1, 64);
    buffer.clear();

    STFT stft;
    const auto spec = stft.analyzeMagnitude(buffer, sampleRate, 1024, 256);

    bool ok = true;
    ok &= expect(spec.frameCount == 1, "Short buffer yields one zero-padded frame");
    ok &= expect(spec.binCount == 513, "Short buffer keeps expected bin count");
    ok &= expect(spec.magnitude.size() == static_cast<size_t>(spec.frameCount * spec.binCount), "Short buffer magnitude size matches dimensions");
    return ok;
}

bool testSineCentroidAndRms()
{
    STFT stft;
    constexpr float frequency = 1000.0f;
    constexpr float amplitude = 0.5f;
    auto sine = makeSine(frequency, amplitude, 0.25);
    const auto spec = stft.analyzeMagnitude(sine, sampleRate, 4096, 512);
    const auto features = stft.computeSpectralFeatures(spec, 100.0f);
    const auto rms = stft.computeRmsCurve(sine, sampleRate, 100.0f);

    bool ok = true;
    ok &= expect(spec.frameCount > 0, "Sine STFT has frames");
    ok &= expectNear(averageCurve(features.centroidHz, 2), frequency, 120.0f, "Sine centroid is near sine frequency");
    ok &= expectNear(averageCurve(rms, 2), amplitude / std::sqrt(2.0f), 0.05f, "Sine RMS is near analytical RMS");
    return ok;
}

bool testWhiteNoiseFeatures()
{
    juce::AudioBuffer<float> noise(1, static_cast<int>(sampleRate * 0.25));
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    auto* x = noise.getWritePointer(0);

    for (int i = 0; i < noise.getNumSamples(); ++i)
        x[i] = dist(rng);

    STFT stft;
    const auto spec = stft.analyzeMagnitude(noise, sampleRate, 2048, 512);
    const auto features = stft.computeSpectralFeatures(spec, 100.0f);
    const float centroid = averageCurve(features.centroidHz, 2);
    const float flatness = averageCurve(features.spectralFlatness, 2);

    bool ok = true;
    ok &= expect(centroid > 9000.0f && centroid < 15000.0f, "White noise centroid is near mid-spectrum");
    ok &= expect(flatness > 0.35f, "White noise spectral flatness is high");
    return ok;
}

bool testWaveformPreview()
{
    juce::AudioBuffer<float> buffer(1, 8);
    auto* x = buffer.getWritePointer(0);
    const float values[] = { -1.0f, -0.5f, 0.25f, 0.5f, -0.25f, 0.75f, -0.75f, 1.0f };
    std::copy(std::begin(values), std::end(values), x);

    STFT stft;
    const auto preview = stft.makeWaveformPreview(buffer, sampleRate, 4);

    bool ok = true;
    ok &= expect(preview.pointCount() == 4, "Waveform preview uses requested maximum points");
    ok &= expect(preview.samplesPerPoint == 2, "Waveform preview computes samples per point");
    ok &= expectNear(preview.minimum[0], -1.0f, 1.0e-6f, "Waveform preview min");
    ok &= expectNear(preview.maximum[3], 1.0f, 1.0e-6f, "Waveform preview max");
    ok &= expect(preview.rms[0] > 0.0f, "Waveform preview RMS is populated");
    return ok;
}

juce::AudioBuffer<float> makeSawLike(float frequencyHz, int harmonics, double durationSeconds)
{
    const int samples = static_cast<int>(std::round(sampleRate * durationSeconds));
    juce::AudioBuffer<float> buffer(1, samples);
    auto* x = buffer.getWritePointer(0);

    for (int i = 0; i < samples; ++i)
    {
        float value = 0.0f;
        for (int h = 1; h <= harmonics; ++h)
        {
            const float phase = juce::MathConstants<float>::twoPi * frequencyHz * static_cast<float>(h) * static_cast<float>(i / sampleRate);
            value += std::sin(phase) / static_cast<float>(h);
        }
        x[i] = 0.25f * value;
    }

    return buffer;
}

bool testPitchTrackerSine()
{
    constexpr float frequency = 220.0f;
    auto sine = makeSine(frequency, 0.7f, 0.35);

    PitchTracker tracker;
    const auto pitch = tracker.estimate(sine, sampleRate, 200.0f);

    bool ok = true;
    ok &= expectNear(pitch.rootHz, frequency, 5.0f, "Pitch tracker root follows sine");
    ok &= expect(pitch.confidence > 0.75f, "Pitch tracker confidence is high for sine");
    ok &= expectNear(averageCurve(pitch.pitchHz, 2), frequency, 6.0f, "Pitch tracker curve follows sine");
    return ok;
}

bool testPitchTrackerRootOverride()
{
    constexpr float detectedFrequency = 220.0f;
    constexpr float overrideFrequency = 329.63f;
    auto sine = makeSine(detectedFrequency, 0.7f, 0.35);

    PitchTracker tracker;
    const auto pitch = tracker.estimate(sine, sampleRate, 200.0f, overrideFrequency);

    bool ok = true;
    ok &= expectNear(pitch.rootHz, overrideFrequency, 0.1f, "Pitch tracker root override sets root");
    ok &= expectNear(averageCurve(pitch.pitchHz, 2), overrideFrequency, 0.1f, "Pitch tracker root override drives pitch curve");
    ok &= expect(pitch.confidence > 0.75f, "Pitch tracker still reports detection confidence under root override");
    return ok;
}

bool testPitchTrackerSineSweep()
{
    auto sweep = makeSineSweep(160.0f, 320.0f, 0.65f, 0.8);

    PitchTracker tracker;
    const auto pitch = tracker.estimate(sweep, sampleRate, 100.0f);
    const float earlyHz = pitch.pitchHz.sample(0.15f);
    const float lateHz = pitch.pitchHz.sample(0.65f);

    bool ok = true;
    ok &= expect(earlyHz > 150.0f && earlyHz < 230.0f, "Pitch tracker follows early sine sweep frequency");
    ok &= expect(lateHz > 260.0f && lateHz < 360.0f, "Pitch tracker follows late sine sweep frequency");
    ok &= expect(lateHz > earlyHz * 1.35f, "Pitch tracker curve rises across sine sweep");
    ok &= expect(pitch.confidence > 0.45f, "Pitch tracker keeps confidence on sine sweep");
    ok &= expect(curveIsFinite(pitch.pitchHz), "Pitch tracker sweep curve is finite");
    return ok;
}

bool testHarmonicExtractorSawStack()
{
    constexpr float rootHz = 220.0f;
    auto saw = makeSawLike(rootHz, 8, 0.35);

    STFT stft;
    const auto spec = stft.analyzeMagnitude(saw, sampleRate, 4096, 512);

    Curve pitch;
    pitch.sampleRateHz = 100.0f;
    pitch.values.assign(35, rootHz);

    HarmonicExtractor extractor;
    const auto harmonics = extractor.extract(spec, pitch, sampleRate, 100.0f, 8);
    const auto expectedFrames = static_cast<size_t>(std::ceil(0.35 * 100.0));

    bool ok = true;
    ok &= expect(harmonics.harmonicCount == 8, "Harmonic extractor keeps requested harmonic count");
    ok &= expect(harmonics.energyExplained > 0.05f, "Harmonic extractor explains nonzero energy");
    ok &= expect(harmonics.harmonicAmplitudes[0].values.size() == expectedFrames, "Harmonic extractor follows source-duration frame count");
    ok &= expect(averageCurve(harmonics.harmonicAmplitudes[0], 2) > averageCurve(harmonics.harmonicAmplitudes[3], 2), "Saw harmonic envelope decays");
    ok &= expect(averageCurve(harmonics.harmonicAmplitudes[0], 2) > 0.0f, "Saw first harmonic is nonzero");
    return ok;
}

bool testPresetSerializerRoundtrip()
{
    juce::ValueTree state("PARAMETERS");
    state.setProperty("body", 0.75f, nullptr);
    state.setProperty("mode", 2, nullptr);
    state.setProperty("quality", 1, nullptr);

    auto model = std::make_shared<TimbreModel>();
    model->durationSeconds = 0.42f;
    model->detectedRootHz = 261.63f;
    model->sourcePath = "/tmp/bifrost-test.wav";
    model->sourceHash = "abc123";

    juce::MemoryBlock data;
    PresetSerializer::writeState(state, model, data);
    const auto restored = PresetSerializer::readState(data.getData(), static_cast<int>(data.getSize()));

    bool ok = true;
    ok &= expect(data.getSize() > 0, "Preset serializer writes binary state");
    ok &= expect(restored.state.isValid(), "Preset serializer restores parameter tree");
    ok &= expect(restored.state.getType() == juce::Identifier("PARAMETERS"), "Preset serializer restores parameter tree type");
    ok &= expect(restored.modelFormatVersion == PresetSerializer::currentModelFormatVersion, "Preset serializer restores current model format version");
    ok &= expect(!restored.modelFormatMigrated, "Preset serializer does not migrate current format");
    ok &= expect(!restored.modelFormatForwardCompatible, "Preset serializer does not flag current format as forward-compatible");
    ok &= expectNear(static_cast<float>(restored.state.getProperty("body")), 0.75f, 1.0e-6f, "Preset serializer restores float property");
    ok &= expect(static_cast<int>(restored.state.getProperty("mode")) == 2, "Preset serializer restores integer property");
    ok &= expect(static_cast<int>(restored.state.getProperty("quality")) == 1, "Preset serializer restores quality property");
    return ok;
}

bool testPresetSerializerMigrationMetadata()
{
    juce::ValueTree legacyState("PARAMETERS");
    legacyState.setProperty("body", 0.25f, nullptr);

    juce::MemoryBlock legacyData;
    std::unique_ptr<juce::XmlElement> legacyXml(legacyState.createXml());
    juce::AudioProcessor::copyXmlToBinary(*legacyXml, legacyData);
    const auto restoredLegacy = PresetSerializer::readState(legacyData.getData(), static_cast<int>(legacyData.getSize()));

    juce::ValueTree futureRoot("BifrostPreset");
    futureRoot.setProperty("model_format_version", PresetSerializer::currentModelFormatVersion + 1, nullptr);
    futureRoot.addChild(legacyState.createCopy(), -1, nullptr);

    juce::MemoryBlock futureData;
    std::unique_ptr<juce::XmlElement> futureXml(futureRoot.createXml());
    juce::AudioProcessor::copyXmlToBinary(*futureXml, futureData);
    const auto restoredFuture = PresetSerializer::readState(futureData.getData(), static_cast<int>(futureData.getSize()));

    bool ok = true;
    ok &= expect(restoredLegacy.state.isValid(), "Preset serializer reads legacy parameter-root state");
    ok &= expect(restoredLegacy.modelFormatVersion == 0, "Preset serializer records legacy model format version");
    ok &= expect(restoredLegacy.modelFormatMigrated, "Preset serializer marks legacy state as migrated");
    ok &= expect(restoredFuture.state.isValid(), "Preset serializer reads newer preset parameter child");
    ok &= expect(restoredFuture.modelFormatForwardCompatible, "Preset serializer flags newer model format");
    ok &= expect(restoredFuture.migrationMessage.isNotEmpty(), "Preset serializer records migration status message");
    return ok;
}

bool testStateGraphNormalization()
{
    TimbreModel model;
    model.durationSeconds = 0.5f;
    model.controlRateHz = 100.0f;
    model.centroid.sampleRateHz = model.controlRateHz;
    model.centroid.values.assign(50, 1200.0f);
    model.spectralFlatness.sampleRateHz = model.controlRateHz;
    model.spectralFlatness.values.assign(50, 0.25f);
    model.loudness.sampleRateHz = model.controlRateHz;
    model.loudness.values.assign(50, 0.4f);

    TimbreStateModel stateModel;
    const auto graph = stateModel.fitPlaceholder(model, 6);

    bool ok = true;
    ok &= expect(graph.stateCount == 6, "State graph keeps requested state count");
    ok &= expect(graph.states.size() == 6, "State graph creates states");
    for (int row = 0; row < graph.stateCount; ++row)
    {
        float rowSum = 0.0f;
        for (int col = 0; col < graph.stateCount; ++col)
            rowSum += graph.transition(row, col);
        ok &= expectNear(rowSum, 1.0f, 1.0e-4f, "State graph transition row normalizes");
    }
    return ok;
}

std::shared_ptr<TimbreModel> makeMinimalTimbreModel()
{
    auto model = std::make_shared<TimbreModel>();
    model->durationSeconds = 0.25f;
    model->detectedRootHz = 440.0f;
    model->controlRateHz = 100.0f;
    model->pitchConfidence = 1.0f;
    model->pitchHz.sampleRateHz = model->controlRateHz;
    model->pitchHz.values.assign(25, model->detectedRootHz);
    model->loudness.sampleRateHz = model->controlRateHz;
    model->loudness.values.assign(25, 0.5f);
    model->harmonics.harmonicCount = 2;
    model->harmonics.harmonicAmplitudes.resize(2);
    model->harmonics.energyExplained = 0.9f;

    model->harmonics.harmonicAmplitudes[0].sampleRateHz = model->controlRateHz;
    model->harmonics.harmonicAmplitudes[0].values.assign(25, 0.8f);
    model->harmonics.harmonicAmplitudes[1].sampleRateHz = model->controlRateHz;
    model->harmonics.harmonicAmplitudes[1].values.assign(25, 0.25f);
    return model;
}

bool testVoiceManagerNoNanOutput()
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 128);

    auto model = makeMinimalTimbreModel();
    VoiceRenderParameters params;
    params.body = 1.0f;
    params.air = 0.0f;
    params.metal = 0.0f;
    params.outputGainDb = -6.0f;
    params.maxVoices = 4;
    params.maxHarmonics = 8;

    float peak = 0.0f;
    bool finite = true;

    for (int block = 0; block < 24; ++block)
    {
        juce::AudioBuffer<float> output(2, 128);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 69, static_cast<juce::uint8>(96)), 0);
        if (block == 16)
            midi.addEvent(juce::MidiMessage::noteOff(1, 69), 0);

        voiceManager.render(output, midi, model, params);

        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            const auto* samples = output.getReadPointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                finite = finite && std::isfinite(samples[i]);
                peak = std::max(peak, std::abs(samples[i]));
            }
        }
    }

    bool ok = true;
    ok &= expect(finite, "VoiceManager output contains no NaN or Inf");
    ok &= expect(peak > 1.0e-5f && peak < 1.5f, "VoiceManager output is audible and bounded");
    return ok;
}

struct AdsrProbe
{
    float firstBlock = 0.0f;
    float sustainedBlock = 0.0f;
    float releasedBlock = 0.0f;
    bool finite = true;
};

AdsrProbe renderAdsrProbe(std::shared_ptr<const TimbreModel> model)
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 256);

    auto params = makeRenderParams();
    params.mode = 1;
    params.attackSeconds = 0.25f;
    params.decaySeconds = 0.05f;
    params.sustainLevel = 0.25f;
    params.releaseSeconds = 0.12f;

    AdsrProbe probe;

    for (int block = 0; block < 120; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 69, static_cast<juce::uint8>(100)), 0);
        if (block == 70)
            midi.addEvent(juce::MidiMessage::noteOff(1, 69), 0);

        voiceManager.render(output, midi, model, params);

        float blockPeak = 0.0f;
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            const auto* samples = output.getReadPointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                probe.finite = probe.finite && std::isfinite(samples[i]);
                blockPeak = std::max(blockPeak, std::abs(samples[i]));
            }
        }

        if (block == 1)
            probe.firstBlock = blockPeak;
        if (block == 64)
            probe.sustainedBlock = blockPeak;
        if (block == 110)
            probe.releasedBlock = blockPeak;
    }

    return probe;
}

bool testAdsrEnvelopeShape()
{
    const auto probe = renderAdsrProbe(makeMinimalTimbreModel());

    bool ok = true;
    ok &= expect(probe.finite, "ADSR render output stays finite");
    ok &= expect(probe.firstBlock > 0.0f, "ADSR attack starts audible");
    ok &= expect(probe.sustainedBlock > probe.firstBlock * 3.0f, "ADSR attack ramps up before sustain");
    ok &= expect(probe.releasedBlock < probe.sustainedBlock * 0.05f, "ADSR release reaches silence");
    return ok;
}

bool testTempoAnalyzerPulseTrain()
{
    Curve flux;
    flux.sampleRateHz = 100.0f;
    flux.values.assign(500, 0.0f);
    for (size_t i = 0; i < flux.values.size(); i += 50)
        flux.values[i] = 1.0f;

    TempoAnalyzer analyzer;
    const auto tempo = analyzer.estimate(flux);

    bool ok = true;
    ok &= expectNear(tempo.bpm, 120.0f, 2.0f, "Tempo analyzer detects 120 BPM pulse train");
    ok &= expect(tempo.confidence > 0.2f, "Tempo analyzer reports useful confidence");
    return ok;
}

float renderSingleNotePeak(std::shared_ptr<const TimbreModel> model,
                           const VoiceRenderParameters& params,
                           int midiNote,
                           juce::uint8 velocity,
                           int targetChannel)
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 256);

    float peak = 0.0f;
    for (int block = 0; block < 8; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, velocity), 0);

        auto renderParams = params;
        voiceManager.render(output, midi, model, renderParams);

        const auto* samples = output.getReadPointer(targetChannel);
        for (int i = 0; i < output.getNumSamples(); ++i)
        {
            if (!std::isfinite(samples[i]))
                return std::numeric_limits<float>::infinity();
            peak = std::max(peak, std::abs(samples[i]));
        }
    }

    return peak;
}

bool testVelocityAndPanRenderControls()
{
    auto model = makeMinimalTimbreModel();
    auto params = makeRenderParams();
    params.attackSeconds = 0.001f;
    params.velocitySensitivity = 1.0f;

    const float lowVelocity = renderSingleNotePeak(model, params, 69, 32, 0);
    const float highVelocity = renderSingleNotePeak(model, params, 69, 127, 0);

    params.velocitySensitivity = 0.0f;
    const float lowFixedVelocity = renderSingleNotePeak(model, params, 69, 32, 0);

    params.velocitySensitivity = 1.0f;
    params.pan = -1.0f;
    const float hardLeftL = renderSingleNotePeak(model, params, 69, 127, 0);
    const float hardLeftR = renderSingleNotePeak(model, params, 69, 127, 1);

    bool ok = true;
    ok &= expect(std::isfinite(lowVelocity) && std::isfinite(highVelocity) && highVelocity > lowVelocity * 2.0f, "Velocity sensitivity changes render level");
    ok &= expect(lowFixedVelocity > lowVelocity * 2.0f, "Velocity sensitivity can flatten low-velocity notes");
    ok &= expect(hardLeftL > 1.0e-5f && hardLeftR < hardLeftL * 0.05f, "Pan routes hard-left output mostly to left channel");
    return ok;
}

std::shared_ptr<TimbreModel> makeFluctuatingTimbreModel()
{
    auto model = makeMinimalTimbreModel();
    model->durationSeconds = 1.0f;
    model->loudness.values.resize(100);
    model->harmonics.harmonicAmplitudes[0].values.resize(100);
    model->harmonics.harmonicAmplitudes[1].values.resize(100);

    for (size_t i = 0; i < 100; ++i)
    {
        const float phase = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 12.0f;
        model->loudness.values[i] = 0.20f + 0.55f * (0.5f + 0.5f * std::sin(phase));
        model->harmonics.harmonicAmplitudes[0].values[i] = 0.35f + 0.50f * (0.5f + 0.5f * std::sin(phase * 0.73f));
        model->harmonics.harmonicAmplitudes[1].values[i] = 0.10f + 0.35f * (0.5f + 0.5f * std::cos(phase * 1.31f));
    }

    return model;
}

bool testSustainModeHoldsStableCachedTone()
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 256);

    auto model = makeFluctuatingTimbreModel();
    auto params = makeRenderParams();
    params.mode = 1;
    params.attackSeconds = 0.001f;
    params.decaySeconds = 0.005f;
    params.sustainLevel = 1.0f;
    params.loopStartNormalized = 0.25f;
    params.loopEndNormalized = 0.85f;
    params.startRandomAmount = 1.0f;
    params.maxHarmonics = 2;

    float minLatePeak = std::numeric_limits<float>::max();
    float maxLatePeak = 0.0f;
    bool finite = true;

    for (int block = 0; block < 100; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);

        voiceManager.render(output, midi, model, params);

        float blockPeak = 0.0f;
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            const auto* samples = output.getReadPointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                finite = finite && std::isfinite(samples[i]);
                blockPeak = std::max(blockPeak, std::abs(samples[i]));
            }
        }

        if (block >= 40)
        {
            minLatePeak = std::min(minLatePeak, blockPeak);
            maxLatePeak = std::max(maxLatePeak, blockPeak);
        }
    }

    bool ok = true;
    ok &= expect(finite, "Sustain cached tone remains finite");
    ok &= expect(maxLatePeak > 1.0e-5f, "Sustain cached tone remains audible");
    ok &= expect(minLatePeak > maxLatePeak * 0.55f, "Sustain mode avoids abnormal timbre/rhythm amplitude fluctuation");
    return ok;
}

bool modelCurvesAreFinite(const TimbreModel& model)
{
    bool finite = curveIsFinite(model.pitchHz)
        && curveIsFinite(model.pitchConfidenceCurve)
        && curveIsFinite(model.loudness)
        && curveIsFinite(model.centroid)
        && curveIsFinite(model.spectralFlux)
        && curveIsFinite(model.spectralFlatness);

    for (const auto& harmonic : model.harmonics.harmonicAmplitudes)
        finite = finite && curveIsFinite(harmonic);

    for (const auto& band : model.noise.bandAmplitudes)
        finite = finite && curveIsFinite(band);

    for (const auto& gain : model.resonators.gains)
        finite = finite && curveIsFinite(gain);

    return finite;
}

bool testAnalysisJobShortAndSilentFiles()
{
    AnalysisJob job;

    auto shortLoaded = makeLoadedAudio(makeSine(440.0f, 0.25f, 0.001), sampleRate);
    auto shortModel = job.run(shortLoaded, 440.0f);

    juce::AudioBuffer<float> silence(1, static_cast<int>(sampleRate * 0.05));
    silence.clear();
    auto silentLoaded = makeLoadedAudio(std::move(silence), sampleRate);
    auto silentModel = job.run(silentLoaded);

    bool ok = true;
    ok &= expect(shortModel != nullptr, "Analysis job returns a model for short files");
    if (shortModel)
    {
        ok &= expect(shortModel->durationSeconds > 0.0f, "Analysis job keeps short file duration");
        ok &= expect(shortModel->harmonics.harmonicCount == 48, "Analysis job extracts harmonic slots for short files");
        ok &= expect(shortModel->waveformPreview.pointCount() > 0, "Analysis job creates a waveform preview for short files");
        ok &= expect(modelCurvesAreFinite(*shortModel), "Analysis job short-file curves are finite");
    }

    ok &= expect(silentModel != nullptr, "Analysis job returns a model for silent files");
    if (silentModel)
    {
        ok &= expect(silentModel->durationSeconds > 0.0f, "Analysis job keeps silent file duration");
        ok &= expectNear(silentModel->pitchConfidence, 0.0f, 1.0e-6f, "Analysis job reports zero pitch confidence for silence");
        ok &= expectNear(averageCurve(silentModel->loudness), 0.0f, 1.0e-6f, "Analysis job silent loudness stays zero");
        ok &= expect(modelCurvesAreFinite(*silentModel), "Analysis job silent-file curves are finite");
    }

    return ok;
}

bool testTinyTimbreEncoderFallback()
{
    auto sine = makeSine(440.0f, 0.5f, 0.25);
    STFT stft;
    const auto spec = stft.analyzeMagnitude(sine, sampleRate, 2048, 512);

    TinyTimbreEncoder encoder;
    const auto patch = encoder.makeLogMelPatch(spec);
    const auto embedding = encoder.encodeLogMelPatch(patch.values.data(), patch.melBins, patch.frames);

    bool ok = true;
    ok &= expect(!encoder.isAvailable(), "Tiny timbre encoder is unavailable without ONNX");
    ok &= expect(patch.melBins == 64 && patch.frames == 16, "Tiny timbre encoder creates expected mel patch shape");
    ok &= expect(embedding.size() == 16, "Tiny timbre encoder creates 16-D fallback embedding");
    ok &= expect(std::any_of(embedding.begin(), embedding.end(), [](float value) { return std::abs(value) > 1.0e-5f; }), "Fallback embedding is nonzero");
    return ok;
}

bool testBuildInfoStamp()
{
    bool ok = true;
    ok &= expect(juce::String(bifrost::build::version).isNotEmpty(), "Build info version is stamped");
    ok &= expect(juce::String(bifrost::build::gitSha).isNotEmpty(), "Build info git SHA is stamped");
    ok &= expect(juce::String(bifrost::build::buildDateUtc).contains("T"), "Build info date is stamped as UTC timestamp");
    return ok;
}

LoadedAudioFile makeLoadedAudio(juce::AudioBuffer<float> mono, double sourceSampleRate)
{
    LoadedAudioFile loaded;
    loaded.audio = mono;
    loaded.monoMid = std::move(mono);
    loaded.sampleRate = sourceSampleRate;
    loaded.originalDurationSeconds = static_cast<double>(loaded.audio.getNumSamples()) / sourceSampleRate;
    loaded.importedDurationSeconds = loaded.originalDurationSeconds;
    loaded.originalNumChannels = 1;
    loaded.importedNumChannels = 1;
    loaded.originalLengthInSamples = loaded.audio.getNumSamples();
    loaded.importedLengthInSamples = loaded.audio.getNumSamples();
    loaded.decoderBackend = "Test";
    return loaded;
}

VoiceRenderParameters makeRenderParams()
{
    VoiceRenderParameters params;
    params.body = 0.9f;
    params.air = 0.0f;
    params.metal = 0.0f;
    params.outputGainDb = -6.0f;
    params.maxVoices = 8;
    params.maxHarmonics = 48;
    params.maxNoiseBands = 8;
    params.maxResonators = 8;
    return params;
}

float renderNotePeak(std::shared_ptr<const TimbreModel> model, int midiNote)
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 256);

    auto params = makeRenderParams();

    float peak = 0.0f;
    for (int block = 0; block < 8; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(100)), 0);

        voiceManager.render(output, midi, model, params);

        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            const auto* samples = output.getReadPointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                if (!std::isfinite(samples[i]))
                    return std::numeric_limits<float>::infinity();
                peak = std::max(peak, std::abs(samples[i]));
            }
        }
    }

    return peak;
}

struct RenderWindowPeaks
{
    float early = 0.0f;
    float late = 0.0f;
};

RenderWindowPeaks renderSustainedNoteWindows(std::shared_ptr<const TimbreModel> model, int mode)
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 256);

    auto params = makeRenderParams();
    params.mode = mode;

    RenderWindowPeaks peaks;
    constexpr int totalBlocks = 350;
    constexpr int windowBlocks = 32;

    for (int block = 0; block < totalBlocks; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);

        voiceManager.render(output, midi, model, params);

        float blockPeak = 0.0f;
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            const auto* samples = output.getReadPointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                if (!std::isfinite(samples[i]))
                    return { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
                blockPeak = std::max(blockPeak, std::abs(samples[i]));
            }
        }

        if (block < windowBlocks)
            peaks.early = std::max(peaks.early, blockPeak);
        if (block >= totalBlocks - windowBlocks)
            peaks.late = std::max(peaks.late, blockPeak);
    }

    return peaks;
}

float renderChordPeak(std::shared_ptr<const TimbreModel> model)
{
    VoiceManager voiceManager;
    voiceManager.prepare(sampleRate, 256);

    auto params = makeRenderParams();
    params.mode = 2;

    float peak = 0.0f;
    for (int block = 0; block < 12; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();

        juce::MidiBuffer midi;
        if (block == 0)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, 48, static_cast<juce::uint8>(95)), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 55, static_cast<juce::uint8>(95)), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(95)), 0);
        }

        voiceManager.render(output, midi, model, params);

        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            const auto* samples = output.getReadPointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                if (!std::isfinite(samples[i]))
                    return std::numeric_limits<float>::infinity();
                peak = std::max(peak, std::abs(samples[i]));
            }
        }
    }

    return peak;
}

bool testAnalysisJobSawModelAndRender()
{
    constexpr float rootHz = 220.0f;
    auto loaded = makeLoadedAudio(makeSawLike(rootHz, 10, 0.4), sampleRate);

    AnalysisJob job;
    auto model = job.run(loaded, rootHz);
    if (!expect(model != nullptr, "Analysis job returns a model"))
        return false;

    const auto expectedFrames = static_cast<size_t>(std::ceil(model->durationSeconds * model->controlRateHz));
    const float lowPeak = renderNotePeak(model, 48);
    const float middlePeak = renderNotePeak(model, 60);
    const float highPeak = renderNotePeak(model, 84);
    const auto oneShot = renderSustainedNoteWindows(model, 0);
    const auto sustain = renderSustainedNoteWindows(model, 1);
    const float chordPeak = renderChordPeak(model);

    bool ok = true;
    ok &= expect(model->isUsable(), "Analysis job creates usable saw model");
    ok &= expectNear(model->detectedRootHz, rootHz, 0.1f, "Analysis job applies root override");
    ok &= expect(model->pitchConfidence > 0.45f, "Analysis job stores pitch confidence");
    ok &= expect(model->harmonics.energyExplained > 0.05f, "Analysis job stores harmonic energy metric");
    ok &= expect(model->harmonics.harmonicAmplitudes[0].values.size() == expectedFrames, "Analysis job harmonic curves match model duration");
    ok &= expect(std::isfinite(lowPeak) && lowPeak > 1.0e-5f && lowPeak < 1.5f, "Low transpose render is finite and audible");
    ok &= expect(std::isfinite(middlePeak) && middlePeak > 1.0e-5f && middlePeak < 1.5f, "Middle transpose render is finite and audible");
    ok &= expect(std::isfinite(highPeak) && highPeak > 1.0e-5f && highPeak < 1.5f, "High transpose render is finite and audible");
    ok &= expect(oneShot.early > 1.0e-5f && oneShot.late < oneShot.early * 0.1f, "One-Shot mode decays after source end");
    ok &= expect(sustain.early > 1.0e-5f && sustain.late > oneShot.late * 4.0f, "Sustain mode keeps looping while held");
    ok &= expect(std::isfinite(chordPeak) && chordPeak > 1.0e-5f && chordPeak < 2.0f, "Harmonizer mode renders multiple MIDI voices");
    return ok;
}

bool testNoiseAndResonatorBanks()
{
    TimbreModel model;
    model.durationSeconds = 0.25f;
    model.controlRateHz = 100.0f;

    model.noise.bandCount = 2;
    model.noise.bandCenterHz = { 500.0f, 2500.0f };
    model.noise.bandAmplitudes.resize(2);
    for (auto& curve : model.noise.bandAmplitudes)
    {
        curve.sampleRateHz = model.controlRateHz;
        curve.values.assign(25, 0.5f);
    }

    model.resonators.resonatorCount = 1;
    model.resonators.frequencyHz = { 900.0f };
    model.resonators.q = { 3.0f };
    model.resonators.gains.resize(1);
    model.resonators.gains[0].sampleRateHz = model.controlRateHz;
    model.resonators.gains[0].values.assign(25, 0.75f);

    auto params = makeRenderParams();
    params.air = 1.0f;
    params.metal = 1.0f;

    NoiseBank noise;
    noise.prepare(sampleRate, 4);
    ResonatorBank resonator;
    resonator.prepare(sampleRate, 2);

    float noisePeak = 0.0f;
    float resonatorPeak = 0.0f;

    for (int i = 0; i < 2048; ++i)
    {
        const float t = static_cast<float>(i / sampleRate);
        const float n = noise.renderSample(model, t, params);
        const float r = resonator.process(i == 0 ? 1.0f : 0.0f, model, t, params);

        if (!std::isfinite(n) || !std::isfinite(r))
            return expect(false, "Noise/resonator output stays finite");

        noisePeak = std::max(noisePeak, std::abs(n));
        resonatorPeak = std::max(resonatorPeak, std::abs(r));
    }

    bool ok = true;
    ok &= expect(noisePeak > 1.0e-5f && noisePeak < 2.0f, "Noise band rendering produces controlled output");
    ok &= expect(resonatorPeak > 1.0e-5f && resonatorPeak < 2.0f, "Resonator rendering produces controlled output");
    return ok;
}
}

int main()
{
    bool ok = true;
    ok &= testCurveInterpolation();
    ok &= testShortBufferStft();
    ok &= testSineCentroidAndRms();
    ok &= testWhiteNoiseFeatures();
    ok &= testWaveformPreview();
    ok &= testPitchTrackerSine();
    ok &= testPitchTrackerRootOverride();
    ok &= testPitchTrackerSineSweep();
    ok &= testHarmonicExtractorSawStack();
    ok &= testPresetSerializerRoundtrip();
    ok &= testPresetSerializerMigrationMetadata();
    ok &= testStateGraphNormalization();
    ok &= testTinyTimbreEncoderFallback();
    ok &= testBuildInfoStamp();
    ok &= testVoiceManagerNoNanOutput();
    ok &= testAdsrEnvelopeShape();
    ok &= testTempoAnalyzerPulseTrain();
    ok &= testVelocityAndPanRenderControls();
    ok &= testSustainModeHoldsStableCachedTone();
    ok &= testAnalysisJobShortAndSilentFiles();
    ok &= testAnalysisJobSawModelAndRender();
    ok &= testNoiseAndResonatorBanks();

    if (!ok)
        return 1;

    std::cout << "Bifrost analysis tests passed\n";
    return 0;
}
