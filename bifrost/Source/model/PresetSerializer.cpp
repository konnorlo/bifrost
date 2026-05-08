#include "model/PresetSerializer.h"

namespace
{
const juce::Identifier presetRootType("BifrostPreset");
const juce::Identifier parametersType("PARAMETERS");

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
    }

    return restored;
}
