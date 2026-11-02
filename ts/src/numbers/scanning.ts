/**
 * Scanning numbers: written numbers at a position of a text, on top of the shared scanning in text/scan.ts, and the
 * shared steps of the languages' readings: values of digits, digit-by-digit decisions, and the parts of codes.
 */
import { digitsEnd, isDigitAfter } from "../text/scan.ts";
import { contains, str, textOf, toLower, type Text } from "../text/strings.ts";
import { isAlpha, isDigit, isLower, isSpace, isUpper } from "../text/unicode.ts";
import type { Fractions, RomanCues } from "./formats.ts";
import { numeral, type Match, type Numeral } from "./match.ts";

// The digits starting at a position, as a number.
export function digitsAt(text: Text, position: number): Numeral {
  const end = digitsEnd(text, position);
  return numeral({ span: { begin: position, end }, integer: str(text, position, end) });
}

/**
 * A written number at a position: 1,000.5 or 1 000,5 or 15.
 * @returns its possible readings, in the order a regex would try them: grouped first, each with and then without its
 *          fraction. Empty if no number starts there.
 */
export function numbersAt(text: Text, position: number, group: string, decimal: string): Numeral[] {
  const run = digitsEnd(text, position) - position;
  if (run === 0) {
    return [];
  }

  // Integer parts: 1-3 digits followed by whole groups (1 000 000), then a plain run of digits
  const integers: Numeral[] = [];
  if (run <= 3) {
    // Every group after the same separator: 1.000.000, but the 1.234 of 1.234,56
    let end = position + run;
    const separator = text[end];
    while (
      end + 3 < text.length &&
      text[end] === separator &&
      contains(group, separator) &&
      digitsEnd(text, end + 1) >= end + 4
    ) {
      end += 4;
    }
    // Indian groups: pairs before the last three digits (3,94,227, 12,34,56,789)
    if (end === position + run && run <= 2) {
      let pairs = end;
      while (pairs + 3 < text.length && text[pairs] === "," && digitsEnd(text, pairs + 1) === pairs + 3) {
        pairs += 3;
      }
      if (pairs > end && pairs + 3 < text.length && text[pairs] === "," && digitsEnd(text, pairs + 1) === pairs + 4) {
        end = pairs + 4;
      }
    }
    // A group may not be followed by a digit, nor by its separator and a digit
    const complete =
      end > position + run &&
      !isDigitAfter(text, end) &&
      !(end < text.length && text[end] === separator && isDigitAfter(text, end + 1));
    if (complete) {
      const digits = text.slice(position, end).filter(isDigit).join("");
      integers.push(numeral({ span: { begin: position, end }, integer: digits, grouped: true }));
    }
  }
  integers.push(digitsAt(text, position));

  // Each with its fraction first, then without it
  const numbers: Numeral[] = [];
  for (const integer of integers) {
    const end = integer.span.end;
    if (end < text.length && contains(decimal, text[end]) && isDigitAfter(text, end + 1)) {
      let number: Numeral = { ...integer, span: { ...integer.span } };
      number.span.end = digitsEnd(text, end + 1);
      number.fraction = str(text, end + 1, number.span.end);
      number.decimalMark = text[end];
      // A fraction shaped like a group is one: the 1.980 of 1.980,6
      if (
        !integer.grouped &&
        integer.integer.length <= 3 &&
        number.fraction.length === 3 &&
        contains(group, number.decimalMark)
      ) {
        number = numeral({ span: number.span, integer: integer.integer + number.fraction, grouped: true });
      }
      numbers.push(number);
    }
    numbers.push(integer);
  }
  return numbers;
}

// The value of a run of digits; undefined if it is empty or longer than 18 digits.
export function parseValue(digits: string): bigint | undefined {
  return digits === "" || digits.length > 18 ? undefined : BigInt(digits);
}

// The value of a number's integer part; 0 if it has none or it is too long to hold.
export function value(number: Numeral): bigint {
  return parseValue(number.integer) ?? 0n;
}

// Whether a number is "1" as written, which counts a singular noun.
export function isOne(number: Numeral): boolean {
  return number.integer === "1" && number.fraction === "";
}

// Whether an integer is read digit by digit: a leading zero, or a long code that is not a round amount.
export function isSpokenDigits(number: Numeral, longest: number): boolean {
  const i = number.integer;
  return (i.length > 1 && i[0] === "0") || (i.length >= longest && !number.grouped && !i.endsWith("000"));
}

/**
 * A day of a month written with a leading zero, without it: 05 de mayo, 04 juin; undefined for any other number.
 * @param months lowercase.
 * @param of a word which may stand between the day and the month: "de".
 */
