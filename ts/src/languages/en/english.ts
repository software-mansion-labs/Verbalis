import {
  GroupedOrdinalPattern,
  DatePattern,
  FractionPattern,
  Pattern,
  PhonePattern,
  ScalePattern,
  SuffixPattern,
  RomanPattern,
  PrimePattern,
} from "../../numbers/patterns.ts";
import { Kind, match as matchOf, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeParts,
  digitsAt,
  toDigits,
  numbersAt,
  fitsClock,
  isSpokenDigits,
  fractionOf,
  isMarkedFraction,
  namedFraction,
  parseValue,
  spacedSuffix,
  value,
  romanRole,
  RomanRole,
  isInchMark as isInchMarkAt,
} from "../../numbers/scanning.ts";
import { PUNCTUATION, stripped, wordsOf } from "../../text/context.ts";
import {
  digitsEnd,
  isClearAfter,
  isClearBefore,
  isDigitAfter,
  join,
  keysAt,
  skipSpace,
  type Keys,
} from "../../text/scan.ts";
import { contains, size, str, toLower, type Text } from "../../text/strings.ts";
import { isAlpha, isDigit, isUpper } from "../../text/unicode.ts";
import { keysOf, primeFormat } from "../../numbers/formats.ts";
import { Prime, type Forms } from "../grammar.ts";
import { PatternLanguage, type LanguageHandle } from "../language.ts";
import {
  AMOUNT_WORDS,
  CENTS,
  CURRENCIES,
  CURRENCY_KEYS,
  DATE_FORMAT,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  ERAS,
  FRACTION_WORDS,
  FRACTIONS,
  GROUP_SCALES,
  IRREGULAR_ORDINALS,
  INCH,
  IRREGULAR_PLURALS,
  JOINERS,
  MONTH_WORDS,
  MONTHS,
  NOTATION,
  PER,
  PER_KEYS,
  ONES,
  ORDINAL_SUFFIXES,
  PHONE_FORMATS,
  PRIME_UNITS,
  RANGE_WORDS,
  SCALE_WORDS,
  SCALES,
  STOP_WORDS,
  SYMBOLS,
  TENS,
  TERMS,
  TIME_WORDS,
  UNITS,
  YEAR_CUES,
  ROMAN_CUES,
  SPELLING_WORDS,
} from "./constants.ts";

// ------------------------
// ----- Number words -----
// ------------------------

function cardinal(n: bigint): string {
  if (n < 20n) {
    return ONES[Number(n)];
  }
  if (n < 100n) {
    return TENS[Number(n / 10n)] + (n % 10n ? `-${ONES[Number(n % 10n)]}` : "");
  }
  if (n < 1000n) {
    return `${ONES[Number(n / 100n)]} hundred` + (n % 100n ? ` ${cardinal(n % 100n)}` : "");
  }
  for (const [scale, word] of SCALE_WORDS) {
    if (n >= scale) {
      return `${cardinal(n / scale)} ${word}` + (n % scale ? ` ${cardinal(n % scale)}` : "");
    }
  }
  return "";
}

// A long grouped number by its groups of three digits: one hundred twenty-four quintillion ...
function groups(text: string): string {
  const words: string[] = [];
  const first = text.length % 3 !== 0 ? text.length % 3 : 3;
  let index = (text.length - first) / 3;
  for (let start = 0; start < text.length; index--) {
    const length = start === 0 ? first : 3;
    const group = parseValue(text.slice(start, start + length)) ?? 0n;
    if (group !== 0n) {
      words.push(cardinal(group) + (index !== 0 ? ` ${GROUP_SCALES[index - 1]}` : ""));
    }
    start += length;
  }
  return join(words);
}

// A day before its month: the twenty-fifth of
function dayOf(day: bigint, afterThe: boolean): string {
  return `${afterThe ? "" : "the "}${ordinal(day)} of`;
}

function ordinal(n: bigint): string {
  // Only the last word changes: twenty-first, one hundred and twelfth
  const words = cardinal(n);
  const start = Math.max(words.lastIndexOf(" "), words.lastIndexOf("-")) + 1;
  const last = words.slice(start);
  const head = words.slice(0, start);

  const irregular = IRREGULAR_ORDINALS[last];
  if (irregular !== undefined) {
    return head + irregular;
  }
  return head + (last.endsWith("y") ? `${last.slice(0, -1)}ieth` : `${last}th`);
}

// Year-style reading, also of 3-4 digit codes: nineteen oh five, two thousand seven, nine fifty-six
function pairs(n: bigint): string {
  const high = n / 100n;
  const low = n % 100n;
  if (n >= 1000n && n % 1000n < 10n) {
    return cardinal(n);
  }
  if (low === 0n) {
    return `${cardinal(high)} hundred`;
  }
  return `${cardinal(high)} ` + (low < 10n ? `oh ${ONES[Number(low)]}` : cardinal(low));
}

function digits(text: string): string {
  return join([...text].map((c) => ONES[Number(c)]));
}

// An amount of money: a cardinal, unless written with a leading zero
function amount(number: Numeral): string {
  const n = parseValue(number.integer);
  const zero = number.integer.length > 1 && number.integer[0] === "0";
  return n !== undefined && !zero ? cardinal(n) : digits(number.integer);
}

// a.m. and p.m. as their lowercase abbreviations are said; AM and PM stay as written
function ampm(suffix: string): string {
  const base = toLower(suffix.replace(/\.+$/, ""));
  const lowercase = ![...suffix].some((c) => isUpper(c));
  if (lowercase && (base === "am" || base === "pm")) {
    return base === "am" ? "a.m." : "p.m.";
  }
  return suffix;
}

