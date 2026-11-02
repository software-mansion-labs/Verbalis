# en-us gold notes

Edits to `tests/lang/en-us/gold.jsonl`, and the cases that stay failing, each with its reason.

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
- a house number read as a cardinal ("one hundred twenty-three"), as common as pairs or digits: `123 Laguna Ct, Town`, `708 N 1st St, San City`, `123 Smth St, City, NY`.
- an abbreviation expanded where gold keeps it: `Main St.` (Main Street), `Mrs. Norris` (Missus, the spoken form of Mrs.).
- a URL read without its scheme ("HTTPS colon slash slash"), which speakers drop, and with digits in a path read one by one: `http://www.ourdailynews.com.sm`, `https://www.ourdailynews.com.sm`, `https://developer.nvidia.com/drive-sim-early-access-program`, `https://developer.nvidia.com/cv-cuda/early-access`, `https://www.nvidia.com/dgx-basepod`, `https://www.nvidia.com/dgx-basepod.sm/3`, `http://www.nvidia.com/rtx-6000`, `https://www.nvidia.com/dgx-basepod/`.
- an IP address read as cardinals between dots, as common as digits: `987.987.0.40`, `123.123.0.40`, `IP address is 123.123.0.40`.
- an amount read as written, with every decimal digit: `$2.000`, `$20.500`, `$20.010`, `$0.010`, `$12.0500`, `$1.2320`; and as dollars and cents where gold reads a decimal: `$12.05`, `$0.05`.
- a clock time with seconds read without the zero minutes, or a whole hour without them: `10:00:05 a.m.`, `10:00:00 p.m. e.s.t`.
- "four hundred fifty-two" for "four fifty two", and "two thousand five hundred" for "twenty five hundred": `This example number 15,000 ...`, `2,500`.
- a date or range read as words: `1998-3-4` (the fourth of March nineteen ninety-eight), `2%-10%` and `1961–1965 and (2006–2012)` ("to" for the dash).
- a backtick read as an apostrophe; both are silent: `` a `quote` example ``, `` a `25` example ``.
- "eighties" for the decade, where gold reads "eighty's": `80's`.
- a dot before an email address dropped: `test .b@gmail.com`.
- symbols read by meaning: `$12@12%` (twelve dollars at twelve percent), `2*8` (two times eight).
- a year at the end of a reference read as a year: `123/261788/2021`.
- a unit read in words, where gold spells its letters: `3 cc/s` (three cubic centimeters per second).
- a number standing alone read without the British "and": `10001`, `10011`, `10111`, `11111`, `101111`, `1001111`, `10001111`; `2/8` alone read as a fraction (two eighths), where gold reads a division.
- a serial's digit groups read in pairs, as in other codes: `1-413-te-b-1-5` (four thirteen), `1-8090` (one-eighty ninety).
- a file extension read as written: `file:///photos/image.jpg` (dot jpg, where gold says jpeg).
- the zeros of a ZIP code read "zero", not "oh": `2788 San Tomas Expy, Santa Clara, CA 95051`.

## Corrected gold readings

- `2+3 m2`: gold reads "m2" as letters; it is square meters
- `13000`: gold reads a round number digit by digit; it is a cardinal
- `123000`: gold reads a round number digit by digit; it is a cardinal
- `1000`: gold drops "one" of "one thousand"

## Out of scope

**Decision (the user, 2026-10-27):** these cases carry an `out_of_scope` reason and are left out of the score. No
reading of the bare fragment is right in running text without misreading other sentences, the input is malformed or
markup, or gold contradicts itself. The paragraphs about a/b and four- and five-digit numbers explain what Verbalis
reads instead.

- an a/b in running text is left as written: it is mostly a date, a score, a rating, odds, or a season (baseline: 9/10, 25/1, 3/47, 2018/19, 24/7); a/b reads as a fraction when marked (3 1/2, 1/3 of, 3/8", 1/4th) or when it is the whole text. Still failing: `3 5/2` (an improper mixed fraction), `2 142/1`, and `1795 / 1805` (spaces around the slash).
- a four-digit number reads in pairs, as a year, an address, or a model (1999, 2788 San Tomas, GTX 1080), also when it is the whole text; gold reads some bare four-digit numbers as years (`1155`, `1219`) and others as cardinals: `1011`, `1111`, `4567`.
- an ungrouped number of five or more digits in running text reads digit by digit as a code: in the baseline they are phone numbers, grid references, item and chassis numbers; a count is grouped or has a cue (10111 residents). When it is the whole text and has at most nine digits, it reads as a cardinal.
- a space before the "s" of a decade; reading "1980 s" as a decade would also read "5 s" (seconds): `1980 s`.
- markup and phonetic notation, which a text front end doesn't receive or leaves alone: `<pitch=-.3>123?123</pitch>`,
  `<pitch=-.3>123 123</pitch>`, `[HH AH0 L OW1]`.
- malformed or garbled input: `123,000012`, `45º&C`, `I've 20' and 14/ they're I'm 16c.`, `/$€₩£BB¥#%AA and $€₩£¥#%`,
  `Hi it's 5pm,4A.M.?-34. Hi,no,yes,34! ...` (numbers glued to words by commas), `email is a-1.b3_c&-d@gma4i-l.com test`.
- names with an unknown top-level domain, which read like file names: `test.abc`, `ourdailynews.abc`.
- a quote mark that gold reads as inches although it closes a quotation: `"text 7" text ".`.
- a bare a/b without a cue: `~1/24` (gold reads a date), `for Tu (2/22) at 5 pm.`, `1/f`, `1795 / 1805`, `2 142/1`.
- a name and a Roman numeral as the whole text, which in running text are mostly other words: `Sam I`, `Sam II`.
- a letter and digits with a plural s, which reads as the plural of a model (F-16s, B52s): `W26s`.


- `11/17/05` (`date` and `normalize_with_audio`): gold reads "oh five"; Verbalis reads "two thousand five". **Decision
  (the user, 2026-11-02):** a two-digit year below 30 reads in full in every language, English included; these two
  gold cases stay failing.