export function unpaddedDay(number: Numeral, match: Match, months: readonly string[], of = ""): Numeral | undefined {
  const i = number.integer;
  if (i.length !== 2 || i[0] !== "0" || i[1] === "0" || number.fraction !== "") {
    return undefined;
  }
  let next = toLower(match.context.after());
  if (of !== "" && next === of) {
    next = toLower(match.context.after(2));
  }
  if (next === "" || !months.includes(next)) {
    return undefined;
  }
  return { ...number, integer: i.slice(1) };
}

// The value of the digits of a code or phone number, or undefined if they read digit by digit: a leading zero, more
// than longest digits, or too long to hold.
export function codeValue(digits: string, longest: number): bigint | undefined {
  if ((digits.length > 1 && digits[0] === "0") || digits.length > longest) {
    return undefined;
  }
  return parseValue(digits);
}

// Whether a time's hours and minutes fit a clock: at most 24 and 59. An hour alone always fits.
export function fitsClock(time: Match): boolean {
  return time.numbers.length < 2 || (value(time.numbers[0]) <= 24n && value(time.numbers[1]) <= 59n);
}

// The words of a fraction with its own words (1/2: half), or undefined.
// The numerator and denominator of a fraction: its last two numbers (3 1/2 has its whole number first).
export function fractionOf(fraction: Match): [bigint, bigint] {
  const n = fraction.numbers.length;
  return [value(fraction.numbers[n - 2]), value(fraction.numbers[n - 1])];
}

export function namedFraction(fractions: Fractions, fraction: Match): string | undefined {
  const [a, b] = fractionOf(fraction);
  return fractions[`${a}/${b}`];
}

/**
 * Whether a fraction is written as one: after a whole number (3 1/2), with a binary denominator before an inch mark
 * (3/8"), or with a common denominator (to 10, 12, 16, 100, 1000) before one of a language's words (1/3 of). A bare
 * 3/4 may be a date, a score, or a rating.
 * @param words lowercase; a word with an apostrophe matches up to it (d', dell')
 */
const NOT_NAMES: ReadonlySet<string> = new Set([
  "Norte",
  "Sur",
  "Sul",
  "Nord",
  "Sud",
  "Este",
  "Est",
  "Leste",
  "Oeste",
  "Oest",
  "Ovest",
  "Ouest",
  "Celsius",
  "Fahrenheit",
  "Kelvin",
]);

/**
 * Whether a degree sign, as a unit or an ordinal suffix, marks an ordinal (el 2° lugar, il 17° Meeting), not degrees (a 30°): one of a language's
 * articles before it, one of its nouns after it, or a name after it, which is not a compass point (33° Sur).
 */
export function isOrdinalDegree(unit: Match, articles: ReadonlySet<string>, nouns: ReadonlySet<string>): boolean {
  if ((unit.unit !== "°" && unit.suffix !== "°") || unit.numbers.length !== 1 || unit.numbers[0].fraction !== "") {
    return false;
  }
  const after = unit.context.after();
  const name = [...after].length >= 3 && isUpper([...after][0]) && !NOT_NAMES.has(after);
  return name || articles.has(unit.context.before()) || nouns.has(toLower(after));
}

export function isMarkedFraction(fraction: Match, words: ReadonlySet<string>): boolean {
  if (fraction.numbers.length === 3) {
    return true;
  }
  // An inch has binary fractions: 3/8"
  const [, b] = fractionOf(fraction);
  const tail = fraction.context.tail();
  const inch = tail[0] === '"' || (tail[0] === "'" && tail[1] === "'") || tail[0] === "″";
  if (inch && b <= 64n && b > 0n && (b & (b - 1n)) === 0n) {
    return true;
  }

  // A common denominator, not a year or a number of a series: 1/3 of, but 40/2023 du, 18/20 da
  let word = toLower(fraction.context.after());
  const apostrophe = word.indexOf("'");
  if (apostrophe !== -1) {
    word = word.slice(0, apostrophe + 1);
  }
  const common = b <= 10n || b === 12n || b === 16n || b === 100n || b === 1000n;
  return common && words.has(word);
}

// The suffix of a match with a space before it, to follow its reading; empty if the match has none.
export function spacedSuffix(match: Match): string {
  return match.suffix === "" ? "" : ` ${match.suffix}`;
}

// A value written as digits: 14n -> "14".
export function toDigits(value: bigint): string {
  return value.toString();
}

// prettier-ignore
const ROMAN_NUMERALS = [
  [1000n, "M"], [900n, "CM"], [500n, "D"], [400n, "CD"], [100n, "C"], [90n, "XC"], [50n, "L"], [40n, "XL"],
  [10n, "X"], [9n, "IX"], [5n, "V"], [4n, "IV"], [1n, "I"],
] as const;

