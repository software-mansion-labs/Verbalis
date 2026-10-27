"""Agent review of Verbalis on real sentences: batches for the review-<lang> agents, their verdicts, and the report.

    .venv/bin/python tests/lang/review.py next pl            # write the next batch of 100, or print STOP
    .venv/bin/python tests/lang/review.py ingest pl 3        # check build/review/pl/003.out.jsonl, record it
    .venv/bin/python tests/lang/review.py confirm pl         # a blind second-pass batch of the sentences flagged wrong
    .venv/bin/python tests/lang/review.py status             # progress and reviewer accuracy per language
    .venv/bin/python tests/lang/review.py report             # confirmed errors by category: build/review/report.md
    .venv/bin/python tests/lang/review.py import-human ~/Downloads/pl-review-results.json

The pool of a language is the sentences of tests/parity/baseline (hi and zh: tests/parity/local, a sample), read by the
installed package. A batch holds 90 pool sentences and 10 hidden controls: 5 known-good readings, which the reviewer
must pass, and 5 planted errors, which it must catch. Their score says how far to trust the verdicts. A language stops
once it has 200 reviewed sentences and 20% or more are wrong, or once its pool is spent.

Verdicts go to tests/lang/<lang>/review.jsonl, one line per sentence and pass (the format is in tests/lang/README.md).
A "wrong" verdict counts as confirmed when a second, blind pass by a fresh agent flags the same sentence.
"""

import argparse
import difflib
import hashlib
import json
import random
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

from verbalis import Verbalizer

ROOT = Path(__file__).parent
REPO = ROOT.parents[1]
WORK = REPO / "build" / "review"
LANGS = ["de", "en-us", "es", "fr", "hi", "it", "pl", "pt", "zh"]
CATEGORIES = {"agreement", "cardinal", "ordinal", "date", "time", "money", "unit", "percent", "range", "decimal",
              "fraction", "phone", "code", "web", "abbreviation", "roman", "symbol", "unread", "misread", "other"}
VERDICTS = {"ok", "wrong", "unsure"}
READABLE = re.compile(r"[0-9०-९%$€£¥₹@&+=°§#]")  # A sentence with something for Verbalis to read
BATCH, CONTROLS, MIN_REVIEWED, STOP_RATE, LOCAL_SAMPLE = 100, 5, 200, 0.20, 2200

# Planted grammar errors: a correct form and a wrong one that replaces it as a whole word
FORM_SWAPS = {
    "pl": {"dwie": "dwa", "dwóch": "dwa", "trzech": "trzy", "tysięcy": "tysiąc", "złotych": "złote",
           "pierwszego": "pierwszy", "roku": "rok", "procent": "procenta", "czterech": "cztery"},
    "de": {"ersten": "erste", "zweiten": "zweite", "dritten": "dritte", "Jahren": "Jahre", "eine": "ein"},
    "en-us": {"dollars": "dollar", "first": "oneth", "second": "twoth", "third": "threeth", "hundred": "hundreds",
              "percent": "percents"},
    "es": {"doscientas": "doscientos", "una": "uno", "millones": "millón", "primero": "uno", "euros": "euro"},
    "fr": {"cents": "cent", "une": "un", "premier": "un", "millions": "million", "euros": "euro"},
    "it": {"una": "uno", "mila": "mille", "primo": "uno", "milioni": "milione"},
    "pt": {"duas": "dois", "milhões": "milhão", "primeiro": "um", "reais": "real", "uma": "um"},
}


def jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line] if path.exists() else []


def write_jsonl(path: Path, rows, mode="w"):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, mode, encoding="utf-8") as file:
        file.writelines(json.dumps(row, ensure_ascii=False) + "\n" for row in rows)


def stable(text: str) -> str:
    return hashlib.sha1(text.encode()).hexdigest()


def pool(lang: str) -> list[str]:
    """Every pool input in a fixed shuffled order."""
    base = REPO / "tests/parity/baseline"
    inputs = [r["input"] for name in (f"{lang}.jsonl", f"{lang}.kinds.jsonl") for r in jsonl(base / name)]
    local = [r for r in jsonl(REPO / "tests/parity/local" / f"{lang}.jsonl") if READABLE.search(r["input"])]
    inputs += [r["input"] for r in sorted(local, key=lambda r: stable(r["input"]))[:LOCAL_SAMPLE]]
    return sorted((s for s in dict.fromkeys(inputs) if READABLE.search(s)), key=stable)


