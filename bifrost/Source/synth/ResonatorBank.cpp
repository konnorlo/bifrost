#include "synth/ResonatorBank.h"

void ResonatorBank::prepare(double sampleRate, int maxResonators)
{
    fs = sampleRate;
    filters.resize(static_cast<size_t>(maxResonators));
    lastFrequencyHz.assign(static_cast<size_t>(maxResonators), -1.0f);
    lastQ.assign(static_cast<size_t>(maxResonators), -1.0f);
    juce::dsp::ProcessSpec spec { sampleRate, 1, 1 };
    for (auto& f : filters)
    {
        f.prepare(spec);
        f.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    }
}

void ResonatorBank::reset()
{
    std::fill(lastFrequencyHz.begin(), lastFrequencyHz.end(), -1.0f);
    std::fill(lastQ.begin(), lastQ.end(), -1.0f);
    for (auto& f : filters) f.reset();
}

float ResonatorBank::process(float input, const TimbreModel& model, float modelTimeSeconds, const VoiceRenderParameters& params)
{
    const int count = std::min({ model.resonators.resonatorCount, static_cast<int>(filters.size()), std::max(0, params.maxResonators) });
    float y = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        auto& f = filters[static_cast<size_t>(i)];
        const auto index = static_cast<size_t>(i);
        const float hz = std::clamp(model.resonators.frequencyHz[static_cast<size_t>(i)], 40.0f, 0.45f * static_cast<float>(fs));
        const float q = std::clamp(model.resonators.q[static_cast<size_t>(i)] * (1.0f + 1.2f * std::clamp(params.metal, 0.0f, 2.0f)), 0.4f, 24.0f);
        const float metalGain = 0.08f + 4.20f * std::pow(std::clamp(params.metal, 0.0f, 2.0f), 1.45f);
        const float g = std::clamp(model.resonators.gains[static_cast<size_t>(i)].sample(modelTimeSeconds) * metalGain, 0.0f, 8.0f);
        if (index >= lastFrequencyHz.size() || std::abs(hz - lastFrequencyHz[index]) > 0.01f || std::abs(q - lastQ[index]) > 0.001f)
        {
            f.setCutoffFrequency(hz);
            f.setResonance(q);
            if (index < lastFrequencyHz.size())
            {
                lastFrequencyHz[index] = hz;
                lastQ[index] = q;
            }
        }
        y += g * f.processSample(0, input);
    }
    return 1.15f * y;
}
