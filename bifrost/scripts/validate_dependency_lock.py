from __future__ import annotations

import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
lock = root / "dependency_lock" / "dependency_lock.json"
cmake = root / "CMakeLists.txt"


def fail(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def require(mapping: dict, path: tuple[str, ...]):
    current = mapping
    for key in path:
        if not isinstance(current, dict) or key not in current:
            fail(f"missing dependency lock key: {'.'.join(path)}")
        current = current[key]
    return current


def capture(pattern: str, text: str, label: str) -> str:
    match = re.search(pattern, text, flags=re.MULTILINE)
    if not match:
        fail(f"could not read {label} from CMakeLists.txt")
    return match.group(1)


data = json.loads(lock.read_text())
cmake_text = cmake.read_text()

if require(data, ("project",)) != "Bifrost":
    fail("dependency lock project must be Bifrost")

cmake_minimum = capture(r"cmake_minimum_required\s*\(\s*VERSION\s+([0-9.]+)", cmake_text, "minimum CMake version")
locked_cmake_minimum = require(data, ("runtime_cpp", "cmake_minimum"))
if cmake_minimum != locked_cmake_minimum:
    fail(f"CMake minimum mismatch: CMakeLists.txt={cmake_minimum} dependency_lock={locked_cmake_minimum}")

juce_tag = capture(r"GIT_TAG\s+([^\s)]+)", cmake_text, "JUCE git tag")
locked_juce_tag = require(data, ("runtime_cpp", "juce", "git_tag"))
locked_juce_version = require(data, ("runtime_cpp", "juce", "version"))
if juce_tag != locked_juce_tag or juce_tag != locked_juce_version:
    fail(f"JUCE pin mismatch: CMakeLists.txt={juce_tag} dependency_lock tag={locked_juce_tag} version={locked_juce_version}")

onnx_option = require(data, ("runtime_cpp", "onnxruntime_optional", "cmake_option"))
if onnx_option not in cmake_text:
    fail(f"optional ONNX CMake option {onnx_option} is not declared")

policy = require(data, ("policy",))
if not isinstance(policy, list) or not policy:
    fail("dependency policy must contain at least one rule")

project_version = capture(r"project\s*\(\s*Bifrost\s+VERSION\s+([0-9.]+)", cmake_text, "project version")
print(f"OK: {lock} project_version={project_version} juce={juce_tag} cmake_minimum={cmake_minimum}")
