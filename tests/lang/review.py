"""Agent review of Verbalis on real sentences: batches for the review-<lang> agents, their verdicts, and the report.

    .venv/bin/python tests/lang/review.py next pl            # write the next batch of 100, or print STOP
    .venv/bin/python tests/lang/review.py ingest pl 3        # check build/review/pl/003.out.jsonl, record it
    .venv/bin/python tests/lang/review.py confirm pl         # a blind second-pass batch of the sentences flagged wrong
    .venv/bin/python tests/lang/review.py status             # progress and reviewer accuracy per language
    .venv/bin/python tests/lang/review.py report             # confirmed errors by category: build/review/report.md
    .venv/bin/python tests/lang/review.py import-human ~/Downloads/pl-review-results.json
    .venv/bin/python tests/lang/review.py seed               # expected readings from the verdicts: <lang>/corpus.jsonl
    .venv/bin/python tests/lang/review.py expect pl          # a batch of unsettled sentences for the expect agent
    .venv/bin/python tests/lang/review.py ingest-expect pl 3 # check build/review/pl/e03.out.jsonl, record it
    .venv/bin/python tests/lang/review.py recheck pl         # a blind second expect batch of the written readings
    .venv/bin/python tests/lang/review.py ingest-expect pl f1 # keep the readings both agents wrote; the rest go to
                                                             # build/review/pl/disagreements.jsonl
    .venv/bin/python tests/lang/review.py deepseek pl        # batches of every reviewed sentence for DeepSeek
    .venv/bin/python tests/lang/review.py ingest-deepseek pl 3  # record build/review/pl/d003.out.jsonl
    .venv/bin/python tests/lang/review.py cross              # mark corpus readings DeepSeek agrees with or disputes
    .venv/bin/python tests/lang/review.py dispute-page pl     # build/review/pl-disputes.html: a person picks the readings
    .venv/bin/python tests/lang/review.py prefer pl          # settle the remaining disputes with DeepSeek's reading
    .venv/bin/python tests/lang/review.py judge de           # batches of the disputes with their readings, for a judge
    .venv/bin/python tests/lang/review.py ingest-judge de j001 MODEL  # settle them with the judge's picks

The pool of a language is the sentences of tests/parity/baseline (hi and zh: tests/parity/local, a sample), read by the
installed package. A batch holds 90 pool sentences and 10 hidden controls: 5 known-good readings, which the reviewer
must pass, and 5 planted errors, which it must catch. Their score says how far to trust the verdicts. A language stops
once it has 200 reviewed sentences and 20% or more are wrong, or once its pool is spent.

Verdicts go to tests/lang/<lang>/review.jsonl, one line per sentence and pass (the format is in tests/lang/README.md).
A "wrong" verdict counts as confirmed when a second, blind pass by a fresh agent flags the same sentence.

The verdicts settle the expected reading of a sentence, in tests/lang/<lang>/corpus.jsonl: a person's verdict or
correction, else an output the first agent passed. The rest (flagged, unsure) goes to the expect agent
(.claude/agents/expect.md), which writes the whole expected sentence, with the same hidden controls.
"""

import argparse
import difflib
import hashlib
import json
import random
import re
import sys
import unicodedata
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


def corpus_path(lang: str) -> Path:
    return ROOT / lang / "corpus.jsonl"


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
                [{"n": i["n"], "input": i["input"], "output": i["output"], "changes": changes(i["input"], i["output"]),
                  **({"notes": i["notes"]} if i.get("notes") else {})} for i in items])
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


