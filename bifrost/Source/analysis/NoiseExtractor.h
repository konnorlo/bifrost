#pragma once

#include "model/TimbreModel.h"

class NoiseExtractor
{
public:
    NoiseModel extractPlaceholder(float durationSeconds, float controlRateHz, int bandCount) const;
};
