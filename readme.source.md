```aura width=800 height=416
<div style={{ position: 'relative', display: 'flex', flexDirection: 'column', width: '100%', height: '100%', background: '#0a1240', borderRadius: 20, overflow: 'hidden', padding: '40px 44px', fontFamily: 'Inter' }}>
  <style>{`
    @keyframes reveal { 0%, 8% { transform: translateX(0); } 45%, 92% { transform: translateX(639px); } 100% { transform: translateX(0); } }
    @keyframes drift-a { 0%, 100% { transform: translate(0, 0); } 50% { transform: translate(30px, -20px); } }
    @keyframes drift-b { 0%, 100% { transform: translate(0, 0); } 50% { transform: translate(-26px, 18px); } }
    @media (prefers-reduced-motion: reduce) { #orb0, #orb1, #cover0, #cover1, #cover2 { animation: none; } }
    #orb0 { animation: drift-a 12s ease-in-out infinite; }
    #orb1 { animation: drift-b 14s ease-in-out infinite; }
    #cover0 { animation: reveal 9s cubic-bezier(0.4, 0, 0.2, 1) infinite 0.0s backwards; }
    #cover1 { animation: reveal 9s cubic-bezier(0.4, 0, 0.2, 1) infinite 0.9s backwards; }
    #cover2 { animation: reveal 9s cubic-bezier(0.4, 0, 0.2, 1) infinite 1.8s backwards; }
  `}</style>
  <svg width="800" height="416" style={{ position: 'absolute', top: 0, left: 0 }}>
    <defs>
      <radialGradient id="g0"><stop offset="0%" stopColor="rgba(127,211,255,0.35)" /><stop offset="100%" stopColor="rgba(127,211,255,0)" /></radialGradient>
      <radialGradient id="g1"><stop offset="0%" stopColor="rgba(255,196,107,0.22)" /><stop offset="100%" stopColor="rgba(255,196,107,0)" /></radialGradient>
    </defs>
    <ellipse id="orb0" cx="700" cy="40" rx="260" ry="200" fill="url(#g0)" />
    <ellipse id="orb1" cx="60" cy="416" rx="240" ry="170" fill="url(#g1)" />
  </svg>
  <div style={{ display: 'flex', alignItems: 'flex-end', justifyContent: 'space-between', height: 92 }}>
    <div style={{ display: 'flex', flexDirection: 'column' }}>
      <span style={{ fontFamily: 'InterBold', fontSize: 56, color: '#ffffff', letterSpacing: -2, lineHeight: 1 }}>Verbalis</span>
      <span style={{ fontSize: 18, color: 'rgba(255,255,255,0.65)', marginTop: 12 }}>Text the way it's read aloud, for text-to-speech.</span>
    </div>
    <span style={{ fontSize: 12, color: 'rgba(255,255,255,0.4)', letterSpacing: 3, marginBottom: 4 }}>C++ · PYTHON · TYPESCRIPT</span>
  </div>
  <div style={{ display: 'flex', flexDirection: 'column', gap: 12, marginTop: 28 }}>
    <div style={{ display: 'flex', alignItems: 'center', height: 64, background: '#111a4d', borderRadius: 12, padding: '0 18px', border: '1px solid rgba(255,255,255,0.08)' }}>
      <span style={{ width: 58, fontSize: 12, color: 'rgba(255,255,255,0.45)', letterSpacing: 1 }}>en-us</span>
      <div style={{ display: 'flex', flexDirection: 'column' }}>
        <div style={{ display: 'flex', whiteSpace: 'pre', fontSize: 14, lineHeight: '18px' }}><span style={{ color: 'rgba(255,255,255,0.5)' }}>I bought </span><span style={{ color: '#ffc46b' }}>3</span><span style={{ color: 'rgba(255,255,255,0.5)' }}> tickets for </span><span style={{ color: '#ffc46b' }}>$49</span><span style={{ color: 'rgba(255,255,255,0.5)' }}>.</span></div>
        <div style={{ display: 'flex', whiteSpace: 'pre', fontSize: 16, lineHeight: '22px', marginTop: 4 }}><span style={{ color: '#ffffff' }}>I bought </span><span style={{ color: '#7fd3ff' }}>three</span><span style={{ color: '#ffffff' }}> tickets for </span><span style={{ color: '#7fd3ff' }}>forty-nine dollars</span><span style={{ color: '#ffffff' }}>.</span></div>
      </div>
    </div>
    <div style={{ display: 'flex', alignItems: 'center', height: 64, background: '#111a4d', borderRadius: 12, padding: '0 18px', border: '1px solid rgba(255,255,255,0.08)' }}>
      <span style={{ width: 58, fontSize: 12, color: 'rgba(255,255,255,0.45)', letterSpacing: 1 }}>fr</span>
      <div style={{ display: 'flex', flexDirection: 'column' }}>
        <div style={{ display: 'flex', whiteSpace: 'pre', fontSize: 14, lineHeight: '18px' }}><span style={{ color: 'rgba(255,255,255,0.5)' }}>Rendez-vous le </span><span style={{ color: '#ffc46b' }}>14/07</span><span style={{ color: 'rgba(255,255,255,0.5)' }}> à </span><span style={{ color: '#ffc46b' }}>20h30</span><span style={{ color: 'rgba(255,255,255,0.5)' }}>.</span></div>
        <div style={{ display: 'flex', whiteSpace: 'pre', fontSize: 16, lineHeight: '22px', marginTop: 4 }}><span style={{ color: '#ffffff' }}>Rendez-vous le </span><span style={{ color: '#7fd3ff' }}>quatorze juillet</span><span style={{ color: '#ffffff' }}> à </span><span style={{ color: '#7fd3ff' }}>vingt heures trente</span><span style={{ color: '#ffffff' }}>.</span></div>
      </div>
    </div>
    <div style={{ display: 'flex', alignItems: 'center', height: 64, background: '#111a4d', borderRadius: 12, padding: '0 18px', border: '1px solid rgba(255,255,255,0.08)' }}>
      <span style={{ width: 58, fontSize: 12, color: 'rgba(255,255,255,0.45)', letterSpacing: 1 }}>pl</span>
      <div style={{ display: 'flex', flexDirection: 'column' }}>
        <div style={{ display: 'flex', whiteSpace: 'pre', fontSize: 14, lineHeight: '18px' }}><span style={{ color: 'rgba(255,255,255,0.5)' }}>Dnia </span><span style={{ color: '#ffc46b' }}>12.03.2025</span><span style={{ color: 'rgba(255,255,255,0.5)' }}> kupiłem </span><span style={{ color: '#ffc46b' }}>3</span><span style={{ color: 'rgba(255,255,255,0.5)' }}> bilety.</span></div>
        <div style={{ display: 'flex', whiteSpace: 'pre', fontSize: 16, lineHeight: '22px', marginTop: 4 }}><span style={{ color: '#ffffff' }}>Dnia </span><span style={{ color: '#7fd3ff' }}>dwunastego marca dwa tysiące dwudziestego piątego</span><span style={{ color: '#ffffff' }}> kupiłem </span><span style={{ color: '#7fd3ff' }}>trzy</span><span style={{ color: '#ffffff' }}> bilety.</span></div>
      </div>
    </div>
  </div>
  <svg width="800" height="416" style={{ position: 'absolute', top: 0, left: 0 }}>
    <defs>
      <clipPath id="clip0"><rect x="119" y="190" width="635" height="26" /></clipPath>
      <clipPath id="clip1"><rect x="119" y="266" width="635" height="26" /></clipPath>
      <clipPath id="clip2"><rect x="119" y="342" width="635" height="26" /></clipPath>
    </defs>
    <g clipPath="url(#clip0)"><g id="cover0" transform="translate(639, 0)"><rect x="119" y="190" width="635" height="26" fill="#111a4d" /><rect x="119" y="193" width="2" height="20" fill="#7fd3ff" /></g></g>
    <g clipPath="url(#clip1)"><g id="cover1" transform="translate(639, 0)"><rect x="119" y="266" width="635" height="26" fill="#111a4d" /><rect x="119" y="269" width="2" height="20" fill="#7fd3ff" /></g></g>
    <g clipPath="url(#clip2)"><g id="cover2" transform="translate(639, 0)"><rect x="119" y="342" width="635" height="26" fill="#111a4d" /><rect x="119" y="345" width="2" height="20" fill="#7fd3ff" /></g></g>
  </svg>
</div>
```

<p align="center">
  <a href="https://www.npmjs.com/package/verbalis"><img src="https://img.shields.io/npm/v/verbalis?style=for-the-badge&logo=npm&color=0a1240" alt="npm version"></a>
  <a href="https://pypi.org/project/verbalis/"><img src="https://img.shields.io/pypi/v/verbalis?style=for-the-badge&logo=pypi&logoColor=white&color=0a1240" alt="PyPI version"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-0a1240?style=for-the-badge" alt="MIT license"></a>
  <a href="https://discord.swmansion.com"><img src="https://img.shields.io/badge/Discord-join%20us-0a1240?style=for-the-badge&logo=discord&logoColor=white" alt="Join the Software Mansion Discord"></a>
</p>

Verbalis rewrites text the way it's read aloud. It's the text front end of a text-to-speech (TTS) pipeline: numbers,
dates, times, phone numbers, money, units, symbols, abbreviations, and web addresses become words before the text
reaches the model.

- **Nine languages:** German, English (US and UK), Spanish, French, Hindi, Italian, Polish, Portuguese, and Chinese.
- **One output, three packages:** a C++17 library, a Python package, and a TypeScript package that produce identical
  text, checked on all 23,814 test sentences.
- **No runtime dependencies:** the TypeScript package runs in Node.js, browsers, and React Native (Hermes), and bundles
  only the languages you import.
- **Traceable:** `process_with_changes` maps every replacement back to its place in the input.
- **Configurable:** turn off any step, or add readings of your own.

## Examples

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

`README.md` and its hero image are generated: edit `readme.source.md`, then run
`npx readme-aura build -f <fonts>`, where `<fonts>` holds `Inter.ttf` and `InterBold.ttf` from
[Inter](https://fonts.google.com/specimen/Inter).

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
