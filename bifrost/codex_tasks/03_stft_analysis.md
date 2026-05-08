# Task 03 — STFT Analysis

Goal: compute reliable spectral/loudness/centroid curves.

Steps:

1. Replace placeholder STFT with correct JUCE FFT magnitude layout.
2. Add Hann window and valid hop handling.
3. Add waveform preview downsampling.
4. Add RMS/loudness curve.
5. Add spectral centroid and flux.
6. Add tests using synthetic sine/noise.

Acceptance:

- sine wave centroid is near sine frequency.
- white noise flatness/centroid behaves sensibly.
- no out-of-bounds reads on short files.
