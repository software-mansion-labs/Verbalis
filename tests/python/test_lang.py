"""The gold set and the corpus (tests/lang/) as a ratchet: no case recorded as passing in scores.json may fail."""

import subprocess
import sys
from pathlib import Path

SCORE = Path(__file__).parents[1] / "lang" / "score.py"


def test_no_gold_case_stops_passing():
    result = subprocess.run([sys.executable, str(SCORE)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout[-3000:]


def test_corpus_is_frozen():
    """The corpus (tests/lang/<lang>/corpus.jsonl) is the target, frozen at tag corpus-v2: it changes only by version."""
    import hashlib

    root = SCORE.parent
    for line in (root / "corpus.sha256").read_text().splitlines():
        digest, name = line.split()
        path = root / name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, f"{name} changed; it is frozen"
