# Verbalis

Verbalis rewrites text the way it's read aloud. It's the text front end of a text-to-speech (TTS) pipeline: numbers,
dates, times, phone numbers, money, units, symbols, abbreviations, and web addresses become words before the text
reaches the model.

| Language | Input | Output |
|---|---|---|
| `en-us` | On Jan 5, 2026 I paid $49.99 & left. | On January fifth, twenty twenty-six I paid forty-nine dollars and ninety-nine cents and left. |
| `en-gb` | Doors open at 7pm on 3 May; tickets cost £12. | Doors open at seven p.m. on the third of May; tickets cost twelve pounds. |
| `de` | Am 3. Oktober kostet es 12,50 €. | Am dritten Oktober kostet es zwölf Euro fünfzig. |
| `es` | El 1 de mayo hubo 25 °C. | El uno de mayo hubo veinticinco grados Celsius. |
| `fr` | Rendez-vous le 14/07 à 20h30. | Rendez-vous le quatorze juillet à vingt heures trente. |
| `hi` | मेरे पास 25 रुपये हैं | मेरे पास पच्चीस रुपये हैं |
| `it` | Ho pagato 3,5 kg di mele. | Ho pagato tre virgola cinque chili di mele. |
| `pl` | Dnia 12.03.2025 kupiłem 3 bilety. | Dnia dwunastego marca dwa tysiące dwudziestego piątego kupiłem trzy bilety. |
| `pt` | Custa R$ 15,90 por 2 kg. | Custa quinze reais e noventa centavos por dois quilos. |
| `zh` | 我在2024年买了3本书 | 我在二零二四年买了三本书 |

Verbalis is a small library with no runtime dependencies, in three versions that produce identical output:

- **C++**: the reference implementation, C++17 headers.
- **Python**: [`verbalis` on PyPI](https://pypi.org/project/verbalis/), bindings to the C++ core, Python 3.10 and later.
- **TypeScript**: [`verbalis` on npm](https://www.npmjs.com/package/verbalis), a pure port for Node.js, browsers, and
  React Native (Hermes).

## Python

```bash
pip install verbalis
```

```python
from verbalis import Options, Verbalizer

english = Verbalizer("en-us")
english.process("I bought 3 tickets for $49.")  # "I bought three tickets for forty-nine dollars."

# Turn steps off, or add readings of your own
custom = Verbalizer("en-us", Options(numbers=False, abbreviations={"approx.": "approximately"}))

# Map the output back to the input: offsets count code points, so they index Python strings
text = "5 apples & pears"
for change in english.process_with_changes(text).changes:
    print(text[change.start : change.end], "->", change.replacement)
```

`Verbalizer("xx")` raises `ValueError` for an unsupported language; `Verbalizer.languages()` lists the supported
codes. A `Verbalizer` is immutable and releases the GIL while it works, so threads can share one instance.

## TypeScript

```bash
npm install verbalis
```

```ts
import { Verbalizer } from "verbalis";

const english = new Verbalizer("en-us");
english.process("I bought 3 tickets for $49."); // "I bought three tickets for forty-nine dollars."

// Turn steps off, or add readings of your own
const custom = new Verbalizer("en-us", { numbers: false, abbreviations: { "approx.": "approximately" } });

// Offsets count UTF-16 code units, like every JavaScript string index: text.slice(start, end)
const { changes } = english.processWithChanges("5 apples & pears");
```

`"verbalis"` includes every language. To bundle only the languages you use, import the core and each language's entry
point (`verbalis/de`, `en`, `es`, `fr`, `hi`, `it`, `pl`, `pt`, `zh`):

```ts
import { Verbalizer } from "verbalis/core";
import { language } from "verbalis/pl";

new Verbalizer(language()).process("Mam 2 koty."); // "Mam dwa koty."
```

`new Verbalizer("xx")` throws a `RangeError` for an unsupported language.

## C++

```cpp
#include <verbalis/verbalizer.h>

verbalis::Verbalizer english("en-us");
english.process("I bought 3 tickets for $49.");  // "I bought three tickets for forty-nine dollars."

// Turn steps off, or add readings of your own
verbalis::Verbalizer custom("en-us", {.numbers = false, .abbreviations = {{"approx.", "approximately"}}});

// Map the output back to the input: each change of [start, end), counted in code points
verbalis::Result result = english.process_with_changes("5 apples & pears");
```

To use the library in a CMake project, add it from source, or install it (`cmake -S cpp -B build && cmake --install
build`) and find it. The header needs C++17; the library builds as C++20 without changing your project's standard.
Invalid UTF-8 input bytes become U+FFFD.

```cmake
add_subdirectory(verbalis/cpp)   # or FetchContent_Declare(verbalis ... SOURCE_SUBDIR cpp)
# or: find_package(verbalis 0.1 REQUIRED)
target_link_libraries(your_target PRIVATE verbalis::verbalis)
```

## Options

Every step is on by default. The names are the same in every API, except that TypeScript spells `symbol_words`
as `symbolWords`.

| Option | Default | What it does |
|---|---|---|
| `normalize` | `true` | Uses one form for equivalent characters: quotes, apostrophes, dashes, and ellipses. |
| `trim` | `true` | Leaves single spaces between words, and none at the edges or before punctuation. |
| `terms` | `true` | Reads abbreviations and web addresses as words: `e.g.` becomes "for example". |
| `numbers` | `true` | Reads numbers, dates, times, money, and units as words: `$5` becomes "five dollars". |
| `symbols` | `true` | Reads the remaining symbols as words: `&` becomes "and". |
| `abbreviations` | empty | Adds or replaces abbreviation readings: `{"approx.": "approximately"}`. |
| `symbol_words` | empty | Adds or replaces symbol readings: `{"™": "trademark"}`. |

## Development

```bash
cmake -S cpp -B cpp/build -DVERBALIS_BUILD_TESTS=ON && cmake --build cpp/build -j
ctest --test-dir cpp/build --output-on-failure

uv venv && uv pip install ".[test]" && .venv/bin/pytest   # Python: build the extension, run tests/python
(cd ts && npm ci && npm run lint && npx vitest run)        # TypeScript: format, types, and ts/test
```

The parity tests check the C++, Python, and TypeScript output against `tests/parity/outputs/`, which every
implementation must match.

## License

Verbalis is licensed under [the MIT License](LICENSE). It's extracted from the text preprocessor of
[Phonemis](https://github.com/IgorSwat/Phonemis). Third-party notices are in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Community Discord

[Join the Software Mansion Community Discord](https://discord.swmansion.com) to chat about Verbalis or other Software
Mansion libraries.

## Verbalis is created by Software Mansion

Since 2012, [Software Mansion](https://swmansion.com) has been a software agency with experience in building web and
mobile apps. We are Core React Native Contributors and experts in dealing with all kinds of React Native issues. We can
help you build your next dream product – [Hire us](https://swmansion.com/contact/projects?utm_source=verbalis&utm_medium=readme).

[![swm](https://logo.swmansion.com/logo?color=white&variant=desktop&width=150&tag=verbalis-github "Software Mansion")](https://swmansion.com)
