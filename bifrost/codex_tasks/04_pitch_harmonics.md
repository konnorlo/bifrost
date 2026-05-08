# Task 04 — Pitch and Harmonics

Goal: build first tonal TimbreModel.

Steps:

1. Replace naive root pitch estimate with YIN/autocorrelation per frame.
2. Add user root-note override support.
3. Extract normalized harmonic amplitudes from STFT.
4. Store control-rate curves.
5. Add noise residual placeholder or first actual residual.
6. Add quality metrics: pitch confidence, harmonic energy explained.

Acceptance:

- imported saw/pluck creates nonzero harmonic curves.
- MIDI note plays with source-like spectral envelope.
- transposing up/down remains stable.
