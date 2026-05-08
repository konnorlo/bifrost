# 10 Codex Operating Manual

## How Codex should approach this repo

Codex should not try to build everything in one pass. Implement in narrow stages.

## Rules for Codex

1. Keep the audio callback free of blocking calls and allocations.
2. Prefer small, testable classes.
3. Keep analysis code independent from UI.
4. Keep model data immutable for realtime use.
5. Add tests before major DSP refactors.
6. Do not add dependencies without editing `dependency_lock/dependency_lock.json`.
7. Do not implement a huge neural generator.
8. Keep ONNX optional and import-time only.
9. Keep CMake simple.
10. Use JUCE APVTS for host-automatable parameters.

## First implementation target

A compileable silent VST3 instrument with:

- CMake JUCE setup
- `BifrostAudioProcessor`
- basic editor
- APVTS parameters
- drag/drop file path capture
- no actual analysis yet

## Second implementation target

WAV import + basic analysis:

- decode file with JUCE
- convert to mono
- compute waveform preview
- compute RMS curve
- display import status

## Third implementation target

First sound:

- pitch estimate for simple tonal sample
- harmonic amplitude curves
- additive oscillator bank
- MIDI note-on renders source timbre envelope at MIDI pitch

## Suggested Codex workflow

1. Open `codex_tasks/00_MASTER_PROMPT.md`.
2. Then implement task files in numerical order.
3. After each task, run build/tests.
4. Keep all TODOs localized.
5. Commit after each working milestone.