// What the words around a match say about reading its numbers
interface Cues {
  afterMonth: boolean; // March 3: a day of the month, read as an ordinal
  counted: boolean; // 1999 people: a count rather than a year
  attributive: boolean; // a $5 bill: before a noun (not a verb in -ed or -ing, nor an adverb), the unit stays singular
  beforeMonth: boolean; // 25 July 2012: a day before the name of its month
  afterThe: boolean; // the 25th, 24/25 May: no article before it
  year: boolean; // in 1999: a year is expected
  era: boolean; // 340 AD: an era follows
  amount: boolean; // $2019, 1980 kg: an amount, never read as a year
  spelled: boolean; // ending in 8876, PIN 1234: digits to read one by one
  alone: boolean; // the whole text: 10111 or 31/32 alone is a number, not a code or a date
}

// Whether a word is an adverb by its ending: $60 million annually, -7.3°C respectively; not rally, family
function isAdverb(word: string): boolean {
  return (
    [...word].length > 6 &&
    (word.endsWith("ally") ||
      word.endsWith("ively") ||
      word.endsWith("fully") ||
      word.endsWith("ously") ||
      word.endsWith("ately"))
  );
}

// A plural noun, lowercased: people, months
function isPluralNoun(word: string): boolean {
  return (
    IRREGULAR_PLURALS.has(word) ||
    (size(word) > 3 && word.endsWith("s") && !word.endsWith("ss") && !STOP_WORDS.has(word))
  );
}

function cuesOf(match: Match): Cues {
  const previous = match.context.before();
  const next = match.context.after();
  const nextLower = toLower(next);

  // 1999 people: a plural noun after a 4-digit number makes it a count, unless a year is expected (in 1999)
  const pluralNoun = isPluralNoun(nextLower);
  // March 3, august 23, but "it may 3 times", "in May, 24 people": a month right before the number, capitalized
  // if it is also a common word
  const words = wordsOf(match.context.head());
  const written = words.length > 0 ? words[words.length - 1] : [];
  const common = previous === "may" || previous === "march" || previous === "mar";
  // July 5 to 12: the end of a range of days from a one-digit day is an ordinal too; the corpus reads March 14-23 as
  // "fourteenth to twenty-three"
  const day = match.context.before(2);
  const rangeEnd = previous === "to" && size(day) === 1 && isDigit(day) && MONTH_WORDS.has(match.context.before(3));
  const afterMonth =
    rangeEnd ||
    (MONTH_WORDS.has(previous) &&
      !contains(PUNCTUATION, written[written.length - 1]) &&
      (!common || isUpper(stripped(written)[0])));
  const yearCue = YEAR_CUES.has(previous) || afterMonth;

  // 25 July, 26th May 2012, but Top 10 March Madness picks, Quarter 1 (Jan
  const tail = match.context.tail();
  const after = wordsOf(tail);
  const closed = after.length > 0 && contains(PUNCTUATION, after[0][after[0].length - 1]); // May, Jan: no word follows
  const following = match.context.after(2);
  // A lowercase month counts too, unless it is a common word (may, march) without a year after it: 17 may 2010
  const commonMonth = nextLower === "may" || nextLower === "march" || nextLower === "mar";
  const lowercaseMonth = !commonMonth || (following !== "" && isDigit(following[0]));
  const month =
    tail.length > 1 &&
    tail[0] === " " &&
    isAlpha(tail[1]) &&
    (isUpper(tail[1]) || lowercaseMonth) &&
    MONTH_WORDS.has(nextLower) &&
    (closed || following === "" || !isUpper(following[0]));
  // 24/25 May, 23-24 June: the article goes before the first day
  const head = match.context.head();
  const inRange = head.length > 0 && (head[head.length - 1] === "/" || head[head.length - 1] === "-");

  return {
    afterMonth,
    counted: pluralNoun && !yearCue,
    attributive:
      next !== "" &&
      isAlpha(String.fromCodePoint(next.codePointAt(0) as number)) &&
      !STOP_WORDS.has(nextLower) &&
      !(tail[0] === " " && tail[1] === "(") &&
      !nextLower.endsWith("ed") &&
      !nextLower.endsWith("ing") &&
      !isAdverb(nextLower),
    beforeMonth: month,
    afterThe: previous === "the" || inRange,
    year: yearCue,
    era: ERAS.has(nextLower),
    amount:
      match.kind === Kind.MONEY || match.kind === Kind.SCALE || match.kind === Kind.PERCENT || match.kind === Kind.UNIT,
    // PIN 1234, your otp is 9453, ending in 8876
    spelled:
      SPELLING_WORDS.has(previous) ||
      ((previous === "is" || previous === "was") && SPELLING_WORDS.has(match.context.before(2))) ||
      (previous === "in" && match.context.before(2) === "ending"),
    alone: match.context.head().length === 0 && tail.length === 0,
  };
}

// --------------------
// ----- Patterns -----
// --------------------

// 10/06/2005, 11/17/05: a date with slashes and a year, month first unless the first number can't be a month
// (25/12/2020). Its numbers are as written.
class SlashDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,/-")) {
      return undefined;
    }
    const parts: Numeral[] = [];
    let end = position;
    for (let i = 0; i < 3; i++) {
      if (i > 0 && (end >= text.length || text[end] !== "/")) {
        return undefined;
      }
      const start = i === 0 ? end : end + 1;
      end = digitsEnd(text, start);
      const length = end - start;
      if (length === 0 || (i < 2 ? length > 2 : length !== 2 && length !== 4)) {
        return undefined;
      }
      parts.push(digitsAt(text, start));
    }
    if (!isClearAfter(text, end, "/") || isDigitAfter(text, end, ".,/")) {
      return undefined;
    }
    const a = value(parts[0]);
    const b = value(parts[1]);
    const month = a <= 12n ? a : b;
    const day = a <= 12n ? b : a;
    if (month < 1n || month > 12n || day < 1n || day > 31n) {
      return undefined;
    }
    return matchOf({ kind: Kind.DATE, span: { begin: position, end }, numbers: parts, separator: "/" });
  }
}

