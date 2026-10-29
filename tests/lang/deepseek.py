"""Runs the blind expect task with DeepSeek, through opencode, as an opinion from a model of another family.

    .venv/bin/python tests/lang/deepseek.py zh               # every unanswered batch of a language
    .venv/bin/python tests/lang/deepseek.py --all            # every language
    .venv/bin/python tests/lang/deepseek.py zh --limit 1     # one batch, to try the setup
    .venv/bin/python tests/lang/deepseek.py --disputes --model opencode/gpt-6-luna de   # a third family on disputes

For each language it writes the batches (review.py deepseek: every reviewed sentence, with hidden controls), sends
each batch with the expect task and the language's rules to the model, keeps the JSON lines of the answer, and records
them (review.py ingest-deepseek) in build/review/<lang>/deepseek.jsonl. A batch that fails or is rejected runs again,
up to three times. Sixteen batches run at a time (--jobs).

The model runs in opencode's read-only plan agent: it gets the files attached and answers in text, with no tools.
Each call runs its own server (--standalone): parallel calls through the shared background service can hang.
OPENCODE overrides the opencode binary (default ~/.opencode/bin/opencode, which must be 2.x).
"""

import argparse
import os
import re
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
WORK = REPO / "build" / "review"
LANGS = ["de", "en-us", "es", "fr", "hi", "it", "pl", "pt", "zh"]
OPENCODE = os.environ.get("OPENCODE", str(Path.home() / ".opencode/bin/opencode"))
PROMPT = ("The first attached file is your task, the second the rules of the language, the third the batch. Do the "
          "task for every sentence of the batch, but use no tools: print the JSON lines of the answer file (one per "
          "sentence, in the format of the task) and nothing else.")
JUDGE_PROMPT = ("The first attached file is your task, the second the rules of the language, the third the batch. Do "
                "the task for every sentence of the batch, but use no tools: print the JSON lines of your answers (one "
                "per sentence, in the format of the task) and nothing else.")
ANSWER = re.compile(r'^\{"n":')
# opencode's errors about the account, not the batch; only its Error line, as answers may hold these words
ACCOUNT_PROBLEM = re.compile(r"Error:.*(expired|funds|Global regions|usage limit)")
STOP = threading.Event()


def review(*args: str) -> str:
    result = subprocess.run([sys.executable, str(REPO / "tests/lang/review.py"), *args], cwd=REPO,
                            capture_output=True, text=True)
    return result.stdout + result.stderr


def run_batch(lang: str, batch: Path, model: str, judge: bool = False) -> str:
    """Asks the model, writes the answer file, and records it; returns the ingest report."""
    out = batch.with_name(batch.stem + ".out.jsonl")
    report = ""
    for attempt in range(1, 4):
        if STOP.is_set():
            return f"{lang} {batch.stem}: no answer (the run stopped)"
        try:
            result = subprocess.run(
                [OPENCODE, "run", "--standalone", "-m", model, "-f", f".claude/agents/{'judge' if judge else 'expect'}.md", "-f",
                 f".claude/agents/review-{lang}.md", "-f", str(batch.relative_to(REPO)), "--", JUDGE_PROMPT if judge else PROMPT],
                cwd=REPO, capture_output=True, text=True, timeout=2400)
        except subprocess.TimeoutExpired:
            print(f"{lang} {batch.stem}: timed out (attempt {attempt})", flush=True)
            continue
        lines = [line.strip() for line in result.stdout.splitlines() if ANSWER.match(line.strip())]
        said = (result.stdout + result.stderr).strip()
        if ACCOUNT_PROBLEM.search(said):
            STOP.set()  # Retrying won't help, nor will the other batches
            report = f"{lang} {batch.stem}: stopped: {said[-200:]}"
            print(report, flush=True)
            return report
        if not lines:
            report = f"{lang} {batch.stem}: no answer (attempt {attempt}): {said[-200:]}"
            print(report, flush=True)
            continue
        out.write_text("\n".join(lines) + "\n", encoding="utf-8")
        report = review("ingest-judge", lang, batch.stem, model) if judge else review("ingest-deepseek", lang,
                                                                                          batch.stem, model)
        print(report.strip(), flush=True)
        if "REJECTED" not in report:
            return report
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("langs", nargs="*")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--limit", type=int, default=0, help="at most this many batches per language")
    parser.add_argument("--model", default="opencode/deepseek-v4.1-flash")
    parser.add_argument("--jobs", type=int, default=16)
    parser.add_argument("--disputes", action="store_true", help="only the disputed sentences (t batches, third.jsonl)")
    parser.add_argument("--judge", action="store_true", help="pick among the disputed readings (j batches, judge.md)")
    args = parser.parse_args()

    jobs = []
    for lang in LANGS if args.all else args.langs:
        prefix = "j" if args.judge else "t" if args.disputes else "d"
        review("judge", lang) if args.judge else review("deepseek", lang, prefix)
        batches = [b for b in sorted((WORK / lang).glob(f"{prefix}[0-9][0-9][0-9].jsonl"))
                   if not b.with_name(b.stem + ".controls.jsonl").exists()]
        jobs += [(lang, b) for b in batches[:args.limit or None]]
    print(f"{len(jobs)} batches with {args.model}, {args.jobs} at a time", flush=True)
    with ThreadPoolExecutor(args.jobs) as pool:
        reports = list(pool.map(lambda job: run_batch(*job, args.model, args.judge), jobs))
    failed = [r for r in reports if "REJECTED" in r or "no answer" in r or "stopped" in r]
    print(f"done: {len(jobs) - len(failed)} recorded, {len(failed)} failed")


main()