def answers(path: Path, problems: list[str]) -> list[tuple[str, dict]]:
    """The JSON lines of an agent's answer file, each with its text; what doesn't parse goes to problems."""
    lines, pending, rows = [], "", []
    for line in path.read_text(encoding="utf-8").splitlines():
        pending += line.strip()  # Agents sometimes break one object over several lines
        if pending.startswith("{") and not pending.endswith("}"):
            continue
        lines.append(pending)
        pending = ""
    for line in lines + [pending]:
        if not line.strip():
            continue
        try:
            rows.append((line, json.loads(repaired(line))))
        except json.JSONDecodeError:
            try:  # A stray closing brace: {"n": 22, "outputs": []}}
                rows.append((line, json.loads(repaired(line.rstrip("}") + "}"))))
            except json.JSONDecodeError:
                # Readings with unescaped quotes inside: {"n": 6, "outputs": ["他说"好"."]}
                if m := re.fullmatch(r'\{"n": (\d+), "outputs": \["(.*)"\]\]?\}+', line):
                    rows.append((line, {"n": int(m[1]), "outputs": m[2].split('", "')}))
                else:
                    problems.append(f"not JSON: {line[:80]}")
    return rows


def cmd_ingest(lang: str, name: str):
    name = name if name.startswith("c") else f"{int(name):03}"
    key = {r["n"]: r for r in jsonl(WORK / lang / f"{name}.key.jsonl")}
    rules, problems, verdicts = rule_ids(lang), [], {}
    for line, row in answers(WORK / lang / f"{name}.out.jsonl", problems):
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
    lang = path.name.split("-review-")[0].split("-ab-")[0].split("-disputes-")[0]
    rows = []
    for item in json.loads(path.read_text(encoding="utf-8")):
        if "accepted" in item:  # The dispute page: every right reading
            if not item["accepted"]:
                continue
            ok = item["verbalis"] in item["accepted"]
            row = {"lang": lang, "input": item["input"], "output": item["verbalis"], "by": "human", "pass": 1,
                   "batch": path.stem, "verdict": "ok" if ok else "wrong", "errors": [], "corrections": item["accepted"]}
            rows.append(row)
            continue
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


def settled(lang: str) -> dict[str, dict]:
    """The expected reading of each sentence the verdicts settle: a person's, else an output the first agent passed."""
    rows = {}
    for r in jsonl(review_path(lang)):
        if r["by"] == "haiku" and r["pass"] == 1 and r["verdict"] == "ok" and r["input"] not in rows:
            rows[r["input"]] = {"outputs": [r["output"]], "by": "haiku", "how": "approved"}
    for r in jsonl(review_path(lang)):  # A person's verdict wins
        # Picking Verbalis's reading over another on an A/B page says it is the better one, not that it is right
        if r["by"] != "human" or (r["verdict"] == "ok" and "-ab-" in r["batch"]):
            continue
        written = r.get("corrections") or ([r["correction"]] if r.get("correction") else [])
        if r["verdict"] == "ok" or written:
            how = "approved" if r["verdict"] == "ok" else "written"
            rows[r["input"]] = {"outputs": written or [r["output"]], "by": "human", "how": how, "batch": r["batch"]}
    return rows


def cmd_seed():
    """Writes <lang>/corpus.jsonl from the verdicts, keeping the readings models wrote (the expect agent, DeepSeek)."""
    for lang in LANGS:
        written = {r["input"]: r for r in jsonl(corpus_path(lang)) if r["by"] != "human" and r["how"] == "written"}
        verdicts = settled(lang)
        # A person's verdict wins over a model's reading, unless the person chose that model for the sentence (prefer)
        people = {s: r for s, r in verdicts.items() if r["by"] == "human" and not written.get(s, {}).get("preferred")}
        rows = {**verdicts, **written, **people}
        reviewed = {r["input"] for r in jsonl(review_path(lang))}
        write_jsonl(corpus_path(lang), [{"lang": lang, "input": s, **rows[s]} for s in sorted(rows, key=stable)])
        print(f"{lang}: {len(rows)} expected readings, {len(reviewed - set(rows))} sentences left for the expect agent")


def expect_batches(lang: str) -> list[Path]:
    return sorted((WORK / lang).glob("e[0-9][0-9].key.jsonl"))


