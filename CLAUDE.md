# Verbalis

Verbalis rewrites text the way it is read aloud: numbers, dates, times, money, units, symbols, abbreviations, and web
addresses become words, in de, en, es, fr, hi, it, pl, pt, and zh. It's the front end a TTS model needs, as a small
add-on library. The C++ core is the reference implementation; Python wraps it, and TypeScript is a pure port. All
three expose the same API: `Verbalizer(language, options)` with `process(text)` and `process_with_changes(text)`.

## Layout

- `cpp/`: the C++ core. `include/verbalis/verbalizer.h` is the public API; `src/verbalis/` holds the processor and
  its layers (`processor/`), one folder per language (`lang/<xx>/`), the language registry (`languages.cpp`), and
  UTF-8/UTF-32 helpers (`utils/`).
- `tests/cpp/`: C++ unit and API tests, and the parity runner. `tests/parity/baseline/`: the shared parity baseline,
  one JSONL file per language.
- `knowledge/`: what was decided and why. Read `knowledge/provenance.md` before changing the core.
- `python/` and `ts/`: planned (Python wrapper next, then the TypeScript port).

## Commands

```bash
cmake -S cpp -B cpp/build -DVERBALIS_BUILD_TESTS=ON && cmake --build cpp/build -j
ctest --test-dir cpp/build --output-on-failure             # unit tests and parity
cpp/build/tests/verbalis_parity tests/parity/baseline       # parity summary per language and category
```

## Parity

- The C++ output is the reference. Every implementation must reproduce `tests/parity/baseline/` exactly.
- A change to the C++ that changes any baseline output is a behaviour change: update the baseline in the same change,
  and say in the commit which outputs changed and why.
- Offsets in `process_with_changes` count Unicode code points in every implementation.

## Code style

- C++ follows `knowledge/code_style.md`, with `.clang-format` and `.clang-tidy`: CamelCase types, snake_case
  functions, `kCamelCase` constants, language behaviour in `lang/<xx>/constants.h` tables first and subclasses second,
  UTF-32 inside and UTF-8 at the API.
- To add a language: add `cpp/src/verbalis/lang/<xx>/` (copy the closest language), add one entry to `kLanguages` in
  `cpp/src/verbalis/languages.cpp`, add `tests/cpp/num2word_<xx>_test.cpp`, and add a baseline file.

## ⚑ Gotchas

- `tests/parity/baseline/` contains sentences from the Leipzig Corpora Collection. Check their terms before the repo
  or a package containing them goes public (`knowledge/parity-dataset.md`).
- Some baseline outputs are wrong (inherited from Phonemis); parity means matching the reference, not being correct.
- `lang/*/num2word.cpp` is mostly AI-generated; take style from `processor/` and `lang/en/` instead.
