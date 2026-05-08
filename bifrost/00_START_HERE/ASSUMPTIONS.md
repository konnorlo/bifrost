# Locked Assumptions

These are the user's choices that shape the plan.

## Platform

- Target macOS + Windows.
- Build and debug first on macOS, likely Apple Silicon/M-series.
- FL Studio is the primary host.

## Plugin format

- VST3 instrument/generator first.
- No need for AU in v1.
- A standalone build may be useful for debugging, but the primary deliverable is VST3.

## Instrument behavior

Primary modes:

1. One-shot tonal resynthesis.
2. Sustained timbre instrument.
3. Timbre harmonizer / chord-capable resynthesis.

The plugin is an instrument first, not a sample reconstruction lab.

## Source material priority

Optimize v1 for:

1. Synth stabs, basses, leads.
2. Tonal plucks and bells.
3. Pads/drones.
4. Vocal vowels later.
5. Acoustic instruments and drums later.

## AI/ML level

Use useful, lightweight ML. Do not use an enormous realtime neural generator. The planned ML is import-time:

- tiny timbre encoder
- model quality classifier/router
- optional ONNX inference during analysis
- no large model inside `processBlock()`

## File formats

Support:

- WAV
- AIFF
- FLAC
- MP3
- Ogg Vorbis if convenient

Decoder path should be swappable and legally reviewed before commercial distribution.

## UI

Use a practical synth layout plus visual empty-space value:

- left: sample import, waveform/spectrogram
- center: state map / constellation visualizer
- right: macro controls
- bottom: playback, ADSR, harmonizer, quality controls

## Presets

Store both:

- embedded compact analyzed model
- source file path + hash for reanalysis

## Performance target

Run well on a MacBook Air M3.

Balanced target:

- 8 voices
- 32 to 48 harmonics per voice
- 8 to 12 noise bands
- 4 to 8 resonators
- no neural inference in the audio callback
- import analysis can take seconds, but must show progress and be cancelable

## Deliverable style

Everything should be compartmentalized so Codex can implement the project in focused passes.