const ROMAN_DIGITS: Readonly<Record<string, number>> = { I: 1, V: 5, X: 10, L: 50, C: 100, D: 500, M: 1000 };

// The value of a Roman numeral written in its canonical form (XIX, not XVIIII), up to 3999; undefined otherwise.
export function romanValue(numeral: string): bigint | undefined {
  if (numeral === "" || numeral.length > 15) {
    return undefined;
  }
  let value = 0;
  for (let i = 0; i < numeral.length; ++i) {
    const d = Object.hasOwn(ROMAN_DIGITS, numeral[i]) ? ROMAN_DIGITS[numeral[i]] : 0;
    if (d === 0) {
      return undefined;
    }
    const next =
      i + 1 < numeral.length && Object.hasOwn(ROMAN_DIGITS, numeral[i + 1]) ? ROMAN_DIGITS[numeral[i + 1]] : 0;
    value += i + 1 < numeral.length && d < next ? -d : d;
  }

  // Only the canonical spelling of its value: IV, not IIII; XC, not LXL
  let canonical = "";
  let rest = BigInt(Math.max(value, 0));
  for (const [numeralValue, symbol] of ROMAN_NUMERALS) {
    for (; rest >= numeralValue; rest -= numeralValue) {
      canonical += symbol;
    }
  }
  return value > 0 && value < 4000 && canonical === numeral ? BigInt(value) : undefined;
}

// What the words around a Roman numeral make it: the number of a name, or of a noun before or after it.
export const RomanRole = { NAME: 0, BEFORE: 1, AFTER: 2 } as const;
export type RomanRole = (typeof RomanRole)[keyof typeof RomanRole];

// The role of a Roman numeral match by a language's cues, or undefined if nothing marks it as a number.
export function romanRole(match: Match, cues: RomanCues): RomanRole | undefined {
  // A name is capitalized as written: Henry VIII, but not "the henry viii"
  const head = match.context.head();
  let start = head.length;
  while (start > 0 && isSpace(head[start - 1])) {
    start--;
  }
  while (start > 0 && !isSpace(head[start - 1])) {
    start--;
  }
  const capitalized = start < head.length && isUpper(head[start]);

  // A name or noun before it belongs to it only with nothing but spaces between: not "na piętrze, II liga"
  let last = head.length;
  while (last > 0 && isSpace(head[last - 1])) {
    last--;
  }
  const adjacent = last > 0 && isAlpha(head[last - 1]);

  // Paul V. Halvorsen: one letter with a dot before a capitalized word is an initial, not a regnal number
  const tail = match.context.tail();
  let next = tail.length > 0 && tail[0] === "." ? 1 : 0;
  while (next < tail.length && isSpace(tail[next])) {
    next++;
  }
  const dotted = match.suffix === "." || (tail.length > 0 && tail[0] === ".");
  const numeral = match.numbers[0].span;
  const initial = numeral.end - numeral.begin === 1 && dotted && next < tail.length && isUpper(tail[next]);

  const previous = match.context.before();
  if (adjacent && capitalized && !initial && cues.names.has(previous)) {
    return RomanRole.NAME;
  }
  if (adjacent && cues.before.has(previous)) {
    return RomanRole.BEFORE;
  }
  if (cues.after.has(toLower(match.context.after()))) {
    return RomanRole.AFTER;
  }
  return undefined;
}

// The parts of a code (MP3, H.264, F-16s): digits with their decimals, ".digits", runs of other characters, and single
// apostrophes, dots and hyphens.
export function codeParts(written: string): string[] {
  const code = textOf(written);
  const isMark = (c: string | undefined) => c === "'" || c === "." || c === "-";

  const parts: string[] = [];
  for (let i = 0; i < code.length;) {
    let end = i + 1;
    if (isDigit(code[i])) {
      // 264, 2.5
      end = digitsEnd(code, i);
      if (end + 1 < code.length && code[end] === "." && isDigit(code[end + 1])) {
        end = digitsEnd(code, end + 1);
      }
    } else if (code[i] === "." && i + 1 < code.length && isDigit(code[i + 1])) {
      // .264
      end = digitsEnd(code, i + 1);
    } else if (!isMark(code[i])) {
      while (end < code.length && !isDigit(code[end]) && !isMark(code[end])) {
        end++;
      }
    }
    parts.push(str(code, i, end));
    i = end;
  }
  return parts;
}

const SINGLE_PRIMES = "'′’´";
const CLOSING_QUOTES = '"”»';

