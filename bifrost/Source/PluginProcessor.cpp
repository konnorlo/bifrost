#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "model/PresetSerializer.h"

BifrostAudioProcessor::BifrostAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout()),
      analysisController(*this)
{
    cacheParameterPointers();
}

BifrostAudioProcessor::~BifrostAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout BifrostAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto percentText = [](float value, int)
    {
        return juce::String(static_cast<int>(std::round(value * 100.0f))) + "%";
    };
    auto secondsText = [](float value, int)
    {
        if (value < 1.0f)
            return juce::String(value * 1000.0f, 0) + " ms";
        return juce::String(value, 2) + " s";
    };
    auto rootText = [](float value, int)
    {
        if (value < 20.0f)
            return juce::String("Auto");
        return juce::String(value, value < 1000.0f ? 1 : 0) + " Hz";
    };
    auto addFloat = [&params](const juce::String& id,
                              const juce::String& name,
                              float min,
                              float max,
                              float def,
                              juce::AudioParameterFloatAttributes attributes = {})
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, juce::NormalisableRange<float>(min, max), def, attributes));
    };

    auto addSkewedFloat = [&params](const juce::String& id,
                                    const juce::String& name,
                                    float min,
                                    float max,
                                    float def,
                                    float skew,
                                    juce::AudioParameterFloatAttributes attributes = {})
    {
        juce::NormalisableRange<float> range(min, max);
        range.setSkewForCentre(skew);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, range, def, attributes));
    };
    const auto percentAttributes = juce::AudioParameterFloatAttributes().withStringFromValueFunction(percentText).withLabel("%");

    addFloat("body", "Body", 0.0f, 1.0f, 0.65f, percentAttributes);
    addFloat("air", "Air", 0.0f, 1.0f, 0.35f, percentAttributes);
    addFloat("metal", "Metal", 0.0f, 1.0f, 0.25f, percentAttributes);
    addFloat("brightness", "Brightness", 0.0f, 1.0f, 0.55f, percentAttributes);
    addFloat("motion", "Motion", 0.0f, 1.0f, 0.75f, percentAttributes);
    addFloat("inertia", "Inertia", 0.0f, 1.0f, 0.35f, percentAttributes);
    addFloat("mutation", "Mutation", 0.0f, 1.0f, 0.0f, percentAttributes);
    addFloat("transient", "Transient", 0.0f, 1.0f, 0.7f, percentAttributes);
    addFloat("formant_lock", "Formant Lock", 0.0f, 1.0f, 0.75f, percentAttributes);
    addFloat("time_stretch", "Time Stretch", 0.25f, 4.0f, 1.0f,
             juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return juce::String(value, 2) + "x"; }));
    addFloat("root_override_hz", "Root Override Hz", 0.0f, 2000.0f, 0.0f,
             juce::AudioParameterFloatAttributes().withStringFromValueFunction(rootText).withLabel("Hz"));
    addFloat("output_gain", "Output Gain", -24.0f, 12.0f, 3.0f,
             juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return juce::String(value, 1) + " dB"; }).withLabel("dB"));
    addSkewedFloat("attack", "Attack", 0.001f, 5.0f, 0.01f, 0.12f,
                   juce::AudioParameterFloatAttributes().withStringFromValueFunction(secondsText));
    addSkewedFloat("decay", "Decay", 0.005f, 5.0f, 0.16f, 0.35f,
                   juce::AudioParameterFloatAttributes().withStringFromValueFunction(secondsText));
    addFloat("sustain", "Sustain", 0.0f, 1.0f, 0.75f, percentAttributes);
    addSkewedFloat("release", "Release", 0.005f, 8.0f, 0.35f, 0.45f,
                   juce::AudioParameterFloatAttributes().withStringFromValueFunction(secondsText));
    addFloat("adsr_curve", "ADSR Curve", 0.0f, 1.0f, 0.5f, percentAttributes);
    addFloat("velocity_sensitivity", "Velocity Sensitivity", 0.0f, 1.0f, 1.0f, percentAttributes);
    addFloat("pan", "Pan", -1.0f, 1.0f, 0.0f,
             juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int)
             {
                 if (std::abs(value) < 0.005f)
                     return juce::String("C");
                 return juce::String(value < 0.0f ? "L " : "R ") + juce::String(static_cast<int>(std::round(std::abs(value) * 100.0f))) + "%";
             }));
    addFloat("stereo_width", "Stereo Width", 0.0f, 1.0f, 0.35f, percentAttributes);
    addFloat("stereo_reconstruction", "Stereo Reconstruction", 0.0f, 1.0f, 0.0f, percentAttributes);
    addFloat("phase_offset", "Phase", 0.0f, 1.0f, 0.18f, percentAttributes);
    addFloat("phase_random", "Phase Random", 0.0f, 1.0f, 0.12f, percentAttributes);
    addFloat("time_sync", "Time Sync", 0.0f, 1.0f, 0.0f, percentAttributes);
    addFloat("root_random", "Start Random", 0.0f, 1.0f, 0.0f, percentAttributes);
    addFloat("random_direction", "Random Direction", 0.0f, 1.0f, 0.0f, percentAttributes);
    addFloat("loop_start", "Loop Start", 0.0f, 0.98f, 0.35f, percentAttributes);
    addFloat("loop_end", "Loop End", 0.01f, 1.0f, 0.80f, percentAttributes);
    addFloat("unison_voices", "Unison Voices", 1.0f, 4.0f, 1.0f,
             juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return juce::String(static_cast<int>(std::round(value))) + "x"; }));
    addFloat("unison_detune", "Unison Detune", 0.0f, 50.0f, 7.0f,
             juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return juce::String(value, 1) + " c"; }).withLabel("c"));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode", juce::StringArray{"One-Shot", "Sustain", "Harmonizer"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("quality", "Quality", juce::StringArray{"Eco", "Balanced", "High"}, 1));

    return { params.begin(), params.end() };
}

void BifrostAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
}

void BifrostAudioProcessor::releaseResources()
{
    voiceManager.reset();
}

bool BifrostAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BifrostAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    auto model = std::atomic_load_explicit(&activeModel, std::memory_order_acquire);

    VoiceRenderParameters renderParams;
    renderParams.body = bodyParam->load();
    renderParams.air = airParam->load();
    renderParams.metal = metalParam->load();
    renderParams.brightness = brightnessParam->load();
    renderParams.motion = motionParam->load();
    renderParams.inertia = inertiaParam->load();
    renderParams.mutation = mutationParam->load();
    renderParams.transient = transientParam->load();
    renderParams.formantLock = formantLockParam->load();
    renderParams.timeStretch = timeStretchParam->load();
    renderParams.outputGainDb = outputGainParam->load();
    renderParams.attackSeconds = attackParam->load();
    renderParams.decaySeconds = decayParam->load();
    renderParams.sustainLevel = sustainParam->load();
    renderParams.releaseSeconds = releaseParam->load();
    renderParams.envelopeCurve = adsrCurveParam->load();
    renderParams.velocitySensitivity = velocitySensitivityParam->load();
    renderParams.pan = panParam->load();
    renderParams.stereoWidth = stereoWidthParam->load();
    renderParams.stereoReconstruction = stereoReconstructionParam->load();
    renderParams.phaseOffset = phaseOffsetParam->load();
    renderParams.phaseRandom = phaseRandomParam->load();
    renderParams.timeSync = timeSyncParam->load();
    renderParams.startRandomAmount = rootRandomParam->load();
    renderParams.randomDirection = randomDirectionParam->load();
    renderParams.loopStartNormalized = std::clamp(loopStartParam->load(), 0.0f, 0.98f);
    renderParams.loopEndNormalized = std::clamp(loopEndParam->load(), renderParams.loopStartNormalized + 0.01f, 1.0f);
    renderParams.unisonVoices = std::clamp(static_cast<int>(std::round(unisonVoicesParam->load())), 1, 4);
    renderParams.unisonDetuneCents = unisonDetuneParam->load();
    if (model)
        renderParams.sourceTempoBpm = model->tempoBpm;
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                renderParams.hostTempoBpm = static_cast<float>(*bpm);
        }
    }
    renderParams.mode = static_cast<int>(modeParam->load());
    renderParams.quality = static_cast<int>(qualityParam->load());

    if (renderParams.quality <= 0)
    {
        renderParams.maxVoices = 4;
        renderParams.maxHarmonics = 16;
        renderParams.maxNoiseBands = 6;
        renderParams.maxResonators = 4;
    }
    else if (renderParams.quality == 1)
    {
        renderParams.maxVoices = 8;
        renderParams.maxHarmonics = 48;
        renderParams.maxNoiseBands = 8;
        renderParams.maxResonators = 8;
    }
    else
    {
        renderParams.maxVoices = 12;
        renderParams.maxHarmonics = 96;
        renderParams.maxNoiseBands = 16;
        renderParams.maxResonators = 16;
    }

    voiceManager.render(buffer, midiMessages, model, renderParams);
    applySoftLimiter(buffer);
}

