"""The gold set (tests/lang/) as a ratchet: no case recorded as passing in scores.json may fail."""

import subprocess
import sys
from pathlib import Path

SCORE = Path(__file__).parents[1] / "lang" / "score.py"


def test_no_gold_case_stops_passing():
    result = subprocess.run([sys.executable, str(SCORE)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout[-3000:]
