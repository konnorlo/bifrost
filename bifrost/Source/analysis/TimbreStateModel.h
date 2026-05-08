#pragma once

#include "model/TimbreModel.h"

class TimbreStateModel
{
public:
    StateGraph fitPlaceholder(const TimbreModel& model, int stateCount) const;
};