def changes(source: str, output: str) -> list[str]:
    """What Verbalis rewrote, word by word: "93. → dziewięćdziesiątej trzeciej"."""
    a, b = source.split(), output.split()
    return [f"{' '.join(a[i1:i2])} → {' '.join(b[j1:j2])}"
            for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes()
            if tag != "equal"]


def review_path(lang: str) -> Path:
    return ROOT / lang / "review.jsonl"


def batches(lang: str) -> list[Path]:
    return sorted((WORK / lang).glob("[0-9][0-9][0-9].key.jsonl")) + sorted((WORK / lang).glob("c[0-9][0-9].key.jsonl"))


def first_pass(lang: str) -> list[dict]:
    return [r for r in jsonl(review_path(lang)) if r["by"] == "haiku" and r["pass"] == 1]


def taken(lang: str) -> set[str]:
    """Inputs in a batch already, recorded or still out with an agent."""
    return {r["input"] for path in batches(lang) for r in jsonl(path) if r["kind"] == "pool"}


def stop_reason(lang: str) -> str | None:
    rows = first_pass(lang)
    wrong = sum(r["verdict"] == "wrong" for r in rows)
    if len(rows) >= MIN_REVIEWED and wrong / len(rows) >= STOP_RATE:
        return f"{wrong} of {len(rows)} reviewed sentences are wrong ({100 * wrong / len(rows):.0f}%)"
    if not set(pool(lang)) - taken(lang):
        return f"the pool is spent ({len(rows)} reviewed)"
    return None


def good_controls(lang: str, verbalizer: Verbalizer, rng: random.Random) -> list[dict]:
    """Readings known to be right: sentences a person passed, else independent gold cases Verbalis passes."""
    human = [r for r in jsonl(review_path(lang)) if r["by"] == "human" and r["verdict"] == "ok"]
    found = [{"input": r["input"], "output": r["output"]} for r in human if verbalizer.process(r["input"]) == r["output"]]
    if len(found) < CONTROLS:
        for case in jsonl(ROOT / lang / "gold.jsonl"):
            if case["source"] != "verbalis" and "out_of_scope" not in case and any(c.isdigit() for c in case["input"]):
                output = verbalizer.process(case["input"])
                if output in case["outputs"] and output != case["input"]:
                    found.append({"input": case["input"], "output": output})
    return rng.sample(found, min(CONTROLS, len(found)))


def plant(lang: str, source: str, verbalizer: Verbalizer, rng: random.Random) -> str | None:
    """A wrong reading of source: a grammatical form swapped, or a digit changed before reading."""
    output = verbalizer.process(source)
    swaps = [(right, wrong) for right, wrong in FORM_SWAPS.get(lang, {}).items()
             if re.search(rf"(?<!\w){re.escape(right)}(?!\w)", output)]
    if swaps and rng.random() < 0.5:
        right, wrong = rng.choice(swaps)
        return re.sub(rf"(?<!\w){re.escape(right)}(?!\w)", wrong, output, count=1)
    digits = [m.start() for m in re.finditer(r"[0-9]", source)]
    if not digits:
        return None
    i = rng.choice(digits)
    changed = verbalizer.process(source[:i] + str((int(source[i]) + rng.randint(1, 8)) % 10) + source[i + 1:])
    return changed if changed != output else None


def write_batch(lang: str, name: str, items: list[dict], verbalizer: Verbalizer, rng: random.Random, pool_inputs):
    """Mixes the controls into items, shuffles, and writes the agent's file and the hidden key."""
    items += [{**c, "kind": "good"} for c in good_controls(lang, verbalizer, rng)]
    spare = [s for s in pool_inputs if s not in {i["input"] for i in items}]
    rng.shuffle(spare)
    planted = 0
    for source in spare:
        if planted == CONTROLS:
            break
        if wrong := plant(lang, source, verbalizer, rng):
            items.append({"input": source, "output": wrong, "kind": "planted"})
            planted += 1
    rng.shuffle(items)
    for n, item in enumerate(items, 1):
        item["n"] = n
    write_jsonl(WORK / lang / f"{name}.jsonl",
                [{"n": i["n"], "input": i["input"], "output": i["output"], "changes": changes(i["input"], i["output"])}
                 for i in items])
    write_jsonl(WORK / lang / f"{name}.key.jsonl", items)
    print(WORK / lang / f"{name}.jsonl")


def cmd_next(lang: str):
    if reason := stop_reason(lang):
        print(f"STOP {lang}: {reason}")
        return
    verbalizer, rng = Verbalizer(lang), random.Random(f"{lang}-{len(batches(lang))}")
    inputs = pool(lang)
    fresh = [s for s in inputs if s not in taken(lang)][:BATCH - 2 * CONTROLS]
    items = [{"input": s, "output": verbalizer.process(s), "kind": "pool"} for s in fresh]
    number = 1 + sum(1 for p in batches(lang) if p.name[0].isdigit())
    write_batch(lang, f"{number:03}", items, verbalizer, rng, inputs)