juce::AudioProcessorEditor* BifrostAudioProcessor::createEditor()
{
    return new BifrostAudioProcessorEditor(*this);
}

void BifrostAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    PresetSerializer::writeState(apvts.copyState(), getActiveModel(), destData);
}

void BifrostAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto restored = PresetSerializer::readState(data, sizeInBytes);
    if (restored.state.isValid())
        apvts.replaceState(restored.state);
    if (restored.model)
        installModel(restored.model);
}

std::shared_ptr<const TimbreModel> BifrostAudioProcessor::getActiveModel() const
{
    return std::atomic_load_explicit(&activeModel, std::memory_order_acquire);
}

std::array<VoiceActivity, VoiceManager::maxVoices> BifrostAudioProcessor::getVoiceActivities() const noexcept
{
    return voiceManager.getVoiceActivities();
}

void BifrostAudioProcessor::installModel(std::shared_ptr<const TimbreModel> model)
{
    auto previous = std::atomic_exchange_explicit(&activeModel, std::move(model), std::memory_order_acq_rel);
    retiredModels[retiredModelIndex++ % retiredModels.size()] = std::move(previous);
}

std::optional<float> BifrostAudioProcessor::getRootOverrideHz() const
{
    const float value = rootOverrideHzParam->load();
    if (value >= 20.0f && value <= 20000.0f)
        return value;

    return std::nullopt;
}

void BifrostAudioProcessor::applySoftLimiter(juce::AudioBuffer<float>& buffer) noexcept
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = samples[i];
            if (!std::isfinite(x))
            {
                samples[i] = 0.0f;
                continue;
            }

            x = std::clamp(x, -8.0f, 8.0f);
            samples[i] = x / (1.0f + std::abs(x));
        }
    }
}

void BifrostAudioProcessor::cacheParameterPointers()
{
    bodyParam = apvts.getRawParameterValue("body");
    airParam = apvts.getRawParameterValue("air");
    metalParam = apvts.getRawParameterValue("metal");
    brightnessParam = apvts.getRawParameterValue("brightness");
    motionParam = apvts.getRawParameterValue("motion");
    inertiaParam = apvts.getRawParameterValue("inertia");
    mutationParam = apvts.getRawParameterValue("mutation");
    transientParam = apvts.getRawParameterValue("transient");
    formantLockParam = apvts.getRawParameterValue("formant_lock");
    timeStretchParam = apvts.getRawParameterValue("time_stretch");
    outputGainParam = apvts.getRawParameterValue("output_gain");
    attackParam = apvts.getRawParameterValue("attack");
    decayParam = apvts.getRawParameterValue("decay");
    sustainParam = apvts.getRawParameterValue("sustain");
    releaseParam = apvts.getRawParameterValue("release");
    adsrCurveParam = apvts.getRawParameterValue("adsr_curve");
    velocitySensitivityParam = apvts.getRawParameterValue("velocity_sensitivity");
    panParam = apvts.getRawParameterValue("pan");
    stereoWidthParam = apvts.getRawParameterValue("stereo_width");
    stereoReconstructionParam = apvts.getRawParameterValue("stereo_reconstruction");
    phaseOffsetParam = apvts.getRawParameterValue("phase_offset");
    phaseRandomParam = apvts.getRawParameterValue("phase_random");
    timeSyncParam = apvts.getRawParameterValue("time_sync");
    rootRandomParam = apvts.getRawParameterValue("root_random");
    randomDirectionParam = apvts.getRawParameterValue("random_direction");
    loopStartParam = apvts.getRawParameterValue("loop_start");
    loopEndParam = apvts.getRawParameterValue("loop_end");
    unisonVoicesParam = apvts.getRawParameterValue("unison_voices");
    unisonDetuneParam = apvts.getRawParameterValue("unison_detune");
    rootOverrideHzParam = apvts.getRawParameterValue("root_override_hz");
    modeParam = apvts.getRawParameterValue("mode");
    qualityParam = apvts.getRawParameterValue("quality");
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BifrostAudioProcessor();
}