// 2016/07/03, 1998/2/30, 2012.01.05: a date with its year first. Its numbers are the day, the month, and the year,
// as an ISO date's.
class YearFirstDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const yearEnd = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, ".,/-") ||
      yearEnd !== position + 4 ||
      yearEnd >= text.length ||
      (text[yearEnd] !== "/" && text[yearEnd] !== ".")
    ) {
      return undefined;
    }
    const separator = text[yearEnd];
    const month = yearEnd + 1;
    const monthEnd = digitsEnd(text, month);
    if (monthEnd === month || monthEnd > month + 2 || monthEnd >= text.length || text[monthEnd] !== separator) {
      return undefined;
    }
    const day = monthEnd + 1;
    const dayEnd = digitsEnd(text, day);
    // With dots, two digits each (2012.01.05), which a version number seldom has
    const shortest = separator === "." ? 2 : 1;
    if (
      dayEnd < day + shortest ||
      dayEnd > day + 2 ||
      monthEnd < month + shortest ||
      !isClearAfter(text, dayEnd, "/") ||
      isDigitAfter(text, dayEnd, ".,/")
    ) {
      return undefined;
    }
    const y = digitsAt(text, position);
    const m = digitsAt(text, month);
    const d = digitsAt(text, day);
    if (value(m) < 1n || value(m) > 12n || value(d) < 1n || value(d) > 31n) {
      return undefined;
    }
    return matchOf({ kind: Kind.DATE, span: { begin: position, end: dayEnd }, numbers: [d, m, y], separator: "-" });
  }
}

// 30-May-2018, 5-Jan-20: a day, the name of its month, and a year, joined by hyphens. Its numbers are the day,
// the month, and the year.
class NamedMonthDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const dayEnd = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, ".,/-") ||
      dayEnd === position ||
      dayEnd > position + 2 ||
      dayEnd >= text.length ||
      text[dayEnd] !== "-"
    ) {
      return undefined;
    }
    let nameEnd = dayEnd + 1;
    while (nameEnd < text.length && isAlpha(text[nameEnd])) {
      nameEnd++;
    }
    const name = toLower(str(text, dayEnd + 1, nameEnd));
    const month = MONTHS.findIndex((m) => toLower(m).startsWith(name));
    // The terms step may have written out an abbreviation already: 5-Jan-2020 is 5-January 2020 here
    if (
      size(name) < 3 ||
      !MONTH_WORDS.has(name) ||
      month < 0 ||
      nameEnd >= text.length ||
      (text[nameEnd] !== "-" && text[nameEnd] !== " ")
    ) {
      return undefined;
    }
    // After a space, a four-digit year only: July 28-August 31 is a range
    const end = digitsEnd(text, nameEnd + 1);
    const year = end - nameEnd - 1;
    if (
      (year !== 2 && year !== 4) ||
      (text[nameEnd] === " " && year !== 4) ||
      !isClearAfter(text, end) ||
      isDigitAfter(text, end, ".,-/")
    ) {
      return undefined;
    }
    const day = digitsAt(text, position);
    if (value(day) < 1n || value(day) > 31n) {
      return undefined;
    }
    return matchOf({
      kind: Kind.DATE,
      span: { begin: position, end },
      numbers: [
        day,
        numeral({ span: { begin: 0, end: 0 }, integer: toDigits(BigInt(month + 1)) }),
        digitsAt(text, nameEnd + 1),
      ],
      separator: "-",
    });
  }
}

// 2Q22, 3Q2022, 1H23: a quarter or a half of a year. Its numbers are the quarter or half and the year.
class QuarterPattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position) || position + 3 >= text.length) {
      return undefined;
    }
    const part = text[position];
    const letter = text[position + 1];
    const quarter = letter === "Q" && part >= "1" && part <= "4";
    const half = letter === "H" && (part === "1" || part === "2");
    const end = digitsEnd(text, position + 2);
    if (!(quarter || half) || (end !== position + 4 && end !== position + 6) || !isClearAfter(text, end)) {
      return undefined;
    }
    return matchOf({
      kind: Kind.DATE,
      span: { begin: position, end },
      numbers: [digitsAt(text, position), digitsAt(text, position + 2)],
      separator: letter,
    });
  }
}

// 1/4th, 1/3RD, 23 4/5th: a fraction marked by the suffix of its denominator. Its numbers are as a FractionPattern's.
class SuffixedFractionPattern extends Pattern {
  constructor() {
    super(Kind.FRACTION);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, "./")) {
      return undefined;
    }
    const numbers: Numeral[] = [];
    let start = position;
    const wholeEnd = digitsEnd(text, position);
    if (wholeEnd > position && wholeEnd + 1 < text.length && text[wholeEnd] === " ") {
      numbers.push(digitsAt(text, position));
      start = wholeEnd + 1;
    }
    const slash = digitsEnd(text, start);
    if (slash === start || slash >= text.length || text[slash] !== "/") {
      return undefined;
    }
    const end = digitsEnd(text, slash + 1);
    const suffix = toLower(str(text, end, Math.min(end + 2, text.length)));
    if (
      end === slash + 1 ||
      (suffix !== "st" && suffix !== "nd" && suffix !== "rd" && suffix !== "th") ||
      !isClearAfter(text, end + 2)
    ) {
      return undefined;
    }
    numbers.push(digitsAt(text, start));
    numbers.push(digitsAt(text, slash + 1));
    return matchOf({
      kind: Kind.FRACTION,
      span: { begin: position, end: end + 2 },
      numbers,
      suffix: str(text, end, end + 2),
      separator: "/",
    });
  }
}

// 1960s-80s, 1960s-1980s: a range of decades. Its numbers are the first and the last decade.
class DecadeRangePattern extends Pattern {
  constructor() {
    super(Kind.DECADE);
  }

