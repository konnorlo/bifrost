# 04 Import-Time Analysis Pipeline

## Design rule

All expensive analysis happens after import on a background thread. The audio callback never runs heavy analysis.

## Analysis inputs

- decoded audio buffer, float32
- original sample rate
- source file path
- source file hash
- user-selected source region
- quality setting
- target mode hint: One-Shot, Sustain, Harmonizer

## Analysis outputs

- immutable `TimbreModel`
- preview waveform/spectrogram data
- quality metrics
- analysis log
- warnings, such as poor pitch confidence

## Step 1: decode

Decode into an owned interleaved or planar float buffer.

Recommended internal format:

```text
float32 planar
channels: mono for v1 analysis, stereo metadata preserved
analysis sample rate: 44100 for quality, 32000 optional Balanced, 22050 Eco
```

For stereo:

```text
M = (L + R) / 2
S = (L - R) / 2
```

Analyze `M`; store side-energy curve as optional stereo modulation.

## Step 2: normalize

- remove DC
- trim leading silence if import option enabled
- normalize analysis level, not stored output level
- detect clipping and warn

## Step 3: region detection

Detect:

- attack start
- attack end
- body start/end
- tail start
- loop candidate region for sustain mode

Use spectral flux:

```text
flux(t) = sum_k max(0, M[t,k] - M[t-1,k])
```

Use RMS derivative for extra attack confidence.

## Step 4: multi-resolution STFT

Use multiple FFT sizes because one size cannot serve every purpose:

```text
N=512/1024: transient/flux
N=2048/4096: harmonic body and timbre
N=8192 optional: low-frequency/pad precision
```

Recommended v1:

```text
Transient STFT: N=1024, hop=256
Body STFT: N=4096, hop=512 or 1024
Control curves: resampled to 200 Hz
```

## Step 5: pitch tracking

Use YIN/autocorrelation initially.

Output:

```text
f0[t]
pitchConfidence[t]
voiced[t]
```

If confidence is low:

- reduce harmonic layer reliance
- increase resonator/noise model
- show warning in UI

For synth-first sources, pitch tracking should be reliable.

## Step 6: harmonic extraction

For each voiced frame and harmonic:

```text
harmonicFrequency = h * f0[t]
if harmonicFrequency < 0.45 * analysisSampleRate:
    A_h[t] = weighted magnitude around harmonicFrequency
```

Use log-frequency or frequency-bin interpolation for smoothness.

Smooth curves at control rate:

- fast for attack
- slower for sustain/body

## Step 7: noise residual

Approximate harmonic mask, subtract or ignore harmonic neighborhoods, then sum residual into bands.

Band choices:

```text
Eco: 6-8 bands
Balanced: 8-12 bands
High: 16-24 bands
```

Use log-spaced, mel-like, or Bark-like bands.

## Step 8: resonator extraction

Find stable spectral peaks not fully explained by harmonic multiples.

Candidate features:

```text
frequency
amplitude
bandwidth/Q
lifetime
stability
harmonicity
```

Keep top K:

```text
Eco: 4
Balanced: 8
High: 12-16
```

## Step 9: timbre feature frames

Build `z_t` from:

- normalized harmonic amplitudes
- noise-band envelope
- resonator strengths
- centroid
- flatness
- flux
- loudness derivative
- pitch confidence
- optional ML embedding

## Step 10: ML-enhanced route

Optional import-time model:

```text
mel patches -> tiny encoder -> 16-D timbre embedding
```

Use embedding for:

- state fitting
- sound-type routing
- analysis confidence
- macro initialization

Do not require ML for v1 correctness. DSP fallback must work.

## Step 11: state modeling

Fit soft timbre states:

- v1: GMM with diagonal covariance
- v2: HMM over GMM emissions
- v3: HSMM duration-aware states

State count:

```text
Eco: 4
Balanced: 6-8
High: 8-12
```

Use BIC/AIC or heuristic to auto-select state count if user does not choose.

## Step 12: model compression

Compress to control-rate curves:

```text
100 Hz Eco
200 Hz Balanced
400 Hz High
```

Store:

- float32 for v1 simplicity
- optional float16 later
- delta-coded later

## Step 13: validation metrics

Compute approximate match metrics by rendering a preview at original pitch and comparing:

- multi-resolution log STFT distance
- loudness curve error
- centroid curve error
- pitch confidence
- harmonic energy explained
- noise residual explained

Use these to show user-facing quality status:

```text
Excellent tonal fit
Good tonal fit
Noisy/unstable source
Pitch uncertain
Use resonator/noise mode
```
