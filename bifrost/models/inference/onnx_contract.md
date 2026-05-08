# ONNX Inference Contract

The plugin must not require ONNX for basic operation. ONNX is optional and import-time only.

## Input tensor

```text
name: mel_patch
shape: [1, 1, 64, 16]
dtype: float32
content: log-mel spectrogram patch
normalization: fixed dataset mean/std preferred
```

## Output tensors

```text
embedding
shape: [1,16]
dtype: float32

class_logits
shape: [1,8]
dtype: float32

macro_init
shape: [1,8]
dtype: float32
range: approximately [0,1]

quality
shape: [1,4]
dtype: float32
range: approximately [0,1]
```

## Class order

```text
0 synth_bass
1 synth_lead
2 synth_stab
3 pad_drone
4 bell_pluck
5 vocal_vowel
6 noise_texture
7 poor_tonal_fit
```

## Macro order

```text
0 Body
1 Air
2 Metal
3 Brightness
4 Motion
5 Inertia
6 Transient
7 Formant Lock
```

## Quality order

```text
0 tonal_fit_score
1 pitch_confidence_score
2 sustain_score
3 harmonizer_score
```
