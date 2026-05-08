# 11 Risk Notes

## Risk: trying to clone raw audio too closely

The main product is an instrument. Prioritize stable playable timbre over perfect reconstruction.

Mitigation:

- expose Clone/Instrument internal bias
- default to Instrument
- smooth curves
- formant lock
- stable sustain/freeze regions

## Risk: too much CPU

Additive + noise + resonator polyphony can get expensive.

Mitigation:

- quality modes
- Nyquist harmonic skipping
- control-rate updates
- max voices
- resonator limit
- no ONNX in audio thread

## Risk: pitch tracking failure

Bad pitch = bad harmonic instrument.

Mitigation:

- pitch confidence UI
- fallback resonator/noise model
- user root-note override
- manual pitch correction
- source material guidance

## Risk: MP3 decoder/legal uncertainty

Mitigation:

- abstract decoder backend
- prefer OS decoders where possible
- include miniaudio path as optional
- review before commercial distribution

## Risk: ML dependency bloats plugin

Mitigation:

- ML optional
- DSP fallback
- import-time only
- small ONNX model

## Risk: preset bloat

Mitigation:

- control-rate curves
- model compression
- embed compact model, not raw audio by default
- optional source file hash/path for reanalysis

## Risk: UI visualizer becoming decorative only

Mitigation:

- state map must control Freeze/Orbit/Mutation
- show active voices
- show transition edges
- let user select states

## Risk: poor source expectations

Mitigation:

- quality badge
- tooltips explaining best sources
- tonal-fit score
- examples: synth stabs, basses, bells, pads
