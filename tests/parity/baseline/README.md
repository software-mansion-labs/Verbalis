# Preprocessing reference dataset

Real sentences paired with what Phonemis preprocessing makes of them. Another implementation of the preprocessing
(for example a TypeScript port) should produce exactly the same output for every input.

## Files

One [JSON Lines](https://jsonlines.org) file per language, with one record per line:

```json
{"lang": "en-us", "category": "numbers", "input": "In July 1940 Churchill replaced him …", "output": "In July nineteen forty Churchill replaced him …"}
```

| Field | Meaning |
|---|---|
| `lang` | The language code passed to `phonemis::Config::lang`: `en-us`, `de`, `fr`, `es`, `it`, `pl`, `pt`. |
| `category` | Why the sentence was picked (see below). For information only. |
| `input` | The sentence, exactly as in the corpus. |
| `output` | `phonemis::Pipeline::preprocess()` of the input, with the default configuration (every layer on). |

`summary.json` gives, per language, how many sentences were preprocessed, how many of them preprocessing changed,
and how many of each category were kept.

| Language | Pairs | numbers | symbols | addresses | normalization | other |
|---|---|---|---|---|---|---|
| en-us | 2,250 | 1,500 | 300 | 150 | 150 | 150 |
| de | 2,134 | 1,500 | 300 | 150 | 150 | 34 |
| fr | 2,250 | 1,500 | 300 | 150 | 150 | 150 |
| es | 2,190 | 1,500 | 300 | 150 | 150 | 90 |
| it | 2,158 | 1,500 | 300 | 150 | 150 | 58 |
| pl | 2,250 | 1,500 | 300 | 150 | 150 | 150 |
| pt | 2,152 | 1,500 | 300 | 150 | 150 | 52 |

## Categories

A sentence belongs to the first category it matches:

1. `numbers`: it has a digit. Number verbalization (Num2Word), usually together with other layers.
2. `addresses`: a web address or an email (`example.com`, `www.`, `https://`, `a@b`). Term2Word.
3. `symbols`: a symbol such as `&`, `%`, `+`, `@`, `°`, `/`. Sym2Word.
4. `normalization`: typographic quotes, apostrophes, dashes, an ellipsis, Unicode spaces. NormalizeLayer.
5. `other`: anything else preprocessing changed, mostly abbreviations (`Mr.`, `np.`) and spacing (TrimLayer).

Within `numbers` and `symbols`, sentences with a written shape not seen yet (`00.00`, `$0,000`, `0:00`, `a00`) were
picked first, so that rare formats (dates, times, phone numbers, codes) are covered, not only years.

Corpus noise is kept on purpose (mis-encoded text such as `roÅ›lina`, truncated URLs, `tod@s`): an implementation
has to treat it the same way too.

## How it was made

- **Phonemis:** commit `1203061` (branch `zh-update`), preprocessing with the default `processor::Config`.
- **Sentences:** the [Leipzig Corpora Collection](https://wortschatz.uni-leipzig.de/en/download), two corpora per
  language (news and web): `eng_news_2025_1M`, `eng-uk_web-public_2018_1M`, `deu_news_2025_1M`, `deu-de_web_2020_1M`,
  `fra_news_2024_1M`, `fra-fr_web_2011_1M`, `spa_news_2024_1M`, `spa-mx_web_2015_1M`, `ita_news_2024_1M`,
  `ita-it_web-public_2019_1M`, `pol_news_2024_1M`, `pol-com_web_2018_1M`, `por_news_2024_1M`, `por-pt_web_2015_1M`.
- **Selection:** from each language, up to 150,000 candidate sentences (with digits, symbols, addresses or characters
  to normalize) and 30,000 others were preprocessed; the ones preprocessing changed were sampled by category, with a
  fixed seed.

To regenerate it, for example after a change of the preprocessing:

```bash
clang++ -std=c++20 -O2 -Isrc -Isrc/third-party scripts/tools/preprocessor_dataset/preprocess_lines.cpp \
    build/libphonemis.a -o build/preprocess_lines
python3 scripts/tools/preprocessor_dataset/create_dataset.py --corpora ../Echo/data/corpora \
    --binary build/preprocess_lines --output test/data/preprocessor
```

The selection depends on which sentences preprocessing changes, so a regenerated dataset may pick different inputs.
To see what a change of Phonemis affects, run it on the inputs of these files instead and compare the outputs.

## Note on redistribution

The sentences come from news and web pages collected by the Leipzig Corpora Collection. Check the collection's terms
before publishing this dataset (for example in a public repository).
