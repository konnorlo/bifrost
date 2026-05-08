#include "analysis/PitchTracker.h"

#include <numeric>

PitchTrack PitchTracker::estimate(const juce::AudioBuffer<float>& mono,
                                  double sampleRate,
                                  float controlRateHz,
                                  std::optional<float> rootOverrideHz) const
{
    PitchTrack out;
    const float pitchFrameRateHz = std::clamp(controlRateHz, 1.0f, 50.0f);
    const bool hasRootOverride = rootOverrideHz && *rootOverrideHz >= 20.0f && *rootOverrideHz <= 20000.0f;
    out.pitchHz.sampleRateHz = pitchFrameRateHz;
    out.confidenceCurve.sampleRateHz = pitchFrameRateHz;

    if (mono.getNumChannels() <= 0 || mono.getNumSamples() <= 0 || sampleRate <= 0.0 || controlRateHz <= 0.0f)
        return out;

    double analysisSampleRate = sampleRate;
    const auto analysisSignal = makeAnalysisSignal(mono, sampleRate, analysisSampleRate);
    const float durationSeconds = static_cast<float>(mono.getNumSamples() / sampleRate);
    const int frames = std::max(1, static_cast<int>(std::ceil(durationSeconds * pitchFrameRateHz)));
    const int frameSize = std::min(static_cast<int>(analysisSignal.size()), static_cast<int>(std::round(analysisSampleRate * 0.085)));

    std::vector<FrameEstimate> estimates(static_cast<size_t>(frames));

    if (frameSize <= 0)
    {
        out.rootHz = chooseRootHz(estimates, rootOverrideHz);
        out.pitchHz.values.assign(static_cast<size_t>(frames), out.rootHz);
        out.confidenceCurve.values.assign(static_cast<size_t>(frames), 0.0f);
        return out;
    }

    for (int frame = 0; frame < frames; ++frame)
    {
        const double timeSeconds = static_cast<double>(frame) / pitchFrameRateHz;
        const int center = static_cast<int>(std::round(timeSeconds * analysisSampleRate));
        const int start = std::clamp(center - frameSize / 2, 0, std::max(0, static_cast<int>(analysisSignal.size()) - frameSize));
        estimates[static_cast<size_t>(frame)] = estimateFrame(analysisSignal, start, frameSize, analysisSampleRate);
    }

    out.rootHz = chooseRootHz(estimates, rootOverrideHz);

    out.pitchHz.values.assign(static_cast<size_t>(frames), out.rootHz);
    out.confidenceCurve.values.assign(static_cast<size_t>(frames), 0.0f);

    double confidenceSum = 0.0;
    int confidentFrames = 0;
    float lastGoodHz = out.rootHz;

    for (int frame = 0; frame < frames; ++frame)
    {
        const auto estimate = estimates[static_cast<size_t>(frame)];
        if (!hasRootOverride && estimate.confidence >= 0.35f && estimate.hz > 0.0f)
            lastGoodHz = estimate.hz;

        out.pitchHz.values[static_cast<size_t>(frame)] = hasRootOverride ? out.rootHz : lastGoodHz;
        out.confidenceCurve.values[static_cast<size_t>(frame)] = estimate.confidence;

        if (estimate.confidence >= 0.35f)
        {
            confidenceSum += estimate.confidence;
            ++confidentFrames;
        }
    }

    out.confidence = confidentFrames > 0 ? static_cast<float>(confidenceSum / confidentFrames) : 0.0f;
    return out;
}

std::vector<float> PitchTracker::makeAnalysisSignal(const juce::AudioBuffer<float>& mono,
                                                    double inputSampleRate,
                                                    double& analysisSampleRate)
{
    const float* input = mono.getReadPointer(0);
    const int samples = mono.getNumSamples();
    const int decimation = inputSampleRate > 16000.0 ? std::max(1, static_cast<int>(std::floor(inputSampleRate / 12000.0))) : 1;
    analysisSampleRate = inputSampleRate / decimation;

    std::vector<float> output(static_cast<size_t>((samples + decimation - 1) / decimation), 0.0f);
    for (size_t i = 0; i < output.size(); ++i)
        output[i] = input[std::min(samples - 1, static_cast<int>(i) * decimation)];

    return output;
}

