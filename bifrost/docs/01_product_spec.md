# 01 Product Spec

## Product Name

- Bifrost

Use `Bifrost` as the product and internal engineering name.

## One-sentence product pitch

A VST3 instrument that converts an imported sound into a playable tonal resynthesis instrument by extracting harmonic body, noise texture, resonant formants, and time-varying timbre motion.

## What it is

- A JUCE/C++ VST3 instrument.
- A sample-to-synth/resynthesis system.
- A tonal timbre extraction instrument.
- A MIDI-playable harmonic/noise/resonator synth.
- A timbre harmonizer that can replay one source as chords without naive pitch-shifting.
- A sound-design plugin that extracts motion curves and timbre states from audio.

## What it is not

- Not a sampler.
- Not a pure phase vocoder.
- Not a Synplant clone.
- Not a plant/genome UI.
- Not a giant neural audio generator.
- Not a black-box model that renders raw audio sample-by-sample.

## User story

1. User opens plugin in FL Studio as a VST3 instrument.
2. User drags a WAV/MP3/AIFF/FLAC file into the UI.
3. Plugin analyzes the file on a background thread.
4. Plugin shows waveform, spectrogram, detected pitch, and timbre-state map.
5. User plays MIDI notes.
6. Plugin renders a synthetic/resynthesized voice using extracted timbre motion.
7. User can choose One-Shot, Sustain, Freeze, or Harmonizer mode.
8. User tweaks macro controls: Body, Air, Metal, Motion, Mutation, Inertia, Formant.
9. Preset saves the compact timbre model and source reference.

## Main modes

### Mode 1: One-Shot Resynth

Goal: The source sound becomes a playable one-shot across the keyboard.

- MIDI note triggers the extracted time-series curves from the beginning.
- Pitch is replaced with MIDI pitch.
- Attack is preserved.
- Harmonic, noise, and resonator curves are replayed.
- Best for stabs, bass hits, plucks, bells, tonal one-shots.

### Mode 2: Sustained Timbre Instrument

Goal: The source sound becomes a stable instrument.

- Attack is played once.
- Body/tail timbre is looped, frozen, or state-cycled.
- Optional ADSR controls shape amplitude.
- Formant lock keeps identity stable across MIDI pitch.
- Best for pads, leads, basses, drones, vocals/vowels later.

### Mode 3: Timbre Harmonizer

Goal: The source timbre becomes chord-capable.

- Each MIDI note uses the same extracted timbre body.
- No naive sample pitch shift.
- Additive harmonic layer follows MIDI pitch.
- Noise/resonator layers are adapted/formant-preserved.
- Best for turning a synth stab, vocal vowel, bell, or one-shot into chord voicings.

## Primary controls

### Global

- Analyze / Reanalyze
- Quality: Eco, Balanced, High, Offline Render
- Mode: One-Shot, Sustain, Harmonizer
- Source Start / End
- Attack Preserve
- Time Stretch
- Pitch Follow
- Formant Lock

### Tone macros

- Body: harmonic low/mid strength and fullness
- Air: high noise-band gain
- Metal: inharmonic resonator gain + Q
- Brightness: spectral tilt / harmonic rolloff
- Roughness: detune, micro-jitter, noise modulation
- Transient: attack layer level

### Motion/generative macros

- Motion: amount of original timbre trajectory
- Freeze: hold selected timbre state
- Inertia: smoothing/lag in state and timbre transitions
- Mutation: random perturbation of model curves/states
- Drift: slow stochastic motion
- Orbit: cyclic traversal through state map
- Markov Memory: how strongly generated paths follow learned transitions

### Harmonizer

- Voice Count
- Chord Spread
- Detune
- Stereo Spread
- Formant Preserve
- Timbre Coupling: same timbre path for all notes vs per-voice random offsets

## Differentiation

The unique hook is not sample cloning. It is:

> Extract what makes the source tonal/timbral, then perform that extracted body as a playable instrument.

The differentiating features are:

- time-varying timbre resynthesis
- harmonic/noise/resonator decomposition
- state-map visualization
- inertia-based motion smoothing
- chord-capable timbre harmonization
- import-time ML assistance without realtime neural audio generation
