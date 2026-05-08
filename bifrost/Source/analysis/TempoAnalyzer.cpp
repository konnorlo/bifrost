#include "analysis/TempoAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <numeric>

TempoEstimate TempoAnalyzer::estimate(const Curve& onsetCurve, float minBpm, float maxBpm) const
{
    TempoEstimate estimate;

    if (onsetCurve.values.size() < 8 || onsetCurve.sampleRateHz <= 0.0f || minBpm <= 0.0f || maxBpm <= minBpm)
        return estimate;

    const auto& values = onsetCurve.values;
    const float mean = std::accumulate(values.begin(), values.end(), 0.0f) / static_cast<float>(values.size());

    std::vector<float> centered(values.size(), 0.0f);
    float energy = 0.0f;
    for (size_t i = 0; i < values.size(); ++i)
    {
        centered[i] = std::max(0.0f, values[i] - mean);
        energy += centered[i] * centered[i];
    }

    if (energy <= 1.0e-9f)
        return estimate;

    const int minLag = std::max(1, static_cast<int>(std::floor(onsetCurve.sampleRateHz * 60.0f / maxBpm)));
    const int maxLag = std::min(static_cast<int>(centered.size()) - 1,
                                static_cast<int>(std::ceil(onsetCurve.sampleRateHz * 60.0f / minBpm)));

    if (maxLag <= minLag)
        return estimate;

    float bestScore = 0.0f;
    int bestLag = 0;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float score = 0.0f;
        for (size_t i = static_cast<size_t>(lag); i < centered.size(); ++i)
            score += centered[i] * centered[i - static_cast<size_t>(lag)];

        const float normalized = score / energy;
        if (normalized > bestScore)
        {
            bestScore = normalized;
            bestLag = lag;
        }
    }

    if (bestLag <= 0 || bestScore <= 0.01f)
        return estimate;

    estimate.bpm = 60.0f * onsetCurve.sampleRateHz / static_cast<float>(bestLag);
    estimate.confidence = std::clamp(bestScore, 0.0f, 1.0f);
    return estimate;
}
