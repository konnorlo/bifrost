#include "synth/VoiceManager.h"

VoiceManager::VoiceManager()
{
    for (int i = 0; i < maxVoices; ++i)
    {
        voiceActive[static_cast<size_t>(i)].store(0);
        voiceNote[static_cast<size_t>(i)].store(-1);
        voiceTimeNormalized[static_cast<size_t>(i)].store(0.0f);
        voiceLevel[static_cast<size_t>(i)].store(0.0f);
    }
}

void VoiceManager::prepare(double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    blockSize = samplesPerBlock;
    for (auto& v : voices)
        v.prepare(sampleRate, samplesPerBlock, 96, 16);
}

void VoiceManager::reset()
{
    for (auto& v : voices) v.stop();
    for (int i = 0; i < maxVoices; ++i)
        publishVoiceActivity(i);
}

void VoiceManager::render(juce::AudioBuffer<float>& buffer,
                          juce::MidiBuffer& midi,
                          std::shared_ptr<const TimbreModel> model,
                          const VoiceRenderParameters& params)
{
    const int voiceLimit = std::clamp(params.maxVoices, 1, maxVoices);
    int currentSample = 0;
    for (const auto metadata : midi)
    {
        const int eventSample = metadata.samplePosition;
        const int segment = std::clamp(eventSample - currentSample, 0, buffer.getNumSamples() - currentSample);
        for (int i = 0; i < voiceLimit; ++i)
            voices[static_cast<size_t>(i)].render(buffer, currentSample, segment, params);
        currentSample += segment;

        const auto msg = metadata.getMessage();
        if (msg.isNoteOn() && model)
            chooseVoice(voiceLimit).start(msg.getNoteNumber(), msg.getFloatVelocity(), model, noteSerial++);
        else if (msg.isNoteOff())
            noteOff(msg.getNoteNumber(), voiceLimit);
    }

    const int remaining = buffer.getNumSamples() - currentSample;
    for (int i = 0; i < voiceLimit; ++i)
        voices[static_cast<size_t>(i)].render(buffer, currentSample, remaining, params);

    for (int i = 0; i < maxVoices; ++i)
        publishVoiceActivity(i);
}

std::array<VoiceActivity, VoiceManager::maxVoices> VoiceManager::getVoiceActivities() const noexcept
{
    std::array<VoiceActivity, maxVoices> activities;
    for (int i = 0; i < maxVoices; ++i)
    {
        const auto index = static_cast<size_t>(i);
        activities[index].active = voiceActive[index].load(std::memory_order_relaxed) != 0;
        activities[index].midiNote = voiceNote[index].load(std::memory_order_relaxed);
        activities[index].modelTimeNormalized = voiceTimeNormalized[index].load(std::memory_order_relaxed);
        activities[index].level = voiceLevel[index].load(std::memory_order_relaxed);
    }
    return activities;
}

Voice& VoiceManager::chooseVoice(int voiceLimit)
{
    for (int i = 0; i < voiceLimit; ++i)
        if (!voices[static_cast<size_t>(i)].isActive()) return voices[static_cast<size_t>(i)];
    return voices.front();
}

void VoiceManager::noteOff(int note, int voiceLimit)
{
    for (int i = 0; i < voiceLimit; ++i)
    {
        auto& voice = voices[static_cast<size_t>(i)];
        if (voice.isActive() && voice.getMidiNote() == note)
            voice.stop();
    }
}

void VoiceManager::publishVoiceActivity(int index) noexcept
{
    auto& voice = voices[static_cast<size_t>(index)];
    const bool active = voice.isActive();
    const auto arrayIndex = static_cast<size_t>(index);
    voiceActive[arrayIndex].store(active ? 1 : 0, std::memory_order_relaxed);
    voiceNote[arrayIndex].store(active ? voice.getMidiNote() : -1, std::memory_order_relaxed);
    voiceTimeNormalized[arrayIndex].store(active ? voice.getModelTimeNormalized() : 0.0f, std::memory_order_relaxed);
    voiceLevel[arrayIndex].store(active ? voice.getLastPeak() : 0.0f, std::memory_order_relaxed);
}
