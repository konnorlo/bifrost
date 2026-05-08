#include "synth/NoiseBank.h"

void NoiseBank::prepare(double sampleRate, int maxBands)
{
    fs = sampleRate;
    filters.resize(static_cast<size_t>(maxBands));

    juce::dsp::ProcessSpec spec { sampleRate, 1, 1 };
    for (auto& filter : filters)
    {
        filter.prepare(spec);
        filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    }

    reset();
}

void NoiseBank::reset()
{
    last = 0.0f;
    for (auto& filter : filters)
        filter.reset();
}

float NoiseBank::renderSample(const TimbreModel& model, float modelTimeSeconds, const VoiceRenderParameters& params)
{
    if (model.noise.bandCount <= 0 || params.air <= 0.001f) return 0.0f;

    const int count = std::min({ model.noise.bandCount, static_cast<int>(filters.size()), std::max(1, params.maxNoiseBands) });
    const float noise = rng.nextFloat() * 2.0f - 1.0f;
    last = 0.98f * last + 0.02f * noise;
    const float source = 0.75f * noise + 0.25f * last;
    float y = 0.0f;

    for (int b = 0; b < count; ++b)
    {
        const auto index = static_cast<size_t>(b);
        const float gain = std::clamp(model.noise.bandAmplitudes[index].sample(modelTimeSeconds) * params.air, 0.0f, 2.0f);
        if (gain <= 1.0e-5f)
            continue;

        auto& filter = filters[index];
        const float centerHz = std::clamp(model.noise.bandCenterHz[index], 40.0f, 0.45f * static_cast<float>(fs));
        filter.setCutoffFrequency(centerHz);
        filter.setResonance(1.0f);
        y += gain * filter.processSample(0, source);
    }

    return 0.35f * y;
}
