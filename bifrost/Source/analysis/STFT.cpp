#include "analysis/STFT.h"

#include <limits>
#include <numeric>

namespace
{
std::vector<float> makeHannWindow(int size)
{
    std::vector<float> window(static_cast<size_t>(size), 1.0f);
    if (size <= 1)
        return window;

    const float denominator = static_cast<float>(size - 1);
    for (int i = 0; i < size; ++i)
    {
        const float phase = juce::MathConstants<float>::twoPi * static_cast<float>(i) / denominator;
        window[static_cast<size_t>(i)] = 0.5f - 0.5f * std::cos(phase);
    }

    return window;
}
}

MagnitudeSpectrogram STFT::analyzeMagnitude(const juce::AudioBuffer<float>& mono,
                                            double sampleRate,
                                            int fftSize,
                                            int hopSize) const
{
    MagnitudeSpectrogram out;
    out.fftSize = fftSize;
    out.hopSize = hopSize;
    out.sampleRate = sampleRate;
    out.sourceSampleCount = mono.getNumSamples();

    if (mono.getNumChannels() <= 0 || mono.getNumSamples() <= 0 || sampleRate <= 0.0 || fftSize < 2 || hopSize <= 0 || !isPowerOfTwo(fftSize))
        return out;

    out.binCount = fftSize / 2 + 1;
    out.frameCount = mono.getNumSamples() <= fftSize
        ? 1
        : 1 + (mono.getNumSamples() - fftSize + hopSize - 1) / hopSize;
    out.magnitude.assign(static_cast<size_t>(out.frameCount * out.binCount), 0.0f);

    if (out.frameCount <= 0) return out;

    const int order = static_cast<int>(std::log2(fftSize));
    juce::dsp::FFT fft(order);
    const auto window = makeHannWindow(fftSize);
    const float windowSum = std::accumulate(window.begin(), window.end(), 0.0f);
    const float magnitudeScale = windowSum > 0.0f ? 2.0f / windowSum : 1.0f;
    std::vector<float> fftData(static_cast<size_t>(fftSize * 2), 0.0f);

    const float* x = mono.getReadPointer(0);
    for (int frame = 0; frame < out.frameCount; ++frame)
    {
        std::fill(fftData.begin(), fftData.end(), 0.0f);
        const int offset = frame * hopSize;
        const int available = std::max(0, std::min(fftSize, mono.getNumSamples() - offset));

        for (int i = 0; i < available; ++i)
            fftData[static_cast<size_t>(i)] = x[offset + i] * window[static_cast<size_t>(i)];

        fft.performFrequencyOnlyForwardTransform(fftData.data(), true);

        for (int bin = 0; bin < out.binCount; ++bin)
        {
            const float scale = (bin == 0 || bin == out.binCount - 1) ? magnitudeScale * 0.5f : magnitudeScale;
            out.magnitude[static_cast<size_t>(frame * out.binCount + bin)] = fftData[static_cast<size_t>(bin)] * scale;
        }
    }
    return out;
}

WaveformPreview STFT::makeWaveformPreview(const juce::AudioBuffer<float>& mono,
                                          double sampleRate,
                                          int maxPoints) const
{
    WaveformPreview preview;
    preview.sampleRate = sampleRate;
    preview.durationSeconds = sampleRate > 0.0 ? static_cast<float>(mono.getNumSamples() / sampleRate) : 0.0f;

    if (mono.getNumChannels() <= 0 || mono.getNumSamples() <= 0 || maxPoints <= 0)
        return preview;

    preview.samplesPerPoint = std::max(1, (mono.getNumSamples() + maxPoints - 1) / maxPoints);
    const int pointCount = (mono.getNumSamples() + preview.samplesPerPoint - 1) / preview.samplesPerPoint;
    preview.minimum.assign(static_cast<size_t>(pointCount), 0.0f);
    preview.maximum.assign(static_cast<size_t>(pointCount), 0.0f);
    preview.rms.assign(static_cast<size_t>(pointCount), 0.0f);

    const float* x = mono.getReadPointer(0);
    for (int point = 0; point < pointCount; ++point)
    {
        const int start = point * preview.samplesPerPoint;
        const int end = std::min(mono.getNumSamples(), start + preview.samplesPerPoint);
        float minValue = std::numeric_limits<float>::max();
        float maxValue = std::numeric_limits<float>::lowest();
        double energy = 0.0;

        for (int i = start; i < end; ++i)
        {
            const float value = x[i];
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            energy += static_cast<double>(value) * value;
        }

        const auto index = static_cast<size_t>(point);
        preview.minimum[index] = minValue;
        preview.maximum[index] = maxValue;
        preview.rms[index] = static_cast<float>(std::sqrt(energy / std::max(1, end - start)));
    }

    return preview;
}

