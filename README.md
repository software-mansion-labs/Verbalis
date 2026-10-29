# Verbalis

Verbalis rewrites text the way it is read aloud, for text-to-speech front ends:

```
Dnia 12.03.2025 kupiłem 3 bilety.
→ Dnia dwunastego marca dwa tysiące dwudziestego piątego kupiłem trzy bilety.
```

Numbers, dates, times, phone numbers, money, units, symbols, abbreviations, and web addresses become words.
Languages: German, English (US, GB), Spanish, French, Hindi, Italian, Polish, Portuguese, and Chinese.

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

Add it to a CMake project:

```cmake
add_subdirectory(verbalis/cpp)   # or FetchContent
target_link_libraries(your_target PRIVATE verbalis::verbalis)
```

## Python

```bash
pip install verbalis   # not yet published; to build from a checkout: pip install .
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
npm install verbalis   # not yet published; to build from a checkout: cd ts && npm ci && npm run build
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

`"verbalis"` includes every language. To bundle only the ones you use, import the core and each language's entry
point (`verbalis/de`, `en`, `es`, `fr`, `hi`, `it`, `pl`, `pt`, `zh`):

```ts
import { Verbalizer } from "verbalis/core";
import { language } from "verbalis/pl";

new Verbalizer(language()).process("Mam 2 koty."); // "Mam dwa koty."
```

A pure TypeScript port with no dependencies, for Node.js 20 and later and browsers. It produces exactly the C++
output. `new Verbalizer("xx")` throws a `RangeError` for an unsupported language.

## Development

```bash
cmake -S cpp -B cpp/build -DVERBALIS_BUILD_TESTS=ON && cmake --build cpp/build -j
ctest --test-dir cpp/build --output-on-failure

uv venv && uv pip install ".[test]" && .venv/bin/pytest   # Python: build the extension, run tests/python
(cd ts && npm ci && npm run lint && npx vitest run)        # TypeScript: format, types, and ts/test
```

The parity tests check the C++, Python, and TypeScript output against `tests/parity/outputs/`, which every implementation must match.

## License

MIT. Verbalis is extracted from the text preprocessor of [Phonemis](https://github.com/IgorSwat/Phonemis). Third-party
notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
