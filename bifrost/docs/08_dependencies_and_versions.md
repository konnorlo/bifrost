# 08 Dependencies and Version Lock Plan

## C++ / plugin runtime

### JUCE

Pinned target: `JUCE 8.0.12`

Reason:

- current recent JUCE 8 release as of dependency research
- CMake integration
- VST3 instrument target
- cross-platform macOS/Windows plugin development

Lock method:

```cmake
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.12
)
```

Commercial release note: JUCE is dual-licensed. Use a proper JUCE commercial license if shipping closed-source/commercially.

### CMake

Recommended: `CMake 4.3.2`
Minimum project target: `3.27`

Reason:

- modern FetchContent support
- compatible with recent JUCE CMake workflows
- recent release available for macOS/Windows

### C++ standard

Pinned: `C++20`

Reason:

- enough modern language support for atomics/RAII/spans-like design
- avoid bleeding-edge C++23 for plugin-host compatibility

### VST3

Use VST3 via JUCE. Do not manually vendor Steinberg SDK unless necessary. JUCE 8.0.11 changelog says JUCE updated VST3 SDK to 3.8.0 MIT license, so using JUCE's path keeps this simpler.

## Audio decoding

### JUCE AudioFormatManager

Use for WAV/AIFF/FLAC/Ogg where supported.

### MP3

Need deliberate backend choice:

1. Use OS decoders when available through JUCE/CoreAudio/WindowsMedia path.
2. Use JUCE `MP3AudioFormat` only with legal review because JUCE docs include a disclaimer for enabling software MP3 decoding.
3. Optional fallback: miniaudio.

### miniaudio

Pinned target: `0.11.25`

Reason:

- single-file C/C++ audio library
- no external dependencies
- public-domain release
- latest release includes WAV decoder bug fixes

Use as optional import backend, not core audio engine.

## ML runtime

### ONNX Runtime

Pinned target: `1.25.1`

Use only if `BIFROST_ENABLE_ONNX=ON`.

Reason:

- stable current release as of dependency research
- C/C++ inference available
- import-time tiny model only

Do not link ONNX Runtime into v1 until the DSP-only plugin works.

## Python training environment

Training is separate from the plugin runtime.

Recommended pinned versions:

```text
Python 3.12.x
numpy==2.4.4
scikit-learn==1.8.0
librosa==0.11.0
onnx==1.21.0
onnxruntime==1.25.1
torch==2.11.0
```

Use `uv` or `pip-tools` to lock exact transitive dependencies on the target platform. The plugin build should not depend on Python.

## Dependency policy

- Keep runtime dependencies minimal.
- Prefer header-only/single-file dependencies for plugin import utilities.
- Keep ML optional.
- Pin every dependency by version/tag.
- Record URL, version, license, and reason in `dependency_lock/dependency_lock.json`.
- Do not hide dependencies in random source files.

## Cross-verification notes

See `dependency_lock/researched_sources.md` for the public sources used to check current releases and docs.
