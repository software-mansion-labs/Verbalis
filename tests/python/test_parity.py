"""The parity contract: tests/parity/outputs/*.jsonl (outputs) and tests/parity/api.jsonl (options and changes)."""

import json
from pathlib import Path

import pytest

from verbalis import Options, Verbalizer

PARITY = Path(__file__).parents[1] / "parity"
OUTPUTS = PARITY / "outputs"


@pytest.mark.parametrize("path", sorted(OUTPUTS.glob("*.jsonl")), ids=lambda path: path.stem)
def test_outputs(path):
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


API_CASES = [json.loads(line) for line in (PARITY / "api.jsonl").read_text(encoding="utf-8").splitlines() if line]


@pytest.mark.parametrize("case", API_CASES, ids=lambda case: f"{case['lang']}:{case['input'][:20]}")
def test_api(case):
    result = Verbalizer(case["lang"], Options(**case["options"])).process_with_changes(case["input"])
    assert result.text == case["output"]
    assert [[c.start, c.end, c.replacement] for c in result.changes] == case["changes"]