  match(text: Text, position: number): Match | undefined {
    const first = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, ".,-") ||
      first !== position + 4 ||
      first + 2 >= text.length ||
      text[first] !== "s" ||
      text[first + 1] !== "-"
    ) {
      return undefined;
    }
    const last = first + 2;
    const lastEnd = digitsEnd(text, last);
    if (
      (lastEnd !== last + 2 && lastEnd !== last + 4) ||
      lastEnd >= text.length ||
      text[lastEnd] !== "s" ||
      !isClearAfter(text, lastEnd + 1)
    ) {
      return undefined;
    }
    return matchOf({
      kind: Kind.DECADE,
      span: { begin: position, end: lastEnd + 1 },
      numbers: [digitsAt(text, position), digitsAt(text, last)],
      suffix: "s",
    });
  }
}

// 21:51:31, 2:08:29: hours, minutes, and seconds. Its numbers are the three parts; a race time (1:03.91) has two,
// minutes and seconds with their fraction.
class SecondsPattern extends Pattern {
  constructor() {
    super(Kind.TIME);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,:")) {
      return undefined;
    }
    const parts: Numeral[] = [];
    let end = position;
    for (let i = 0; i < 3; i++) {
      if (i === 2 && text[end] === "." && isDigit(text[end + 1])) {
        const fractionEnd = digitsEnd(text, end + 1);
        const seconds = parts[1];
        parts[1] = numeral({
          ...seconds,
          span: { begin: seconds.span.begin, end: fractionEnd },
          fraction: str(text, end + 1, fractionEnd),
          decimalMark: ".",
        });
        end = fractionEnd;
        break;
      }
      if (i > 0 && (end >= text.length || text[end] !== ":")) {
        return undefined;
      }
      const start = i === 0 ? end : end + 1;
      end = digitsEnd(text, start);
      const length = end - start;
      if (length === 0 || length > 2 || (i > 0 && (length !== 2 || text[start] > "5"))) {
        return undefined;
      }
      parts.push(digitsAt(text, start));
    }
    if (!isClearAfter(text, end) || isDigitAfter(text, end, ".,:")) {
      return undefined;
    }
    return matchOf({ kind: Kind.TIME, span: { begin: position, end }, numbers: parts, separator: ":" });
  }
}

// $.01, $20/mo, £10/wk, $12/kg: an amount after its currency, with no units (.01) or followed by what it is counted by.
// The suffix is the key of PER.
class PricePattern extends Pattern {
  constructor() {
    super(Kind.MONEY);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position)) {
      return undefined;
    }
    for (const currency of keysAt(text, position, CURRENCY_KEYS)) {
      const start = skipSpace(text, position + size(currency));
      let amount: Numeral;
      if (start + 1 < text.length && text[start] === "." && isDigit(text[start + 1])) {
        const end = digitsEnd(text, start + 1);
        amount = numeral({ span: { begin: start, end }, fraction: str(text, start + 1, end), decimalMark: "." });
      } else {
        const numbers = numbersAt(text, start, ",", ".");
        if (numbers.length === 0) {
          continue;
        }
        amount = numbers[0];
      }

      const money = matchOf({
        kind: Kind.MONEY,
        span: { begin: position, end: amount.span.end },
        numbers: [amount],
        unit: currency,
      });
      const slash = amount.span.end;
      if (slash < text.length && text[slash] === "/") {
        for (const key of keysAt(text, slash + 1, PER_KEYS)) {
          if (isClearAfter(text, slash + 1 + size(key))) {
            return { ...money, span: { begin: position, end: slash + 1 + size(key) }, suffix: key };
          }
        }
      }
      if (amount.integer === "" && isClearAfter(text, amount.span.end) && !isDigitAfter(text, amount.span.end)) {
        return money;
      }
    }
    return undefined;
  }
}

// 2pm-5pm, 5%-10%, $250-$300, 4:00 am - 8:12 pm: the hyphen of a range whose ends are marked as amounts or times.
// It has no numbers; the generic RangePattern takes the ranges of bare numbers.
class MarkedRangePattern extends Pattern {
  constructor() {
    super(Kind.RANGE);
  }

  match(text: Text, position: number): Match | undefined {
    const span = { begin: position, end: position + 1 };
    if (text[position] === " " && text[position + 1] === "-" && text[position + 2] === " ") {
      span.end = position + 3;
    } else if (text[position] !== "-") {
      return undefined;
    }
    const head = toLower(str(text, 0, span.begin));
    const tail = text.slice(span.end);

    // The end before it: a percentage or a time with am or pm; or any amount before a currency
    let mark = head;
    while (mark !== "" && (mark.endsWith(".") || isAlpha(mark[mark.length - 1]))) {
      mark = mark.slice(0, -1);
    }
    const suffix = head.slice(mark.length);
    mark = mark.replace(/ +$/, "");
    const time =
      (suffix === "am" || suffix === "pm" || suffix === "a.m." || suffix === "p.m.") &&
      mark !== "" &&
      isDigit(mark[mark.length - 1]);
    const percent = head.length > 1 && head.endsWith("%") && isDigit(head[head.length - 2]);
    const amount = head !== "" && isDigit(head[head.length - 1]);

    // The end after it: a number, or an amount after its currency
    const currency = keysAt(tail, 0, CURRENCY_KEYS).some(
      (key) => isDigitAfter(tail, size(key)) || isDigitAfter(tail, skipSpace(tail, size(key))),
    );
    const number = tail.length > 0 && isDigit(tail[0]);
    if (((time || percent) && (number || currency)) || (amount && currency)) {
      return matchOf({ kind: Kind.RANGE, span, separator: "-" });
    }
    return undefined;
  }
}

// 2*8, 2 * 10: the times of a product, between numbers
class ProductPattern extends Pattern {
  constructor() {
    super(Kind.DIMENSION);
  }

  match(text: Text, position: number): Match | undefined {
    const star = skipSpace(text, position);
    if (position === 0 || !isDigit(text[position - 1]) || star >= text.length || text[star] !== "*") {
      return undefined;
    }
    const end = skipSpace(text, star + 1);
    if (end >= text.length || !isDigit(text[end])) {
      return undefined;
    }
    return matchOf({ kind: Kind.DIMENSION, span: { begin: position, end }, separator: "*" });
  }
}

