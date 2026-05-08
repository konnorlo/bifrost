# Tests

Tests should be added after the initial plugin shell compiles.

Recommended first tests:

1. `Curve` interpolation edge cases.
2. STFT sine wave centroid.
3. Pitch tracker synthetic sine.
4. Harmonic extractor synthetic saw stack.
5. Preset serialization roundtrip.
6. Voice rendering does not output NaN.

Do not require FL Studio for unit tests. Host tests are manual/integration tests.
