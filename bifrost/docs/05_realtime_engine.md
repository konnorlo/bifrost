# 05 Realtime Engine

## Hard rule

`processBlock()` must be deterministic, non-blocking, and allocation-free in the hot path.

## Realtime inputs

From host:

- MIDI note on/off
- velocity
- pitch bend
- modulation wheel
- sustain pedal
- sample rate
- block size
- tempo/transport if needed

From plugin state:

- active immutable `TimbreModel`
- user parameters from APVTS
- active mode
- polyphony limit

## Realtime outputs

- stereo audio buffer
- optional debug meters copied to lock-free FIFO for UI

## Voice lifecycle

On note-on:

1. allocate/reuse a `Voice` from pool
2. set MIDI note, velocity, phase seeds
3. bind current `TimbreModel` pointer
4. reset curve readers to source start
5. initialize envelopes and inertia states
6. start attack/body playback

On note-off:

- trigger release envelope
- keep rendering until envelope silence

## Voice model cursor

Each voice maintains:

```text
ageSeconds
modelTimeSeconds
playbackRate
timbreState
stateRandomSeed
```

In One-Shot mode:

```text
modelTime += dt * playbackRate
```

In Sustain mode:

```text
attack: modelTime follows attack region
sustain: modelTime loops/freezes/body-cycles
release: tail or release envelope
```

In Harmonizer mode:

- each MIDI note is a voice
- optionally share root timbre path across chord voices
- optional per-voice phase and state offsets for width

## Harmonic rendering

Use an oscillator bank per voice.

Optimizations:

- skip harmonics above Nyquist
- update amplitudes at control rate and linearly ramp inside block
- use lookup or recursive oscillators if `std::sin` is too expensive
- vectorize later if needed

Balanced target:

```text
8 voices * 48 harmonics = 384 oscillators maximum before Nyquist skip
```

This should be okay on M3 if optimized and if noise/resonator layers are modest.

## Noise rendering

Use one noise source per voice or shared decorrelated source.

Filter strategies:

- Eco: simple one-pole tilt/noise shelves
- Balanced: small bank of bandpass filters
- High: SVF bank or FFT-domain noise shaping if offline only

## Resonator rendering

Use biquad/SVF resonators.

Balanced target:

```text
8 voices * 8 resonators = 64 resonators
```

Keep Q stable and clamp gains to avoid ringing explosions.

## Modulation and smoothing

Every extracted curve can drive parameters:

- harmonic tilt
- filter cutoff
- noise amount
- resonator gain
- stereo width
- drive
- reverb send later

Use per-voice inertia smoothing:

```text
v = alpha * v + (1-alpha) * (target - y)
y = y + beta * v
```

## Performance guardrails

- hard maximum harmonics per voice by quality mode
- hard maximum resonators per voice
- voice stealing
- disable resonator/noise layer when CPU guard triggers
- denormal protection
- no dynamic allocation during audio callback
- no logging from audio thread

## Suggested quality modes

### Eco

- 16 harmonics
- 6 noise bands
- 4 resonators
- 4 voices default
- 100 Hz control curves

### Balanced

- 32-48 harmonics
- 8-12 noise bands
- 6-8 resonators
- 8 voices default
- 200 Hz control curves

### High

- 64-96 harmonics
- 16 noise bands
- 12 resonators
- 8-12 voices
- 400 Hz control curves

### Offline Render

- 128 harmonics
- higher noise resolution
- more resonators
- no realtime CPU guarantee

## Parameter safety

Clamp everything:

- harmonic amplitudes >= 0
- resonator Q in safe range
- total voice gain normalized
- output limiter/soft clipper optional
- avoid NaN/Inf propagation

## First audio target

The first working milestone should render:

```text
MIDI note -> sine/saw-ish harmonic bank using source harmonic curves -> stereo output
```

Do not implement all layers before hearing sound.
