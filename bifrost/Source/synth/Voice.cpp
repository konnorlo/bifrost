#include "synth/Voice.h"

void Voice::prepare(double sampleRate, int, int maxHarmonics, int maxResonators)
{
    fs = sampleRate;
    for (auto& layer : additiveLayers)
        layer.prepare(sampleRate, maxHarmonics);
    noise.prepare(sampleRate, 16);
    resonators.prepare(sampleRate, maxResonators);
}

void Voice::start(int midiNote, float vel, std::shared_ptr<const TimbreModel> m, uint32_t noteSerial)
{
    note = midiNote;
    midiHz = midiNoteToHz(note);
    velocity = vel;
    model = std::move(m);
    ageSeconds = 0.0f;
    currentModelTimeSeconds = 0.0f;
    lastPeak = 0.0f;
    envelopeLevel = 0.0f;
    releaseStartLevel = 0.0f;
    releaseAgeSeconds = 0.0f;
    randomUnit = randomUnitFromSerial(noteSerial, midiNote, vel);
    randomStartNormalized = 0.5f + 0.5f * randomUnit;
    for (auto& hz : lastBaseFrequencyHz)
        hz = -1.0f;
    cachedHarmonicCount = 0;
    cachedMaxHarmonics = 0;
    cachedModelTimeSeconds = -1.0f;
    cachedLoudness = 0.0f;
    cachedHarmonicAmplitudes.clear();
    releasing = false;
    active = static_cast<bool>(model) && model->isUsable();
    for (auto& layer : additiveLayers)
        layer.reset();
    noise.reset();
    resonators.reset();
}

void Voice::stop()
{
    beginRelease();
}

float Voice::getModelTimeNormalized() const noexcept
{
    if (!model || model->durationSeconds <= 0.0f)
        return 0.0f;

    return std::clamp(currentModelTimeSeconds / model->durationSeconds, 0.0f, 1.0f);
}

void Voice::render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, const VoiceRenderParameters& params)
{
    if (!active || !model)
    {
        lastPeak *= 0.85f;
        return;
    }

    updateBaseFrequencies(params);

    const float velocityGain = juce::jmap(std::clamp(params.velocitySensitivity, 0.0f, 1.0f), 1.0f, std::clamp(velocity, 0.0f, 1.0f));
    const float gain = juce::Decibels::decibelsToGain(params.outputGainDb) * velocityGain * 1.35f;
    const float dt = 1.0f / static_cast<float>(fs);
    const float modelDuration = std::max(0.001f, model->durationSeconds);
    const float effectiveTimeStretch = getEffectiveTimeStretch(params);
    const int layerCount = getEffectiveUnisonVoices(params);
    const float layerGain = 1.0f / std::sqrt(static_cast<float>(layerCount));
    float blockPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float stretchedAge = ageSeconds / effectiveTimeStretch;
        if (params.mode == 0 && !releasing && stretchedAge >= modelDuration)
            beginRelease();

        currentModelTimeSeconds = getModelTime(stretchedAge, modelDuration, params);

        float left = 0.0f;
        float right = 0.0f;
        float mono = 0.0f;
        for (int layer = 0; layer < layerCount; ++layer)
        {
            auto layerParams = getLayerParameters(layer, layerCount, params);
            const float layerTime = getLayerModelTime(stretchedAge, modelDuration, layerParams, layer, layerCount);
            const float alternateMix = params.mode == 1 ? std::clamp(0.18f + 0.32f * layerParams.motion, 0.0f, 0.55f) : 0.0f;
            const float alternateOffset = (0.018f + 0.070f * layerParams.motion) * (modelDuration / std::max(0.001f, modelDuration));
            const float alternateTime = params.mode == 1
                ? getLoopScanTime(stretchedAge, modelDuration, layerParams, layer, layerCount, alternateOffset)
                : -1.0f;
            float y = additiveLayers[static_cast<size_t>(layer)].renderSample(*model,
                                                                               layerTime,
                                                                               layerParams,
                                                                               getLayerPhaseOffset(layer, layerCount, layerParams),
                                                                               alternateTime,
                                                                               alternateMix);

            const float movingColour = params.mode == 1 ? std::clamp(0.35f + 0.65f * layerParams.motion, 0.0f, 1.0f) : 1.0f;
            y += movingColour * noise.renderSample(*model, layerTime, layerParams);
            y += movingColour * resonators.process(y, *model, layerTime, layerParams);

            y *= layerGain;
            mono += y;

            const float clampedPan = getLayerPan(layer, layerCount, layerParams);
            const float panAngle = (clampedPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            left += y * std::cos(panAngle);
            right += y * std::sin(panAngle);
        }

        const float reconstruction = std::clamp(params.stereoReconstruction, 0.0f, 1.0f);
        if (reconstruction > 0.0f && layerCount == 1)
        {
            const float side = mono * (0.12f + 0.16f * params.phaseOffset) * reconstruction;
            left += side;
            right -= side;
        }

        const float envelope = computeEnvelope(params);
        if (!active)
            break;

        mono *= gain * envelope;
        left *= gain * envelope;
        right *= gain * envelope;
        blockPeak = std::max({ blockPeak, std::abs(mono), std::abs(left), std::abs(right) });
        if (buffer.getNumChannels() == 1)
        {
            buffer.addSample(0, startSample + i, mono);
        }
        else
        {
            buffer.addSample(0, startSample + i, left);
            buffer.addSample(1, startSample + i, right);
            for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample(ch, startSample + i, mono);
        }

        ageSeconds += dt;
        if (releasing)
            releaseAgeSeconds += dt;
    }

    lastPeak = std::max(blockPeak, lastPeak * 0.85f);
}

