#include "ml/TinyTimbreEncoder.h"

namespace
{
constexpr int embeddingSize = 16;
}

TinyTimbreEncoder::TinyTimbreEncoder()
    : modelFile(discoverModelFile())
{
}

bool TinyTimbreEncoder::isAvailable() const noexcept
{
#if BIFROST_ENABLE_ONNX
    return modelFile.existsAsFile();
#else
    return false;
#endif
}

juce::String TinyTimbreEncoder::getBackendName() const
{
#if BIFROST_ENABLE_ONNX
    if (modelFile.existsAsFile())
        return "ONNX model discovered";
#endif
    return "DSP embedding fallback";
}

juce::File TinyTimbreEncoder::getModelFile() const
{
    return modelFile;
}

MelPatch TinyTimbreEncoder::makeLogMelPatch(const MagnitudeSpectrogram& spec, int melBins, int frames) const
{
    MelPatch patch;
    patch.melBins = std::max(0, melBins);
    patch.frames = std::max(0, frames);

    if (!spec.isValid() || patch.melBins <= 0 || patch.frames <= 0)
        return patch;

    patch.values.assign(static_cast<size_t>(patch.melBins * patch.frames), 0.0f);
    const float nyquist = static_cast<float>(spec.sampleRate * 0.5);
    const float melMin = hzToMel(20.0f);
    const float melMax = hzToMel(std::max(40.0f, nyquist));

    for (int outFrame = 0; outFrame < patch.frames; ++outFrame)
    {
        const int sourceFrame = std::clamp(static_cast<int>(std::round(static_cast<float>(outFrame) * static_cast<float>(spec.frameCount - 1) / static_cast<float>(std::max(1, patch.frames - 1)))), 0, spec.frameCount - 1);

        for (int melBin = 0; melBin < patch.melBins; ++melBin)
        {
            const float mel0 = melMin + (melMax - melMin) * static_cast<float>(melBin) / static_cast<float>(patch.melBins + 1);
            const float mel1 = melMin + (melMax - melMin) * static_cast<float>(melBin + 2) / static_cast<float>(patch.melBins + 1);
            const float hz0 = melToHz(mel0);
            const float hz1 = melToHz(mel1);
            const int bin0 = std::clamp(static_cast<int>(std::floor(hz0 * static_cast<float>(spec.fftSize) / static_cast<float>(spec.sampleRate))), 1, spec.binCount - 1);
            const int bin1 = std::clamp(static_cast<int>(std::ceil(hz1 * static_cast<float>(spec.fftSize) / static_cast<float>(spec.sampleRate))), bin0, spec.binCount - 1);

            double energy = 1.0e-9;
            for (int bin = bin0; bin <= bin1; ++bin)
            {
                const double magnitude = spec.get(sourceFrame, bin);
                energy += magnitude * magnitude;
            }

            patch.values[static_cast<size_t>(outFrame * patch.melBins + melBin)] = static_cast<float>(std::log(energy));
        }
    }

    double mean = 0.0;
    for (float value : patch.values)
        mean += value;
    mean /= std::max<size_t>(1, patch.values.size());

    double variance = 1.0e-9;
    for (float value : patch.values)
        variance += (value - mean) * (value - mean);
    const float stddev = static_cast<float>(std::sqrt(variance / std::max<size_t>(1, patch.values.size())));

    for (auto& value : patch.values)
        value = (value - static_cast<float>(mean)) / std::max(1.0e-5f, stddev);

    return patch;
}

std::vector<float> TinyTimbreEncoder::encodeLogMelPatch(const float* data, int melBins, int frames) const
{
    std::vector<float> embedding(embeddingSize, 0.0f);
    if (data == nullptr || melBins <= 0 || frames <= 0)
        return embedding;

    const int count = melBins * frames;
    for (int i = 0; i < count; ++i)
    {
        const float value = data[i];
        const int mel = i % melBins;
        const int frame = i / melBins;

        embedding[0] += value;
        embedding[1] += std::abs(value);
        embedding[2] += value * value;
        embedding[3] += static_cast<float>(mel) * value;
        embedding[4] += static_cast<float>(frame) * value;
        embedding[static_cast<size_t>(5 + (mel % 6))] += value;
        embedding[static_cast<size_t>(11 + (frame % 5))] += value;
    }

    const float normalizer = 1.0f / std::max(1, count);
    float norm = 1.0e-6f;
    for (auto& value : embedding)
    {
        value *= normalizer;
        norm += value * value;
    }

    norm = std::sqrt(norm);
    for (auto& value : embedding)
        value = std::clamp(value / norm, -1.0f, 1.0f);

    return embedding;
}

juce::File TinyTimbreEncoder::discoverModelFile()
{
#if BIFROST_ENABLE_ONNX
    if (juce::String(BIFROST_ONNX_MODEL_PATH).isNotEmpty())
    {
        juce::File configured(BIFROST_ONNX_MODEL_PATH);
        if (configured.existsAsFile())
            return configured;
    }

    auto current = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    for (int i = 0; i < 8 && current != juce::File(); ++i)
    {
        auto candidate = current.getSiblingFile("models").getChildFile("inference").getChildFile("tiny_timbre_encoder.onnx");
        if (candidate.existsAsFile())
            return candidate;

        candidate = current.getChildFile("models").getChildFile("inference").getChildFile("tiny_timbre_encoder.onnx");
        if (candidate.existsAsFile())
            return candidate;

        current = current.getParentDirectory();
    }
#endif
    return {};
}

float TinyTimbreEncoder::hzToMel(float hz) noexcept
{
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

float TinyTimbreEncoder::melToHz(float mel) noexcept
{
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}
