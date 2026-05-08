#pragma once

#include "model/Curve.h"

struct TempoEstimate
{
    float bpm = 0.0f;
    float confidence = 0.0f;
};

class TempoAnalyzer
{
public:
    TempoEstimate estimate(const Curve& onsetCurve, float minBpm = 60.0f, float maxBpm = 200.0f) const;
};
