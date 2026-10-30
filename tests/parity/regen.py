"""Writes the parity outputs: what the C++ core (through the installed Python package) says for every input of the
corpus (tests/lang/<lang>/corpus.jsonl) and of the gold set (tests/lang/<lang>/gold.jsonl), one file per language in
tests/parity/outputs/, and the expected text and changes of every case in tests/parity/api.jsonl. Every
implementation must reproduce them exactly. Run after a behaviour change, and commit the outputs with it; prints each
changed output.

An API case is {"lang", "options", "input"}; the script sets "output" and "changes" ([start, end, replacement],
offsets in code points). A new case needs only the first three keys.

    .venv/bin/python tests/parity/regen.py [-q]
"""

import json
import sys
from pathlib import Path

from verbalis import Options, Verbalizer

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "parity" / "outputs"


def jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line] if path.exists() else []


changed = 0
for folder in sorted(p for p in (ROOT / "lang").iterdir() if (p / "corpus.jsonl").exists()):
    lang = folder.name
    verbalizer = Verbalizer(lang)
    # The category is the set the input comes from: corpus, or gold/<class>
    inputs = {r["input"]: "corpus" for r in jsonl(folder / "corpus.jsonl")}
    for case in jsonl(folder / "gold.jsonl"):
        inputs.setdefault(case["input"], f"gold/{case['class']}")
    path = OUT / f"{lang}.jsonl"
    before = {r["input"]: r["output"] for r in jsonl(path)}
    rows = []
    for text, category in inputs.items():
        output = verbalizer.process(text)
        if text in before and before[text] != output:
            changed += 1
            if "-q" not in sys.argv:
                print(f"[{lang}] {text[:200]}\n  - {before[text][:200]}\n  + {output[:200]}")
        rows.append({"lang": lang, "category": category, "input": text, "output": output})
    OUT.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in rows), encoding="utf-8")

api = ROOT / "parity" / "api.jsonl"
cases = jsonl(api)
for case in cases:
    result = Verbalizer(case["lang"], Options(**case.get("options", {}))).process_with_changes(case["input"])
    changes = [[change.start, change.end, change.replacement] for change in result.changes]
    if "output" in case and (case["output"], case["changes"]) != (result.text, changes):
        changed += 1
        if "-q" not in sys.argv:
            print(f"[api {case['lang']}] {case['input'][:200]}\n  - {case['output'][:200]}\n  + {result.text[:200]}")
    case["output"], case["changes"] = result.text, changes
api.write_text("".join(json.dumps(case, ensure_ascii=False) + "\n" for case in cases), encoding="utf-8")
print("changed:", changed)