def cmd_expect(lang: str):
    """A batch of sentences without an expected reading, with what the reviewers said about them; and sentences an agent
    passed whose output has changed since, to settle which reading is right."""
    verbalizer = Verbalizer(lang)
    held = {r["input"] for p in expect_batches(lang) for r in jsonl(p)}
    done, notes = set(), defaultdict(list)
    for r in jsonl(corpus_path(lang)):
        output = verbalizer.process(r["input"])
        if r["by"] == "haiku" and r["how"] == "approved" and score_key(output) != score_key(r["outputs"][0]):
            notes[r["input"]].append(f"an earlier reviewer passed: {r['outputs'][0]}")
        else:
            done.add(r["input"])
    for r in jsonl(review_path(lang)):
        for e in r.get("errors") or []:
            notes[r["input"]].append(f"{e.get('span')} → {e.get('correction')} ({e.get('why', '')})")
        if r.get("correction"):
            notes[r["input"]].append(f"→ {r['correction']}")
    todo = [s for s in dict.fromkeys(r["input"] for r in jsonl(review_path(lang))) if s not in done | held]
    todo.sort(key=lambda s: not any(n.startswith("an earlier") for n in notes[s]))
    if not todo:
        print(f"nothing to settle in {lang}")
        return
    rng = random.Random(f"{lang}-expect-{len(expect_batches(lang))}")
    items = [{"input": s, "output": verbalizer.process(s), "kind": "pool", "notes": list(dict.fromkeys(notes[s]))}
             for s in todo[:BATCH - 2 * CONTROLS]]
    write_batch(lang, f"e{1 + len(expect_batches(lang)):02}", items, verbalizer, rng, pool(lang))


def cmd_recheck(lang: str):
    """A blind second expect batch: readings an agent wrote, shown as Verbalis's output without the first answer."""
    held = {r["input"] for p in sorted((WORK / lang).glob("f[0-9][0-9].key.jsonl")) for r in jsonl(p)}
    rows = [r for r in jsonl(corpus_path(lang))
            if r["by"] == "haiku" and r["how"] == "written" and not r.get("checked") and r["input"] not in held]
    if not rows:
        print(f"nothing to recheck in {lang}")
        return
    verbalizer = Verbalizer(lang)
    rng = random.Random(f"{lang}-recheck-{len(held)}")
    items = [{"input": r["input"], "output": verbalizer.process(r["input"]), "kind": "pool"}
             for r in rows[:BATCH - 2 * CONTROLS]]
    number = 1 + len(list((WORK / lang).glob("f[0-9][0-9].key.jsonl")))
    write_batch(lang, f"f{number:02}", items, verbalizer, rng, pool(lang))


