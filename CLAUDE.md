# Verbalis

Verbalis rewrites text the way it is read aloud: numbers, dates, times, money, units, symbols, abbreviations, and web
addresses become words, in de, en, es, fr, hi, it, pl, pt, and zh. It's the front end a TTS model needs, as a small
add-on library. The C++ core is the reference implementation; Python wraps it, and TypeScript is a pure port. All
three expose the same API: `Verbalizer(language, options)` with `process(text)` and `process_with_changes(text)`.

## Layout

- `cpp/`: the C++ core. `include/verbalis/verbalizer.h` is the public API. Under `src/verbalis/`:
  - `verbalizer.cpp`: the fixed pipeline, normalize → trim → terms → symbols with a condition → numbers → symbols
    (a conditional reading, such as `<` before a number, must see the digits before numbers turns them into words).
  - `text/`: the document and its edits, the context around a match, scanning, Unicode, and UTF-8.
  - `steps/`: normalize, trim, terms, and symbols.
  - `numbers/`: the number engine: matches, formats, patterns, and the scanners and helpers they share.
  - `languages/`: `language.h` (`Language`, and `PatternLanguage` with its default readings), `grammar.h` (joiners,
    noun forms, case, gender), the registry, and one folder per language: `<xx>/constants.h` (tables) and `<xx>/<name>.cpp` (the class).
- `tests/cpp/`: C++ tests, through the public API (`verbalize.h`), and the parity runner.
  `tests/parity/outputs/`: the parity outputs, one JSONL file per language.
- `tests/lang/`: the test data. `<lang>/corpus.jsonl` is the target (below); `<lang>/gold.jsonl` holds the NeMo and
  WeTextProcessing cases; `accepted.jsonl` holds readings the user accepted beyond the corpus's; `score.py` scores
  them. The scripts and agents that built the corpus are at tag `corpus-v1`.
- `knowledge/`: what was decided and why. Read `knowledge/provenance.md` before changing the core.
- `python/`: the Python package. `src/bindings.cpp` binds the public C++ API with nanobind (module
  `verbalis._verbalis`; its stub `verbalis/_verbalis.pyi` is committed and checked by `test_stub.py`); `verbalis/` is the package; `CMakeLists.txt` builds it
  from `cpp/`. `pyproject.toml` (repository root, scikit-build-core) also sets what the sdist ships: the C++ sources,
  the bindings, and licences, never `tests/`.
- `tests/python/`: pytest: the API (`test_verbalizer.py`), the parity outputs (`test_parity.py`), the corpus and gold
  ratchet (`test_lang.py`), and the committed stub (`test_stub.py`). `tools/unicode-table.py` writes the TypeScript
  Unicode table from the C++ one.
- `ts/`: the TypeScript package, a pure port of the C++ (`src/` mirrors `cpp/src/verbalis/`). Read
  `knowledge/ts-port.md` before porting or changing it.
- `ts/test/`: vitest: the API (`verbalizer.test.ts`) and the parity contract (`parity.test.ts`).

## Commands

```bash
cmake -S cpp -B cpp/build -DVERBALIS_BUILD_TESTS=ON && cmake --build cpp/build -j
ctest --test-dir cpp/build --output-on-failure             # unit tests and parity
cpp/build/tests/verbalis_parity tests/parity/outputs        # parity summary per language and category
uv venv && uv pip install ".[test]" && .venv/bin/pytest     # Python package: build, API and parity tests
(cd ts && npm ci && npm run lint && npx vitest run)        # TypeScript: format and types, API and parity tests
.venv/bin/python tests/parity/regen.py                      # regenerate the parity outputs after a behaviour change
.venv/bin/python tests/lang/score.py [--corpus pl]        # accuracy against the corpus and the gold set, as a ratchet
HERMES=<hermes-engine destroot> tools/hermes/run.sh       # parity on Hermes (React Native), after a rule change
```

A refactor must keep every parity output identical.

## Parity

- One set of sentences feeds every test: the corpus, plus the gold cases. `tests/parity/outputs/` holds what the C++
  says for each input; every implementation must reproduce it, and `tests/parity/api.jsonl` (options, and the changes
  of `process_with_changes`), exactly.
- A change to the C++ that changes any output is a behaviour change: regenerate the outputs (`regen.py`) in the same
  change, and say in the commit which outputs changed and why. `regen.py` also rewrites `api.jsonl`.
- Offsets in `process_with_changes` count Unicode code points in C++ and Python, and UTF-16 code units in TypeScript
  (converted in `ts/src/core.ts`, so they index JavaScript strings); `api.jsonl` records code points.
- Correctness: `tests/lang/<lang>/corpus.jsonl` is the target, the expected reading of every reviewed real sentence,
  frozen at tag `corpus-v2` (never edit it; a new version is the user's decision); the gold set adds NeMo's and
  WeTextProcessing's cases. A behaviour change
  must keep every case in `tests/lang/scores.json` passing; record newly passing ones with `score.py --write`.
  `score.py --corpus <lang>` lists the sentences Verbalis still misreads.

## Code style

- C++ follows `knowledge/code_style.md`, with `.clang-format` and `.clang-tidy`: CamelCase types, snake_case
  functions, `kCamelCase` constants, language behaviour in `languages/<xx>/constants.h` tables first and overridden
  readings second, UTF-32 inside and UTF-8 at the API. Format with `clang-format -i`; tables are excluded in
  `.clang-format-ignore`.
- A reading returns `std::optional`; `std::nullopt` leaves the match as written. A reading that builds on another
  calls a private helper, not the other reading, so each kind's acceptance checks run once. What the words around a
  match say (a case, a gender, a year rather than a count) is the language's `Cues`, from `cues_of(match)`. Helpers
  more than one language needs go in `numbers/scanning.h`.
- To add a language: add `cpp/src/verbalis/languages/<xx>/` (copy the closest language) and list its `.cpp` in
  `cpp/CMakeLists.txt`, declare its `language()` and add it to `kLanguages` in `languages/registry.cpp`, add `tests/cpp/numbers_<xx>_test.cpp`, and regenerate the parity outputs.

## ⚑ Gotchas

- `keys_at` returns views into its `Keys`: pass a lasting table, never a braced list, in a range-for (C++20 destroys
  the temporary before the loop body). To catch such bugs, build with `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`
  and run `ctest`, which covers every parity input.

- The corpus (and so the parity outputs) contains sentences from the Leipzig Corpora Collection. Check their terms
  before the repo or a package containing them goes public (`knowledge/test-data.md`).
- Parity outputs are what the C++ says, right or wrong; parity means matching the reference, not being correct.
- `knowledge/code_style.md` describes Phonemis. Its naming, formatting, and comment rules apply; its architecture and
  layout sections don't (the layout is in this file).
