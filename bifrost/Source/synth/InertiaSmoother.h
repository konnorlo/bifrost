#pragma once

class InertiaSmoother
{
public:
    void reset(float value = 0.0f) noexcept;
    float process(float target, float inertia) noexcept;

private:
    float y = 0.0f;
    float v = 0.0f;
};
