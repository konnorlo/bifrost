#pragma once

struct VoiceRenderParameters
{
    float body = 0.65f;
    float air = 0.35f;
    float metal = 0.25f;
    float brightness = 0.55f;
    float motion = 0.75f;
    float inertia = 0.35f;
    float mutation = 0.0f;
    float transient = 0.7f;
    float formantLock = 0.75f;
    float timeStretch = 1.0f;
    float outputGainDb = -6.0f;
    float polyphonyGainDb = 0.0f;
    float attackSeconds = 0.01f;
    float decaySeconds = 0.16f;
    float sustainLevel = 0.75f;
    float releaseSeconds = 0.35f;
    float envelopeCurve = 0.5f;
    float velocitySensitivity = 1.0f;
    float pan = 0.0f;
    float stereoWidth = 0.35f;
    float stereoReconstruction = 0.0f;
    float phaseOffset = 0.0f;
    float phaseRandom = 0.0f;
    float timeSync = 0.0f;
    float sourceTempoBpm = 0.0f;
    float hostTempoBpm = 0.0f;
    float startRandomAmount = 0.0f;
    float randomDirection = 0.0f;
    float loopStartNormalized = 0.35f;
    float loopEndNormalized = 0.80f;
    int unisonVoices = 1;
    float unisonDetuneCents = 7.0f;
    int mode = 0;
    int quality = 1;
    int maxVoices = 8;
    int maxHarmonics = 48;
    int maxNoiseBands = 8;
    int maxResonators = 8;
};