def cmd_ingest_expect(lang: str, name: str):
    """Checks the expect agent's readings against the hidden controls, and adds them to <lang>/corpus.jsonl. A second
    pass (f1) keeps the readings both agents wrote; a sentence they disagree on leaves the corpus."""
    second = name.startswith("f")
    name = f"f{int(name[1:]):02}" if second else f"e{int(name):02}"
    key = {r["n"]: r for r in jsonl(WORK / lang / f"{name}.key.jsonl")}
    verbalizer, problems, readings = Verbalizer(lang), [], {}
    for line, row in answers(WORK / lang / f"{name}.out.jsonl", problems):
        n, item = row.get("n"), key.get(row.get("n"))
        outputs = row.get("outputs")
        if item is None or not (isinstance(outputs, list) and all(isinstance(o, str) and o.strip() for o in outputs)):
            problems.append(f"bad n or outputs: {line[:80]}")
            continue
        far = [o for o in outputs if difflib.SequenceMatcher(None, o, item["output"], autojunk=False).ratio() < 0.5]
        if far:
            problems.append(f"{n}: a reading far from the output: {far[0][:80]}")
            continue
        readings[n] = outputs  # Empty: the agent can't tell
    missing = sorted(set(key) - set(readings))
    print(f"{lang} {name}: {len(readings)} answers, {len(missing)} missing, {len(problems)} problems")
    for problem in problems[:20]:
        print("  " + problem)
    if len(missing) + len(problems) > BATCH // 10:
        print("REJECTED: more than 10% of the batch is missing or malformed; run the batch again with a fresh agent")
        return

    # A good control must come back as it is, a planted error as the real reading
    score = defaultdict(Counter)
    for n, item in key.items():
        if item["kind"] == "pool" or n not in readings:
            continue
        right = item["output"] if item["kind"] == "good" else verbalizer.process(item["input"])
        score[item["kind"]][any(score_key(o) == score_key(right) for o in readings[n])] += 1
    print(f"  good readings kept: {score['good'][True]} of {sum(score['good'].values())}; "
          f"planted errors repaired: {score['planted'][True]} of {sum(score['planted'].values())}")
    write_jsonl(WORK / lang / f"{name}.controls.jsonl",
                [{"kind": k, "right": right} for k, c in score.items() for right, m in c.items() for _ in range(m)])
    rows = {r["input"]: r for r in jsonl(corpus_path(lang))}
    agreed = dropped = 0
    for n, item in key.items():
        if item["kind"] != "pool":
            continue
        if second and item["input"] in rows:
            first = rows[item["input"]]["outputs"]
            both = [o for o in first if score_key(o) in {score_key(r) for r in readings.get(n, [])}]
            if both:
                rows[item["input"]] = {**rows[item["input"]], "outputs": both, "checked": True}
                agreed += 1
            else:
                write_jsonl(WORK / lang / "disagreements.jsonl",
                            [{"input": item["input"], "first": first, "second": readings.get(n, [])}], mode="a")
                del rows[item["input"]]
                dropped += 1
        elif not second and readings.get(n):
            rows[item["input"]] = {"lang": lang, "input": item["input"], "outputs": readings[n], "by": "haiku",
                                   "how": "written"}
    if second:
        print(f"  second pass: {agreed} readings agreed, {dropped} dropped")
    write_jsonl(corpus_path(lang), sorted(rows.values(), key=lambda r: stable(r["input"])))
    left = len({r["input"] for r in jsonl(review_path(lang))} - set(rows))
    print(f"  {lang}: {len(rows)} expected readings, {left} left")


# Opinions from models of other families: d batches (deepseek.jsonl) read every reviewed sentence; t batches
# (third.jsonl) read the sentences the corpus and DeepSeek dispute, to break the tie
OPINIONS = {"d": "deepseek", "t": "third"}


def cmd_deepseek(lang: str, prefix: str = "d"):
    """Blind expect batches for a model of another family (tests/lang/deepseek.py): every reviewed sentence (d), or the
    disputed ones (t)."""
    held = {r["input"] for p in (WORK / lang).glob(f"{prefix}[0-9][0-9][0-9].key.jsonl") for r in jsonl(p)}
    if prefix == "t":
        todo = [r["input"] for r in jsonl(corpus_path(lang)) if "disputed" in r and r["input"] not in held]
    else:
        todo = [s for s in dict.fromkeys(r["input"] for r in jsonl(review_path(lang))) if s not in held]
    verbalizer = Verbalizer(lang)
    number = len(list((WORK / lang).glob(f"{prefix}[0-9][0-9][0-9].key.jsonl")))
    for start in range(0, len(todo), BATCH - 2 * CONTROLS):
        number += 1
        items = [{"input": s, "output": verbalizer.process(s), "kind": "pool"}
                 for s in todo[start:start + BATCH - 2 * CONTROLS]]
        write_batch(lang, f"{prefix}{number:03}", items, verbalizer, random.Random(f"{lang}-{OPINIONS[prefix]}-{number}"),
                    pool(lang))


