# Bifrost - Codex-Ready Plugin Plan

This folder is intentionally structured like a small engineering repo. Read files in this order:

1. `00_START_HERE/README.md` — this orientation file.
2. `00_START_HERE/ASSUMPTIONS.md` — locked product assumptions from the user.
3. `docs/01_product_spec.md` — what the plugin is and is not.
4. `docs/02_system_architecture.md` — full pipeline from file import to VST3 output.
5. `docs/03_signal_model_theory.md` — the math and DSP representation.
6. `docs/04_analysis_pipeline.md` — import-time analysis details.
7. `docs/05_realtime_engine.md` — audio-thread rendering plan.
8. `docs/06_ml_pipeline.md` — lightweight but useful ML plan.
9. `docs/08_dependencies_and_versions.md` — pinned dependencies and rationale.
10. `codex_tasks/00_MASTER_PROMPT.md` — give this to Codex first.

## Project goal

Build a C++20 JUCE VST3 instrument/generator plugin for macOS and Windows, developed first on macOS. The user drags in an audio file, initially synth-first source material such as stabs, basses, leads, pads, bells, and tonal one-shots. The plugin analyzes the sound, extracts a compact timbre-motion model, and turns it into a playable MIDI instrument with one-shot resynthesis, sustained timbre playback, and timbre harmonizer/chord modes.

## Core product distinction

Do not clone Synplant's plant/genome UX. The shared broad idea is only this:

`audio -> compact generative representation -> playable instrument`

This plugin's unique representation is:

`audio -> harmonic/noise/resonator curves + timbre states + modulation field -> playable resynthesis instrument`

## Implementation style

The code skeleton in `Source/` is not a finished plugin. It is a compartmentalized starting point designed so Codex can fill each part without needing to infer the full architecture from chat history.

Heavy analysis and ML happen on file import/background threads. The audio callback only renders precomputed model curves using deterministic DSP.
