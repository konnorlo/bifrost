# CMake Notes

The top-level `CMakeLists.txt` is intentionally simple.

Important options:

```bash
-DBIFROST_BUILD_STANDALONE=ON
-DBIFROST_ENABLE_ONNX=ON
-DBIFROST_ENABLE_MINIAUDIO=ON
```

Do not enable optional dependencies until the DSP-only VST3 builds and runs.