def cmd_ingest_deepseek(lang: str, name: str, model: str = "opencode/deepseek-v4-pro"):
    """Records DeepSeek's readings of a batch in build/review/<lang>/deepseek.jsonl, with its control scores."""
    prefix = name[0] if name[0] in OPINIONS else "d"
    name = f"{prefix}{int(name.lstrip('dt')):03}"
    key = {r["n"]: r for r in jsonl(WORK / lang / f"{name}.key.jsonl")}
    verbalizer, problems, readings = Verbalizer(lang), [], {}
    for line, row in answers(WORK / lang / f"{name}.out.jsonl", problems):
        item, outputs = key.get(row.get("n")), row.get("outputs")
        if item is None or not (isinstance(outputs, list) and all(isinstance(o, str) and o.strip() for o in outputs)):
            problems.append(f"bad n or outputs: {line[:80]}")
            continue
        readings[row["n"]] = outputs
    missing = sorted(set(key) - set(readings))
    score = defaultdict(Counter)
    for n, item in key.items():
        if item["kind"] != "pool" and n in readings:
            right = item["output"] if item["kind"] == "good" else verbalizer.process(item["input"])
            score[item["kind"]][any(score_key(o) == score_key(right) for o in readings[n])] += 1
    print(f"{lang} {name}: {len(readings)} answers, {len(missing)} missing, {len(problems)} problems; "
          f"good kept {score['good'][True]}/{sum(score['good'].values())}, "
          f"planted repaired {score['planted'][True]}/{sum(score['planted'].values())}")
    if len(missing) + len(problems) > BATCH // 10:
        print("REJECTED: more than 10% of the batch is missing or malformed; run it again")
        return
    write_jsonl(WORK / lang / f"{name}.controls.jsonl",
                [{"kind": k, "right": right} for k, c in score.items() for right, m in c.items() for _ in range(m)])
    path = WORK / lang / f"{OPINIONS[prefix]}.jsonl"
    rows = {r["input"]: r for r in jsonl(path)}
    for n, item in key.items():
        if item["kind"] == "pool" and n in readings:
            rows[item["input"]] = {"input": item["input"], "output": item["output"], "outputs": readings[n],
                                   "model": model}
    write_jsonl(path, rows.values())


def cmd_cross():
    """Compares the corpus with DeepSeek's blind readings: a reading DeepSeek also wrote is cross-checked, one it reads
    otherwise is disputed (and left out of the score); a sentence the two expect agents disagreed on is settled where
    DeepSeek wrote one of their readings."""
    for lang in LANGS:
        deepseek = {r["input"]: r["outputs"] for r in jsonl(WORK / lang / "deepseek.jsonl")}
        if not deepseek:
            continue
        rows = {r["input"]: r for r in jsonl(corpus_path(lang))}
        for row in rows.values():
            if row.get("by") == "deepseek" or row.get("preferred"):  # Settled by a third opinion, a judge, or a person
                continue
            row.pop("deepseek", None)
            row.pop("disputed", None)
            row.pop("third", None)
            theirs = deepseek.get(row["input"])
            if theirs is None or "-disputes-" in row.get("batch", ""):  # A person settled the dispute
                continue
            if not theirs:
                row["deepseek"] = "unsure"
            elif {score_key(o) for o in theirs} & {score_key(o) for o in row["outputs"]}:
                row["deepseek"] = "agrees"
            else:
                row["disputed"] = theirs
        third = {r["input"]: {score_key(o) for o in r["outputs"]} for r in jsonl(WORK / lang / "third.jsonl")}
        for row in rows.values():  # A disputed sentence where a third family sides with one reading: 2 of 3
            votes = third.get(row["input"])
            if "disputed" not in row or not votes:
                continue
            if votes & {score_key(o) for o in row["outputs"]}:
                row["third"] = "corpus"
                del row["disputed"]
            elif votes & {score_key(o) for o in row["disputed"]}:
                row.update(outputs=row.pop("disputed"), by="deepseek", how="written", third="deepseek")
        settled = 0
        for d in jsonl(WORK / lang / "disagreements.jsonl"):
            theirs = {score_key(o) for o in deepseek.get(d["input"], [])}
            both = [o for o in d["first"] + d["second"] if score_key(o) in theirs]
            if both and d["input"] not in rows:
                rows[d["input"]] = {"lang": lang, "input": d["input"], "outputs": list(dict.fromkeys(both)),
                                    "by": "haiku", "how": "written", "deepseek": "agrees"}
                settled += 1
        write_jsonl(corpus_path(lang), sorted(rows.values(), key=lambda r: stable(r["input"])))
        counts = Counter("disputed" if "disputed" in r else "chosen" if r.get("preferred") else
                         "third" if "third" in r else r.get("deepseek", "not read") for r in rows.values())
        print(f"{lang}: {counts['agrees']} cross-checked, {counts['third']} settled by a third family, "
              f"{counts['chosen']} by a judge or a person's choice, {counts['disputed']} disputed, "
              f"{counts['unsure']} unsure, {counts['not read']} not read by DeepSeek; {settled} disagreements settled")