// 1-2=5, 1- 2 = 5: the minus of an equation
class EquationMinusPattern extends Pattern {
  constructor() {
    super(Kind.DIFFERENCE);
  }

  match(text: Text, position: number): Match | undefined {
    if (position === 0 || text[position] !== "-" || !isDigit(text[position - 1])) {
      return undefined;
    }
    const number = skipSpace(text, position + 1);
    const end = digitsEnd(text, number);
    if (end === number || skipSpace(text, end) >= text.length || text[skipSpace(text, end)] !== "=") {
      return undefined;
    }
    return matchOf({ kind: Kind.DIFFERENCE, span: { begin: position, end: number }, separator: "-" });
  }
}

// A straight double quote as a unit: 27" monitor
const INCH_MARK = '"';

function unitForms(unit: string): Forms {
  return unit === INCH_MARK ? INCH : UNITS[unit];
}

// Whether a straight double quote right after a number is an inch mark (27" monitor), not the end of a quotation
// ("67" matters): the quotes before it, other than inch marks, are all closed
function isInchMark(text: Text, position: number): boolean {
  return position < text.length && text[position] === '"' && isInchMarkAt(text, position);
}

// 27" monitor, 21.5" iMac: a number of inches, of at most three digits
class InchPattern extends Pattern {
  constructor() {
    super(Kind.UNIT);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,'’′")) {
      return undefined;
    }
    for (const number of numbersAt(text, position, NOTATION.groupSeparators, NOTATION.decimalSeparators)) {
      if (number.integer.length <= 3 && isInchMark(text, number.span.end)) {
        return matchOf({
          kind: Kind.UNIT,
          span: { begin: position, end: number.span.end + 1 },
          numbers: [number],
          unit: INCH_MARK,
        });
      }
    }
    return undefined;
  }
}

// The unit right after a match, of at least two letters after a space, or an inch mark: 4 1/2 lbs, 100 million kg,
// 3/8"
const LONG_UNITS: Keys = keysOf(UNITS).filter((key) => size(key) > 1);

function withUnit(text: Text, match: Match): Match | undefined {
  const start = match.span.end;
  if (isInchMark(text, start)) {
    return { ...match, span: { begin: match.span.begin, end: start + 1 }, unit: INCH_MARK };
  }
  if (start >= text.length || text[start] !== " ") {
    return undefined;
  }
  for (const unit of keysAt(text, start + 1, LONG_UNITS)) {
    if (isClearAfter(text, start + 1 + size(unit))) {
      return { ...match, span: { begin: match.span.begin, end: start + 1 + size(unit) }, unit };
    }
  }
  return undefined;
}

// 4 1/2 lbs, 1/2 kg: a fraction of a unit
class FractionUnitPattern extends Pattern {
  readonly #fractions = new FractionPattern();

  constructor() {
    super(Kind.FRACTION);
  }

  match(text: Text, position: number): Match | undefined {
    const match = this.#fractions.match(text, position);
    return match ? withUnit(text, match) : undefined;
  }
}

// 100 million kg: an amount of a unit with a scale word
class ScaleUnitPattern extends Pattern {
  readonly #scales = new ScalePattern(NOTATION);

  constructor() {
    super(Kind.SCALE);
  }

  match(text: Text, position: number): Match | undefined {
    const match = this.#scales.match(text, position);
    if (!match || !(match.scale in SCALES) || size(match.scale) < 4) {
      return undefined;
    }
    return withUnit(text, match);
  }
}

// 12/kg, 12kg/kg, 3 cc/s: an amount per unit, whose numerator unit is optional. The unit after the slash is the
// match's suffix.
class PerUnitPattern extends Pattern {
  readonly #units: Keys = keysOf(UNITS);

  constructor() {
    super(Kind.UNIT);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, "./")) {
      return undefined;
    }
    for (const number of numbersAt(text, position, NOTATION.groupSeparators, NOTATION.decimalSeparators)) {
      let slash = number.span.end;
      let unit = "";
      for (const key of keysAt(text, skipSpace(text, slash), this.#units)) {
        const end = skipSpace(text, slash) + size(key);
        if (end < text.length && text[end] === "/") {
          unit = key;
          slash = end;
          break;
        }
      }
      if (slash >= text.length || text[slash] !== "/") {
        continue;
      }
      for (const table of [PER_KEYS, this.#units]) {
        for (const per of keysAt(text, slash + 1, table)) {
          if (isClearAfter(text, slash + 1 + size(per))) {
            return matchOf({
              kind: Kind.UNIT,
              span: { begin: position, end: slash + 1 + size(per) },
              numbers: [number],
              unit,
              suffix: per,
            });
          }
        }
      }
    }
    return undefined;
  }
}

// --------------------
// ----- Language -----
// --------------------

