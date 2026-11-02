# Language tests: the corpus and the gold set

One folder per language: `corpus.jsonl` (the target), `gold.jsonl` (NeMo and WeTextProcessing cases, not in pl), and
`notes.md` (edits and unreachable gold cases). These two sets feed every test: `tests/parity/regen.py` writes the
parity outputs from their inputs, and `score.py` scores Verbalis against their readings.

## Corpus: the target

**Decision (the user, 2026-10-29):** the corpus is the target Verbalis must reach, and it is frozen: its files never
change between versions (checksums in `corpus.sha256`, checked by `tests/python/test_lang.py`). A new version needs
the user's decision. `corpus-v1` (2026-10-29) is the reviewed sentences; `corpus-v2` (2026-11-02) adds the readings
the user accepted and 900 plain sentences.

`<lang>/corpus.jsonl` holds 20,672 real sentences from the Leipzig Corpora Collection (news and web; check its terms
before the repo goes public, `knowledge/test-data.md`): 19,772 with digits or symbols and 900 plain ones (100 per
language, `"how": "plain"`), each with its expected readings:

```json
{"lang": "pl", "input": "Mam 2 koty.", "outputs": ["Mam dwa koty."], "by": "haiku", "how": "written"}
```

- `by`: who settled the reading: `human` (the user's verdicts, for Polish), a model (`haiku`, `deepseek`,
  `gemini-3.8-flash`). A person's verdict wins.
- `how`: `approved` (a reviewer passed Verbalis's output as it stood), `written` (a reviewer wrote the reading), or
  `plain` (a sentence with no digits or symbols, from the same downloads; its reading is the input, but for 7 with an
  abbreviation or a Roman numeral, whose readings the user checked in Polish).
- `accepted`: the user accepted Verbalis's reading, now first in `outputs`, beside the reviewers' ones (the
  decision's date and commit; corpus-v2 merged 101 of them).
- `deepseek: "agrees"`: DeepSeek wrote the same reading, blind; `third`, `preferred`, `judged`: how a dispute between
  models was settled (a third family, the user's choice of model, or a judge).

How it was built (2026-10-28): Haiku agents reviewed every sentence and wrote readings for the ones they flagged; a
blind second Haiku pass, DeepSeek (every sentence), GPT-6 Luna (disputes), and Gemini 3.8 Flash (the last disputes, as
a judge) cross-checked them; the user settled 257 Polish sentences. The scripts and agent files are at tag `corpus-v1`
(`tools/review/`, `.claude/agents/`).

## Accepted readings

`accepted.jsonl` holds, one line per sentence, a reading the user accepted where it differs from the corpus's:
`{"lang", "input", "output", "by", "date", "commit", "decision"}`. The corpus stays frozen; `score.py` passes a
sentence that matches either, and the ratchet guards the accepted readings like any other. Each line comes from a
decision recorded in its commit (an expanded abbreviation, a score read with "a", a short year read in full). The
next corpus version merges them, as corpus-v2 did; it is empty after each merge.

## Gold set: NeMo and WeTextProcessing cases

`<lang>/gold.jsonl` holds 3,197 short cases written by those projects, each with its acceptable readings:

```json
{"lang": "en-us", "source": "nemo", "class": "normalize_with_audio", "input": "$123", "outputs": ["one hundred twenty three dollars", "one hundred and twenty three dollars"]}
```

A case with an `"out_of_scope"` reason (markup, malformed input, gold that contradicts itself, inputs gold leaves as
written) is left out of the score; `<lang>/notes.md` lists them. The cases are short fragments, often without a
sentence around them, so some are ambiguous: Verbalis leaves a German `11.` as written unless a noun follows, where
the gold reading is "elfter".

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

## Scoring and the ratchet

A case passes when the output equals one of its readings, ignoring case, whitespace, and punctuation: "twenty-one"
and "twenty one" are the same speech. `scores.json` lists every passing gold case and corpus sentence (the latter by a
hash of the input). `score.py` exits 1 if one no longer passes: a change may fix cases, never break them
(`tests/python/test_lang.py`). After a change that fixes cases, record them with `--write`.

```bash
.venv/bin/python tests/lang/score.py              # both tables, and changes since scores.json
.venv/bin/python tests/lang/score.py --corpus pl  # every corpus sentence Verbalis still misreads
.venv/bin/python tests/lang/score.py --show de    # every failing gold case
```
