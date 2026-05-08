from __future__ import annotations

import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cmake_text = (root / "CMakeLists.txt").read_text()
match = re.search(r"project\s*\(\s*Bifrost\s+VERSION\s+([0-9.]+)", cmake_text)
if not match:
    raise SystemExit("ERROR: could not read Bifrost project version from CMakeLists.txt")

print(f"version={match.group(1)}")