def cmd_confirm(lang: str):
    """The first-pass "wrong" sentences no confirm batch holds yet, without the first verdict."""
    verbalizer, rng = Verbalizer(lang), random.Random(f"{lang}-confirm-{len(batches(lang))}")
    held = {r["input"] for p in batches(lang) if p.name.startswith("c") for r in jsonl(p)}
    flagged = [r for r in first_pass(lang) if r["verdict"] == "wrong" and r["input"] not in held]
    items = [{"input": r["input"], "output": r["output"], "kind": "pool"} for r in flagged[:BATCH - 2 * CONTROLS]]
    if not items:
        print(f"nothing to confirm in {lang}")
        return
    number = 1 + sum(1 for p in batches(lang) if p.name.startswith("c"))
    write_batch(lang, f"c{number:02}", items, verbalizer, rng, pool(lang))


def rule_ids(lang: str) -> set[str]:
    text = (REPO / ".claude/agents" / f"review-{lang}.md").read_text(encoding="utf-8")
    return set(re.findall(r"\b[A-Z]{2}-[A-Z]+-\d+\b", text))


def repaired(line: str) -> str:
    """Escapes the quotes agents leave inside the free text of "why"."""
    try:
        json.loads(line)
        return line
    except json.JSONDecodeError:
        return re.sub(r'("(?:span|correction|why)": ")(.*?)(?="(?:, "\w+": |\}\s*[,\]]))',
                      lambda m: m[1] + m[2].replace('\\"', '"').replace('"', '\\"'), line)


def cmd_ingest(lang: str, name: str):
    name = name if name.startswith("c") else f"{int(name):03}"
    key = {r["n"]: r for r in jsonl(WORK / lang / f"{name}.key.jsonl")}
    out_path = WORK / lang / f"{name}.out.jsonl"
    rules, problems, verdicts = rule_ids(lang), [], {}
    lines, pending = [], ""
    for line in out_path.read_text(encoding="utf-8").splitlines():
        pending += line.strip()  # Agents sometimes break one object over several lines
        if pending.startswith("{") and not pending.endswith("}"):
            continue
        lines.append(pending)
        pending = ""
    for line in lines + [pending]:
        if not line.strip():
            continue
        try:
            row = json.loads(repaired(line))
        except json.JSONDecodeError:
            problems.append(f"not JSON: {line[:80]}")
            continue
        n, item = row.get("n"), key.get(row.get("n"))
        if item is None or row.get("verdict") not in VERDICTS:
            problems.append(f"bad n or verdict: {line[:80]}")
            continue
        errors = row.get("errors") or []
        for error in errors:
            if error.get("span", "") not in item["output"]:
                problems.append(f"{n}: span not in the output: {error.get('span')!r}")
            if error.get("category") not in CATEGORIES:
                problems.append(f"{n}: unknown category {error.get('category')!r}")
            if row["verdict"] == "wrong" and error.get("rule") not in rules:
                problems.append(f"{n}: unknown rule {error.get('rule')!r}")
        if row["verdict"] == "wrong" and not errors:
            problems.append(f"{n}: wrong without errors")
        verdicts[n] = {"verdict": row["verdict"], "errors": errors}
    missing = sorted(set(key) - set(verdicts))
    print(f"{lang} {name}: {len(verdicts)} verdicts, {len(missing)} missing, {len(problems)} problems")
    for problem in problems[:20]:
        print("  " + problem)
    if len(missing) + len(problems) > BATCH // 10:
        print("REJECTED: more than 10% of the batch is missing or malformed; run the batch again with a fresh agent")
        return

    by_kind = defaultdict(Counter)
    for n, item in key.items():
        by_kind[item["kind"]][verdicts.get(n, {}).get("verdict", "missing")] += 1
    print(f"  planted errors caught: {by_kind['planted']['wrong']} of {sum(by_kind['planted'].values())}; "
          f"good readings flagged: {by_kind['good']['wrong']} of {sum(by_kind['good'].values())}")
    batch = f"{lang}-{name}"
    write_jsonl(review_path(lang), [r for r in jsonl(review_path(lang)) if r["batch"] != batch])
    write_jsonl(review_path(lang), [
        {"lang": lang, "input": item["input"], "output": item["output"], "by": "haiku",
         "pass": 2 if name.startswith("c") else 1, "batch": batch, **verdicts[n]}
        for n, item in sorted(key.items()) if item["kind"] == "pool" and n in verdicts], mode="a")
    write_jsonl(WORK / lang / f"{name}.controls.jsonl",
                [{"kind": item["kind"], **verdicts.get(n, {"verdict": "missing"})} for n, item in key.items()
                 if item["kind"] != "pool"])
    rows = first_pass(lang)
    wrong = sum(r["verdict"] == "wrong" for r in rows)
    print(f"  {lang} so far: {wrong} wrong of {len(rows)} ({100 * wrong / max(len(rows), 1):.0f}%)")
    if reason := stop_reason(lang):
        print(f"STOP {lang}: {reason}")


