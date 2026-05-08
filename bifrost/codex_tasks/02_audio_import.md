# Task 02 — Audio Import

Goal: drag/drop supported files and decode into an internal buffer.

Steps:

1. Finish `AudioFileLoader`.
2. Support WAV/AIFF/FLAC through JUCE first.
3. Add MP3 path behind a clear backend abstraction.
4. Add file hash and source metadata.
5. Convert to mono mid signal for analysis.
6. Show file name, duration, sample rate, and errors in UI.

Acceptance:

- Dragging a WAV starts analysis.
- Unsupported file gives UI error, not crash.
- Imported buffer is owned and safe after file closes.
- Long files are capped or region-limited.
