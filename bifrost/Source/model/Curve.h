#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

struct Curve
{
    float sampleRateHz = 200.0f;
    std::vector<float> values;

    bool empty() const noexcept { return values.empty(); }
    float durationSeconds() const noexcept { return sampleRateHz > 0.0f ? static_cast<float>(values.size()) / sampleRateHz : 0.0f; }

    float sample(float timeSeconds) const noexcept
    {
        if (values.empty()) return 0.0f;
        const float x = std::clamp(timeSeconds * sampleRateHz, 0.0f, static_cast<float>(values.size() - 1));
        const auto i0 = static_cast<size_t>(std::floor(x));
        const auto i1 = std::min(i0 + 1, values.size() - 1);
        const float frac = x - static_cast<float>(i0);
        return values[i0] + frac * (values[i1] - values[i0]);
    }
};
