#pragma once

#include "model/TimbreModel.h"

class ResonatorExtractor
{
public:
    ResonatorModel extractPlaceholder(float durationSeconds, float controlRateHz, int resonatorCount) const;
};
