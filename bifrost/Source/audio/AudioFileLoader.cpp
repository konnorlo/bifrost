#include "audio/AudioFileLoader.h"

#include <cstdint>

AudioFileLoader::AudioFileLoader()
{
    formatManager.registerBasicFormats();
}

LoadedAudioFile AudioFileLoader::load(const juce::File& file)
{
    LoadedAudioFile out;
    out.file = file;
    out.formatExtension = file.getFileExtension().toLowerCase();

    const auto backend = chooseDecoderBackend(file);
    out.decoderBackend = getBackendName(backend);

    if (!file.existsAsFile())
    {
        out.error = "File does not exist";
        return out;
    }

    if (backend == DecoderBackend::unsupported)
    {
        out.error = "Unsupported file type. Supported: " + supportedExtensionsDescription();
        return out;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader)
    {
        if (backend == DecoderBackend::jucePlatform)
            out.error = "No platform decoder available for this file";
        else
            out.error = "No JUCE decoder available for this file";

        return out;
    }

    if (reader->lengthInSamples <= 0 || reader->numChannels == 0)
    {
        out.error = "File contains no readable audio";
        return out;
    }

    out.sampleRate = reader->sampleRate;
    out.originalNumChannels = static_cast<int>(reader->numChannels);
    out.originalLengthInSamples = reader->lengthInSamples;
    out.originalDurationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;

    const int channels = static_cast<int>(std::min<uint32_t>(reader->numChannels, 2));
    const int samples = static_cast<int>(std::min<int64_t>(reader->lengthInSamples, static_cast<int64_t>(reader->sampleRate * 60.0)));
    out.importedNumChannels = channels;
    out.importedLengthInSamples = samples;
    out.importedDurationSeconds = static_cast<double>(samples) / reader->sampleRate;
    out.wasTruncated = reader->lengthInSamples > samples;

    out.audio.setSize(channels, samples);
    reader->read(&out.audio, 0, samples, 0, true, true);
    out.monoMid = makeMonoMid(out.audio);
    out.hash = computeFastHash(file);
    return out;
}

bool AudioFileLoader::isSupportedExtension(const juce::File& file)
{
    return chooseDecoderBackend(file) != DecoderBackend::unsupported;
}

juce::String AudioFileLoader::supportedExtensionsDescription()
{
    return "WAV, AIFF, FLAC, MP3, OGG";
}

AudioFileLoader::DecoderBackend AudioFileLoader::chooseDecoderBackend(const juce::File& file)
{
    const auto ext = file.getFileExtension().toLowerCase();

    if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".ogg")
        return DecoderBackend::juceBasic;

    if (ext == ".mp3")
        return DecoderBackend::jucePlatform;

    return DecoderBackend::unsupported;
}

juce::String AudioFileLoader::getBackendName(DecoderBackend backend)
{
    switch (backend)
    {
        case DecoderBackend::juceBasic: return "JUCE basic formats";
        case DecoderBackend::jucePlatform: return "JUCE platform decoder";
        case DecoderBackend::unsupported: break;
    }

    return "Unsupported";
}

juce::AudioBuffer<float> AudioFileLoader::makeMonoMid(const juce::AudioBuffer<float>& input)
{
    juce::AudioBuffer<float> mono(1, input.getNumSamples());
    mono.clear();

    if (input.getNumChannels() == 0)
        return mono;

    if (input.getNumChannels() == 1)
    {
        mono.copyFrom(0, 0, input, 0, 0, input.getNumSamples());
        return mono;
    }

    mono.addFrom(0, 0, input, 0, 0, input.getNumSamples(), 0.5f);
    mono.addFrom(0, 0, input, 1, 0, input.getNumSamples(), 0.5f);
    return mono;
}

juce::String AudioFileLoader::computeFastHash(const juce::File& file)
{
    juce::FileInputStream stream(file);
    if (!stream.openedOk()) return {};

    std::uint64_t hash = 14695981039346656037ull;
    auto mixByte = [&hash](std::uint8_t byte)
    {
        hash ^= byte;
        hash *= 1099511628211ull;
    };

    auto mixInteger = [&mixByte](std::uint64_t value)
    {
        for (int i = 0; i < 8; ++i)
        {
            mixByte(static_cast<std::uint8_t>(value & 0xffu));
            value >>= 8;
        }
    };

    mixInteger(static_cast<std::uint64_t>(file.getSize()));
    mixInteger(static_cast<std::uint64_t>(file.getLastModificationTime().toMilliseconds()));

    constexpr int chunkSize = 8192;
    juce::HeapBlock<char> block(chunkSize);
    int64 remaining = std::min<int64>(file.getSize(), 1024 * 1024);
    while (remaining > 0)
    {
        const int toRead = static_cast<int>(std::min<int64>(chunkSize, remaining));
        const int got = stream.read(block.getData(), toRead);
        if (got <= 0) break;
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(block.getData());
        for (int i = 0; i < got; ++i)
            mixByte(bytes[i]);
        remaining -= got;
    }
    return juce::String::toHexString(static_cast<int64>(hash));
}
