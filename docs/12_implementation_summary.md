# Bifrost Implementation Summary

## Current Scope

This pass focused on making Bifrost behave more like a playable sustained synth inside FL Studio and other VST3 hosts. The main goals were:

- Stabilize Sustain mode so held notes produce a constant pitched tone instead of following source-file rhythmic, loudness, or timbre fluctuations.
- Raise the default perceived output level.
- Rework root randomization so it does not detune pitch.
- Add more host-automatable controls for ADSR shape, loop region, start randomization, direction, stereo, and unison.
- Reduce realtime CPU in Sustain mode by caching the static spectral frame used by the held tone.
- Keep the UI cleaner and label the new controls directly where they are used.

## Engine Changes

### Stable Sustain Mode

Sustain mode now freezes the analyzed timbre at a selected source position and renders it as a steady additive oscillator tone. It no longer continuously scans through the source model in Sustain mode.

The implementation:

- Caches one harmonic amplitude frame and one loudness value per voice.
- Reuses that cached frame while the note is held.
- Skips the noise and resonator engines in Sustain mode to avoid source-rhythm/timbre movement leaking into held notes.
- Keeps MIDI note pitch as the only primary pitch source, with unison detune applied only when requested.

Relevant files:

- `Source/synth/Voice.cpp`
- `Source/synth/AdditiveOscBank.cpp`
- `Source/synth/VoiceRenderParameters.h`

### Louder Default Output

The default output gain parameter was raised from `-6 dB` to `-3 dB`, and voice rendering now has a small internal trim multiplier. The soft limiter still runs at the processor output to keep pathological peaks bounded.

Relevant files:

- `Source/PluginProcessor.cpp`
- `Source/synth/Voice.cpp`

### Root Random Reworked

The existing `root_random` parameter is now presented as `Start Random`. It no longer changes pitch. Instead, it randomizes where each new voice samples the analyzed timbre inside the editable loop range.

This keeps the played MIDI note stable while still allowing variation between notes.

Relevant files:

- `Source/PluginProcessor.cpp`
- `Source/PluginEditor.cpp`
- `Source/synth/Voice.cpp`

## New Parameters

The following parameters were added or repurposed in the APVTS, so they should be visible to FL Studio automation/mapping:

| ID | Label | Purpose |
| --- | --- | --- |
| `adsr_curve` | ADSR Curve | Shapes attack, decay, and release with curved/quadratic-style response. |
| `stereo_width` | Stereo Width | Spreads unison layers around the pan position. |
| `stereo_reconstruction` | Stereo Reconstruction | Adds stereo side reconstruction when the source is effectively mono. |
| `root_random` | Start Random | Randomizes source start position, not pitch. |
| `random_direction` | Random Direction | Lets voices reverse loop direction based on per-note random seed. |
| `loop_start` | Loop Start | Sets the normalized start of the source loop region. |
| `loop_end` | Loop End | Sets the normalized end of the source loop region. |
| `unison_voices` | Unison Voices | Adds up to 4 additive synth layers per voice. |
| `unison_detune` | Unison Detune | Detunes unison layers in cents. |

Existing relevant parameters remain available:

- `velocity_sensitivity`
- `pan`
- `time_sync`
- `time_stretch`
- `attack`
- `decay`
- `sustain`
- `release`
- `output_gain`

## ADSR

The ADSR is now more complete:

- Attack, decay, sustain, and release remain direct controls.
- `adsr_curve` shapes the time response for attack, decay, and release.
- The visual ADSR panel remains in the envelope section.
- Loop Start, Loop End, Curve, and Time Sync are grouped under the envelope area so the time-domain controls live together.

The curve mapping uses a power response, with lower settings giving slower/quadratic-style movement and higher settings giving faster/log-style movement.

## Stereo And Unison

Stereo behavior now has three layers:

- `pan` moves the whole voice left/right.
- `stereo_width` spreads unison layers around that pan position.
- `stereo_reconstruction` creates a side signal when only one layer would otherwise be active.

Unison now supports:

- 1 to 4 additive layers.
- Detune from 0 to 50 cents.
- Equal-power layer scaling so adding voices does not multiply level linearly.

## Looping And Randomization

Loop timing is now editable through `loop_start` and `loop_end`.

For Sustain mode:

- The synth chooses one cached point inside the loop range.
- `Start Random` controls how much the per-note random seed moves that point.
- Pitch does not randomize.

For looping non-one-shot behavior:

- The loop region uses the same editable start/end controls.
- `Random Direction` can reverse loop traversal per note.

## CPU Improvements

The biggest CPU reduction is in Sustain mode:

- Harmonic/loudness curve sampling is done once per voice for the selected sustain frame.
- The render loop uses `renderStaticSample()` against cached amplitudes.
- Noise and resonator processing are skipped for Sustain mode.

This does not eliminate all realtime work because additive synthesis still runs per sample, but it removes the expensive per-sample model-curve scanning for the most important sustained-synth use case.

## UI Changes

The editor was widened to make room for cleaner control grouping.

New/updated labels:

- `Amp ADSR`
- `Curve`
- `Loop Start`
- `Loop End`
- `Time Sync`
- `Velocity`
- `Pan`
- `Width`
- `Unison`
- `Detune`
- `Stereo`
- `Start Rand`
- `Direction`

Performance controls are arranged as two rows:

- Velocity, Pan, Width, Unison
- Detune, Stereo Reconstruction, Start Random, Direction

The timbre macro grid remains separate from performance and envelope controls.

## Math/Behavior Checks

Added a regression test that creates a deliberately fluctuating timbre model, renders Sustain mode, and checks that the late held tone remains finite, audible, and not abnormally modulated by the source fluctuations.

Existing analysis/render tests still cover:

- STFT handling
- RMS/centroid behavior
- pitch tracking and root override
- harmonic extraction
- preset state serialization/migration
- state graph normalization
- ADSR behavior
- tempo analyzer behavior
- velocity and pan rendering
- noise and resonator bounded output

## Verification

Commands run successfully:

```bash
cmake --build build-xcode --config RelWithDebInfo --target BifrostTests
build-xcode/BifrostTests_artefacts/RelWithDebInfo/BifrostTests
cmake --build build-xcode --config RelWithDebInfo --target Bifrost
ctest --test-dir build-xcode -C RelWithDebInfo --output-on-failure
codesign --verify --deep --strict --verbose=2 build-xcode/Bifrost_artefacts/RelWithDebInfo/VST3/Bifrost.vst3
```

Results:

- `BifrostTests` passed.
- `ctest` passed: 1/1 tests.
- VST3 build succeeded.
- VST3 codesign verification passed.

Artifact:

```text
build-xcode/Bifrost_artefacts/RelWithDebInfo/VST3/Bifrost.vst3
```

## Remaining Host-Level Work

The code is built and tests pass, but final musical validation still needs to happen inside FL Studio:

- Confirm Sustain mode feels steady on long notes with several source samples.
- Confirm automation lanes expose the new APVTS parameters as expected.
- Tune default values for width, unison detune, output gain, and ADSR curve by ear.
- Decide whether the current stock JUCE rotary controls are acceptable or whether custom-designed knobs/envelope UI should replace them.

`pluginval` was not available locally, so host/plugin validation beyond build, unit tests, and codesign was not run.
