"""Every package has the version of cpp/CMakeLists.txt, the one source."""

import json
import re
from pathlib import Path

import verbalis

ROOT = Path(__file__).parents[2]


def test_versions_match():
    cmake = re.search(r"project\(verbalis VERSION ([0-9.]+)", (ROOT / "cpp" / "CMakeLists.txt").read_text()).group(1)
    npm = json.loads((ROOT / "ts" / "package.json").read_text())["version"]
    assert verbalis.__version__ == cmake == npm
