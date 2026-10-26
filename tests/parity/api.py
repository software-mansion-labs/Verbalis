"""Writes the expected text and changes of every case in tests/parity/api.jsonl, from the C++ core.

Run it after a deliberate behaviour change, with the Python package installed from this checkout:

    .venv/bin/python tests/parity/api.py

A case is {"lang", "options", "input"}; the script sets "output" and "changes" ([start, end, replacement], offsets in
code points). New cases need only the first three keys.
"""

import json
from pathlib import Path

from verbalis import Options, Verbalizer

PATH = Path(__file__).with_name("api.jsonl")

cases = [json.loads(line) for line in PATH.read_text(encoding="utf-8").splitlines() if line]
with PATH.open("w", encoding="utf-8") as file:
    for case in cases:
        result = Verbalizer(case["lang"], Options(**case.get("options", {}))).process_with_changes(case["input"])
        case["output"] = result.text
        case["changes"] = [[change.start, change.end, change.replacement] for change in result.changes]
        file.write(json.dumps(case, ensure_ascii=False) + "\n")
print(f"{len(cases)} cases written to {PATH}")
