#include "synth/AdditiveOscBank.h"

void AdditiveOscBank::prepare(double sampleRate, int maxHarmonics)
{
    fs = sampleRate;
    sine.assign(static_cast<size_t>(maxHarmonics), 0.0f);
    cosine.assign(static_cast<size_t>(maxHarmonics), 1.0f);
    rotatorSine.assign(static_cast<size_t>(maxHarmonics), 0.0f);
    rotatorCosine.assign(static_cast<size_t>(maxHarmonics), 1.0f);
}

void AdditiveOscBank::reset()
{
    std::fill(sine.begin(), sine.end(), 0.0f);
    std::fill(cosine.begin(), cosine.end(), 1.0f);
}

void AdditiveOscBank::setBaseFrequency(float midiHz)
{
    baseFrequencyHz = std::max(0.0f, midiHz);

    for (size_t h = 0; h < rotatorSine.size(); ++h)
    {
        const float harmonic = static_cast<float>(h + 1);
        const float phaseDelta = juce::MathConstants<float>::twoPi * baseFrequencyHz * harmonic / static_cast<float>(fs);
        rotatorSine[h] = std::sin(phaseDelta);
        rotatorCosine[h] = std::cos(phaseDelta);
    }
}

float AdditiveOscBank::renderSample(const TimbreModel& model,
                                    float modelTimeSeconds,
                                    const VoiceRenderParameters& params,
                                    float phaseOffsetCycles,
                                    float alternateModelTimeSeconds,
                                    float alternateMix)
{
    if (model.harmonics.harmonicCount <= 0) return 0.0f;

    float y = 0.0f;
    const int count = std::min({ model.harmonics.harmonicCount, static_cast<int>(sine.size()), std::max(1, params.maxHarmonics) });
    const float altMix = std::clamp(alternateMix, 0.0f, 1.0f);
    const bool useAlternate = alternateModelTimeSeconds >= 0.0f && altMix > 0.001f;
    const float loud = useAlternate
        ? juce::jmap(altMix, model.loudness.sample(modelTimeSeconds), model.loudness.sample(alternateModelTimeSeconds))
        : model.loudness.sample(modelTimeSeconds);
    const float brightnessTilt = 0.35f + 1.3f * params.brightness;

    for (int h = 1; h <= count; ++h)
    {
        const float hz = baseFrequencyHz * static_cast<float>(h);
        if (hz > 0.45f * static_cast<float>(fs)) break;

        const auto index = static_cast<size_t>(h - 1);
        const float primaryAmp = model.harmonics.harmonicAmplitudes[index].sample(modelTimeSeconds);
        const float analysedAmp = useAlternate
            ? juce::jmap(altMix, primaryAmp, model.harmonics.harmonicAmplitudes[index].sample(alternateModelTimeSeconds))
            : primaryAmp;
        const float amp = analysedAmp
                        * loud
                        * params.body
                        * std::pow(1.0f / static_cast<float>(h), 1.0f - brightnessTilt);

        const float phaseOffset = juce::MathConstants<float>::twoPi * phaseOffsetCycles * static_cast<float>(h);
        y += amp * (sine[index] * std::cos(phaseOffset) + cosine[index] * std::sin(phaseOffset));

        const float nextSine = sine[index] * rotatorCosine[index] + cosine[index] * rotatorSine[index];
        const float nextCosine = cosine[index] * rotatorCosine[index] - sine[index] * rotatorSine[index];
        sine[index] = nextSine;
        cosine[index] = nextCosine;
    }

    return y;
}

float AdditiveOscBank::renderStaticSample(const std::vector<float>& harmonicAmplitudes,
                                          float loudness,
                                          const VoiceRenderParameters& params,
                                          float phaseOffsetCycles)
{
    if (harmonicAmplitudes.empty())
        return 0.0f;

    float y = 0.0f;
    const int count = std::min({ static_cast<int>(harmonicAmplitudes.size()), static_cast<int>(sine.size()), std::max(1, params.maxHarmonics) });
    const float brightnessTilt = 0.35f + 1.3f * params.brightness;

    for (int h = 1; h <= count; ++h)
    {
        const float hz = baseFrequencyHz * static_cast<float>(h);
        if (hz > 0.45f * static_cast<float>(fs)) break;

        const float amp = harmonicAmplitudes[static_cast<size_t>(h - 1)]
                        * loudness
                        * params.body
                        * std::pow(1.0f / static_cast<float>(h), 1.0f - brightnessTilt);

        const auto index = static_cast<size_t>(h - 1);
        const float phaseOffset = juce::MathConstants<float>::twoPi * phaseOffsetCycles * static_cast<float>(h);
        y += amp * (sine[index] * std::cos(phaseOffset) + cosine[index] * std::sin(phaseOffset));

        const float nextSine = sine[index] * rotatorCosine[index] + cosine[index] * rotatorSine[index];
        const float nextCosine = cosine[index] * rotatorCosine[index] - sine[index] * rotatorSine[index];
        sine[index] = nextSine;
        cosine[index] = nextCosine;
    }

    return y;
}
