# Language tests: gold readings and reviews

One folder per language: `gold.jsonl` (the gold set), `notes.md` (its edits and unreachable cases), and
`review.jsonl` (verdicts on Verbalis's output for real sentences, by people and by review agents).

## Gold set: inputs with readings written by people

Unlike the parity baseline (`tests/parity/`), which records what the C++ core does, every reading here was written by
a person: the gold set measures whether Verbalis reads text right, not whether it changed. 3,197 cases in de, en-us,
es, fr, hi, it, pt, and zh from NeMo and WeTextProcessing, and cases written for Verbalis (`"source": "verbalis"`),
the only ones in pl. `<lang>/notes.md` records edited readings and unreachable cases.

### Files

`<lang>/gold.jsonl`, one case per line; every reading in `outputs` is acceptable:

```json
{"lang": "en-us", "source": "nemo", "class": "normalize_with_audio", "input": "$123", "outputs": ["one hundred twenty three dollars", "one hundred and twenty three dollars"]}
```

Cases with `"source": "verbalis"` were written for Verbalis by the same model that wrote its readings, so a mistake
can sit in both and pass ("mieszkania" for "mieszkanie" did). `score.py` reports them apart from the independent
NeMo and WeTextProcessing cases ("own" and "independent"); count them as evidence only once a person has checked
them (`review/`).

A case with an `"out_of_scope"` reason (markup, malformed input, gold that contradicts itself, inputs gold leaves as
written) is left out of the score; `score.py` counts these apart ("excluded"), and `notes/<lang>.md` lists them.

`scores.json` lists the cases Verbalis passes. `score.py` reports accuracy per language (and `--classes`), shows the
failing cases (`--show <lang>`), and exits 1 if a recorded case no longer passes: a change may fix gold cases but not
break them (`tests/python/test_gold.py`). After a change that fixes cases, record them with `--write`.

```bash
.venv/bin/python tests/lang/score.py --classes
```

A case passes when the output equals one of its readings, ignoring case, whitespace, and punctuation: "twenty-one"
and "twenty one", "einhundertdreiundzwanzig" and "ein hundert drei und zwanzig" are the same speech. The cases are
short fragments, often without a sentence around them, so some are ambiguous: Verbalis leaves a German `11.` as
written unless a noun follows, where the gold reading is "elfter".

### Sources

Built by `build.py` from two checkouts, both Apache-2.0 (`licenses/`):

- [NeMo text processing](https://github.com/NVIDIA/NeMo-text-processing) (NVIDIA), commit `ddadfb2`:
  `tests/nemo_text_processing/<lang>/data_text_normalization/test_cases_<class>.txt`, every class but
  `punctuation_match_input`. The German files are written spoken~written; `build.py` swaps them.
  `normalize_with_audio` gives several acceptable readings per input.
- [WeTextProcessing](https://github.com/wenet-e2e/WeTextProcessing) (WeNet), commit `039007e`:
  `tn/chinese/test/data/` and `tn/english/test/data/`, the classes of numbers, dates, money, time, measures, math,
  and sports (not its own preprocessing, such as filler-word removal).

```bash
git clone --depth 1 https://github.com/NVIDIA/NeMo-text-processing.git /tmp/nemo
git clone --depth 1 https://github.com/wenet-e2e/WeTextProcessing.git /tmp/wetext
python3 tests/lang/build.py /tmp/nemo /tmp/wetext
```

## Reviews: verdicts on real sentences

`review.py` runs the agent review: it cuts the sentences of `tests/parity/baseline/` into batches of 100 for the
`review-<lang>` agents (`.claude/agents/`), hides 10 controls in each (5 known-good readings, 5 planted errors), checks
the agents' answers, and stops a language at 20% wrong after 200 sentences. Batches and answers live in
`build/review/<lang>/`; the docstring of `review.py` lists the commands.

`<lang>/review.jsonl` holds one verdict per line, for the output it judged (a later change of Verbalis makes it stale):

```json
{"lang": "pl", "input": "Mam 2 koty.", "output": "Mam dwie koty.", "by": "haiku", "pass": 1, "batch": "pl-003", "verdict": "wrong", "errors": [{"span": "dwie koty", "correction": "dwa koty", "category": "agreement", "rule": "PL-LICZ-1", "why": "kot jest rodzaju męskiego"}]}
```

`by` is `haiku` or `human` (clicker exports, `review.py import-human`, which may add a whole corrected sentence as
`correction`). `pass` 2 is the blind second pass that confirms a first-pass `wrong`. Reviewed sentences are not gold:
`score.py` doesn't count them. `hi/review.jsonl` and `zh/review.jsonl` hold sentences from `tests/parity/local/` and
stay local.

## Corpus: expected readings of real sentences

**Decision (the user, 2026-10-29):** the corpus is the target Verbalis must reach, and it is frozen: its files never
change (tag `corpus-v1`, checksums in `corpus.sha256`, checked by `tests/python/test_gold.py`; `review.py` refuses to
write them). `score.py` adds its passing sentences to the ratchet in `scores.json`: a change may make more of them pass,
never fewer. A new corpus needs the user's decision.

`<lang>/corpus.jsonl` holds the expected reading of each reviewed sentence, so a change can be scored without a new
review. `score.py` reports it after the gold table, and `score.py --corpus <lang>` lists the sentences Verbalis
reads differently.

```json
{"lang": "pl", "input": "Mam 2 koty.", "outputs": ["Mam dwa koty."], "by": "haiku", "how": "written"}
```

- `by`: `human` or `haiku`; a person's verdict wins over an agent's.
- `how`: `approved` (a reviewer passed Verbalis's output as it stood) or `written` (a reviewer wrote the reading).

`review.py seed` builds it from `review.jsonl`: a person's verdict or correction, else an output the first agent
passed. The rest goes to the `expect` agent (`.claude/agents/expect.md`, with the rules of `review-<lang>.md`), which
writes whole sentences (`review.py expect`, `review.py ingest-expect`). When a change moves an output away from a
reading an agent approved, `review.py expect` sends that sentence back to the agent with both readings.

The corpus is not gold: one agent's "ok" misses errors (it passed "B and B" in Italian and undecoded HTML entities).
`score.py` reports it and never fails on it. `hi/corpus.jsonl` and `zh/corpus.jsonl` stay local.
