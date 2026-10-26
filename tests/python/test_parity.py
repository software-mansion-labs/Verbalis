"""The parity baseline: every input of tests/parity/baseline/*.jsonl verbalizes to exactly its recorded output."""

import json
from pathlib import Path

import pytest

from verbalis import Verbalizer

BASELINE = Path(__file__).parents[1] / "parity" / "baseline"


@pytest.mark.parametrize("path", sorted(BASELINE.glob("*.jsonl")), ids=lambda path: path.stem)
def test_baseline(path):
    records = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line]
    verbalizers = {}
    mismatches = []
    for record in records:
        verbalizer = verbalizers.setdefault(record["lang"], Verbalizer(record["lang"]))
        output = verbalizer.process(record["input"])
        if output != record["output"]:
            mismatches.append((record["input"], record["output"], output))

    assert records
    assert not mismatches, f"{len(mismatches)}/{len(records)} differ, first: {mismatches[0]}"