def control_scores(lang: str) -> tuple[Counter, Counter]:
    planted, good = Counter(), Counter()
    for path in (WORK / lang).glob("*.controls.jsonl"):
        for row in jsonl(path):
            (planted if row["kind"] == "planted" else good)[row["verdict"]] += 1
    return planted, good


def confirmed(lang: str) -> list[dict]:
    second = {r["input"]: r for r in jsonl(review_path(lang)) if r["by"] == "haiku" and r["pass"] == 2}
    return [{**r, "second": second[r["input"]]} for r in first_pass(lang)
            if r["verdict"] == "wrong" and second.get(r["input"], {}).get("verdict") == "wrong"]


def cmd_status():
    print(f"{'lang':7}{'pool':>6}{'reviewed':>10}{'wrong':>8}{'unsure':>8}{'confirmed':>11}"
          f"{'plants caught':>15}{'good flagged':>14}  state")
    for lang in LANGS:
        rows = first_pass(lang)
        verdicts = Counter(r["verdict"] for r in rows)
        planted, good = control_scores(lang)
        print(f"{lang:7}{len(pool(lang)):>6}{len(rows):>10}{verdicts['wrong']:>8}{verdicts['unsure']:>8}"
              f"{len(confirmed(lang)):>11}{planted['wrong']:>9} of {sum(planted.values()):<3}"
              f"{good['wrong']:>7} of {sum(good.values()):<3}  {stop_reason(lang) or 'open'}")


def cmd_report():
    lines = ["# Agent review: confirmed errors by category", ""]
    for lang in LANGS:
        rows = confirmed(lang)
        if not rows:
            continue
        lines += [f"## {lang}", ""]
        by_category = defaultdict(list)
        for row in rows:
            for error in row["errors"]:
                by_category[(error["category"], error["rule"])].append((row, error))
        for (category, rule), hits in sorted(by_category.items(), key=lambda kv: -len(kv[1])):
            lines.append(f"### {category} ({rule}): {len(hits)}")
            for row, error in hits[:8]:
                lines += [f"- `{row['input']}`", f"  - `{error['span']}` → `{error.get('correction', '')}`: "
                                                  f"{error.get('why', '')}"]
            lines.append("")
    (WORK / "report.md").write_text("\n".join(lines), encoding="utf-8")
    print(WORK / "report.md")


def cmd_import_human(path: Path):
    """A clicker export: review pages give "verdict" and "correct"; A/B pages give "choice" between two readings."""
    lang = path.name.split("-review-")[0].split("-ab-")[0]
    rows = []
    for item in json.loads(path.read_text(encoding="utf-8")):
        if "choice" in item:
            if item["choice"] is None:
                continue
            verdict = "ok" if item["choice"] in ("verbalis", "both") else "wrong"
        elif item.get("verdict") in VERDICTS:
            verdict = item["verdict"]
        else:
            continue
        row = {"lang": lang, "input": item["input"], "output": item["verbalis"], "by": "human", "pass": 1,
               "batch": path.stem, "verdict": verdict, "errors": []}
        if item.get("correct"):
            row["correction"] = item["correct"]
        rows.append(row)
    kept = [r for r in jsonl(review_path(lang)) if r["batch"] != path.stem]
    write_jsonl(review_path(lang), kept + rows)
    print(f"{lang}: {len(rows)} human verdicts from {path.name}")


parser = argparse.ArgumentParser()
parser.add_argument("command", choices=["next", "ingest", "confirm", "status", "report", "import-human"])
parser.add_argument("args", nargs="*")
args = parser.parse_args()
if args.command == "next":
    cmd_next(args.args[0])
elif args.command == "ingest":
    cmd_ingest(*args.args[:2])
elif args.command == "confirm":
    cmd_confirm(args.args[0])
elif args.command == "status":
    cmd_status()
elif args.command == "report":
    cmd_report()
else:
    cmd_import_human(Path(args.args[0]).expanduser())
sys.exit(0)
