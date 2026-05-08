#include "analysis/ResonatorExtractor.h"

ResonatorModel ResonatorExtractor::extractPlaceholder(float durationSeconds, float controlRateHz, int resonatorCount) const
{
    ResonatorModel model;
    model.resonatorCount = resonatorCount;
    model.frequencyHz.resize(static_cast<size_t>(resonatorCount));
    model.q.resize(static_cast<size_t>(resonatorCount));
    model.gains.resize(static_cast<size_t>(resonatorCount));
    const int frames = std::max(1, static_cast<int>(durationSeconds * controlRateHz));

    for (int i = 0; i < resonatorCount; ++i)
    {
        model.frequencyHz[static_cast<size_t>(i)] = 300.0f * std::pow(2.0f, static_cast<float>(i));
        model.q[static_cast<size_t>(i)] = 2.0f;
        model.gains[static_cast<size_t>(i)].sampleRateHz = controlRateHz;
        model.gains[static_cast<size_t>(i)].values.assign(static_cast<size_t>(frames), 0.0f);
    }
    return model;
}
