# Task 05 — Realtime Synth

Goal: make the plugin a useful playable instrument.

Steps:

1. Optimize additive oscillator bank.
2. Add basic ADSR/release behavior.
3. Add Sustain mode loop/freeze region.
4. Add noise band rendering.
5. Add resonator bank rendering.
6. Add harmonizer mode with multiple MIDI voices.
7. Add denormal protection and output limiting/soft clipping.

Acceptance:

- 8 voices Balanced mode runs well on MacBook Air M3.
- no crackles from allocations/locks in audio callback.
- One-Shot and Sustain both work.
