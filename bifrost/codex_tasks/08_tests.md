# Task 08 — Tests

Goal: add regression coverage for core DSP/model behavior.

Tests:

- Curve interpolation.
- STFT synthetic sine/noise.
- Pitch tracker sine sweep.
- Harmonic extractor saw-like stack.
- Serializer roundtrip.
- VoiceManager no-NaN output.
- AnalysisJob handles short/silent files.

Acceptance:

- tests run locally through CTest or a simple JUCE console test target.
- failures point to a specific module.
