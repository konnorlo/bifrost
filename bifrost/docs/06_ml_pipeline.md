# 06 ML Pipeline: Useful but Lightweight

## Philosophy

Use ML to improve analysis, not to generate realtime audio.

The plugin should feel intelligent because it makes good extraction decisions and builds useful timbre states. It should not depend on a large model in the audio callback.

## Import-time ML tasks

### 1. Timbre embedding

Input:

```text
mel spectrogram patch: 64 mel bins x 16 frames
```

Output:

```text
16-D embedding vector z_t
```

Purpose:

- cluster/state modeling in a perceptual space
- identify stable timbre regions
- separate attack/body/noise states better than hand descriptors alone

### 2. Source-type routing

Classify the imported sound as one or more of:

```text
synth_bass
synth_lead
synth_stab
pad_drone
bell_pluck
vocal_vowel
noise_texture
percussion
poor_tonal_fit
```

Use this to choose:

- harmonic count
- pitch tracking aggressiveness
- resonator count
- noise model strength
- sustain/loop strategy

### 3. Macro initialization

Predict initial values:

```text
Body
Air
Metal
Brightness
Roughness
Transient
Motion
Formant Lock
```

### 4. Quality scoring

Predict whether the source is likely to become a good tonal instrument.

Output:

```text
tonal_fit_score in [0,1]
pitch_confidence_score
sustainability_score
harmonizer_score
```

## Tiny encoder architecture

Keep it small enough to run instantly on import.

```text
Input: [1, 64, 16] log-mel patch
Conv2D 1->16, kernel 3x3, stride 1, GELU
Conv2D 16->32, kernel 3x3, stride 2, GELU
Conv2D 32->32, kernel 3x3, stride 2, GELU
Global average pool
Linear 32->64, GELU
Linear 64->16 embedding
Optional heads:
    Linear 16->N_classes
    Linear 16->N_macros
    Linear 16->quality_scores
```

This is intentionally tiny. It should be import-time only.

## Training data strategy

Start self-supervised/weakly supervised:

1. Collect royalty-free or internal synth one-shots, basses, leads, plucks, pads.
2. Segment into short clips.
3. Compute log-mel patches.
4. Train embedding using contrastive augmentations:
   - small EQ changes
   - small gain changes
   - small time crop shifts
   - mild noise
   - pitch-preserving augmentations
5. Add supervised heads later with manually labeled classes.

## Why not train sample-to-parameters first

Directly training:

```text
audio -> all synth parameters
```

is harder because the target parameters are not naturally labeled. First build the DSP extractor and use ML only for features/routing. Later, once the system generates models, you can use synthetic data:

```text
random TimbreModel -> render audio -> train inverse predictor
```

## Synthetic training loop later

1. Randomly generate valid timbre models.
2. Render audio using the plugin engine offline.
3. Train a network to estimate model parameters from audio.
4. Use the estimator as an initialization for import-time fitting.

This becomes closer to inverse synthesis, but with your own resynthesis representation.

## ONNX contract

Runtime input:

```text
name: mel_patch
shape: [1, 1, 64, 16]
dtype: float32
normalization: log-mel, per-clip mean/std or fixed dataset stats
```

Runtime outputs:

```text
embedding: [1,16]
class_logits: [1,N]
macro_init: [1,M]
quality: [1,Q]
```

## Deployment strategy

- Build plugin without ONNX first.
- Add `BIFROST_ENABLE_ONNX` CMake option.
- If model/dylib is unavailable, analysis falls back to DSP-only.
- Do not make basic use depend on ML.

## Recommended training dependencies

Pinned in `models/training/requirements.lock.txt`:

- Python 3.12
- torch 2.11.0
- numpy 2.4.4
- scipy 1.16.x or latest compatible when resolved
- librosa 0.11.0
- scikit-learn 1.8.0
- onnx 1.21.0
- onnxruntime 1.25.1

Before actual training, run `pip-compile` or `uv lock` on the target machine to resolve transitive dependencies for macOS arm64.
