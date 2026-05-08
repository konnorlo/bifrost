#pragma once

#include <JuceHeader.h>
#include "model/TimbreModel.h"

struct RestoredPresetState
{
    juce::ValueTree state;
    std::shared_ptr<const TimbreModel> model;
    int modelFormatVersion = 0;
    bool modelFormatMigrated = false;
    bool modelFormatForwardCompatible = false;
    juce::String migrationMessage;
};

class PresetSerializer
{
public:
    static constexpr int currentModelFormatVersion = 1;

    static void writeState(const juce::ValueTree& apvtsState,
                           std::shared_ptr<const TimbreModel> model,
                           juce::MemoryBlock& destData);

    static RestoredPresetState readState(const void* data, int sizeInBytes);
};
