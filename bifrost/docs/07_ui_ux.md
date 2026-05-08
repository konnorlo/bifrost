# 07 UI / UX Plan

## Layout

```text
┌──────────────────────────────────────────────────────────────┐
│ Top Bar: file name | Analyze | Mode | Quality | CPU | Preset │
├───────────────┬───────────────────────────────┬──────────────┤
│ Import/Source │      State Map Visualizer      │ Macro Knobs  │
│ Waveform      │      / Spectral Constellation  │ Body Air ... │
│ Region/Loop   │                               │              │
├───────────────┴───────────────────────────────┴──────────────┤
│ Playback: OneShot/Sustain/Harmonizer | ADSR | Voice controls │
└──────────────────────────────────────────────────────────────┘
```

## Empty-space visualizer idea

Use a constellation/state-map rather than a plant/genome.

- each state is a node/star
- node size = state usage / loudness
- node color = brightness/noise/metal mapping
- edges = learned transition probabilities
- moving dot = current voice's timbre position
- multiple dots = active MIDI voices
- user can click a node to Freeze state
- Orbit mode moves through nodes rhythmically

## Why this visualizer is useful

It is not just decoration:

- shows whether sample has simple or complex timbre behavior
- gives a playable state selector
- explains Mutation/Drift/Orbit controls
- makes sustained mode intuitive
- differentiates plugin from normal samplers and synths

## Pages / tabs

### Perform

- state map
- macro knobs
- mode controls
- harmonizer controls

### Source

- waveform
- spectrogram
- source start/end
- attack/body/tail markers
- loop/freeze region

### Model

- harmonic count
- noise bands
- resonators
- state count
- quality metrics
- pitch confidence

### Advanced

- curve smoothing
- formant lock strength
- Markov memory
- mutation depth
- ML on/off
- export model diagnostics

## Macro controls

Recommended visible macro set:

```text
Body
Air
Metal
Brightness
Motion
Inertia
Mutation
Transient
```

Secondary controls in drawers:

```text
Formant Lock
Noise Tilt
Resonator Q
Harmonic Roll-Off
Stereo Drift
State Randomness
```

## File import UX

- drag file onto plugin
- accepted extension highlight
- show progress: Decoding, STFT, Pitch, Harmonics, States, Model Ready
- allow cancel
- show quality badge

Example quality badges:

```text
Excellent tonal source
Good synth source
Pitch uncertain, using resonator/noise assist
Noisy source, harmonizer may be unstable
```

## Instrument modes UX

### One-Shot

- source timeline plays from start on note-on
- time stretch knob prominent
- attack preserve visible

### Sustain

- attack/body/tail shown in waveform
- loop/freeze region editable
- ADSR visible

### Harmonizer

- voice count
- chord spread
- formant preserve
- stereo spread
- detune
- timbre sync: shared / independent

## Visual style

Professional, dark, high-contrast spectral UI.

Avoid biological/plant visuals. Use:

- constellation
- spectral glass
- oscilloscope/spectrogram
- orbital motion
- state graph

## Accessibility

- clear labels
- numerical readouts on hover or modifier-drag
- no information conveyed by color alone
- keyboard focus for controls later

## MVP UI

Do not overbuild the first UI. MVP UI needs:

- drag/drop target
- source waveform
- Analyze status
- mode selector
- 8 macro sliders
- simple state map placeholder
- MIDI/audio output working
