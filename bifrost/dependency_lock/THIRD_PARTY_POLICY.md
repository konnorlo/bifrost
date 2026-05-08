# Third-Party Dependency Policy

## Runtime plugin dependencies

Runtime plugin dependencies must be:

- pinned
- license-reviewed
- easy to disable or replace
- documented in `dependency_lock.json`

## MP3 policy

MP3 support is required by product goals, but implementation must remain swappable.

Order of preference:

1. Use platform-supported decoders when possible.
2. Use JUCE built-in formats for non-MP3 common formats.
3. Add miniaudio as optional decoder backend if needed.
4. Use JUCE `MP3AudioFormat` only after reviewing its disclaimer and commercial implications.
5. Avoid FFmpeg in the plugin binary unless licensing/redistribution is fully understood.

## ML policy

- ONNX Runtime is optional.
- The plugin must function without ONNX.
- ML models run during import/analysis, not during audio rendering.
- Python is only for training/export scripts, never the plugin runtime.

## CMake policy

- Prefer FetchContent for JUCE pinned by tag.
- Do not auto-download large binary runtimes unless explicitly enabled.
- Keep all dependency toggles visible in top-level `CMakeLists.txt`.
