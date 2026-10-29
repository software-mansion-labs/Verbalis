# Verbalis

Verbalis rewrites text the way it is read aloud: numbers, dates, times, money, units, symbols, abbreviations, and web
addresses become words, in de, en, es, fr, hi, it, pl, pt, and zh. It's the front end a TTS model needs, as a small
add-on library. The C++ core is the reference implementation; Python wraps it, and TypeScript is a pure port. All
three expose the same API: `Verbalizer(language, options)` with `process(text)` and `process_with_changes(text)`.

## Layout

- `cpp/`: the C++ core. `include/verbalis/verbalizer.h` is the public API. Under `src/verbalis/`:
  - `verbalizer.cpp`: the fixed pipeline, normalize → trim → terms → numbers → symbols.
  - `text/`: the document and its edits, the context around a match, scanning, Unicode, and UTF-8.
  - `steps/`: normalize, trim, terms, and symbols.
  - `numbers/`: the number engine: matches, formats, patterns, and the scanners and helpers they share.
  - `languages/`: `language.h` (`Language`, and `PatternLanguage` with its default readings), `grammar.h` (joiners,
    noun forms, case, gender), the registry, and one folder per language: `<xx>/constants.h` (tables) and `<xx>/<name>.cpp` (the class).
- `tests/cpp/`: C++ tests, through the public API (`verbalize.h`), and the parity runner. `tests/compare/`: the
  reference comparison. `tests/parity/baseline/`: the shared parity baseline,
  one JSONL file per language.
- `knowledge/`: what was decided and why. Read `knowledge/provenance.md` before changing the core.
- `python/`: the Python package. `src/bindings.cpp` binds the public C++ API with nanobind (module
  `verbalis._verbalis`, typed stubs generated at build time); `verbalis/` is the package; `CMakeLists.txt` builds it
  from `cpp/`. `pyproject.toml` (repository root, scikit-build-core) also sets what the sdist ships: the C++ sources,
  the bindings, and licences, never `tests/`.
- `tests/python/`: pytest: the API (`test_verbalizer.py`) and the parity baseline (`test_parity.py`).
- `ts/`: the TypeScript package, a pure port of the C++ (`src/` mirrors `cpp/src/verbalis/`). Read
  `knowledge/ts-port.md` before porting or changing it.
- `tests/ts/`: vitest: the API (`verbalizer.test.ts`) and the parity contract (`parity.test.ts`).

## Commands

```bash
cmake -S cpp -B cpp/build -DVERBALIS_BUILD_TESTS=ON && cmake --build cpp/build -j
ctest --test-dir cpp/build --output-on-failure             # unit tests and parity
cpp/build/tests/verbalis_parity tests/parity/baseline       # parity summary per language and category
uv venv && uv pip install ".[test]" && .venv/bin/pytest     # Python package: build, API and parity tests
(cd ts && npm ci && npm run lint && npx vitest run)        # TypeScript: format and types, API and parity tests
.venv/bin/python tests/lang/score.py [--classes]          # accuracy against hand-written readings (tests/lang/)
tests/compare/cross.sh [lang ...]                           # TypeScript outputs identical to the C++ working tree
tests/compare/compare.sh [commit]                           # outputs identical to a reference commit (default 3143074)
```

A refactor must keep `compare.sh` at 100% identical. It also runs on any `tests/parity/local/*.jsonl` (gitignored):
local hi and zh sentences that can't be committed yet.

## Parity

- The C++ output is the reference. Every implementation must reproduce `tests/parity/baseline/` (outputs of
  `process`) and `tests/parity/api.jsonl` (options, and the changes of `process_with_changes`) exactly.
- A change to the C++ that changes any baseline output is a behaviour change: update the baseline in the same change,
  and say in the commit which outputs changed and why.
  Regenerate `api.jsonl` with `.venv/bin/python tests/parity/api.py`.
- Offsets in `process_with_changes` count Unicode code points in every implementation.
- `tests/lang/` measures correctness against readings written by people (`knowledge/gold-set.md`). A behaviour change
  must keep every case in `tests/lang/scores.json` passing; record newly passing cases with `score.py --write`.
- `tests/lang/<lang>/corpus.jsonl` is the target: the expected reading of every reviewed real sentence, frozen at tag
  `corpus-v1` (never edit it). Its passing sentences are in the same ratchet; `score.py --corpus <lang>` lists the rest.

## Code style

- C++ follows `knowledge/code_style.md`, with `.clang-format` and `.clang-tidy`: CamelCase types, snake_case
  functions, `kCamelCase` constants, language behaviour in `languages/<xx>/constants.h` tables first and overridden
  readings second, UTF-32 inside and UTF-8 at the API. Format with `clang-format -i`; tables are excluded in
  `.clang-format-ignore`.
- A reading returns `std::optional`; `std::nullopt` leaves the match as written. A reading that builds on another
  calls a private helper, not the other reading, so each kind's acceptance checks run once. What the words around a
  match say (a case, a gender, a year rather than a count) is the language's `Cues`, from `cues_of(match)`. Helpers
  more than one language needs go in `numbers/scanning.h`.
- To add a language: add `cpp/src/verbalis/languages/<xx>/` (copy the closest language), declare its `language()` and
  add it to `kLanguages` in `languages/registry.cpp`, add `tests/cpp/numbers_<xx>_test.cpp`, and add a baseline file.

## ⚑ Gotchas

- `keys_at` returns views into its `Keys`: pass a lasting table, never a braced list, in a range-for (C++20 destroys
  the temporary before the loop body). To catch such bugs, build with `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`
  and run the tests and `tests/compare/run.cpp` on the inputs.

- `tests/parity/baseline/` contains sentences from the Leipzig Corpora Collection. Check their terms before the repo
  or a package containing them goes public (`knowledge/parity-dataset.md`).
- Some baseline outputs are wrong (inherited from Phonemis); parity means matching the reference, not being correct.
- `knowledge/code_style.md` describes Phonemis. Its naming, formatting, and comment rules apply; its architecture and
  layout sections don't (the layout is in this file).
