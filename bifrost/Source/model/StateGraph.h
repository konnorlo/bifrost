#pragma once

#include <vector>

struct TimbreState
{
    float x = 0.0f;
    float y = 0.0f;
    float brightness = 0.0f;
    float noisiness = 0.0f;
    float usage = 0.0f;
};

struct StateGraph
{
    int stateCount = 0;
    std::vector<TimbreState> states;
    std::vector<float> transitionMatrix;

    float transition(int from, int to) const noexcept
    {
        if (from < 0 || to < 0 || from >= stateCount || to >= stateCount) return 0.0f;
        return transitionMatrix[static_cast<size_t>(from * stateCount + to)];
    }
};