float Voice::midiNoteToHz(int midiNote) noexcept
{
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

float Voice::randomUnitFromSerial(uint32_t noteSerial, int midiNote, float velocity) noexcept
{
    uint32_t x = noteSerial + 0x9e3779b9u;
    x ^= static_cast<uint32_t>(std::max(0, midiNote)) * 0x85ebca6bu;
    x ^= static_cast<uint32_t>(std::round(std::clamp(velocity, 0.0f, 1.0f) * 127.0f)) * 0xc2b2ae35u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    const float normalized = static_cast<float>(x & 0x00ffffffu) / static_cast<float>(0x00ffffffu);
    return normalized * 2.0f - 1.0f;
}

float Voice::randomUnitFromLayer(float noteRandom, int layer, int salt) noexcept
{
    uint32_t x = static_cast<uint32_t>((noteRandom + 1.0f) * 8388607.0f);
    x ^= static_cast<uint32_t>(layer + 17) * 0x9e3779b9u;
    x ^= static_cast<uint32_t>(salt + 31) * 0x85ebca6bu;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return (static_cast<float>(x & 0x00ffffffu) / static_cast<float>(0x00ffffffu)) * 2.0f - 1.0f;
}

void Voice::beginRelease() noexcept
{
    if (releasing)
        return;

    releasing = true;
    releaseStartLevel = envelopeLevel;
    releaseAgeSeconds = 0.0f;
}

float Voice::computeEnvelope(const VoiceRenderParameters& params) noexcept
{
    const float attack = std::clamp(params.attackSeconds, 0.001f, 10.0f);
    const float decay = std::clamp(params.decaySeconds, 0.001f, 10.0f);
    const float sustain = std::clamp(params.sustainLevel, 0.0f, 1.0f);
    const float release = std::clamp(params.releaseSeconds, 0.001f, 20.0f);

    if (releasing)
    {
        const float releaseProgress = shapeEnvelopeProgress(releaseAgeSeconds / release, params.envelopeCurve);
        envelopeLevel = releaseStartLevel * (1.0f - releaseProgress);
        if (envelopeLevel <= 1.0e-4f || releaseProgress >= 1.0f)
        {
            envelopeLevel = 0.0f;
            active = false;
        }
        return envelopeLevel;
    }

    if (ageSeconds < attack)
    {
        envelopeLevel = shapeEnvelopeProgress(ageSeconds / attack, params.envelopeCurve);
        return envelopeLevel;
    }

    const float decayAge = ageSeconds - attack;
    if (decayAge < decay)
    {
        const float decayProgress = shapeEnvelopeProgress(decayAge / decay, params.envelopeCurve);
        envelopeLevel = 1.0f + (sustain - 1.0f) * decayProgress;
        return envelopeLevel;
    }

    envelopeLevel = sustain;
    return envelopeLevel;
}

float Voice::shapeEnvelopeProgress(float progress, float curve) noexcept
{
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    const float power = juce::jmap(std::clamp(curve, 0.0f, 1.0f), 2.5f, 0.45f);
    return std::pow(clamped, power);
}

float Voice::getEffectiveTimeStretch(const VoiceRenderParameters& params) const noexcept
{
    const float manualStretch = std::clamp(params.timeStretch, 0.25f, 4.0f);
    const float sync = std::clamp(params.timeSync, 0.0f, 1.0f);
    if (sync <= 0.0f || params.sourceTempoBpm < 20.0f || params.hostTempoBpm < 20.0f)
        return manualStretch;

    const float syncedStretch = std::clamp(params.sourceTempoBpm / params.hostTempoBpm, 0.25f, 4.0f);
    return std::max(0.001f, manualStretch + (syncedStretch - manualStretch) * sync);
}

int Voice::getEffectiveUnisonVoices(const VoiceRenderParameters& params) const noexcept
{
    const int requested = std::clamp(params.unisonVoices, 1, static_cast<int>(additiveLayers.size()));
    if (params.stereoReconstruction > 0.01f || params.phaseOffset > 0.01f || params.phaseRandom > 0.01f)
        return std::max(2, requested);

    return requested;
}

void Voice::updateBaseFrequencies(const VoiceRenderParameters& params)
{
    const int layerCount = getEffectiveUnisonVoices(params);
    const float detuneCents = std::clamp(params.unisonDetuneCents, 0.0f, 50.0f);
    for (int layer = 0; layer < layerCount; ++layer)
    {
        const float spread = layerCount <= 1 ? 0.0f : (static_cast<float>(layer) / static_cast<float>(layerCount - 1)) * 2.0f - 1.0f;
        const float branch = randomUnitFromLayer(randomUnit, layer, 11);
        const float mutationDetune = branch * std::clamp(params.mutation, 0.0f, 1.0f) * 3.5f;
        const float stereoDetune = spread * std::clamp(params.stereoReconstruction, 0.0f, 1.0f) * 1.5f;
        const float cents = spread * detuneCents + mutationDetune + stereoDetune;
        const float targetHz = midiHz * std::pow(2.0f, cents / 1200.0f);
        if (std::abs(targetHz - lastBaseFrequencyHz[layer]) > 0.001f)
        {
            additiveLayers[static_cast<size_t>(layer)].setBaseFrequency(targetHz);
            lastBaseFrequencyHz[layer] = targetHz;
        }
    }
}

void Voice::updateStaticSustainCache(const VoiceRenderParameters& params)
{
    if (!model)
        return;

    const float time = getSustainModelTime(std::max(0.001f, model->durationSeconds), params);
    const int count = std::min(model->harmonics.harmonicCount, std::max(1, params.maxHarmonics));
    if (count == cachedHarmonicCount && params.maxHarmonics == cachedMaxHarmonics && std::abs(time - cachedModelTimeSeconds) <= 1.0e-5f)
        return;

    cachedHarmonicCount = count;
    cachedMaxHarmonics = params.maxHarmonics;
    cachedModelTimeSeconds = time;
    cachedLoudness = std::max(0.10f, model->loudness.sample(time));
    cachedHarmonicAmplitudes.assign(static_cast<size_t>(count), 0.0f);

    for (int h = 0; h < count; ++h)
        cachedHarmonicAmplitudes[static_cast<size_t>(h)] = model->harmonics.harmonicAmplitudes[static_cast<size_t>(h)].sample(time);
}

float Voice::getSustainModelTime(float durationSeconds, const VoiceRenderParameters& params) const noexcept
{
    const float loopStart = std::clamp(params.loopStartNormalized, 0.0f, 0.98f);
    const float loopEnd = std::clamp(params.loopEndNormalized, loopStart + 0.01f, 1.0f);
    const float randomAmount = std::clamp(params.startRandomAmount, 0.0f, 1.0f);
    const float position = loopStart + (loopEnd - loopStart) * randomStartNormalized * randomAmount;
    return std::clamp(position * durationSeconds, 0.0f, durationSeconds);
}

float Voice::getLayerModelTime(float stretchedAgeSeconds, float durationSeconds, const VoiceRenderParameters& params, int layer, int layerCount) const noexcept
{
    if (params.mode != 1)
    {
        const float baseTime = getModelTime(stretchedAgeSeconds, durationSeconds, params);
        if (layerCount <= 1)
            return baseTime;

        const float layerSpread = (static_cast<float>(layer) / static_cast<float>(layerCount - 1)) * 2.0f - 1.0f;
        const float branch = randomUnitFromLayer(randomUnit, layer, 23);
        const float offsetNorm = (0.010f * layerSpread * std::clamp(params.stereoReconstruction, 0.0f, 1.0f))
                               + (0.020f * branch * std::clamp(params.startRandomAmount, 0.0f, 1.0f));
        return std::clamp(baseTime + offsetNorm * durationSeconds, 0.0f, durationSeconds);
    }

    return getLoopScanTime(stretchedAgeSeconds, durationSeconds, params, layer, layerCount, 0.0f);
}

float Voice::getLoopScanTime(float stretchedAgeSeconds,
                             float durationSeconds,
                             const VoiceRenderParameters& params,
                             int layer,
                             int layerCount,
                             float phaseOffset) const noexcept
{
    const float loopStartNorm = std::clamp(params.loopStartNormalized, 0.0f, 0.98f);
    const float loopEndNorm = std::clamp(params.loopEndNormalized, loopStartNorm + 0.01f, 1.0f);
    const float loopStart = loopStartNorm * durationSeconds;
    const float loopEnd = std::max(loopStart + 0.001f, loopEndNorm * durationSeconds);
    const float loopLength = std::max(0.001f, loopEnd - loopStart);
    const float layerSpread = layerCount <= 1 ? 0.0f : (static_cast<float>(layer) / static_cast<float>(layerCount - 1)) * 2.0f - 1.0f;
    const float branch = randomUnitFromLayer(randomUnit, layer, 37);
    const float startRandom = std::clamp(params.startRandomAmount, 0.0f, 1.0f);
    const float stereo = std::clamp(params.stereoReconstruction, 0.0f, 1.0f);
    const float motion = std::clamp(params.motion, 0.0f, 1.0f);
    const float mutation = std::clamp(params.mutation, 0.0f, 1.0f);

    const float stableStart = 0.5f;
    float position = stableStart
                   + 0.35f * startRandom * randomStartNormalized
                   + 0.12f * stereo * layerSpread
                   + 0.10f * mutation * branch
                   + phaseOffset;

    const float hostBpm = params.hostTempoBpm > 20.0f ? params.hostTempoBpm : 120.0f;
    const float syncedCyclesPerSecond = hostBpm / 240.0f;
    const float freeCyclesPerSecond = juce::jmap(motion, 0.015f, 1.15f);
    const float sync = std::clamp(params.timeSync, 0.0f, 1.0f);
    const float cyclesPerSecond = juce::jmap(sync, freeCyclesPerSecond, syncedCyclesPerSecond);
    position += stretchedAgeSeconds * cyclesPerSecond;

    const float lfoRate = 0.05f + 1.65f * motion;
    const float lfo = std::sin(juce::MathConstants<float>::twoPi * (stretchedAgeSeconds * lfoRate + 0.37f * branch));
    position += lfo * (0.010f + 0.085f * motion) * (0.35f + 0.65f * mutation);

    position = position - std::floor(position);
    if (std::clamp(params.randomDirection, 0.0f, 1.0f) > 0.5f && branch < 0.0f)
        position = 1.0f - position;

    return std::clamp(loopStart + position * loopLength, 0.0f, durationSeconds);
}

float Voice::getLayerPhaseOffset(int layer, int layerCount, const VoiceRenderParameters& params) const noexcept
{
    const float spread = layerCount <= 1 ? 0.0f : (static_cast<float>(layer) / static_cast<float>(layerCount - 1)) * 2.0f - 1.0f;
    const float branch = randomUnitFromLayer(randomUnit, layer, 47);
    const float phase = spread * std::clamp(params.phaseOffset, 0.0f, 1.0f) * 0.25f
                      + branch * std::clamp(params.phaseRandom, 0.0f, 1.0f) * 0.5f
                      + spread * std::clamp(params.stereoReconstruction, 0.0f, 1.0f) * 0.08f;
    return phase - std::floor(phase);
}

VoiceRenderParameters Voice::getLayerParameters(int layer, int layerCount, const VoiceRenderParameters& params) const noexcept
{
    auto layerParams = params;
    const float spread = layerCount <= 1 ? 0.0f : (static_cast<float>(layer) / static_cast<float>(layerCount - 1)) * 2.0f - 1.0f;
    const float branch = randomUnitFromLayer(randomUnit, layer, 59);
    const float mutation = std::clamp(params.mutation, 0.0f, 1.0f);
    const float stereo = std::clamp(params.stereoReconstruction, 0.0f, 1.0f);

    layerParams.body = std::clamp(params.body * (1.0f - 0.045f * std::abs(branch) * mutation), 0.0f, 1.5f);
    layerParams.air = std::clamp(params.air + (0.055f * branch * mutation) + (0.035f * std::abs(spread) * stereo), 0.0f, 1.5f);
    layerParams.metal = std::clamp(params.metal + 0.050f * spread * stereo + 0.040f * branch * mutation, 0.0f, 1.5f);
    layerParams.brightness = std::clamp(params.brightness + 0.060f * spread * stereo + 0.055f * branch * mutation, 0.0f, 1.5f);
    return layerParams;
}

float Voice::getLayerPan(int layer, int layerCount, const VoiceRenderParameters& params) const noexcept
{
    const float basePan = std::clamp(params.pan, -1.0f, 1.0f);
    if (layerCount <= 1)
        return basePan;

    const float spread = (static_cast<float>(layer) / static_cast<float>(layerCount - 1)) * 2.0f - 1.0f;
    return std::clamp(basePan + spread * std::clamp(params.stereoWidth, 0.0f, 1.0f), -1.0f, 1.0f);
}

float Voice::getModelTime(float stretchedAgeSeconds, float durationSeconds, const VoiceRenderParameters& params) const noexcept
{
    if (params.mode == 0)
        return std::clamp(stretchedAgeSeconds, 0.0f, durationSeconds);

    const float loopStartNorm = std::clamp(params.loopStartNormalized, 0.0f, 0.98f);
    const float loopEndNorm = std::clamp(params.loopEndNormalized, loopStartNorm + 0.01f, 1.0f);
    const float sustainStart = loopStartNorm * durationSeconds;
    const float sustainEnd = std::max(sustainStart + 0.001f, loopEndNorm * durationSeconds);

    if (params.mode == 1)
        return getLoopScanTime(stretchedAgeSeconds, durationSeconds, params, 0, 1, 0.0f);

    if (stretchedAgeSeconds <= sustainStart)
        return std::clamp(stretchedAgeSeconds, 0.0f, durationSeconds);

    const float loopLength = std::max(0.001f, sustainEnd - sustainStart);
    float loopPosition = std::fmod(stretchedAgeSeconds - sustainStart, loopLength);
    if (std::clamp(params.randomDirection, 0.0f, 1.0f) > 0.5f && randomUnit < 0.0f)
        loopPosition = loopLength - loopPosition;

    return sustainStart + loopPosition;
}
