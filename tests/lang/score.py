"""Scores Verbalis against the gold test set (tests/lang/<lang>/gold.jsonl): how many inputs it reads as a person would.

    .venv/bin/python tests/lang/score.py              # accuracy per language, and changes since scores.json
    .venv/bin/python tests/lang/score.py --classes    # also per class
    .venv/bin/python tests/lang/score.py --show de    # every failing case of a language
    .venv/bin/python tests/lang/score.py --write      # record the current passing cases in scores.json
    .venv/bin/python tests/lang/score.py --corpus pl  # every corpus sentence of a language Verbalis misreads

A case passes when the output equals one of its acceptable readings, after normalization: case, whitespace, and
punctuation are ignored ("twenty-one" = "twenty one"). "exact" counts equality as written. A case with an
"out_of_scope" reason (markup, malformed input, gold that contradicts itself) is left out of the score and counted
apart; tests/lang/<lang>/notes.md lists them. Exits 1 if a case that
passed in scores.json fails now: a behaviour change may fix gold cases, but must not break them.

It also scores the corpus (tests/lang/<lang>/corpus.jsonl): real sentences with readings that reviewers approved or
wrote, mostly agents. It is the target Verbalis must reach, frozen at tag corpus-v1 (tests/lang/corpus.sha256): its
passing sentences join the ratchet. "approved" are outputs a reviewer passed; "written" are readings a reviewer or
judge wrote, which Verbalis may not reach yet.
"cross-checked" readings DeepSeek also wrote, blind; "disputed" ones it reads otherwise, left out of the match.
"""

import argparse
import hashlib
import json
import sys
import unicodedata
from collections import defaultdict
from pathlib import Path

from verbalis import Verbalizer

ROOT = Path(__file__).parent
SCORES = ROOT / "scores.json"

parser = argparse.ArgumentParser()
parser.add_argument("--classes", action="store_true")
parser.add_argument("--show", metavar="LANG")
parser.add_argument("--write", action="store_true")
parser.add_argument("--corpus", metavar="LANG")
args = parser.parse_args()


def normalized(text: str) -> str:
    text = unicodedata.normalize("NFKC", text).casefold()
    return "".join(c for c in text if not c.isspace() and not unicodedata.category(c).startswith("P"))


def key(case: dict) -> str:
    return f"{case['source']}/{case['class']}/{case['input']}"


passing, totals, failures, excluded = defaultdict(list), {}, defaultdict(list), defaultdict(int)
for path in sorted(ROOT.glob("*/gold.jsonl")):
    cases = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line]
    lang = cases[0]["lang"]
    verbalizer = Verbalizer(lang)
    by_class = defaultdict(lambda: [0, 0, 0])  # cases, exact, normalized
    for case in cases:
        if "out_of_scope" in case:
            excluded[lang] += 1
            continue
        output = verbalizer.process(case["input"])
        exact = output in case["outputs"]
        ok = exact or normalized(output) in {normalized(o) for o in case["outputs"]}
        origin = "own" if case["source"] == "verbalis" else "independent"
        for name in (case["class"], "all", origin):
            counts = by_class[name]
            counts[0] += 1
            counts[1] += exact
            counts[2] += ok
        if ok:
            passing[lang].append(key(case))
        else:
            failures[lang].append((case, output))
    totals[lang] = by_class

def share(counts):
    return f"{100 * counts[2] / counts[0]:.1f}% of {counts[0]}" if counts[0] else "-"


# "independent": cases from NeMo and WeTextProcessing; "own": cases written for Verbalis (source "verbalis"),
# which are only as right as their author, until a native speaker has checked them
print(f"{'language':10}{'cases':>7}{'exact':>9}{'normalized':>12}   {'independent':>18}   {'own':>18}   {'excluded':>8}")
for lang, by_class in totals.items():
    n, exact, ok = by_class["all"]
    print(f"{lang:10}{n:>7}{100 * exact / n:>8.1f}%{100 * ok / n:>11.1f}%   "
          f"{share(by_class['independent']):>18}   {share(by_class['own']):>18}   {excluded[lang] or '-':>8}")
    if args.classes:
        for name, (n, exact, ok) in sorted(by_class.items()):
            if name not in ("all", "independent", "own"):
                print(f"  {name:24}{n:>5}{100 * exact / n:>8.1f}%{100 * ok / n:>11.1f}%")
all_n = sum(t["all"][0] for t in totals.values())
all_ok = sum(t["all"][2] for t in totals.values())
print(f"{'total':10}{all_n:>7}{'':>9}{100 * all_ok / all_n:>11.1f}%")

if args.show:
    for case, output in failures[args.show]:
        print(f"[{case['source']}/{case['class']}] {case['input']}\n   got:      {output}\n   expected: "
              + " | ".join(case["outputs"]))

# The corpus: real sentences with reviewed readings, the target (frozen at tag corpus-v1). Its passing sentences join
# the ratchet under "corpus/<lang>", by a hash of the input; hi and zh count only where their local corpus exists.
corpus_passing = {}
print(f"\n{'corpus':10}{'sentences':>10}{'match':>9}   {'approved':>18}   {'written':>18}   {'by a person':>12}"
      f"{'cross-checked':>15}{'disputed':>10}")
for path in sorted(ROOT.glob("*/corpus.jsonl")):
    rows = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line]
    if not rows:
        continue
    lang, verbalizer = rows[0]["lang"], Verbalizer(rows[0]["lang"])
    counts = defaultdict(lambda: [0, 0, 0])  # sentences, exact, normalized
    passing_inputs = []
    disputed = sum("disputed" in row for row in rows)
    for row in rows:
        if "disputed" in row:  # DeepSeek reads it otherwise: no expected reading to score against
            continue
        counts["cross"][0] += row.get("deepseek") == "agrees"
        output = verbalizer.process(row["input"])
        ok = normalized(output) in {normalized(o) for o in row["outputs"]}
        for name in ("all", row["how"], row["by"]):
            counts[name][0] += 1
            counts[name][2] += ok
        if ok:
            passing_inputs.append(hashlib.sha1(row["input"].encode()).hexdigest()[:12])
        if not ok and args.corpus == lang:
            print(f"[{row['by']} {row['how']}] {row['input']}\n   got:      {output}\n   expected: "
                  + " | ".join(row["outputs"]))
    n, _, ok = counts["all"]
    corpus_passing[f"corpus/{lang}"] = passing_inputs
    print(f"{lang:10}{n:>10}{100 * ok / n:>8.1f}%   {share(counts['approved']):>18}   {share(counts['written']):>18}"
          f"   {counts['human'][0] or '-':>12}{counts['cross'][0]:>15}{disputed:>10}")

# The ratchet: compare with the recorded passing cases
passing.update(corpus_passing)
recorded = json.loads(SCORES.read_text(encoding="utf-8")) if SCORES.exists() else {}
recorded = {key: keys for key, keys in recorded.items() if not key.startswith("corpus/") or key in corpus_passing}
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
