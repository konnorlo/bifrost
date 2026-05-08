#include "analysis/HarmonicExtractor.h"

namespace
{
int getControlFrameCount(float durationSeconds, float controlRateHz) noexcept
{
    if (durationSeconds <= 0.0f || controlRateHz <= 0.0f)
        return 0;

    return std::max(1, static_cast<int>(std::ceil(durationSeconds * controlRateHz)));
}
}

HarmonicModel HarmonicExtractor::extract(const MagnitudeSpectrogram& spec,
                                         const Curve& pitchHz,
                                         double sampleRate,
                                         float controlRateHz,
    int harmonicCount) const
{
    HarmonicModel model;
    model.harmonicCount = std::max(0, harmonicCount);
    model.harmonicAmplitudes.resize(static_cast<size_t>(model.harmonicCount));

    const float durationSeconds = spec.durationSeconds() > 0.0f
        ? spec.durationSeconds()
        : static_cast<float>(spec.frameCount * spec.hopSize / sampleRate);
    const int frames = std::max(1, getControlFrameCount(durationSeconds, controlRateHz));
    for (auto& c : model.harmonicAmplitudes)
    {
        c.sampleRateHz = controlRateHz;
        c.values.assign(static_cast<size_t>(frames), 0.0f);
    }

    if (!spec.isValid() || spec.binCount < 3 || sampleRate <= 0.0 || controlRateHz <= 0.0f || model.harmonicCount <= 0)
        return model;

    double explainedEnergy = 0.0;
    double totalEnergy = 1.0e-9;

    for (int outFrame = 0; outFrame < frames; ++outFrame)
    {
        const float t = static_cast<float>(outFrame) / controlRateHz;
        const int specFrame = std::clamp(static_cast<int>(std::round(t * sampleRate / spec.hopSize)), 0, spec.frameCount - 1);
        const float f0 = std::max(20.0f, pitchHz.sample(t));
        float sum = 1.0e-6f;
        float amps[128]{};

        double frameEnergy = 1.0e-9;
        for (int bin = 1; bin < spec.binCount; ++bin)
        {
            const double magnitude = spec.get(specFrame, bin);
            frameEnergy += magnitude * magnitude;
        }

        double harmonicEnergy = 0.0;
        for (int h = 1; h <= model.harmonicCount && h < 128; ++h)
        {
            const float hz = h * f0;
            if (hz > 0.45f * static_cast<float>(sampleRate)) break;
            const float exactBin = hz / static_cast<float>(sampleRate) * static_cast<float>(spec.fftSize);
            const int bin0 = std::clamp(static_cast<int>(std::floor(exactBin)), 1, spec.binCount - 2);
            const float frac = std::clamp(exactBin - static_cast<float>(bin0), 0.0f, 1.0f);
            const float interpolated = spec.get(specFrame, bin0) + frac * (spec.get(specFrame, bin0 + 1) - spec.get(specFrame, bin0));
            const float shoulders = 0.5f * (spec.get(specFrame, bin0 - 1) + spec.get(specFrame, bin0 + 1));
            const float a = 0.75f * interpolated + 0.25f * shoulders;
            amps[h - 1] = a;
            sum += a;

            for (int offset = -1; offset <= 1; ++offset)
            {
                const int energyBin = std::clamp(static_cast<int>(std::round(exactBin)) + offset, 1, spec.binCount - 1);
                const double magnitude = spec.get(specFrame, energyBin);
                harmonicEnergy += magnitude * magnitude;
            }
        }

        for (int h = 0; h < model.harmonicCount; ++h)
            model.harmonicAmplitudes[static_cast<size_t>(h)].values[static_cast<size_t>(outFrame)] = amps[h] / sum;

        explainedEnergy += std::min(harmonicEnergy, frameEnergy);
        totalEnergy += frameEnergy;
    }

    model.energyExplained = static_cast<float>(std::clamp(explainedEnergy / totalEnergy, 0.0, 1.0));
    return model;
}

Curve HarmonicExtractor::extractLoudness(const juce::AudioBuffer<float>& mono, double sampleRate, float controlRateHz) const
{
    return STFT().computeRmsCurve(mono, sampleRate, controlRateHz);
}

Curve HarmonicExtractor::extractCentroid(const MagnitudeSpectrogram& spec, double sampleRate, float controlRateHz) const
{
    juce::ignoreUnused(sampleRate);
    return STFT().computeSpectralFeatures(spec, controlRateHz).centroidHz;
}
