# 03 Signal Model Theory

## Core representation

The imported source audio is converted into a compact time-varying model:

```text
TimbreModel = HarmonicModel + NoiseModel + ResonatorModel + TransientModel + StateGraph + MacroMap
```

At control frame `t`:

```text
theta(t) = [f0(t), L(t), A_1..A_H(t), N_1..N_B(t), R_1..R_K(t), state probabilities]
```

where:

- `f0(t)` = estimated original pitch curve
- `L(t)` = loudness curve
- `A_h(t)` = normalized harmonic amplitude curve
- `N_b(t)` = noise band energy curve
- `R_k(t)` = resonator/formant/body strength curve
- state probabilities = soft timbre-state membership

## Why this is instrument-first

For MIDI playback, the original pitch is replaced by MIDI pitch:

```text
f_midi = 440 * 2^((note - 69) / 12)
```

The source timbre motion is reused, but the pitch identity comes from MIDI.

This is the fundamental difference between:

- sample playback: `play x[n] faster/slower`
- this plugin: `play extracted timbre behavior at a new pitch`

## Additive harmonic layer

For one voice:

```text
y_harm[n] = sum_h a_h[n] * sin(phi_h[n])
phi_h[n+1] = phi_h[n] + 2*pi*h*f_midi/Fs
```

Only render harmonics below Nyquist:

```text
h * f_midi < 0.45 * Fs
```

This prevents obvious aliasing at high notes and saves CPU.

## Harmonic amplitude extraction

Given STFT magnitude `M[t,k]` and estimated `f0[t]`, harmonic bin neighborhoods are:

```text
Omega_h(t) = bins near h * f0(t)
A_h(t) = sum_{k in Omega_h(t)} M[t,k]
```

Normalize harmonic distribution:

```text
Atilde_h(t) = A_h(t) / (sum_j A_j(t) + eps)
```

Then store loudness separately:

```text
L(t) = RMS or perceptual loudness
```

At playback:

```text
a_h(t) = L(t) * Atilde_h(t) * macro_gain_h
```

## Noise layer

The residual/non-harmonic part is grouped into perceptual bands:

```text
N_b(t) = sum_{k in band_b} |X_noise[t,k]|^2
```

Realtime rendering uses white/pink-ish noise through bandpass filters rather than stored noise samples:

```text
y_noise[n] = sum_b gain_b[n] * BPF_b(noise[n])
```

For tonal synth-first material, noise is secondary but important for attacks, air, grit, and identity.

## Resonator layer

Stable peaks or formant-like regions are modeled as resonators:

```text
y_res[n] = sum_k g_k[n] * Resonator(f_k, Q_k)
```

Use this for:

- bell/metallic body
- vowel/formant regions
- hollow synth body
- glass/pluck resonance

The resonator bank should be optional in Eco mode.

## Transient layer

For instrument quality, attack identity matters disproportionately. Preserve or resynthesize a short attack layer:

Options:

1. Pure synthetic transient from noise + harmonic attack curves.
2. Tiny attack micro-sample layer, only 10-80 ms, heavily optional.
3. Spectral attack model using high-resolution early STFT frames.

The clean v1 approach:

- no raw sample playback by default
- optional attack assist layer behind a knob called `Attack Preserve`
- attack assist should be short enough that the instrument identity remains resynthetic rather than sampler-like

## Timbre states

Build a feature vector per control frame:

```text
z_t = [Atilde_1..Atilde_H, N_1..N_B, R_1..R_K, centroid, flatness, flux, loudness_delta]
```

Use PCA/whitening or tiny ML embedding to reduce dimension.

Use GMM/HMM for soft states:

```text
p(s=j | z_t) = pi_j * N(z_t | mu_j, Sigma_j) / sum_l pi_l * N(z_t | mu_l, Sigma_l)
```

HMM adds transition structure:

```text
p(s_t | s_{t-1}) = A[s_{t-1}, s_t]
```

## Inertia smoothing

For organic motion, avoid direct jumps between target parameters:

Simple smoother:

```text
y_t = rho * y_{t-1} + (1-rho) * target_t
```

Spring/inertia smoother:

```text
v_t = alpha * v_{t-1} + (1-alpha) * (target_t - y_{t-1})
y_t = y_{t-1} + beta * v_t
```

Expose this as `Inertia`.

## Clone vs Instrument axis

Even if the main use is instrument-first, keep a conceptual axis:

```text
Clone ---------------- Instrument
```

Clone side:

- more raw time-series detail
- less smoothing
- attack more preserved
- closer source matching

Instrument side:

- smoother timbre curves
- stable formants
- loop/freeze regions
- less exact but more playable

## Why not pure FFT reconstruction for instrument mode

IFFT/phase-vocoder reconstruction can reproduce the source but often becomes sample-like, smeared, or unstable under large pitch changes. For a tonal instrument, harmonic/noise/resonator controls are better because they generalize across pitch and chords.
