# 09 Build, Release, and Testing Plan

## Local macOS build

```bash
cmake -S . -B build -G Xcode -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

Alternative Ninja build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Windows build later

Use Visual Studio generator or Ninja with MSVC:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo
```

JUCE 8.0.12 notes mention Visual Studio 2026 defaults in Projucer, but CMake + VS 2022 is still a safe common target until VS 2026 is validated for the project.

## Plugin install paths

macOS VST3:

```text
~/Library/Audio/Plug-Ins/VST3
/Library/Audio/Plug-Ins/VST3
```

Windows VST3:

```text
C:\Program Files\Common Files\VST3
```

## CI

GitHub Actions is configured in `.github/workflows/build.yml`:

- macOS `macos-26` Xcode build
- Windows `windows-2022` Visual Studio 2022 build
- dependency lock validation
- console test target build
- CTest execution
- versioned VST3 artifact upload with missing-artifact failure
- no public code signing in CI

## Testing layers

### Unit tests

Test pure DSP/model functions:

- STFT window/hop sanity
- curve interpolation
- pitch tracker on synthetic sine waves
- harmonic extractor on synthetic harmonic stacks
- noise bands on generated noise
- state graph normalization
- serializer roundtrip

### Golden audio tests

Use synthetic test sources:

- sine
- saw-like harmonic stack
- pluck envelope
- bell partial set
- noise burst
- FM-ish metallic hit

For each source:

1. analyze
2. render original-pitch preview
3. compute log-STFT distance
4. assert under threshold
5. render MIDI transposition and check no NaNs/clipping

### Host tests

Manual early:

- FL Studio macOS if available
- FL Studio Windows later
- REAPER as secondary host sanity check
- plugin scan/reopen/preset recall

### Performance tests

Measure:

- import analysis time
- memory during analysis
- CPU in Balanced mode at 8 voices
- CPU in High mode
- denormal behavior in silence/release
- UI repaint cost

## Performance budget for MacBook Air M3

Balanced target:

```text
8 voices
32-48 harmonics per voice
8-12 noise bands
6-8 resonators
under roughly 10-15% DAW CPU for typical polyphony
analysis can take 1-10 seconds depending on source length/quality
```

## Release constraints

Before any public binary:

- confirm JUCE license situation
- confirm MP3 decoder licensing path
- confirm ONNX Runtime binary distribution licensing
- add crash reporting only if privacy/legal clear
- code signing and notarization for macOS
- installer/package for Windows later

## Versioning

Use semantic versions:

```text
0.1.0: first playable prototype
0.2.0: noise/resonator/sustain
0.3.0: state map + harmonizer
0.4.0: optional ML analysis
1.0.0: stable public release
```

Preset model versioning:

```text
model_format_version = 1
```

Never break old presets without migration code.

## Preset Migration Plan

Current reader behavior:

- `BifrostPreset` roots must carry `model_format_version`.
- Version `1` is the current format.
- Legacy parameter-root states without a `BifrostPreset` wrapper are accepted and marked as migrated.
- Presets from newer model formats keep their parameter state loadable, but are marked forward-compatible so the UI/host layer can warn later.

Migration rule for future changes:

1. Add a new integer format version when serialized model data changes shape.
2. Keep readers for every older shipped version.
3. Convert old model metadata into the current in-memory shape before installing it.
4. Preserve parameter state even when model metadata cannot be migrated.
5. Add a console regression test for each migration path.

## Release Checklist

Before tagging:

- Update `project(Bifrost VERSION ...)` in `CMakeLists.txt`.
- Run `python3 scripts/validate_dependency_lock.py`.
- Run `scripts/build_xcode.sh` on macOS.
- Confirm CTest passes locally.
- Confirm the generated VST3 loads in at least one host.
- Confirm preset save/reopen works for a current preset and a legacy parameter-root preset.
- Confirm old plugin names do not appear in source or release packaging.
- Review JUCE, ONNX Runtime, and decoder licensing notes.
- Decide whether ONNX/miniaudio are enabled for the release build.

Before publishing artifacts:

- Confirm GitHub Actions macOS and Windows jobs pass from a clean checkout.
- Download CI artifacts and inspect that each contains a `Bifrost.vst3`.
- Verify artifact names include the project version and commit SHA.
- Run macOS code signing and notarization for public binaries.
- Smoke test the signed/notarized macOS VST3.
- Build and smoke test Windows VST3 on a Windows host.
- Create release notes with known limitations and preset-format version.
- Archive the exact dependency lock with the release.
