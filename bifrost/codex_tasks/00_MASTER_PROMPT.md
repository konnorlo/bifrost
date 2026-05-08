# Codex Master Prompt

You are implementing `Bifrost`, a C++20 JUCE VST3 instrument plugin.

## Product

Build a plugin that imports WAV/MP3/AIFF/FLAC audio and converts tonal source material into a playable MIDI instrument using harmonic/noise/resonator timbre resynthesis. The plugin is instrument-first, optimized first for synth stabs, basses, leads, pads, bells, and tonal one-shots.

## Core architecture

Heavy work happens during file import/background analysis:

- decode audio
- STFT
- pitch/loudness/transient detection
- harmonic/noise/resonator extraction
- optional tiny ML embedding
- timbre state graph
- model compression

Realtime audio callback only:

- process MIDI
- render additive harmonics
- render filtered noise
- render resonators
- interpolate curves
- apply smoothing/envelopes

## Non-negotiable rules

1. No blocking work in `processBlock()`.
2. No file I/O in `processBlock()`.
3. No heavy FFT/model fitting in `processBlock()`.
4. No Python in plugin runtime.
5. ONNX is optional and import-time only.
6. Keep code compartmentalized by folder.
7. Update docs and dependency lock if adding dependencies.
8. Preserve C++20 and JUCE/CMake project structure.
9. Keep v1 focused on playable tonal synth behavior.
10. Add tests for pure DSP/model code.

## Implementation order

Use these task files in order:

1. `01_bootstrap_build.md`
2. `02_audio_import.md`
3. `03_stft_analysis.md`
4. `04_pitch_harmonics.md`
5. `05_realtime_synth.md`
6. `06_ui_state_map.md`
7. `07_ml_optional.md`
8. `08_tests.md`
9. `09_ci_and_release.md`

Do not attempt all phases at once.
