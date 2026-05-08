#!/usr/bin/env bash
set -euo pipefail
python3 scripts/validate_dependency_lock.py
cmake -S . -B build-xcode -G Xcode -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBIFROST_BUILD_TESTS=ON -DBIFROST_ENABLE_ONNX=OFF -DBIFROST_ENABLE_MINIAUDIO=OFF
cmake --build build-xcode --config RelWithDebInfo --target BifrostTests
ctest --test-dir build-xcode -C RelWithDebInfo --output-on-failure
cmake --build build-xcode --config RelWithDebInfo --target Bifrost_VST3
