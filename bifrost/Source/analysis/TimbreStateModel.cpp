#include "analysis/TimbreStateModel.h"

namespace
{
float sampleOrZero(const Curve& curve, float timeSeconds)
{
    return curve.empty() ? 0.0f : curve.sample(timeSeconds);
}
}

StateGraph TimbreStateModel::fitPlaceholder(const TimbreModel& model, int stateCount) const
{
    StateGraph graph;
    graph.stateCount = std::max(0, stateCount);
    graph.states.resize(static_cast<size_t>(stateCount));
    graph.transitionMatrix.assign(static_cast<size_t>(stateCount * stateCount), 0.0f);

    if (graph.stateCount <= 0)
        return graph;

    const float duration = std::max(0.001f, model.durationSeconds);
    float usageSum = 0.0f;

    for (int i = 0; i < graph.stateCount; ++i)
    {
        const float norm = graph.stateCount <= 1 ? 0.0f : static_cast<float>(i) / static_cast<float>(graph.stateCount - 1);
        const float t = norm * duration;
        const float centroidNorm = std::clamp(sampleOrZero(model.centroid, t) / 10000.0f, 0.0f, 1.0f);
        const float flatness = std::clamp(sampleOrZero(model.spectralFlatness, t), 0.0f, 1.0f);
        const float loudness = std::clamp(sampleOrZero(model.loudness, t) * 2.0f, 0.0f, 1.0f);
        const float embeddingX = model.timbreEmbedding.empty() ? 0.0f : model.timbreEmbedding[static_cast<size_t>(i) % model.timbreEmbedding.size()];
        const float embeddingY = model.timbreEmbedding.empty() ? 0.0f : model.timbreEmbedding[(static_cast<size_t>(i) * 3u + 1u) % model.timbreEmbedding.size()];
        const float angle = juce::MathConstants<float>::twoPi * norm;
        const float radius = 0.34f + 0.62f * centroidNorm;

        auto& state = graph.states[static_cast<size_t>(i)];
        state.x = std::clamp(std::cos(angle) * radius + embeddingX * 0.18f, -1.0f, 1.0f);
        state.y = std::clamp(std::sin(angle) * radius + embeddingY * 0.18f, -1.0f, 1.0f);
        state.brightness = centroidNorm;
        state.noisiness = flatness;
        state.usage = 0.05f + loudness;
        usageSum += state.usage;
    }

    if (usageSum > 0.0f)
        for (auto& state : graph.states)
            state.usage /= usageSum;

    for (int i = 0; i < graph.stateCount; ++i)
    {
        float rowSum = 0.0f;
        for (int j = 0; j < graph.stateCount; ++j)
        {
            const int distance = std::abs(i - j);
            const float weight = distance == 0 ? 0.58f : std::exp(-static_cast<float>(distance));
            graph.transitionMatrix[static_cast<size_t>(i * graph.stateCount + j)] = weight;
            rowSum += weight;
        }

        if (rowSum > 0.0f)
            for (int j = 0; j < graph.stateCount; ++j)
                graph.transitionMatrix[static_cast<size_t>(i * graph.stateCount + j)] /= rowSum;
    }
    return graph;
}
