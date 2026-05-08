#pragma once

#include <JuceHeader.h>

struct LoadedAudioFile
{
    juce::AudioBuffer<float> audio;
    juce::AudioBuffer<float> monoMid;
    double sampleRate = 0.0;
    double originalDurationSeconds = 0.0;
    double importedDurationSeconds = 0.0;
    int originalNumChannels = 0;
    int importedNumChannels = 0;
    int64 originalLengthInSamples = 0;
    int64 importedLengthInSamples = 0;
    bool wasTruncated = false;
    juce::File file;
    juce::String hash;
    juce::String decoderBackend;
    juce::String formatExtension;
    juce::String error;

    bool ok() const noexcept { return sampleRate > 0.0 && audio.getNumSamples() > 0 && error.isEmpty(); }
};

class AudioFileLoader
{
public:
    AudioFileLoader();
    LoadedAudioFile load(const juce::File& file);
    static bool isSupportedExtension(const juce::File& file);
    static juce::String supportedExtensionsDescription();

private:
    enum class DecoderBackend
    {
        unsupported,
        juceBasic,
        jucePlatform
    };

    juce::AudioFormatManager formatManager;

    static DecoderBackend chooseDecoderBackend(const juce::File& file);
    static juce::String getBackendName(DecoderBackend backend);
    static juce::AudioBuffer<float> makeMonoMid(const juce::AudioBuffer<float>& input);
    static juce::String computeFastHash(const juce::File& file);
};