class English extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new SlashDatePattern());
    this.addPattern(new YearFirstDatePattern());
    this.addPattern(new NamedMonthDatePattern());
    this.addPattern(new QuarterPattern());
    this.addPattern(new DatePattern(DATE_FORMAT));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new EquationMinusPattern());
    this.addPattern(new MarkedRangePattern());
    this.addPattern(new ProductPattern());
    this.addPattern(new SecondsPattern());
    this.addPattern(new PrimePattern(NOTATION, primeFormat({ feet: true })));
    this.addPattern(new PricePattern());
    this.addPattern(new SuffixedFractionPattern());
    this.addPattern(new FractionUnitPattern());
    this.addPattern(new ScaleUnitPattern());
    this.addPattern(new PerUnitPattern());
    this.addPattern(new InchPattern());
    this.addPattern(new GroupedOrdinalPattern(ORDINAL_SUFFIXES, ","));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new DecadeRangePattern());
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern());
  }

  protected override readNumber(number: Numeral, match: Match): string {
    if (number.fraction === "") {
      return this.#integer(number, cuesOf(match));
    }

    // .45 caliber: forty-five; .5 has no zero before its point
    const next = toLower(match.context.after());
    if (number.integer === "" && (next === "caliber" || next === "calibre" || next === "cal")) {
      return cardinal(parseValue(number.fraction) ?? 0n);
    }
    const head = number.integer === "" ? "" : `${cardinal(value(number))} `;
    return head + (number.fraction === "0" ? "point oh" : `point ${digits(number.fraction)}`);
  }

  protected override readCode(match: Match): string {
    if (!cuesOf(match).spelled) {
      return super.readCode(match);
    }
    // otp ab9453: after a spelling cue, digits one by one
    const words: string[] = [];
    for (const part of codeParts(match.text)) {
      if ([...part].every(isDigit)) {
        words.push(digits(part));
      } else if (part !== "-" && part !== ".") {
        words.push(this.readCodeLetters(part));
      }
    }
    return join(words);
  }

  protected override readCodeDigits(text: string): string {
    const n = parseValue(text);
    if (text.length > 1 && text[0] === "0") {
      return join([...text].map((c) => (c === "0" ? "oh" : ONES[Number(c)])));
    }
    if (n === undefined || text.length > 4) {
      return digits(text);
    }
    return text.length <= 2 ? cardinal(n) : pairs(n);
  }

  protected override readDate(match: Match): string | undefined {
    // 2Q22: the second quarter of twenty-two; 1H23: the first half of twenty-three
    if (match.separator === "Q" || match.separator === "H") {
      const year = match.numbers[1];
      return (
        `the ${ordinal(value(match.numbers[0]))}${match.separator === "Q" ? " quarter of " : " half of "}` +
        (year.integer.length === 4 ? pairs(value(year)) : cardinal(value(year)))
      );
    }
    // 4.2.16 is a version: a dotted date with a two-digit year has a two-digit day and month (17.02.16)
    const numbers = match.numbers;
    if (
      match.separator === "." &&
      numbers.length === 3 &&
      numbers[2].span.end - numbers[2].span.begin === 2 &&
      (numbers[0].integer.length === 1 || numbers[1].integer.length === 1)
    ) {
      return undefined;
    }
    // 10/06/2005: October sixth (25/12/2020 has its day first); 15.01.2020: the fifteenth of January
    let day = value(match.numbers[0]);
    let month = value(match.numbers[1]);
    const monthFirst = match.separator === "/" && day <= 12n;
    if (monthFirst) {
      [day, month] = [month, day];
    }
    const name = MONTHS[Number(month) - 1];
    const words = monthFirst ? `${name} ${ordinal(day)}` : `${dayOf(day, match.context.before() === "the")} ${name}`;
    if (match.numbers.length < 3) {
      return words;
    }
    // 1998: nineteen ninety-eight; 05: oh five
    const year = match.numbers[2];
    const y = value(year);
    const yearWords = year.integer.length === 4 ? pairs(y) : y < 10n ? `oh ${cardinal(y)}` : cardinal(y);
    return `${words} ${yearWords}`;
  }

  protected override readPhone(match: Match): string | undefined {
    // 1-555-...: the leading one joins the area code
    const groups = match.numbers.map((number) => number.integer);
    if (match.text.startsWith("1-")) {
      groups[0] = `1${groups[0]}`;
    }
    return join(groups.map(digits), ", ");
  }

  protected override readFraction(match: Match): string | undefined {
    const words = namedFraction(FRACTIONS, match);
    if (words !== undefined) {
      return words;
    }
    // one fifth, two thirds, three quarters, one hundredth
    const [a, b] = fractionOf(match);
    const suffixed = match.suffix !== ""; // 1/4th
    const alone = cuesOf(match).alone; // 22/3 alone: also an improper fraction
    if (
      b < 1n ||
      (a >= b && !alone) ||
      !(suffixed || alone || match.unit !== "" || isMarkedFraction(match, FRACTION_WORDS))
    ) {
      return undefined;
    }
    if (b === 1n) {
      return `${cardinal(a)} over one`;
    }
    let denominator = b === 2n ? "half" : b === 4n ? "quarter" : ordinal(b);
    if (a !== 1n) {
      denominator = b === 2n ? "halves" : `${denominator}s`;
    }
    return `${cardinal(a)} ${denominator}`;
  }

  protected override readMixedFraction(match: Match): string | undefined {
    const words = super.readMixedFraction(match);
    if (words === undefined || match.unit === "") {
      return words;
    }
    const unit = unitForms(match.unit);
    // a 3/8" bolt: before a noun, the unit stays singular
    if (cuesOf(match).attributive) {
      return `${words} ${unit.one}`;
    }
    if (match.numbers.length === 3) {
      return `${words} ${unit.many}`;
    }
    // 1/2 kg: half a kilogram; 3/4 in: three quarters of an inch
    const article = "aeiou".includes(unit.one[0]) ? "an" : "a";
    return `${words === "one half" ? "half" : `${words} of`} ${article} ${unit.one}`;
  }

  protected override readTime(match: Match): string | undefined {
    const hours = value(match.numbers[0]);
    const suffix = ampm(match.suffix);

    // 1:03.91: one minute three point nine one seconds
    if (match.numbers.length === 2 && match.numbers[1].fraction !== "") {
      const minutes = hours ? `${cardinal(hours)} ${hours === 1n ? "minute" : "minutes"} ` : "";
      return `${minutes}${this.readNumber(match.numbers[1], match)} seconds`;
    }

    // 10:00:00 p.m.: a clock time, ten p.m. (its a.m. or p.m. stays after it)
    const next = toLower(match.context.after());
    const clock = ["a.m.", "p.m.", "am", "pm", "a.m", "p.m"].includes(next);
    if (
      match.numbers.length === 3 &&
      clock &&
      value(match.numbers[1]) === 0n &&
      value(match.numbers[2]) === 0n &&
      hours >= 1n &&
      hours <= 12n
    ) {
      return cardinal(hours);
    }
    // 1:38:33 p.m., at 01:38:33: a clock time, one thirty-eight thirty-three; but "coming in at 8:51:00" is a duration
    const stamp = match.context.before() === "at" && match.numbers[0].integer.length === 2;
    if (match.numbers.length === 3 && (clock || stamp) && value(match.numbers[1]) !== 0n) {
      let words = this.readTime({ ...match, numbers: match.numbers.slice(0, 2) });
      const seconds = value(match.numbers[2]);
      if (words !== undefined && seconds !== 0n) {
        words += ` ${seconds < 10n ? `oh ${cardinal(seconds)}` : cardinal(seconds)}`;
      }
      return words;
    }
    // 2:08:29: two hours eight minutes and twenty-nine seconds, without the parts that are zero
    if (match.numbers.length === 3) {
      const parts: string[] = [];
      for (const [number, one, many] of [
        [hours, "hour", "hours"],
        [value(match.numbers[1]), "minute", "minutes"],
        [value(match.numbers[2]), "second", "seconds"],
      ] as const) {
        if (number !== 0n) {
          parts.push(`${cardinal(number)} ${number === 1n ? one : many}`);
        }
      }
      if (parts.length > 1) {
        parts[parts.length - 1] = `and ${parts[parts.length - 1]}`;
      }
      return parts.length === 0 ? "zero seconds" : join(parts);
    }

    // 7pm: an hour alone needs am or pm
    if (match.numbers.length === 1) {
      if (hours > 12n || !contains("apAP", match.suffix[0])) {
        return undefined;
      }
      return `${cardinal(hours)} ${suffix}`;
    }

    // 1.13 is a decimal, unless a time is expected: at 10.30; but 2.53% and 4.00%, at 1.50 times, and 2.30 kg
    const tail = match.context.tail();
    const word = match.context.after();
    const amount =
      tail[0] === "%" || (tail[0] === " " && tail[1] === "%") || AMOUNT_WORDS.has(word) || Object.hasOwn(UNITS, word);
    const decimal =
      match.separator === "." && match.suffix === "" && (amount || !TIME_WORDS.has(match.context.before()));
    if (!fitsClock(match) || decimal) {
      return undefined;
    }

    // ten thirty, ten oh five, ten o'clock
    const minutes = match.numbers[1].integer;
    let words = cardinal(hours);
    if (minutes !== "00") {
      const m = value(match.numbers[1]);
      words += minutes[0] === "0" ? ` oh ${ONES[Number(m)]}` : ` ${cardinal(m)}`;
    }
    if (suffix !== "") {
      return `${words} ${suffix}`;
    }
    return words + (minutes === "00" ? " o'clock" : "");
  }

  protected override readOrdinal(match: Match): string | undefined {
    // the 26th May: the twenty-sixth of May
    const n = value(match.numbers[0]);
    const cues = cuesOf(match);
    return cues.beforeMonth && n >= 1n && n <= 31n ? dayOf(n, cues.afterThe) : ordinal(n);
  }

  protected override readRoman(match: Match): string | undefined {
    // Henry the Eighth, the twenty-first century, Chapter four
    const n = value(match.numbers[0]);
    // IV as the whole text: a number (but I alone is the pronoun)
    if (match.context.head().length === 0 && match.context.tail().length === 0 && match.text.length > 1) {
      return cardinal(n);
    }
    const role = romanRole(match, ROMAN_CUES);
    switch (role ?? RomanRole.BEFORE) {
      case RomanRole.NAME:
        return `the ${ordinal(n)}`;
      case RomanRole.AFTER:
        return ordinal(n);
      default:
        return role !== undefined ? cardinal(n) : undefined;
    }
  }

  protected override readDecade(match: Match): string | undefined {
    // 1960s-80s: nineteen sixties to eighties
    let words = this.plural(this.readNumber(match.numbers[0], match)) ?? "";
    if (match.numbers.length === 2) {
      const to = match.numbers[1];
      words += ` to ${this.plural(to.integer.length === 4 ? pairs(value(to)) : cardinal(value(to)))}`;
    }
    return words;
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];
    const cues = cuesOf(match);

    // $20/mo: twenty dollars per month
    const per = match.suffix === "" ? "" : ` per ${PER[match.suffix]}`;

    // A price: four dollars and ninety-nine cents, two dollars, fifty cents; $20.5 twenty dollars and fifty cents
    // (a range of a larger amount, $2.5-$3 billion, or a glued abbreviation, £10.8bn2)
    const after = str(match.context.tail());
    const range =
      after.startsWith("-") ||
      after.startsWith(" -") ||
      after.startsWith(" to ") ||
      (after !== "" && isAlpha(after[0]));
    const places = number.fraction.length;
    if ((places === 2 || (places === 1 && !range)) && match.scale === "" && Object.hasOwn(CENTS, match.unit)) {
      const cents = CENTS[match.unit];
      const whole = value(number);
      const n = (parseValue(number.fraction) ?? 0n) * (places === 1 ? 10n : 1n);
      const centWords = `${cardinal(n)} ${n === 1n ? cents.one : cents.many}`;
      if (whole === 0n && n !== 0n) {
        return centWords + per;
      }
      const one = whole === 1n || (n === 0n && cues.attributive && per === ""); // a $69.00 target: a sixty-nine dollar target
      const words = `${amount(number)} ${one ? currency.one : currency.many}`;
      return (n !== 0n ? `${words} and ${centWords}` : words) + per;
    }

    // $18854, $2019: an amount, never digits by default nor a year
    let words = number.fraction === "" ? amount(number) : this.readNumber(number, match);
    if (match.scale !== "") {
      words += ` ${SCALES[match.scale]}`;
    }
    const one = number.integer === "1" && number.fraction === "" && match.scale === "";
    return `${words} ` + (one || (cues.attributive && per === "") ? currency.one : currency.many) + per;
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    // 4K: a resolution; 5b, 3 m: a letter or a unit, a scale only after a currency
    if (match.scale === "K" || match.scale === "b" || match.scale === "B" || match.scale === "m") {
      return undefined;
    }
    const words = `${this.readNumber(number, match)} ${SCALES[match.scale]}`;
    return match.unit === "" ? words : `${words} ${unitForms(match.unit).many}`;
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${this.readNumber(number, match)} percent${spacedSuffix(match)}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    let words = this.readNumber(number, match);
    if (match.unit !== "") {
      const unit = unitForms(match.unit);
      const one = number.fraction === "" && value(number) === 1n;
      words += ` ${one || cuesOf(match).attributive ? unit.one : unit.many}`;
    }
    // 12 kg/m²: per square meter
    if (match.suffix !== "") {
      words += ` per ${PER[match.suffix] ?? UNITS[match.suffix].one}`;
    }
    return words;
  }

  protected override readJoiner(match: Match): string | undefined {
    // A marked range has no numbers: 2pm-5pm; a product: 2*8
    if (match.kind === Kind.RANGE && match.numbers.length === 0) {
      return this.joiners().range;
    }
    if (match.kind === Kind.DIMENSION && match.separator === "*") {
      return " times ";
    }
    // 10 - 30 seconds, 2025 - 2027: a range; 5 - 3 = 2: a minus; 2017 - 1:00pm: a pause before a time
    if (match.kind === Kind.DIFFERENCE && match.text === " - ") {
      const tail = match.context.tail();
      const numberEnd = digitsEnd(tail, 0);
      const end = skipSpace(tail, numberEnd);
      if (numberEnd < tail.length && tail[numberEnd] === ":") {
        return " - ";
      }
      const equation = tail[end] === "=" || str(tail, end, end + 6) === "equals";
      return equation ? ` ${this.joiners().minus} ` : this.joiners().range;
    }
    return super.readJoiner(match);
  }

  protected override readRange(from: Numeral, to: Numeral, match: Match): string {
    if (from.fraction !== "" || to.fraction !== "") {
      return this.joiners().range;
    }

    // A range of amounts: 1980-1970 kg, 10-15%
    if (match.kind === Kind.MONEY || match.kind === Kind.PERCENT || match.kind === Kind.UNIT) {
      return this.joiners().range;
    }

    // 10-15 is "ten to fifteen", 1960-80 "nineteen sixty to eighty", 2024-25 "twenty twenty-four to twenty-five"; but
    // scores and two-digit seasons keep "-": 51-44, 21-22
    const a = value(from);
    const b = value(to);
    const season = from.integer.length === 2 && b === a + 1n;
    const decades = from.integer.length === 4 && to.integer.length === 2 && b > a % 100n;
    // 1-59 months, sizes 0-14, July 5-12: rising across a digit, before a plural noun, after a range word, or as days
    // of a month; but bowling figures keep "-": took 3-53
    const tail = match.context.tail();
    const next = tail[to.integer.length];
    const glued = to.integer.length < tail.length && (isAlpha(next) || next === "-");
    const longer =
      from.integer.length < to.integer.length &&
      b > a &&
      !glued &&
      (isPluralNoun(toLower(match.context.after(2))) ||
        RANGE_WORDS.has(match.context.before(2)) ||
        (MONTH_WORDS.has(match.context.before(2)) && b <= 31n));
    return ((from.integer.length === to.integer.length && b > a) || decades || longer) && !season
      ? this.joiners().range
      : "-";
  }

  protected override plural(words: string): string | undefined {
    if (words.endsWith("y")) {
      return `${words.slice(0, -1)}ies`;
    }
    return words + (words.endsWith("x") ? "es" : "s");
  }

  // An integer as a year, a day of a month or a count, digit by digit if it is a code
  // 5'9": five feet nine inches; 1'27"23: one minute twenty-seven point two three seconds
  protected override readPrime(number: Numeral, unit: Prime, match: Match): string {
    const forms = PRIME_UNITS[unit];
    const one = number.fraction === "" && value(number) === 1n;
    const count = number.fraction === "" ? cardinal(value(number)) : this.readNumber(number, match);
    return `${count} ${one ? forms.one : forms.many}`;
  }

  #integer(number: Numeral, cues: Cues): string {
    const n = parseValue(number.integer);
    // 124,444,234,854,823,834,553: too long to hold, read by its groups
    if (
      n === undefined &&
      number.grouped &&
      number.integer[0] !== "0" &&
      number.integer.length <= 3 * (GROUP_SCALES.length + 1)
    ) {
      return groups(number.integer);
    }
    // 12345: digit by digit, as a code; but 10111 residents, 12345 km
    const spoken =
      isSpokenDigits(number, 5) &&
      !(number.integer[0] !== "0" && (cues.counted || cues.amount)) &&
      !(cues.alone && number.integer[0] !== "0" && number.integer.length <= 9);
    if (n === undefined || spoken || (cues.spelled && number.integer.length > 2 && !number.grouped)) {
      return digits(number.integer);
    }
    if (cues.afterMonth && n >= 1n && n <= 31n) {
      return ordinal(n);
    }
    if (cues.beforeMonth && n >= 1n && n <= 31n && !number.grouped) {
      return dayOf(n, cues.afterThe);
    }
    // 340 AD: a year of three digits
    if (cues.era && number.integer.length === 3 && !number.grouped && !cues.amount) {
      return pairs(n);
    }
    // Four digits in pairs, as a year, an address, or a model (1999, 2788 San Tomas, GTX 1080), unless they count
    if (number.integer.length === 4 && !number.grouped && !cues.amount) {
      return cues.counted ? cardinal(n) : pairs(n);
    }
    return cardinal(n);
  }
}

let instance: English | undefined;

export function language(): LanguageHandle {
  return (instance ??= new English());
}
