"""Scores Verbalis against the gold test set (tests/gold/<lang>.jsonl): how many inputs it reads as a person would.

    .venv/bin/python tests/gold/score.py              # accuracy per language, and changes since scores.json
    .venv/bin/python tests/gold/score.py --classes    # also per class
    .venv/bin/python tests/gold/score.py --show de    # every failing case of a language
    .venv/bin/python tests/gold/score.py --write      # record the current passing cases in scores.json

A case passes when the output equals one of its acceptable readings, after normalization: case, whitespace, and
punctuation are ignored ("twenty-one" = "twenty one"). "exact" counts equality as written. Exits 1 if a case that
passed in scores.json fails now: a behaviour change may fix gold cases, but must not break them.
"""

import argparse
import json
import sys
import unicodedata
from collections import defaultdict
from pathlib import Path

from verbalis import Verbalizer

GOLD = Path(__file__).parent
SCORES = GOLD / "scores.json"

parser = argparse.ArgumentParser()
parser.add_argument("--classes", action="store_true")
parser.add_argument("--show", metavar="LANG")
parser.add_argument("--write", action="store_true")
args = parser.parse_args()


def normalized(text: str) -> str:
    text = unicodedata.normalize("NFKC", text).casefold()
    return "".join(c for c in text if not c.isspace() and not unicodedata.category(c).startswith("P"))


def key(case: dict) -> str:
    return f"{case['source']}/{case['class']}/{case['input']}"


passing, totals, failures = defaultdict(list), {}, defaultdict(list)
for path in sorted(GOLD.glob("*.jsonl")):
    cases = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line]
    lang = cases[0]["lang"]
    verbalizer = Verbalizer(lang)
    by_class = defaultdict(lambda: [0, 0, 0])  # cases, exact, normalized
    for case in cases:
        output = verbalizer.process(case["input"])
        exact = output in case["outputs"]
        ok = exact or normalized(output) in {normalized(o) for o in case["outputs"]}
        for name in (case["class"], "all"):
            counts = by_class[name]
            counts[0] += 1
            counts[1] += exact
            counts[2] += ok
        if ok:
            passing[lang].append(key(case))
        else:
            failures[lang].append((case, output))
    totals[lang] = by_class

print(f"{'language':10}{'cases':>7}{'exact':>9}{'normalized':>12}")
for lang, by_class in totals.items():
    n, exact, ok = by_class["all"]
    print(f"{lang:10}{n:>7}{100 * exact / n:>8.1f}%{100 * ok / n:>11.1f}%")
    if args.classes:
        for name, (n, exact, ok) in sorted(by_class.items()):
            if name != "all":
                print(f"  {name:24}{n:>5}{100 * exact / n:>8.1f}%{100 * ok / n:>11.1f}%")
all_n = sum(t["all"][0] for t in totals.values())
all_ok = sum(t["all"][2] for t in totals.values())
print(f"{'total':10}{all_n:>7}{'':>9}{100 * all_ok / all_n:>11.1f}%")

if args.show:
    for case, output in failures[args.show]:
        print(f"[{case['source']}/{case['class']}] {case['input']}\n   got:      {output}\n   expected: "
              + " | ".join(case["outputs"]))

# The ratchet: compare with the recorded passing cases
recorded = json.loads(SCORES.read_text(encoding="utf-8")) if SCORES.exists() else {}
broken = {lang: sorted(set(keys) - set(passing.get(lang, []))) for lang, keys in recorded.items()}
fixed = {lang: sorted(set(keys) - set(recorded.get(lang, []))) for lang, keys in passing.items()}
for lang in sorted(set(broken) | set(fixed)):
    for k in fixed.get(lang, []) if recorded else []:
        print(f"newly passing [{lang}] {k}")
    for k in broken.get(lang, []):
        print(f"NO LONGER PASSING [{lang}] {k}")
if args.write:
    SCORES.write_text(json.dumps({lang: sorted(keys) for lang, keys in sorted(passing.items())}, ensure_ascii=False,
                                 indent=1) + "\n", encoding="utf-8")
    print(f"recorded {sum(len(k) for k in passing.values())} passing cases in {SCORES.name}")
sys.exit(1 if any(broken.values()) and not args.write else 0)