DISPUTE_PAGE = """<!doctype html><html><head><meta charset="utf-8"><title>Verbalis: LANG disputes</title><style>
body{font:16px/1.5 -apple-system,system-ui,sans-serif;max-width:900px;margin:2em auto;padding:0 1em;color:#222}
.box{border:1px solid #ddd;border-radius:8px;padding:.8em 1em;margin:.5em 0;background:#fafafa}
label.box{display:flex;gap:.7em;cursor:pointer;background:#fff} label.box.on{border-color:#2a8a3e;background:#eef8f0}
mark{background:#ffe58a;border-radius:3px;padding:0 2px} .meta{color:#777;font-size:.85em}
textarea{width:100%;font:inherit;min-height:3.5em;box-sizing:border-box;padding:.5em}
button{font:inherit;padding:.4em 1em;margin-right:.5em}
</style></head><body><h1>LANG: which readings are right?</h1>
<p class="meta">Tick every right reading (keys 1-3), or write the right one. Enter: next. Answers stay in this browser;
Export saves them for <code>review.py import-human</code>.</p>
<div class="meta" id="count"></div><div class="box" id="input"></div><div id="choices"></div>
<textarea id="own" placeholder="The right reading, if none above is"></textarea>
<p><button onclick="go(-1)">Back</button><button onclick="go(1)">Next</button><button onclick="exportAll()">Export</button></p>
<script>
const items = ITEMS, key = "verbalis-disputes-LANG";
const answers = JSON.parse(localStorage.getItem(key) || "{}");
let i = Math.min(Object.keys(answers).length, items.length - 1);
const esc = s => s.replace(/[&<>]/g, c => ({"&": "&amp;", "<": "&lt;", ">": "&gt;"})[c]);
function marked(text, others) {
  const seen = new Set(others.flatMap(o => o.split(" ")));
  return text.split(" ").map(w => seen.has(w) && others.every(o => o.split(" ").includes(w)) ? esc(w) : "<mark>" + esc(w) + "</mark>").join(" ");
}
function save() {
  const it = items[i], picked = [...document.querySelectorAll("#choices input:checked")].map(c => it.readings[+c.value]);
  const own = document.getElementById("own").value.trim();
  if (picked.length || own) answers[it.input] = {picked, own}; else delete answers[it.input];
  localStorage.setItem(key, JSON.stringify(answers));
}
function show() {
  const it = items[i], a = answers[it.input] || {picked: [], own: ""};
  document.getElementById("count").textContent = (i + 1) + " / " + items.length + " · answered " + Object.keys(answers).length;
  document.getElementById("input").textContent = it.input;
  document.getElementById("choices").innerHTML = it.readings.map((r, n) =>
    `<label class="box${a.picked.includes(r) ? " on" : ""}"><input type="checkbox" value="${n}" ${a.picked.includes(r) ? "checked" : ""}
      onchange="this.parentNode.classList.toggle('on', this.checked); save()"><span><b>${n + 1}</b> ${marked(r, it.readings.filter(o => o !== r))}</span></label>`).join("");
  document.getElementById("own").value = a.own;
}
function go(d) { save(); i = Math.max(0, Math.min(items.length - 1, i + d)); show(); }
function exportAll() {
  save();
  const out = items.filter(it => answers[it.input]).map(it => ({input: it.input, verbalis: it.verbalis,
    accepted: [...answers[it.input].picked, ...(answers[it.input].own ? [answers[it.input].own] : [])]}));
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([JSON.stringify(out, null, 1)], {type: "application/json"}));
  a.download = "LANG-disputes-results.json"; a.click();
}
document.getElementById("own").addEventListener("input", save);
document.addEventListener("keydown", e => {
  if (e.target.tagName === "TEXTAREA") return;
  if (e.key === "Enter") go(1);
  const box = document.querySelectorAll("#choices input")[+e.key - 1];
  if (box) { box.checked = !box.checked; box.onchange(); }
});
show();
</script></body></html>
"""


