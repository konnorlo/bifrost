#include "analysis/NoiseExtractor.h"

NoiseModel NoiseExtractor::extractPlaceholder(float durationSeconds, float controlRateHz, int bandCount) const
{
    NoiseModel model;
    model.bandCount = bandCount;
    model.bandCenterHz.resize(static_cast<size_t>(bandCount));
    model.bandAmplitudes.resize(static_cast<size_t>(bandCount));
    const int frames = std::max(1, static_cast<int>(durationSeconds * controlRateHz));

    for (int b = 0; b < bandCount; ++b)
    {
        const float norm = static_cast<float>(b + 1) / static_cast<float>(bandCount + 1);
        model.bandCenterHz[static_cast<size_t>(b)] = 80.0f * std::pow(160.0f, norm);
        model.bandAmplitudes[static_cast<size_t>(b)].sampleRateHz = controlRateHz;
        model.bandAmplitudes[static_cast<size_t>(b)].values.assign(static_cast<size_t>(frames), 0.0f);
    }
    return model;
}
