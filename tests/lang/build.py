"""Builds the gold test set (tests/lang/<lang>/gold.jsonl) from the hand-written test cases of two text normalization
projects, both Apache-2.0:

- NeMo text processing (https://github.com/NVIDIA/NeMo-text-processing),
  tests/nemo_text_processing/<lang>/data_text_normalization/test_cases_<class>.txt;
- WeTextProcessing (https://github.com/wenet-e2e/WeTextProcessing), tn/<language>/test/data/<class>.txt.

    python3 tests/lang/build.py <NeMo-text-processing checkout> <WeTextProcessing checkout>

A record is {"lang", "source", "class", "input", "outputs"}: every reading in "outputs" is acceptable.
"""

import json
import sys
from collections import defaultdict
from pathlib import Path

NEMO, WETEXT = (Path(p) for p in sys.argv[1:3])
OUT = Path(__file__).parent

LANGUAGES = {"de": "de", "en": "en-us", "es": "es", "fr": "fr", "it": "it", "pt": "pt", "hi": "hi", "zh": "zh"}
SPOKEN_FIRST = {"de"}  # Its files are written spoken~written, the other way round
SKIP_NEMO = {"punctuation_match_input"}  # Tests NeMo's own punctuation option
WETEXT_CLASSES = {
    "chinese": ["cardinal", "date", "fraction", "math", "measure", "money", "number", "sport", "time"],
    "english": ["cardinal", "date", "decimal", "electronic", "fraction", "measure", "money", "ordinal", "range",
                "roman", "telephone", "time"],
}


def nemo_cases(path: Path, spoken_first: bool):
    """input~output lines; normalize_with_audio lists acceptable outputs under a ~input line."""
    lines = path.read_text(encoding="utf-8").splitlines()
    if path.stem.endswith("normalize_with_audio"):
        current = None
        for line in lines:
            if line.startswith("~"):
                current = line[1:]
            elif current is not None and line.strip():
                yield current, line.strip()
        return
    for line in lines:
        if "~" in line:
            a, b = line.split("~", 1)
            yield (b, a) if spoken_first else (a, b)


cases = defaultdict(lambda: defaultdict(list))  # (lang, source, class, input) -> outputs, in order
for code, lang in LANGUAGES.items():
    for path in sorted((NEMO / "tests/nemo_text_processing" / code / "data_text_normalization").glob("*.txt")):
        name = path.stem.removeprefix("test_cases_")
        if name in SKIP_NEMO:
            continue
        for written, spoken in nemo_cases(path, code in SPOKEN_FIRST):
            if written.strip():
                cases[lang][("nemo", name, written)].append(spoken.strip())
for language, lang in [("chinese", "zh"), ("english", "en-us")]:
    for name in WETEXT_CLASSES[language]:
        path = WETEXT / "tn" / language / "test/data" / f"{name}.txt"
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            if " => " in line:
                written, spoken = line.split(" => ", 1)
                cases[lang][("wetext", name, written)].append(spoken.strip())

total = 0
for lang, by_case in sorted(cases.items()):
    with open(OUT / lang / "gold.jsonl", "w", encoding="utf-8") as file:
        for (source, name, written), outputs in by_case.items():
            record = {"lang": lang, "source": source, "class": name, "input": written,
                      "outputs": list(dict.fromkeys(outputs))}
            file.write(json.dumps(record, ensure_ascii=False) + "\n")
    total += len(by_case)
    print(f"{lang}: {len(by_case)} cases")
print(f"total: {total}")