Curve STFT::computeRmsCurve(const juce::AudioBuffer<float>& mono,
                            double sampleRate,
                            float controlRateHz) const
{
    Curve curve;
    curve.sampleRateHz = controlRateHz;

    if (mono.getNumChannels() <= 0 || mono.getNumSamples() <= 0 || sampleRate <= 0.0 || controlRateHz <= 0.0f)
        return curve;

    const int frames = getControlFrameCount(static_cast<float>(mono.getNumSamples() / sampleRate), controlRateHz);
    curve.values.assign(static_cast<size_t>(frames), 0.0f);

    const int window = std::max(64, static_cast<int>(std::round(sampleRate / static_cast<double>(controlRateHz))));
    const float* x = mono.getReadPointer(0);

    for (int frame = 0; frame < frames; ++frame)
    {
        const int start = std::clamp(static_cast<int>(std::round(frame * sampleRate / controlRateHz)), 0, mono.getNumSamples() - 1);
        const int end = std::min(mono.getNumSamples(), start + window);
        double energy = 0.0;
        for (int i = start; i < end; ++i)
            energy += static_cast<double>(x[i]) * x[i];

        curve.values[static_cast<size_t>(frame)] = static_cast<float>(std::sqrt(energy / std::max(1, end - start)));
    }

    return curve;
}

SpectralFeatureCurves STFT::computeSpectralFeatures(const MagnitudeSpectrogram& spec,
                                                    float controlRateHz) const
{
    SpectralFeatureCurves curves;
    curves.centroidHz.sampleRateHz = controlRateHz;
    curves.spectralFlux.sampleRateHz = controlRateHz;
    curves.spectralFlatness.sampleRateHz = controlRateHz;

    if (!spec.isValid() || controlRateHz <= 0.0f)
        return curves;

    const int frames = getControlFrameCount(spec.durationSeconds(), controlRateHz);
    curves.centroidHz.values.assign(static_cast<size_t>(frames), 0.0f);
    curves.spectralFlux.values.assign(static_cast<size_t>(frames), 0.0f);
    curves.spectralFlatness.values.assign(static_cast<size_t>(frames), 0.0f);

    for (int outFrame = 0; outFrame < frames; ++outFrame)
    {
        const float timeSeconds = static_cast<float>(outFrame) / controlRateHz;
        const int specFrame = std::clamp(static_cast<int>(std::round(timeSeconds * spec.sampleRate / spec.hopSize)), 0, spec.frameCount - 1);

        double weightedFrequency = 0.0;
        double magnitudeSum = 1.0e-12;
        double logMagnitudeSum = 0.0;
        int usedBins = 0;

        for (int bin = 1; bin < spec.binCount; ++bin)
        {
            const double magnitude = std::max(0.0f, spec.get(specFrame, bin));
            weightedFrequency += spec.getBinFrequencyHz(bin) * magnitude;
            magnitudeSum += magnitude;
            logMagnitudeSum += std::log(magnitude + 1.0e-12);
            ++usedBins;
        }

        curves.centroidHz.values[static_cast<size_t>(outFrame)] = static_cast<float>(weightedFrequency / magnitudeSum);

        const double arithmeticMean = magnitudeSum / std::max(1, usedBins);
        const double geometricMean = std::exp(logMagnitudeSum / std::max(1, usedBins));
        curves.spectralFlatness.values[static_cast<size_t>(outFrame)] = static_cast<float>(std::clamp(geometricMean / std::max(1.0e-12, arithmeticMean), 0.0, 1.0));

        if (specFrame > 0)
        {
            double positiveDelta = 0.0;
            double currentEnergy = 1.0e-12;
            for (int bin = 1; bin < spec.binCount; ++bin)
            {
                const double current = spec.get(specFrame, bin);
                const double previous = spec.get(specFrame - 1, bin);
                positiveDelta += std::max(0.0, current - previous);
                currentEnergy += current;
            }

            curves.spectralFlux.values[static_cast<size_t>(outFrame)] = static_cast<float>(positiveDelta / currentEnergy);
        }
    }

    return curves;
}

bool STFT::isPowerOfTwo(int value) noexcept
{
    return value > 0 && (value & (value - 1)) == 0;
}

int STFT::getControlFrameCount(float durationSeconds, float controlRateHz) noexcept
{
    if (durationSeconds <= 0.0f || controlRateHz <= 0.0f)
        return 0;

    return std::max(1, static_cast<int>(std::ceil(durationSeconds * controlRateHz)));
}
