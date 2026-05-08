# 02 System Architecture

## Top-level pipeline

```text
User imports audio file
    ↓
AudioFileLoader decodes/resamples
    ↓
AnalysisController starts background AnalysisJob
    ↓
STFT + transient + pitch + harmonic/noise/resonator extraction
    ↓
Optional TinyML timbre encoder inference
    ↓
TimbreStateModel fits states and transitions
    ↓
ModelCompressor creates compact immutable TimbreModel
    ↓
Audio thread atomically receives model
    ↓
VoiceManager renders MIDI notes with AdditiveOscBank + NoiseBank + ResonatorBank
    ↓
Stereo VST3 output to FL Studio
```

## Thread model

### Audio thread

Allowed:

- read current immutable model pointer
- process MIDI
- start/stop voices
- render oscillators/noise/resonators
- interpolate control curves
- apply smoothing/envelopes/effects

Forbidden:

- file I/O
- allocations in hot loop
- blocking locks
- model training/fitting
- large FFT analysis
- UI calls
- Python calls
- ONNX inference unless it is trivial and explicitly moved out of realtime

### Analysis thread

Allowed:

- file decode
- resampling
- STFT analysis
- pitch tracking
- harmonic/noise split
- resonator extraction
- ML inference
- GMM/HMM fitting
- model compression
- validation metrics

### UI/message thread

Allowed:

- drag/drop
- parameter edits
- progress display
- visualizations
- selecting states
- file dialogs

Do not let the UI thread block waiting for analysis. Use progress state and cancellation.

## Immutable model swapping

The audio thread should hold a shared immutable model pointer. A background job builds a complete new model, validates it, then swaps it atomically.

```cpp
std::atomic<std::shared_ptr<const TimbreModel>> activeModel;
```

If the compiler/library combination does not support atomic shared_ptr ergonomically, use a lock-free pointer wrapper or a short non-audio-thread swap plus RCU-style lifetime management. The audio callback must not block.

## Major modules

### `audio/`

- `AudioFileLoader`: decodes WAV/AIFF/FLAC/MP3 through a swappable backend.
- `Resampler`: converts input to internal analysis sample rate.
- `SampleBuffer`: owned analysis buffer format.

### `analysis/`

- `AnalysisJob`: owns the offline analysis lifecycle.
- `STFT`: multi-resolution spectral analysis.
- `TransientDetector`: flux/onset and attack/body/tail regions.
- `PitchTracker`: YIN/autocorrelation pitch estimation.
- `HarmonicExtractor`: harmonic amplitude curves.
- `NoiseExtractor`: residual and noise-band curves.
- `ResonatorExtractor`: stable peak/formant/resonator candidates.
- `TimbreStateModel`: GMM/HMM state model.
- `ModelCompressor`: curve quantization/decimation.

### `ml/`

- `TinyTimbreEncoder`: optional import-time ONNX inference.
- `ModelRouter`: classifies sound type and chooses extraction path.
- `MLFeatureFrame`: mel patch / embedding contract.

### `model/`

- `TimbreModel`: immutable compact model consumed by synth voices.
- `Curve`: control-rate curve storage and interpolation.
- `StateGraph`: timbre-state centers and transition matrix.
- `PresetSerializer`: save/load embedded model + source reference.

### `synth/`

- `Voice`: one active MIDI note.
- `VoiceManager`: polyphony, stealing, MIDI events.
- `AdditiveOscBank`: harmonic oscillator bank.
- `NoiseBank`: filtered noise bands.
- `ResonatorBank`: resonant/formant/body filters.
- `InertiaSmoother`: physically inspired smoothing.
- `ModMatrix`: maps extracted curves and macros to render controls.

### `ui/`

- `SampleDropTarget`: file drag/drop.
- `WaveformView`: waveform with attack/body/tail.
- `SpectrogramView`: spectral preview.
- `StateMapView`: timbre constellation / visualizer.
- `MacroControls`: knobs/sliders.
- `AnalysisProgressView`: job status/cancel.

## Development phases

### Phase 0: compileable shell

- JUCE VST3 instrument builds on macOS.
- Empty synth emits silence.
- Parameters appear in FL Studio.
- Drag/drop accepts file paths.

### Phase 1: first playable tone

- WAV import.
- STFT/loudness/pitch/harmonic extraction.
- Additive one-shot resynthesis from MIDI.
- Basic waveform UI.

### Phase 2: good instrument behavior

- Noise bands.
- Attack/body/tail detection.
- Sustain/freeze mode.
- Formant lock.
- Polyphony and voice stealing.

### Phase 3: differentiating features

- Timbre state graph.
- Inertia/mutation/orbit controls.
- Harmonizer mode.
- Visual state map.

### Phase 4: ML-enhanced analysis

- Tiny ONNX timbre encoder.
- Analysis routing.
- Embedding-assisted state modeling.
- Model quality scoring.

### Phase 5: release hardening

- Windows build.
- MP3 path reviewed and stabilized.
- CI builds.
- Regression tests.
- Preset compatibility strategy.
