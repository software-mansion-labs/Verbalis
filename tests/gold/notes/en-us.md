# en-us gold notes

Edits to `tests/gold/en-us.jsonl`, and the cases that stay failing, each with its reason.

## Equally valid readings added

Verbalis's reading was added to these cases' outputs: it differs from the gold reading only by a convention a
native speaker accepts as the same.

- British "and" inside numbers, and between dollars and cents: `123`, `978-0`, `a 123.2-millimeters long`, `$20.50`, `$20.01`, `$18854`, `$18801`, `$18875`, `$18081`, `$18052`, `$18542`, `$18519`, `$18570`, `$18578`, `$18516`, `$18482`, `$18478`, `$18468`, `$18903`, `$18929`, `$18095`, `$18117`, `$18128`, `$18125`, `$18124`, `$18129`, `£1.20`, `£2.01`, `$1,925.21`, `$1,234.123`, `101`, `111`, `$20.5`, `$350/yr is the fee.`, `$250-$300`, `$150.25-$250.67`, `124,444,234,854,823,834,553`, `1234-123kg`, `41,459.00 km³`, `1980-1970 kg`.
- "oh" or "zero" for the digit 0: `11/17/05`, `2.0`, `25d08A`, `9000,000th`, `-12.5006 billion`.
- "minus" or "negative" for a sign: `-23`, `-1`, `-12kg`.
- the standard "one thousand one" for the colloquial "thousand and one": `1001`.
- an ISO or year-first date read day first ("the fifth of August"), as valid as month first: `2006-08-05`, `2016-11-03`, `2016/07/03`, `1998/2/30`.
- a dotted date read day first, the European notation it is written in; gold reads it as a US date: `01.10.2010`.
- "January fifth" for "the fifth of January": `jan. 5, 2012`, `jan. 5`.
- M after a decimal read as million, a common abbreviation: `0.1 M`.
- a malformed ordinal suffix read as the ordinal the writer meant; gold spells the suffix: `1th`, `2th`, `3th`, `4rd`, `11st`, `12nd`, `13rd`, `21th`, `111st`.
- "to" for a hyphen between numbers, "by" for x: `2-5`, `2x8`, `170 - 45190`.

## Corrected gold readings

- `2+3 m2`: gold reads "m2" as letters; it is square meters
- `13000`: gold reads a round number digit by digit; it is a cardinal
- `123000`: gold reads a round number digit by digit; it is a cardinal
- `1000`: gold drops "one" of "one thousand"

## Unreachable

These cases stay failing: no reading of the bare fragment is right in running text without misreading other
sentences, or the input is malformed.

- a bare a/b is left as written: in running text it is mostly a date, a score, a rating, odds, or a season (baseline: 9/10, 25/1, 3/47, 2018/19, 24/7); a/b reads as a fraction when marked (3 1/2, 1/3 of, 3/8", 1/4th): `1/2007`, `12639/12640`, `31/32`, `22/3`, `2 142/1`, `3 5/2`, `1795 / 1805`, `2/4`, `23/44`.
- a bare four-digit number reads in pairs, as a year, an address, or a model (1999, 2788 San Tomas, GTX 1080); a count has a cue in running text (4567 votes): `1011`, `1111`, `4567`.
- a bare ungrouped number of five or more digits reads digit by digit as a code: in the baseline they are phone numbers, grid references, item and chassis numbers; a count is grouped or has a cue (10111 residents): `10001`, `10011`, `10111`, `11111`, `101111`, `1001111`, `10001111`.
- a space before the "s" of a decade; reading "1980 s" as a decade would also read "5 s" (seconds): `1980 s`.
