"""The committed stub, python/verbalis/_verbalis.pyi, matches the built module."""

import subprocess
import sys
from pathlib import Path

STUB = Path(__file__).parents[2] / "python" / "verbalis" / "_verbalis.pyi"


def test_stub_matches_module(tmp_path):
    generated = tmp_path / "_verbalis.pyi"
    subprocess.run([sys.executable, "-m", "nanobind.stubgen", "-q", "-m", "verbalis._verbalis", "-o", str(generated)],
                   check=True)
    assert generated.read_text() == STUB.read_text(), (
        "Regenerate it: python -m nanobind.stubgen -m verbalis._verbalis -o python/verbalis/_verbalis.pyi")