def cmd_prefer(lang: str):
    """Settles a language's remaining disputes with DeepSeek's reading, where a person found DeepSeek the most right."""
    rows = jsonl(corpus_path(lang))
    for row in rows:
        if "disputed" in row:
            row.update(outputs=row.pop("disputed"), by="deepseek", how="written", preferred=True)
    write_jsonl(corpus_path(lang), rows)
    print(f"{lang}: {sum(bool(r.get('preferred')) for r in rows)} disputes settled with DeepSeek's reading")


def cmd_judge(lang: str):
    """Batches of the disputed sentences with their proposed readings (the corpus, DeepSeek, a third family), shuffled,
    for a judge model (.claude/agents/judge.md); with controls: a right reading among two planted errors."""
    verbalizer = Verbalizer(lang)
    third = {r["input"]: r["outputs"] for r in jsonl(WORK / lang / "third.jsonl")}
    held = {r["input"] for p in (WORK / lang).glob("j[0-9][0-9][0-9].key.jsonl") for r in jsonl(p)}
    todo = [r for r in jsonl(corpus_path(lang)) if "disputed" in r and r["input"] not in held]
    number = len(list((WORK / lang).glob("j[0-9][0-9][0-9].key.jsonl")))
    for start in range(0, len(todo), BATCH - 2 * CONTROLS):
        number += 1
        rng = random.Random(f"{lang}-judge-{number}")
        items = []
        for row in todo[start:start + BATCH - 2 * CONTROLS]:
            readings = list(dict.fromkeys(row["outputs"][:1] + row["disputed"][:1] + third.get(row["input"], [])[:1]))
            items.append({"input": row["input"], "kind": "pool", "readings": readings})
        for control in good_controls(lang, verbalizer, rng):
            wrong = []
            for _ in range(20):
                planted = plant(lang, control["input"], verbalizer, rng)
                if planted and planted != control["output"] and planted not in wrong:
                    wrong.append(planted)
                if len(wrong) == 2:
                    break
            if wrong:
                items.append({"input": control["input"], "kind": "good", "readings": [control["output"], *wrong],
                              "right": control["output"]})
        rng.shuffle(items)
        for n, item in enumerate(items, 1):
            item["n"] = n
            readings = item["readings"][:]
            rng.shuffle(readings)
            item["options"] = dict(zip("ABC", readings))
        name = f"j{number:03}"
        write_jsonl(WORK / lang / f"{name}.jsonl",
                    [{"n": i["n"], "input": i["input"], "options": i["options"]} for i in items])
        write_jsonl(WORK / lang / f"{name}.key.jsonl", items)
        print(WORK / lang / f"{name}.jsonl")


