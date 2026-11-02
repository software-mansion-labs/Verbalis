# Verbalis

Verbalis rewrites text the way it's read aloud. It's the text front end of a text-to-speech (TTS) pipeline: numbers,
dates, times, phone numbers, money, units, symbols, abbreviations, and web addresses become words before the text
reaches the model. This package binds the Verbalis C++ core, for Python 3.10 and later, with no runtime dependencies.

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

## Installation

```bash
pip install verbalis
```

## Usage

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

## Options

Every step is on by default. Pass options as `Options(...)`.

| Option | Default | What it does |
|---|---|---|
| `normalize` | `true` | Uses one form for equivalent characters: quotes, apostrophes, dashes, and ellipses. |
| `trim` | `true` | Leaves single spaces between words, and none at the edges or before punctuation. |
| `terms` | `true` | Reads abbreviations and web addresses as words: `e.g.` becomes "for example". |
| `numbers` | `true` | Reads numbers, dates, times, money, and units as words: `$5` becomes "five dollars". |
| `symbols` | `true` | Reads the remaining symbols as words: `&` becomes "and". |
| `abbreviations` | empty | Adds or replaces abbreviation readings: `{"approx.": "approximately"}`. |
| `symbol_words` | empty | Adds or replaces symbol readings: `{"™": "trademark"}`. |

## C++, Python, and TypeScript

Verbalis is also a [C++ library](https://github.com/software-mansion-labs/Verbalis#c) and a [TypeScript package](https://www.npmjs.com/package/verbalis), both
with the same API and identical output.

## License

Verbalis is licensed under [the MIT License](https://github.com/software-mansion-labs/Verbalis/blob/main/LICENSE). It's extracted from the text preprocessor of
[Phonemis](https://github.com/IgorSwat/Phonemis). Third-party notices are in
[THIRD_PARTY_NOTICES.md](https://github.com/software-mansion-labs/Verbalis/blob/main/THIRD_PARTY_NOTICES.md).

## Community Discord

[Join the Software Mansion Community Discord](https://discord.swmansion.com) to chat about Verbalis or other Software
Mansion libraries.

## Verbalis is created by Software Mansion

Since 2012, [Software Mansion](https://swmansion.com) has been a software agency with experience in building web and
mobile apps. We are Core React Native Contributors and experts in dealing with all kinds of React Native issues. We can
help you build your next dream product – [Hire us](https://swmansion.com/contact/projects?utm_source=verbalis&utm_medium=readme).

[![swm](https://logo.swmansion.com/logo?color=white&variant=desktop&width=150&tag=verbalis-pypi "Software Mansion")](https://swmansion.com)