// The length of a mark of seconds or inches at a position: 1 (", ″, ”), 2 ('', ’’), or 0 if none stands there.
function markLength(text: Text, position: number): number {
  if (position < text.length && contains('"″”', text[position])) {
    return 1;
  }
  const two =
    position + 1 < text.length &&
    contains(SINGLE_PRIMES, text[position]) &&
    contains(SINGLE_PRIMES, text[position + 1]);
  return two ? 2 : 0;
}

// The quotations open before a mark: curly ones („ “ « … ” ») counted, a straight one toggled.
interface Quotes {
  curly: number;
  straight: boolean;
}

// Whether the mark at a position, right after a digit, marks inches or seconds, given the quotations open before it
function inchMarkIn(text: Text, position: number, quotes: Quotes): boolean {
  const next = position + markLength(text, position);
  if (next < text.length && contains(CLOSING_QUOTES, text[next])) {
    return true; // 55"": inches, then the quotation mark
  }
  // The number before the mark, with its decimals: 10.1"
  let start = position;
  while (
    start > 0 &&
    (isDigit(text[start - 1]) ||
      ((text[start - 1] === "." || text[start - 1] === ",") &&
        start > 1 &&
        isDigit(text[start - 2]) &&
        isDigit(text[start])))
  ) {
    start--;
  }
  const feet = start > 0 && isSpace(text[start - 1]) ? start - 1 : start;
  if (feet > 1 && contains(SINGLE_PRIMES, text[feet - 1]) && isDigit(text[feet - 2])) {
    return true; // 5'5": the inches of feet and inches
  }
  if (quotes.curly > 0 || quotes.straight) {
    return false; // "Redmi 7", „7×7″: the mark closes the quotation
  }
  if (text[position] === "″") {
    return true;
  }
  // Redmi 7”: a small whole number after a capitalized name (not an acronym: UHD 110'') is a model, and the mark
  // closes a quotation opened before the text
  const digits = text.slice(start, position);
  let wordEnd = start;
  while (wordEnd > 0 && isSpace(text[wordEnd - 1])) {
    wordEnd--;
  }
  let wordStart = wordEnd;
  while (wordStart > 0 && isAlpha(text[wordStart - 1])) {
    wordStart--;
  }
  const name = text.slice(wordStart, wordEnd);
  const named = wordEnd < start && name.length > 0 && isUpper(name[0]) && name.some((c) => isLower(c));
  const small =
    digits.length > 0 &&
    digits.length <= 2 &&
    digits.every((c) => isDigit(c)) &&
    (digits.length === 1 || digits[0] === "1" || digits[0] === "0");
  return !(named && small);
}

// The quotations open before a position, and whether the mark there (if any) marks inches or seconds
function scanQuotes(text: Text, position: number): [Quotes, boolean] {
  const quotes: Quotes = { curly: 0, straight: false };
  for (let i = 0; i < position;) {
    const c = text[i];
    const length = markLength(text, i);
    const digit = i > 0 && isDigit(text[i - 1]);
    if (length > 0 && digit) {
      if (!inchMarkIn(text, i, quotes)) {
        // „7×7″, "Redmi 7": the mark closes the open quotation
        if (quotes.curly > 0) {
          quotes.curly--;
        } else {
          quotes.straight = false;
        }
      }
      i += length;
      continue;
    }
    if (c === "„" || c === "«" || (c === "“" && quotes.curly === 0)) {
      quotes.curly++;
    } else if ((c === "“" || c === "”" || c === "»" || c === "″") && quotes.curly > 0) {
      quotes.curly--;
    } else if ((c === '"' || length === 2) && (!digit || quotes.straight)) {
      quotes.straight = !quotes.straight; // also ``text`` as two apostrophes
    }
    i += length === 2 ? 2 : 1;
  }
  const inch =
    position > 0 &&
    position < text.length &&
    markLength(text, position) > 0 &&
    isDigit(text[position - 1]) &&
    inchMarkIn(text, position, quotes);
  return [quotes, inch];
}

/**
 * Whether the double mark at a position right after a number (", ″, ”, '') marks inches or seconds rather than a
 * quotation: a mark another quotation mark follows (55""), and the inches of feet and inches (5'5"); else only with no
 * quotation open before it („7×7″ closes one) and, but for ″, no name before the number (27" monitor, but Redmi 7”).
 */
export function isInchMark(text: Text, position: number): boolean {
  return scanQuotes(text, position)[1];
}

/** Whether a double quotation is open before a position: its quotation marks counted, inch and seconds marks skipped. */
export function inDoubleQuotation(text: Text, position: number): boolean {
  const [quotes] = scanQuotes(text, position);
  return quotes.curly > 0 || quotes.straight;
}
