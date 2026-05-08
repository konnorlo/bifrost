#pragma once

#include <JuceHeader.h>
#include <optional>
#include "model/Curve.h"

struct PitchTrack
{
    Curve pitchHz;
    Curve confidenceCurve;
    float rootHz = 440.0f;
    float confidence = 0.0f;
};

class PitchTracker
{
public:
    PitchTrack estimate(const juce::AudioBuffer<float>& mono,
                        double sampleRate,
                        float controlRateHz,
                        std::optional<float> rootOverrideHz = std::nullopt) const;

private:
    struct FrameEstimate
    {
        float hz = 0.0f;
        float confidence = 0.0f;
    };

    static std::vector<float> makeAnalysisSignal(const juce::AudioBuffer<float>& mono, double inputSampleRate, double& analysisSampleRate);
    static FrameEstimate estimateFrame(const std::vector<float>& samples, int frameStart, int frameSize, double sampleRate);
    static float chooseRootHz(const std::vector<FrameEstimate>& estimates, std::optional<float> rootOverrideHz);
};
