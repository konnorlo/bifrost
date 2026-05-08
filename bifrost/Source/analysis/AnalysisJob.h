#pragma once

#include <memory>
#include <optional>
#include <functional>
#include "audio/AudioFileLoader.h"
#include "model/TimbreModel.h"

class AnalysisJob
{
public:
    using ProgressCallback = std::function<void(juce::String, float)>;
    using ShouldCancelCallback = std::function<bool()>;

    std::shared_ptr<const TimbreModel> run(const LoadedAudioFile& loaded,
                                           std::optional<float> rootOverrideHz = std::nullopt,
                                           ProgressCallback progress = {},
                                           ShouldCancelCallback shouldCancel = {});

private:
    static juce::AudioBuffer<float> makeMono(const juce::AudioBuffer<float>& input);
    static bool cancelled(const ShouldCancelCallback& shouldCancel);
};
