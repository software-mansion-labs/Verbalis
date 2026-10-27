# Gold set: inputs with readings written by people

Unlike the parity baseline (`tests/parity/`), which records what the C++ core does, every reading here was written by
a person: the gold set measures whether Verbalis reads text right, not whether it changed. 3,197 cases in de, en-us,
es, fr, hi, it, pt, and zh from NeMo and WeTextProcessing, and cases written for Verbalis (`"source": "verbalis"`),
the only ones in pl. `notes/<lang>.md` records edited readings and unreachable cases.

## Files

`<lang>.jsonl`, one case per line; every reading in `outputs` is acceptable:

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
.venv/bin/python tests/gold/score.py --classes
```

A case passes when the output equals one of its readings, ignoring case, whitespace, and punctuation: "twenty-one"
and "twenty one", "einhundertdreiundzwanzig" and "ein hundert drei und zwanzig" are the same speech. The cases are
short fragments, often without a sentence around them, so some are ambiguous: Verbalis leaves a German `11.` as
written unless a noun follows, where the gold reading is "elfter".

## Sources

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
python3 tests/gold/build.py /tmp/nemo /tmp/wetext
```