def cmd_ingest_judge(lang: str, name: str, model: str):
    """Settles the disputes of a judge batch with the reading the judge picked or wrote, after checking its controls."""
    key = {r["n"]: r for r in jsonl(WORK / lang / f"{name}.key.jsonl")}
    problems, chosen = [], {}
    for line, row in answers(WORK / lang / f"{name}.out.jsonl", problems):
        item = key.get(row.get("n"))
        if item is None:
            problems.append(f"bad n: {line[:80]}")
        elif row.get("pick") in item["options"]:
            chosen[row["n"]] = ("pick", item["options"][row["pick"]])
        elif isinstance(row.get("write"), str) and row["write"].strip() and max(
                difflib.SequenceMatcher(None, row["write"], o, autojunk=False).ratio() for o in item["options"].values()) >= 0.5:
            chosen[row["n"]] = ("write", row["write"])
        else:
            problems.append(f"{row.get('n')}: no valid pick or reading: {line[:80]}")
    missing = sorted(set(key) - set(chosen))
    controls = [chosen.get(n, ("", ""))[1] == item["right"] for n, item in key.items() if item["kind"] == "good"]
    writes = sum(how == "write" for how, _ in chosen.values())
    print(f"{lang} {name}: {len(chosen)} answers ({writes} written), {len(missing)} missing, {len(problems)} problems; "
          f"controls picked right {sum(controls)}/{len(controls)}")
    for problem in problems[:10]:
        print("  " + problem)
    if len(missing) + len(problems) > BATCH // 10:
        print("REJECTED: more than 10% of the batch is missing or malformed; run it again")
        return
    write_jsonl(WORK / lang / f"{name}.controls.jsonl", [{"kind": "good", "right": c} for c in controls])
    rows = jsonl(corpus_path(lang))
    by_input = {item["input"]: chosen[n] for n, item in key.items() if item["kind"] == "pool" and n in chosen}
    for row in rows:
        if row["input"] in by_input and "disputed" in row:
            how, reading = by_input[row["input"]]
            row.pop("disputed")
            row.update(outputs=[reading], by=model.split("/")[-1], how="written", preferred=True, judged=how)
    write_jsonl(corpus_path(lang), rows)


def cmd_dispute_page(lang: str):
    """A page where a person picks the right readings of the disputed sentences: Verbalis's, the corpus's, DeepSeek's."""
    verbalizer = Verbalizer(lang)
    items = []
    for row in jsonl(corpus_path(lang)):
        if "disputed" in row:
            output = verbalizer.process(row["input"])
            readings = list(dict.fromkeys([output, *row["outputs"], *row["disputed"]]))
            items.append({"input": row["input"], "verbalis": output, "readings": readings})
    path = WORK / f"{lang}-disputes.html"
    path.write_text(DISPUTE_PAGE.replace("LANG", lang).replace("ITEMS", json.dumps(items, ensure_ascii=False)),
                    encoding="utf-8")
    print(f"{path}: {len(items)} sentences")


def score_key(text: str) -> str:
    """Speech, not spelling: case, spaces, and punctuation ignored, as in score.py."""
    text = unicodedata.normalize("NFKC", text).casefold()
    return "".join(c for c in text if not c.isspace() and not unicodedata.category(c).startswith("P"))


parser = argparse.ArgumentParser()
parser.add_argument("command", choices=["next", "ingest", "confirm", "status", "report", "import-human", "seed",
                                        "expect", "ingest-expect", "recheck", "deepseek", "ingest-deepseek",
                                        "cross", "dispute-page", "prefer", "judge", "ingest-judge"])
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
elif args.command == "seed":
    cmd_seed()
elif args.command == "expect":
    cmd_expect(args.args[0])
elif args.command == "judge":
    cmd_judge(args.args[0])
elif args.command == "ingest-judge":
    cmd_ingest_judge(*args.args[:3])
elif args.command == "prefer":
    cmd_prefer(args.args[0])
elif args.command == "dispute-page":
    cmd_dispute_page(args.args[0])
elif args.command == "cross":
    cmd_cross()
elif args.command == "deepseek":
    cmd_deepseek(*args.args[:2])
elif args.command == "ingest-deepseek":
    cmd_ingest_deepseek(*args.args[:3])
elif args.command == "recheck":
    cmd_recheck(args.args[0])
elif args.command == "ingest-expect":
    cmd_ingest_expect(*args.args[:2])
else:
    cmd_import_human(Path(args.args[0]).expanduser())
sys.exit(0)
