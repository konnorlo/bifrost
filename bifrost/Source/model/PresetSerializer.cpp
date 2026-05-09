#include "model/PresetSerializer.h"

namespace
{
const juce::Identifier presetRootType("BifrostPreset");
const juce::Identifier parametersType("PARAMETERS");
const juce::Identifier modelType("TimbreModel");
const juce::Identifier curveType("Curve");
const juce::Identifier waveformType("WaveformPreview");
const juce::Identifier harmonicsType("Harmonics");
const juce::Identifier noiseType("Noise");
const juce::Identifier resonatorsType("Resonators");
const juce::Identifier transientType("Transient");
const juce::Identifier statesType("States");

int readModelFormatVersion(const juce::ValueTree& root)
{
    return static_cast<int>(root.getProperty("model_format_version", 0));
}

void recordFormatStatus(RestoredPresetState& restored, int version)
{
    restored.modelFormatVersion = version;

    if (version == PresetSerializer::currentModelFormatVersion)
        return;

    restored.modelFormatMigrated = version < PresetSerializer::currentModelFormatVersion;
    restored.modelFormatForwardCompatible = version > PresetSerializer::currentModelFormatVersion;

    if (restored.modelFormatMigrated)
        restored.migrationMessage = "Preset model format migrated to current reader";
    else
        restored.migrationMessage = "Preset model format is newer than this reader";
}

juce::MemoryBlock encodeFloatVector(const std::vector<float>& values)
{
    juce::MemoryBlock block;
    juce::MemoryOutputStream stream(block, false);
    stream.writeInt(static_cast<int>(values.size()));
    for (const auto value : values)
        stream.writeFloat(value);
    return block;
}

std::vector<float> decodeFloatVector(const juce::var& encoded)
{
    std::vector<float> values;
    if (!encoded.isBinaryData())
        return values;

    auto* block = encoded.getBinaryData();
    if (block == nullptr || block->getSize() < sizeof(int))
        return values;

    juce::MemoryInputStream stream(*block, false);
    const int count = stream.readInt();
    if (count < 0 || count > 5000000)
        return values;

    values.resize(static_cast<size_t>(count));
    for (auto& value : values)
        value = stream.readFloat();

    return values;
}

juce::ValueTree writeCurve(const juce::Identifier& type, const Curve& curve)
{
    juce::ValueTree tree(type);
    tree.setProperty("sample_rate_hz", curve.sampleRateHz, nullptr);
    tree.setProperty("values", juce::var(encodeFloatVector(curve.values)), nullptr);
    return tree;
}

Curve readCurve(const juce::ValueTree& tree)
{
    Curve curve;
    curve.sampleRateHz = static_cast<float>(tree.getProperty("sample_rate_hz", 200.0f));
    curve.values = decodeFloatVector(tree.getProperty("values"));
    return curve;
}

juce::ValueTree writeWaveformPreview(const WaveformPreview& preview)
{
    juce::ValueTree tree(waveformType);
    tree.setProperty("sample_rate", preview.sampleRate, nullptr);
    tree.setProperty("duration", preview.durationSeconds, nullptr);
    tree.setProperty("samples_per_point", preview.samplesPerPoint, nullptr);
    tree.setProperty("minimum", juce::var(encodeFloatVector(preview.minimum)), nullptr);
    tree.setProperty("maximum", juce::var(encodeFloatVector(preview.maximum)), nullptr);
    tree.setProperty("rms", juce::var(encodeFloatVector(preview.rms)), nullptr);
    return tree;
}

WaveformPreview readWaveformPreview(const juce::ValueTree& tree)
{
    WaveformPreview preview;
    preview.sampleRate = static_cast<double>(tree.getProperty("sample_rate", 0.0));
    preview.durationSeconds = static_cast<float>(tree.getProperty("duration", 0.0f));
    preview.samplesPerPoint = static_cast<int>(tree.getProperty("samples_per_point", 0));
    preview.minimum = decodeFloatVector(tree.getProperty("minimum"));
    preview.maximum = decodeFloatVector(tree.getProperty("maximum"));
    preview.rms = decodeFloatVector(tree.getProperty("rms"));
    return preview;
}

juce::ValueTree writeModel(const TimbreModel& model)
{
    juce::ValueTree tree(modelType);
    tree.setProperty("format_version", model.modelFormatVersion, nullptr);
    tree.setProperty("analysis_sample_rate", model.analysisSampleRate, nullptr);
    tree.setProperty("control_rate_hz", model.controlRateHz, nullptr);
    tree.setProperty("duration", model.durationSeconds, nullptr);
    tree.setProperty("root_hz", model.detectedRootHz, nullptr);
    tree.setProperty("pitch_confidence", model.pitchConfidence, nullptr);
    tree.setProperty("tempo_bpm", model.tempoBpm, nullptr);
    tree.setProperty("tempo_confidence", model.tempoConfidence, nullptr);
    tree.setProperty("source_path", model.sourcePath, nullptr);
    tree.setProperty("source_hash", model.sourceHash, nullptr);
    tree.setProperty("used_ml_embedding", model.usedMlEmbedding, nullptr);
    tree.setProperty("ml_backend", model.mlBackend, nullptr);
    tree.setProperty("timbre_embedding", juce::var(encodeFloatVector(model.timbreEmbedding)), nullptr);

    tree.addChild(writeCurve("Loudness", model.loudness), -1, nullptr);
    tree.addChild(writeCurve("Centroid", model.centroid), -1, nullptr);
    tree.addChild(writeCurve("SpectralFlux", model.spectralFlux), -1, nullptr);
    tree.addChild(writeCurve("SpectralFlatness", model.spectralFlatness), -1, nullptr);
    tree.addChild(writeCurve("PitchHz", model.pitchHz), -1, nullptr);
    tree.addChild(writeCurve("PitchConfidenceCurve", model.pitchConfidenceCurve), -1, nullptr);
    tree.addChild(writeWaveformPreview(model.waveformPreview), -1, nullptr);

    juce::ValueTree harmonics(harmonicsType);
    harmonics.setProperty("count", model.harmonics.harmonicCount, nullptr);
    harmonics.setProperty("energy_explained", model.harmonics.energyExplained, nullptr);
    for (const auto& curve : model.harmonics.harmonicAmplitudes)
        harmonics.addChild(writeCurve(curveType, curve), -1, nullptr);
    tree.addChild(harmonics, -1, nullptr);

    juce::ValueTree noise(noiseType);
    noise.setProperty("count", model.noise.bandCount, nullptr);
    noise.setProperty("centers_hz", juce::var(encodeFloatVector(model.noise.bandCenterHz)), nullptr);
    for (const auto& curve : model.noise.bandAmplitudes)
        noise.addChild(writeCurve(curveType, curve), -1, nullptr);
    tree.addChild(noise, -1, nullptr);

    juce::ValueTree resonators(resonatorsType);
    resonators.setProperty("count", model.resonators.resonatorCount, nullptr);
    resonators.setProperty("frequencies_hz", juce::var(encodeFloatVector(model.resonators.frequencyHz)), nullptr);
    resonators.setProperty("q", juce::var(encodeFloatVector(model.resonators.q)), nullptr);
    for (const auto& curve : model.resonators.gains)
        resonators.addChild(writeCurve(curveType, curve), -1, nullptr);
    tree.addChild(resonators, -1, nullptr);

    juce::ValueTree transient(transientType);
    transient.setProperty("attack_end_seconds", model.transient.attackEndSeconds, nullptr);
    transient.addChild(writeCurve("AttackEnergy", model.transient.attackEnergy), -1, nullptr);
    tree.addChild(transient, -1, nullptr);

    juce::ValueTree states(statesType);
    states.setProperty("count", model.states.stateCount, nullptr);
    states.setProperty("transitions", juce::var(encodeFloatVector(model.states.transitionMatrix)), nullptr);
    for (const auto& state : model.states.states)
    {
        juce::ValueTree node("State");
        node.setProperty("x", state.x, nullptr);
        node.setProperty("y", state.y, nullptr);
        node.setProperty("brightness", state.brightness, nullptr);
        node.setProperty("noisiness", state.noisiness, nullptr);
        node.setProperty("usage", state.usage, nullptr);
        states.addChild(node, -1, nullptr);
    }
    tree.addChild(states, -1, nullptr);

    return tree;
}

std::shared_ptr<TimbreModel> readModel(const juce::ValueTree& tree)
{
    if (!tree.isValid() || tree.getType() != modelType)
        return {};

    auto model = std::make_shared<TimbreModel>();
    model->modelFormatVersion = static_cast<int>(tree.getProperty("format_version", 1));
    model->analysisSampleRate = static_cast<double>(tree.getProperty("analysis_sample_rate", 44100.0));
    model->controlRateHz = static_cast<float>(tree.getProperty("control_rate_hz", 200.0f));
    model->durationSeconds = static_cast<float>(tree.getProperty("duration", 0.0f));
    model->detectedRootHz = static_cast<float>(tree.getProperty("root_hz", 440.0f));
    model->pitchConfidence = static_cast<float>(tree.getProperty("pitch_confidence", 0.0f));
    model->tempoBpm = static_cast<float>(tree.getProperty("tempo_bpm", 0.0f));
    model->tempoConfidence = static_cast<float>(tree.getProperty("tempo_confidence", 0.0f));
    model->sourcePath = tree.getProperty("source_path", {});
    model->sourceHash = tree.getProperty("source_hash", {});
    model->usedMlEmbedding = static_cast<bool>(tree.getProperty("used_ml_embedding", false));
    model->mlBackend = tree.getProperty("ml_backend", {});
    model->timbreEmbedding = decodeFloatVector(tree.getProperty("timbre_embedding"));

    model->loudness = readCurve(tree.getChildWithName("Loudness"));
    model->centroid = readCurve(tree.getChildWithName("Centroid"));
    model->spectralFlux = readCurve(tree.getChildWithName("SpectralFlux"));
    model->spectralFlatness = readCurve(tree.getChildWithName("SpectralFlatness"));
    model->pitchHz = readCurve(tree.getChildWithName("PitchHz"));
    model->pitchConfidenceCurve = readCurve(tree.getChildWithName("PitchConfidenceCurve"));
    model->waveformPreview = readWaveformPreview(tree.getChildWithName(waveformType));

    auto harmonics = tree.getChildWithName(harmonicsType);
    model->harmonics.harmonicCount = static_cast<int>(harmonics.getProperty("count", 0));
    model->harmonics.energyExplained = static_cast<float>(harmonics.getProperty("energy_explained", 0.0f));
    for (int i = 0; i < harmonics.getNumChildren(); ++i)
        model->harmonics.harmonicAmplitudes.push_back(readCurve(harmonics.getChild(i)));

    auto noise = tree.getChildWithName(noiseType);
    model->noise.bandCount = static_cast<int>(noise.getProperty("count", 0));
    model->noise.bandCenterHz = decodeFloatVector(noise.getProperty("centers_hz"));
    for (int i = 0; i < noise.getNumChildren(); ++i)
        model->noise.bandAmplitudes.push_back(readCurve(noise.getChild(i)));

    auto resonators = tree.getChildWithName(resonatorsType);
    model->resonators.resonatorCount = static_cast<int>(resonators.getProperty("count", 0));
    model->resonators.frequencyHz = decodeFloatVector(resonators.getProperty("frequencies_hz"));
    model->resonators.q = decodeFloatVector(resonators.getProperty("q"));
    for (int i = 0; i < resonators.getNumChildren(); ++i)
        model->resonators.gains.push_back(readCurve(resonators.getChild(i)));

    auto transient = tree.getChildWithName(transientType);
    model->transient.attackEndSeconds = static_cast<float>(transient.getProperty("attack_end_seconds", 0.05f));
    model->transient.attackEnergy = readCurve(transient.getChildWithName("AttackEnergy"));

    auto states = tree.getChildWithName(statesType);
    model->states.stateCount = static_cast<int>(states.getProperty("count", 0));
    model->states.transitionMatrix = decodeFloatVector(states.getProperty("transitions"));
    for (int i = 0; i < states.getNumChildren(); ++i)
    {
        const auto node = states.getChild(i);
        TimbreState state;
        state.x = static_cast<float>(node.getProperty("x", 0.0f));
        state.y = static_cast<float>(node.getProperty("y", 0.0f));
        state.brightness = static_cast<float>(node.getProperty("brightness", 0.0f));
        state.noisiness = static_cast<float>(node.getProperty("noisiness", 0.0f));
        state.usage = static_cast<float>(node.getProperty("usage", 0.0f));
        model->states.states.push_back(state);
    }

    if (!model->isUsable())
        return {};

    return model;
}
}