PitchTracker::FrameEstimate PitchTracker::estimateFrame(const std::vector<float>& samples,
                                                        int frameStart,
                                                        int frameSize,
                                                        double sampleRate)
{
    FrameEstimate estimate;

    const int minLag = std::max(2, static_cast<int>(std::floor(sampleRate / 1200.0)));
    const int maxLag = std::min(frameSize / 2, static_cast<int>(std::ceil(sampleRate / 40.0)));
    const int compareCount = std::min(frameSize - maxLag, static_cast<int>(samples.size()) - frameStart - maxLag);

    if (compareCount <= 16 || maxLag <= minLag)
        return estimate;

    double energy = 0.0;
    for (int i = 0; i < compareCount; ++i)
    {
        const float value = samples[static_cast<size_t>(frameStart + i)];
        energy += static_cast<double>(value) * value;
    }

    if (energy < 1.0e-8)
        return estimate;

    std::vector<float> cmnd(static_cast<size_t>(maxLag + 1), 1.0f);
    double runningDifference = 0.0;
    int bestLag = 0;
    float bestValue = 1.0f;

    for (int lag = 1; lag <= maxLag; ++lag)
    {
        double difference = 0.0;
        for (int i = 0; i < compareCount; ++i)
        {
            const float delta = samples[static_cast<size_t>(frameStart + i)] - samples[static_cast<size_t>(frameStart + i + lag)];
            difference += static_cast<double>(delta) * delta;
        }

        runningDifference += difference;
        cmnd[static_cast<size_t>(lag)] = runningDifference > 0.0f
            ? static_cast<float>(difference * lag / runningDifference)
            : 1.0f;
    }

    constexpr float threshold = 0.14f;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        const float value = cmnd[static_cast<size_t>(lag)];
        if (value < threshold)
        {
            int localLag = lag;
            while (localLag + 1 <= maxLag && cmnd[static_cast<size_t>(localLag + 1)] < cmnd[static_cast<size_t>(localLag)])
                ++localLag;

            bestLag = localLag;
            bestValue = cmnd[static_cast<size_t>(localLag)];
            break;
        }
    }

    if (bestLag == 0)
    {
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            const float value = cmnd[static_cast<size_t>(lag)];
            if (value < bestValue)
            {
                bestValue = value;
                bestLag = lag;
            }
        }
    }

    if (bestLag <= 0 || bestValue > 0.65f)
        return estimate;

    float refinedLag = static_cast<float>(bestLag);
    if (bestLag > minLag && bestLag < maxLag)
    {
        const float left = cmnd[static_cast<size_t>(bestLag - 1)];
        const float center = cmnd[static_cast<size_t>(bestLag)];
        const float right = cmnd[static_cast<size_t>(bestLag + 1)];
        const float denominator = left - 2.0f * center + right;
        if (std::abs(denominator) > 1.0e-8f)
            refinedLag += 0.5f * (left - right) / denominator;
    }

    estimate.hz = static_cast<float>(sampleRate / std::max(1.0f, refinedLag));
    estimate.confidence = std::clamp(1.0f - bestValue, 0.0f, 1.0f);
    return estimate;
}

float PitchTracker::chooseRootHz(const std::vector<FrameEstimate>& estimates, std::optional<float> rootOverrideHz)
{
    if (rootOverrideHz && *rootOverrideHz >= 20.0f && *rootOverrideHz <= 20000.0f)
        return *rootOverrideHz;

    std::vector<float> confidentPitches;
    confidentPitches.reserve(estimates.size());

    for (const auto& estimate : estimates)
        if (estimate.confidence >= 0.55f && estimate.hz > 0.0f)
            confidentPitches.push_back(estimate.hz);

    if (confidentPitches.empty())
        return 440.0f;

    const auto middle = confidentPitches.begin() + static_cast<std::ptrdiff_t>(confidentPitches.size() / 2);
    std::nth_element(confidentPitches.begin(), middle, confidentPitches.end());
    return *middle;
}
