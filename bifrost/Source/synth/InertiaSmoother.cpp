#include "synth/InertiaSmoother.h"
#include <algorithm>

void InertiaSmoother::reset(float value) noexcept
{
    y = value;
    v = 0.0f;
}

float InertiaSmoother::process(float target, float inertia) noexcept
{
    const float alpha = std::clamp(inertia, 0.0f, 0.98f);
    const float beta = 0.08f + (1.0f - alpha) * 0.25f;
    v = alpha * v + (1.0f - alpha) * (target - y);
    y += beta * v;
    return y;
}