void PresetSerializer::writeState(const juce::ValueTree& apvtsState,
                                  std::shared_ptr<const TimbreModel> model,
                                  juce::MemoryBlock& destData)
{
    juce::ValueTree root(presetRootType);
    root.setProperty("model_format_version", currentModelFormatVersion, nullptr);
    root.addChild(apvtsState.createCopy(), -1, nullptr);

    if (model)
    {
        juce::ValueTree meta("ModelMeta");
        meta.setProperty("duration", model->durationSeconds, nullptr);
        meta.setProperty("root_hz", model->detectedRootHz, nullptr);
        meta.setProperty("tempo_bpm", model->tempoBpm, nullptr);
        meta.setProperty("source_path", model->sourcePath, nullptr);
        meta.setProperty("source_hash", model->sourceHash, nullptr);
        root.addChild(meta, -1, nullptr);
        root.addChild(writeModel(*model), -1, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(root.createXml());
    juce::AudioProcessor::copyXmlToBinary(*xml, destData);
}

RestoredPresetState PresetSerializer::readState(const void* data, int sizeInBytes)
{
    RestoredPresetState restored;
    std::unique_ptr<juce::XmlElement> xml(juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));
    if (!xml) return restored;

    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.isValid()) return restored;

    if (root.getType() == parametersType)
    {
        restored.state = root;
        recordFormatStatus(restored, 0);
        return restored;
    }

    if (root.getType() != presetRootType)
        return restored;

    recordFormatStatus(restored, readModelFormatVersion(root));

    for (int i = 0; i < root.getNumChildren(); ++i)
    {
        auto child = root.getChild(i);
        if (child.getType() == parametersType)
            restored.state = child;
        else if (child.getType() == modelType)
            restored.model = readModel(child);
    }

    return restored;
}
