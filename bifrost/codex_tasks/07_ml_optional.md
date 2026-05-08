# Task 07 — Optional ML

Goal: add useful lightweight ML only after DSP pipeline works.

Steps:

1. Add CMake option for ONNX Runtime.
2. Add model file discovery/loading.
3. Implement `TinyTimbreEncoder` wrapper.
4. Add mel patch extraction.
5. Use embedding in timbre state model.
6. Add fallback to DSP-only if ONNX/model unavailable.

Acceptance:

- plugin works without ONNX.
- with ONNX enabled, import analysis can use embedding.
- audio callback never calls ONNX.
