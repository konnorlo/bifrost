# Task 01 — Bootstrap Build

Goal: make the project compile as a VST3 instrument shell.

Steps:

1. Validate top-level CMake with JUCE FetchContent.
2. Ensure `Bifrost` builds as VST3 on macOS.
3. Confirm plugin has MIDI input and stereo output.
4. Confirm parameters are visible to host.
5. Fix any compile issues in skeleton classes.
6. Do not implement DSP yet beyond silence/basic oscillator sanity.

Acceptance:

- `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`
- `cmake --build build`
- VST3 artifact produced.
- Plugin loads in a host/plugin validator.
