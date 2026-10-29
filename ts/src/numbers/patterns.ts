import {
  digitsEnd,
  isClearAfter,
  isClearBefore,
  isDigitAfter,
  isSentenceEnd,
  keysAt,
  skipSpace,
  type Keys,
} from "../text/scan.ts";
import { contains, size, startsWith, str, type Text } from "../text/strings.ts";
import { isAlnum, isAlpha, isDigit, isLatin, isSpace, isWord } from "../text/unicode.ts";
import type { DateFormat, Notation, PhoneFormat, SuffixFormat } from "./formats.ts";
import { suffixFormat } from "./formats.ts";
import { Kind, match, numeral, type Match, type Numeral } from "./match.ts";
import { digitsAt, numbersAt, romanValue, toDigits, value } from "./scanning.ts";

/**
 * A pattern of a numeric expression. It finds the structure of a match only; how to read it is a language's decision.
 */
export abstract class Pattern {
  readonly kind: Kind;

  constructor(kind: Kind) {
    this.kind = kind;
  }

  /**
   * Matches the expression starting at a position, if there is one.
   * @param text the text, as rewritten by the patterns before.
   * @param position where the expression has to start.
   */
  abstract match(text: Text, position: number): Match | undefined;
}

// What may not stand right before a number, besides letters and digits.
const NUMBER_BOUNDARIES = ".,";

// What may not stand right before an amount: also a hyphen, as in i9-14900K.
const AMOUNT_BOUNDARIES = ".,-";

// Value of a short run of digits (a day, a month or an hour).
function smallValue(digits: string): number {
  return digits === "" ? 0 : Number(digits);
}

// The numbers of an amount at a position - a range (10-15) or a single number - in the order a regex tries them.
function amountsAt(text: Text, position: number, notation: Notation): Numeral[][] {
  const amounts: Numeral[][] = [];
  if (!isClearBefore(text, position, AMOUNT_BOUNDARIES)) {
    return amounts;
  }

  const firsts = numbersAt(text, position, notation.groupSeparators, notation.decimalSeparators);
  for (const first of firsts) {
    const hyphen = first.span.end;
    if (hyphen < text.length && text[hyphen] === "-") {
      for (const last of numbersAt(text, hyphen + 1, notation.groupSeparators, notation.decimalSeparators)) {
        amounts.push([first, last]);
      }
    }
  }
  for (const number of firsts) {
    amounts.push([number]);
  }

  // .5 kg: a fraction without its integer part
  if (
    firsts.length === 0 &&
    (position === 0 || isSpace(text[position - 1])) && // not a sentence dot: (jury).1° Lugar
    text[position] === "." &&
    contains(notation.decimalSeparators, ".") &&
    isDigitAfter(text, position + 1)
  ) {
    const end = digitsEnd(text, position + 1);
    amounts.push([
      numeral({ span: { begin: position, end }, fraction: str(text, position + 1, end), decimalMark: "." }),
    ]);
  }
  return amounts;
}

// Keys of a single capital letter (W, V), or all the others.
function singleLetters(keys: Keys, single: boolean): string[] {
  return keys.filter((key) => (key.length === 1 && key >= "A" && key <= "Z") === single);
}

// ----------------------------
// ----- Generic patterns -----
// ----------------------------

// The generic patterns of a language, which read its Notation.
export function genericPatterns(notation: Notation): Pattern[] {
  const patterns: Pattern[] = [
    new IsoDatePattern(),
    new InternationalPhonePattern(),
    new VersionPattern(notation),
    new DifferencePattern(),
    new SumPattern(),
    new RangePattern(),
    new DimensionPattern(notation),
  ];
  patterns.push(new SignPattern());
  patterns.push(new RatingPattern(notation));
  if (notation.fractions) {
    patterns.push(new FractionPattern());
  }
  patterns.push(new TimePattern(notation));
  if (notation.timeSuffixes.length > 0) {
    patterns.push(new HourPattern(notation));
  }
  patterns.push(
    new ScorePattern(),
    new PrefixedMoneyPattern(notation),
    new MoneyPattern(notation),
    new ScalePattern(notation),
    new PercentPattern(notation),
    new UnitPattern(notation),
    new CompoundPattern(notation),
    new CodePattern(),
    new NumberPattern(notation),
  );
  return patterns;
}

// 2024-05-12: an ISO date. Its numbers are ordered as day, month, year.
class IsoDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,/-") || digitsEnd(text, position) !== position + 4) {
      return undefined;
    }

    // YYYY-MM-DD, with months and days of one or two digits
    const parts = [digitsAt(text, position)];
    for (let p = position + 4; parts.length < 3;) {
      const end = digitsEnd(text, p + 1);
      if (p >= text.length || text[p] !== "-" || end === p + 1 || end > p + 3) {
        return undefined;
      }
      parts.push(digitsAt(text, p + 1));
      p = end;
    }

    const end = parts[2].span.end;
    const month = smallValue(parts[1].integer);
    const day = smallValue(parts[2].integer);
    if (!isClearAfter(text, end) || isDigitAfter(text, end, ".,/-") || month < 1 || month > 12 || day < 1 || day > 31) {
      return undefined;
    }
    return match({
      kind: Kind.DATE,
      span: { begin: position, end },
      numbers: [parts[2], parts[1], parts[0]],
      separator: "-",
    });
  }
}

// 3.8.10, 10.0.0.51; a chain of thousands groups (1.000.000) is left to numbers.
class VersionPattern extends Pattern {
  readonly #group: string;

  constructor(notation: Notation) {
    super(Kind.VERSION);
    this.#group = notation.groupSeparators;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, NUMBER_BOUNDARIES) || !isDigit(text[position])) {
      return undefined;
    }

    const parts = [digitsAt(text, position)];
    for (
      let last = parts[0];
      last.span.end + 1 < text.length && text[last.span.end] === "." && isDigit(text[last.span.end + 1]);
    ) {
      last = digitsAt(text, last.span.end + 1);
      parts.push(last);
    }

    const end = parts[parts.length - 1].span.end;
    if (parts.length < 3 || !isClearAfter(text, end)) {
      return undefined;
    }

    // 1.000.000 is a number
    const thousands =
      contains(this.#group, ".") &&
      parts[0].integer.length <= 3 &&
      parts.slice(1).every((part) => part.integer.length === 3);
    if (thousands) {
      return undefined;
    }
    return match({ kind: Kind.VERSION, span: { begin: position, end }, numbers: parts, separator: "." });
  }
}

// The " - " of 5 - 3.
class DifferencePattern extends Pattern {
  constructor() {
    super(Kind.DIFFERENCE);
  }

  match(text: Text, position: number): Match | undefined {
    if (
      position === 0 ||
      position + 3 >= text.length ||
      str(text, position, position + 3) !== " - " ||
      !isDigit(text[position - 1]) ||
      !isDigit(text[position + 3])
    ) {
      return undefined;
    }
    return match({ kind: Kind.DIFFERENCE, span: { begin: position, end: position + 3 }, separator: "-" });
  }
}

// The "+" of 2 + 2 or 90+6, with the spaces around it.
class SumPattern extends Pattern {
  constructor() {
    super(Kind.SUM);
  }

  match(text: Text, position: number): Match | undefined {
    if (position === 0 || !isDigit(text[position - 1])) {
      return undefined;
    }
    let plus = position;
    while (plus < text.length && isSpace(text[plus])) {
      plus++;
    }
    let end = plus + 1;
    while (end < text.length && isSpace(text[end])) {
      end++;
    }
    if (plus >= text.length || text[plus] !== "+" || end >= text.length || !isDigit(text[end])) {
      return undefined;
    }
    return match({ kind: Kind.SUM, span: { begin: position, end }, separator: "+" });
  }
}

// The "-" of 10-15 and 2020-2022. Its numbers are the digits on both sides.
class RangePattern extends Pattern {
  constructor() {
    super(Kind.RANGE);
  }

  match(text: Text, position: number): Match | undefined {
    if (position === 0 || !isDigit(text[position - 1])) {
      return undefined;
    }

    // A hyphen (normalization writes dashes between numbers as one: 2020 – 2022)
    const end = position + 1;
    if (text[position] !== "-" || !isDigitAfter(text, end)) {
      return undefined;
    }

    // The digits on both sides
    let start = position;
    while (start > 0 && isDigit(text[start - 1])) {
      start--;
    }
    // Not a link of a chain of numbers or single letters: 4-1-4-1, b-c-1-5-b, but 14-29-jährigen
    const link = (letter: number, outer: number): boolean =>
      isDigit(text[letter]) || (isAlpha(text[letter]) && (outer >= text.length || !isAlnum(text[outer])));
    const after = digitsEnd(text, end);
    if (
      (start > 1 && text[start - 1] === "-" && link(start - 2, start - 2 > 0 ? start - 3 : text.length)) ||
      (after + 1 < text.length && text[after] === "-" && link(after + 1, after + 2))
    ) {
      return undefined;
    }
    return match({
      kind: Kind.RANGE,
      span: { begin: position, end },
      numbers: [digitsAt(text, start), digitsAt(text, end)],
      separator: "-",
    });
  }
}

// The "x" of 1920x1080, or of 3 m x 4 m (the units are left to UnitPattern).
class DimensionPattern extends Pattern {
  readonly #units: Keys;

  constructor(notation: Notation) {
    super(Kind.DIMENSION);
    this.#units = singleLetters(notation.units, false);
  }

  match(text: Text, position: number): Match | undefined {
    // A number on the left, possibly followed by a unit: 3x4, 3 m x 4
    let numberEnd = 0;
    if (position > 0 && isDigit(text[position - 1])) {
      numberEnd = position;
    } else {
      for (const unit of this.#units) {
        const length = size(unit);
        if (position < length || !startsWith(text, position - length, unit)) {
          continue;
        }
        let start = position - length;
        start = start > 0 && isSpace(text[start - 1]) ? start - 1 : start;
        if (start > 0 && isDigit(text[start - 1])) {
          numberEnd = start;
          break;
        }
      }
    }

    // Not a hexadecimal number: 0x1F
    if (numberEnd === 0 || (text[numberEnd - 1] === "0" && isClearBefore(text, numberEnd - 1))) {
      return undefined;
    }

    const x = skipSpace(text, position);
    if (x >= text.length || (text[x] !== "x" && text[x] !== "×")) {
      return undefined;
    }
    const end = skipSpace(text, x + 1);
    if (!isDigitAfter(text, end)) {
      return undefined;
    }
    return match({ kind: Kind.DIMENSION, span: { begin: position, end }, separator: "x" });
  }
}

// 3/4, and 3 1/2: its numbers are then the whole number, the numerator, and the denominator.
export class FractionPattern extends Pattern {
  constructor() {
    super(Kind.FRACTION);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, "./")) {
      return undefined;
    }

    // 3 1/2, -4 1/3, 1 5/3: a whole number of up to three digits, possibly signed, a space, and a fraction
    const whole = text[position] === "-" ? position + 1 : position;
    const wholeEnd = digitsEnd(text, whole);
    if (
      wholeEnd > whole &&
      wholeEnd <= whole + 3 &&
      text[whole] !== "0" &&
      isClearBefore(text, position, "./-:,") &&
      wholeEnd + 1 < text.length &&
      text[wholeEnd] === " "
    ) {
      const fraction = simpleFraction(text, wholeEnd + 1);
      if (fraction && fraction.numbers[0].integer[0] !== "0" && value(fraction.numbers[1]) > 1n) {
        return {
          ...fraction,
          span: { begin: position, end: fraction.span.end },
          numbers: [digitsAt(text, whole), ...fraction.numbers],
        };
      }
    }
    return simpleFraction(text, position);
  }
}

// A fraction without a whole number: 3/4
function simpleFraction(text: Text, position: number): Match | undefined {
  const slash = digitsEnd(text, position);
  if (slash === position || slash >= text.length || text[slash] !== "/") {
    return undefined;
  }

  const end = digitsEnd(text, slash + 1);
  if (end === slash + 1 || !isClearAfter(text, end, "/") || isDigitAfter(text, end, ".,")) {
    return undefined;
  }
  return match({
    kind: Kind.FRACTION,
    span: { begin: position, end },
    numbers: [digitsAt(text, position), digitsAt(text, slash + 1)],
    separator: "/",
  });
}

// 14:30:05: hours, minutes, and seconds
export class SecondsPattern extends Pattern {
  readonly #suffixes: Keys;
  readonly #loose: boolean;

  /**
   * @param suffixes words which may follow it, after an optional space, as part of it: 02:02:23 Uhr.
   * @param loose takes seconds above 59, but no time right after a colon.
   */
  constructor(suffixes: Keys = [], loose = false) {
    super(Kind.TIME);
    this.#suffixes = suffixes;
    this.#loose = loose;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, this.#loose ? ".,:" : ".,")) {
      return undefined;
    }
    const time = match({ kind: Kind.TIME, separator: ":" });
    let end = position;
    for (let i = 0; i < 3; ++i) {
      if (i > 0) {
        if (end >= text.length || text[end] !== ":") {
          return undefined;
        }
        end++;
      }
      const length = digitsEnd(text, end) - end;
      if (length < (i === 0 ? 1 : 2) || length > 2) {
        return undefined;
      }
      time.numbers.push(digitsAt(text, end));
      end += length;
    }
    if (!isClearAfter(text, end) || isDigitAfter(text, end, ".,:") || (!this.#loose && value(time.numbers[2]) > 59n)) {
      return undefined;
    }
    time.span = { begin: position, end };
    const word = skipSpace(text, end);
    for (const key of keysAt(text, word, this.#suffixes)) {
      if (isClearAfter(text, word + key.length)) {
        time.span = { begin: position, end: word + key.length };
        time.suffix = key;
        break;
      }
    }
    return time;
  }
}

// XIXe, XIIवीं: an ordinal written in Roman numerals, with a suffix; one letter only if it is I, V or X.
export class RomanOrdinalPattern extends Pattern {
  readonly #suffixes: Keys;

  constructor(suffixes: Keys) {
    super(Kind.ORDINAL);
    this.#suffixes = suffixes;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position)) {
      return undefined;
    }
    let end = position;
    while (end < text.length && contains("IVXLCDM", text[end])) {
      end++;
    }
    const written = str(text, position, end);
    const roman = romanValue(written);
    if (roman === undefined || (written.length === 1 && written !== "I" && written !== "V" && written !== "X")) {
      return undefined;
    }
    for (const suffix of keysAt(text, end, this.#suffixes)) {
      if (isClearAfter(text, end + suffix.length)) {
        return match({
          kind: Kind.ORDINAL,
          span: { begin: position, end: end + suffix.length },
          numbers: [numeral({ span: { begin: position, end }, integer: toDigits(roman) })],
          suffix,
        });
      }
    }
    return undefined;
  }
}

// 1,000th, 1.000.º, 1 001.ᵉʳ: an ordinal of a grouped number
export class GroupedOrdinalPattern extends Pattern {
  readonly #suffixes: Keys;
  readonly #groupSeparators: string;

  constructor(suffixes: Keys, groupSeparators: string) {
    super(Kind.ORDINAL);
    this.#suffixes = suffixes;
    this.#groupSeparators = groupSeparators;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,")) {
      return undefined;
    }
    for (const number of numbersAt(text, position, this.#groupSeparators, "")) {
      if (!number.grouped) {
        continue;
      }
      for (const key of keysAt(text, number.span.end, this.#suffixes)) {
        if (isClearAfter(text, number.span.end + size(key))) {
          return match({
            kind: Kind.ORDINAL,
            span: { begin: position, end: number.span.end + size(key) },
            numbers: [number],
            suffix: key,
          });
        }
      }
    }
    return undefined;
  }
}

// The "/" of 4,2/5 and 4.5/10: a rating, a decimal out of 5, 10, 20, or 100.
class RatingPattern extends Pattern {
  readonly #decimals: string;

  constructor(notation: Notation) {
    super(Kind.RATING);
    this.#decimals = notation.decimalSeparators;
  }

  match(text: Text, position: number): Match | undefined {
    if (text[position] !== "/" || position < 3 || !isDigit(text[position - 1])) {
      return undefined;
    }
    // A decimal before the slash: 4,2 or 4.5
    let fraction = position;
    while (fraction > 0 && isDigit(text[fraction - 1])) {
      fraction--;
    }
    let whole = fraction - 1;
    if (fraction < 2 || !contains(this.#decimals, text[whole]) || !isDigit(text[whole - 1])) {
      return undefined;
    }
    while (whole > 0 && isDigit(text[whole - 1])) {
      whole--;
    }
    if (!isClearBefore(text, whole, ".,/")) {
      return undefined;
    }
    // 5, 10, 20, or 100 after it
    const end = digitsEnd(text, position + 1);
    const denominator = str(text, position + 1, end);
    if (
      !["5", "10", "20", "100"].includes(denominator) ||
      !isClearAfter(text, end, "/") ||
      isDigitAfter(text, end, ".,")
    ) {
      return undefined;
    }
    return match({ kind: Kind.RATING, span: { begin: position, end: position + 1 }, separator: "/" });
  }
}

// The minus of -5.
class SignPattern extends Pattern {
  constructor() {
    super(Kind.SIGN);
  }

  match(text: Text, position: number): Match | undefined {
    if (
      (text[position] !== "-" && text[position] !== "−") ||
      !isClearBefore(text, position, NUMBER_BOUNDARIES) ||
      !isDigitAfter(text, position + 1)
    ) {
      return undefined;
    }
    return match({ kind: Kind.SIGN, span: { begin: position, end: position + 1 } });
  }
}

// 18:30, 10.30 pm, 21h30: hours and minutes, with an optional suffix.
class TimePattern extends Pattern {
  readonly #separators: string;
  readonly #suffixes: Keys;
  readonly #measures: Keys;

  constructor(notation: Notation) {
    super(Kind.TIME);
    this.#separators = notation.timeSeparators;
    this.#suffixes = notation.timeSuffixes;
    this.#measures = [...singleLetters(notation.units, false), "min"];
  }

  match(text: Text, position: number): Match | undefined {
    const hoursEnd = digitsEnd(text, position);
    if (
      !(isClearBefore(text, position, NUMBER_BOUNDARIES) || this.#afterSuffix(text, position)) ||
      hoursEnd === position ||
      hoursEnd > position + 2 ||
      hoursEnd >= text.length
    ) {
      return undefined;
    }

    // A separator (18:30, 18.30), or an "h" with optional spaces around it (18h30, 18 h 30)
    let separator = "";
    let minutes = hoursEnd + 1;
    const h = skipSpace(text, hoursEnd);
    if (text[hoursEnd] !== "h" && contains(this.#separators, text[hoursEnd])) {
      separator = text[hoursEnd];
    } else if (contains(this.#separators, "h") && h < text.length && text[h] === "h") {
      separator = "h";
      minutes = skipSpace(text, h + 1);
    }
    if (separator === "" || digitsEnd(text, minutes) < minutes + 2) {
      return undefined;
    }

    const time = match({
      kind: Kind.TIME,
      numbers: [
        digitsAt(text, position),
        numeral({ span: { begin: minutes, end: minutes + 2 }, integer: str(text, minutes, minutes + 2) }),
      ],
      separator,
    });

    // With a suffix first (10:30 pm), then without it
    const suffix = skipSpace(text, minutes + 2);
    for (const key of keysAt(text, suffix, this.#suffixes)) {
      if (this.#endsAt(text, suffix + size(key), true)) {
        time.span = { begin: position, end: suffix + size(key) };
        time.suffix = key;
        return time;
      }
    }
    if (this.#endsAt(text, minutes + 2)) {
      time.span = { begin: position, end: minutes + 2 };
      return time;
    }
    return undefined;
  }

  // Whether a comma before a position follows a suffix: 11:20AM,2:40PM
  #afterSuffix(text: Text, position: number): boolean {
    if (position === 0 || text[position - 1] !== ",") {
      return false;
    }
    return this.#suffixes.some(
      (key) => size(key) < position && str(text, position - 1 - size(key), position - 1) === key,
    );
  }

  // Whether a time may end at a position: not a longer number, nor a measure (1.30 min), unless a suffix closes it
  #endsAt(text: Text, position: number, suffixed = false): boolean {
    if (!isClearAfter(text, position) || (!suffixed && isDigitAfter(text, position, ".,:"))) {
      return false;
    }
    for (const start of [position, skipSpace(text, position)]) {
      for (const key of keysAt(text, start, this.#measures)) {
        if (isClearAfter(text, start + size(key))) {
          return false;
        }
      }
    }
    return true;
  }
}

// 7pm, 18 Uhr, 21h: an hour marked by its suffix.
class HourPattern extends Pattern {
  readonly #suffixes: Keys;

  constructor(notation: Notation) {
    super(Kind.TIME);
    this.#suffixes = notation.timeSuffixes;
  }

  match(text: Text, position: number): Match | undefined {
    const hoursEnd = digitsEnd(text, position);
    if (!isClearBefore(text, position, NUMBER_BOUNDARIES) || hoursEnd === position || hoursEnd > position + 2) {
      return undefined;
    }

    const suffix = skipSpace(text, hoursEnd);
    for (const key of keysAt(text, suffix, this.#suffixes)) {
      if (isClearAfter(text, suffix + size(key))) {
        return match({
          kind: Kind.TIME,
          span: { begin: position, end: suffix + size(key) },
          numbers: [digitsAt(text, position)],
          suffix: key,
        });
      }
    }
    return undefined;
  }
}

// The ":" of 2:1.
class ScorePattern extends Pattern {
  constructor() {
    super(Kind.SCORE);
  }

  match(text: Text, position: number): Match | undefined {
    if (position === 0 || text[position] !== ":" || !isDigit(text[position - 1]) || !isDigitAfter(text, position + 1)) {
      return undefined;
    }
    return match({ kind: Kind.SCORE, span: { begin: position, end: position + 1 }, separator: ":" });
  }
}

// $5, € 1,5 million: a currency symbol before an amount.
export class PrefixedMoneyPattern extends Pattern {
  readonly #notation: Notation;
  readonly #currencies: Keys; // Only the ones which are not words

  // words: currencies written as words which also precede an amount: USD 5
  constructor(notation: Notation, words: Keys = []) {
    super(Kind.MONEY);
    this.#notation = notation;
    this.#currencies = [...words, ...notation.currencies.filter((currency) => ![...currency].every((c) => isAlpha(c)))];
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position)) {
      return undefined;
    }

    for (const currency of keysAt(text, position, this.#currencies)) {
      const start = skipSpace(text, position + size(currency));
      const numbers = numbersAt(text, start, this.#notation.groupSeparators, this.#notation.decimalSeparators);
      if (numbers.length === 0) {
        continue;
      }

      // An optional scale after the amount: $1.5 million
      const money = match({
        kind: Kind.MONEY,
        span: { begin: position, end: numbers[0].span.end },
        numbers: [numbers[0]],
        unit: currency,
      });
      const scale = skipSpace(text, money.span.end);
      for (const key of keysAt(text, scale, this.#notation.scales)) {
        if (isClearAfter(text, scale + size(key))) {
          money.scale = key;
          money.span.end = scale + size(key);
          break;
        }
      }
      return money;
    }
    return undefined;
  }
}

// 50$, 12,99 zł, 10-15 €, 3 mln zł: an amount, possibly a range, followed by its currency.
class MoneyPattern extends Pattern {
  readonly #notation: Notation;

  constructor(notation: Notation) {
    super(Kind.MONEY);
    this.#notation = notation;
  }

  match(text: Text, position: number): Match | undefined {
    for (const numbers of amountsAt(text, position, this.#notation)) {
      const end = numbers[numbers.length - 1].span.end;

      // The currency, after an optional scale: 3 mln zł, 50$
      const currencyAt = (start: number, scale: string): Match | undefined => {
        for (const currency of keysAt(text, start, this.#notation.currencies)) {
          if (isClearAfter(text, start + size(currency))) {
            return match({
              kind: Kind.MONEY,
              span: { begin: position, end: start + size(currency) },
              numbers,
              unit: currency,
              scale,
            });
          }
        }
        return undefined;
      };

      const scale = skipSpace(text, end);
      for (const key of keysAt(text, scale, this.#notation.scales)) {
        const money = currencyAt(skipSpace(text, scale + size(key)), key);
        if (money) {
          return money;
        }
      }
      const money = currencyAt(skipSpace(text, end), "");
      if (money) {
        return money;
      }
    }
    return undefined;
  }
}

// 3 mln, 500k, 1.5 million
export class ScalePattern extends Pattern {
  readonly #notation: Notation;

  constructor(notation: Notation) {
    super(Kind.SCALE);
    this.#notation = notation;
  }

  match(text: Text, position: number): Match | undefined {
    for (const numbers of amountsAt(text, position, this.#notation)) {
      const scale = skipSpace(text, numbers[numbers.length - 1].span.end);
      for (const key of keysAt(text, scale, this.#notation.scales)) {
        if (isClearAfter(text, scale + size(key))) {
          return match({ kind: Kind.SCALE, span: { begin: position, end: scale + size(key) }, numbers, scale: key });
        }
      }
    }
    return undefined;
  }
}

// 20%, 10-15 %, 8%ige
class PercentPattern extends Pattern {
  readonly #notation: Notation;

  constructor(notation: Notation) {
    super(Kind.PERCENT);
    this.#notation = notation;
  }

  match(text: Text, position: number): Match | undefined {
    for (const numbers of amountsAt(text, position, this.#notation)) {
      const percent = skipSpace(text, numbers[numbers.length - 1].span.end);
      if (percent >= text.length || text[percent] !== "%") {
        continue;
      }

      // Letters right after it belong to it: 8%ige
      let end = percent + 1;
      while (end < text.length && isWord(text[end])) {
        end++;
      }
      return match({
        kind: Kind.PERCENT,
        span: { begin: position, end },
        numbers,
        suffix: str(text, percent + 1, end),
      });
    }
    return undefined;
  }
}

// 5 kg, 10-15 km, 230 V; a unit of a single capital letter only after a space.
class UnitPattern extends Pattern {
  readonly #notation: Notation;
  readonly #letters: Keys; // Units of a single capital letter (W, V)

  constructor(notation: Notation) {
    super(Kind.UNIT);
    this.#notation = { ...notation, units: singleLetters(notation.units, false) };
    this.#letters = singleLetters(notation.units, true);
  }

  match(text: Text, position: number): Match | undefined {
    for (const numbers of amountsAt(text, position, this.#notation)) {
      const end = numbers[numbers.length - 1].span.end;

      const unitAt = (start: number, units: Keys): Match | undefined => {
        for (const unit of keysAt(text, start, units)) {
          let unitEnd = start + size(unit);
          if (!isClearAfter(text, unitEnd, "'")) {
            continue;
          }
          // A dot closing both the unit and the text stays with the text: 5 Min.
          if (unit.endsWith(".") && isSentenceEnd(text, unitEnd)) {
            unitEnd--;
          }
          return match({ kind: Kind.UNIT, span: { begin: position, end: unitEnd }, numbers, unit });
        }
        return undefined;
      };

      // A unit after an optional space (5 kg, 5kg), or a single letter after a space (230 V)
      for (const start of [skipSpace(text, end), end]) {
        const unit = unitAt(start, this.#notation.units);
        if (unit) {
          return unit;
        }
      }
      if (end < text.length && isSpace(text[end])) {
        const unit = unitAt(end + 1, this.#letters);
        if (unit) {
          return unit;
        }
      }
    }
    return undefined;
  }
}

// 21-year-old, 45-metrowe: a number joined to a word by a hyphen.
class CompoundPattern extends Pattern {
  readonly #notation: Notation;

  constructor(notation: Notation) {
    super(Kind.COMPOUND);
    this.#notation = notation;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, AMOUNT_BOUNDARIES)) {
      return undefined;
    }

    for (const number of numbersAt(text, position, this.#notation.groupSeparators, this.#notation.decimalSeparators)) {
      const hyphen = number.span.end;
      if (hyphen + 1 >= text.length || text[hyphen] !== "-" || !isLatin(text[hyphen + 1])) {
        continue;
      }
      let end = hyphen + 1;
      while (end < text.length && isLatin(text[end])) {
        end++;
      }
      return match({
        kind: Kind.COMPOUND,
        span: { begin: position, end },
        numbers: [number],
        suffix: str(text, hyphen + 1, end),
      });
    }
    return undefined;
  }
}

// MP3, A380, COVID-19, H.264: a token mixing letters and digits.
class CodePattern extends Pattern {
  constructor() {
    super(Kind.CODE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, "'.-")) {
      return undefined;
    }

    // A token of word characters, apostrophes, dots and hyphens, with both a digit and a Latin letter
    const start = text[position] === "." ? position + 1 : position;
    let digit = false;
    let letter = false;
    for (
      let i = start;
      i < text.length && (isWord(text[i]) || text[i] === "'" || text[i] === "." || text[i] === "-");
      ++i
    ) {
      digit ||= isDigit(text[i]);
      letter ||= isLatin(text[i]);
    }
    if (!digit || !letter || start >= text.length || !isWord(text[start])) {
      return undefined;
    }

    // A dot only continues it before a digit: H.264
    let end = start + 1;
    while (
      end < text.length &&
      (isWord(text[end]) ||
        text[end] === "'" ||
        text[end] === "-" ||
        (text[end] === "." && isDigitAfter(text, end + 1)))
    ) {
      end++;
    }
    return match({ kind: Kind.CODE, span: { begin: position, end } });
  }
}

// 42, 1 000,5, .5: any other number.
class NumberPattern extends Pattern {
  readonly #notation: Notation;

  constructor(notation: Notation) {
    super(Kind.NUMBER);
    this.#notation = notation;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, NUMBER_BOUNDARIES)) {
      return undefined;
    }

    for (const number of numbersAt(text, position, this.#notation.groupSeparators, this.#notation.decimalSeparators)) {
      if (isClearAfter(text, number.span.end)) {
        return match({ kind: Kind.NUMBER, span: number.span, numbers: [number] });
      }
    }

    // .5: a fraction without its integer part
    if (text[position] === "." && contains(this.#notation.decimalSeparators, ".") && isDigitAfter(text, position + 1)) {
      const end = digitsEnd(text, position + 1);
      if (isClearAfter(text, end)) {
        const number = numeral({
          span: { begin: position, end },
          fraction: str(text, position + 1, end),
          decimalMark: ".",
        });
        return match({ kind: Kind.NUMBER, span: number.span, numbers: [number] });
      }
    }
    return undefined;
  }
}

// ---------------------------------
// ----- Configurable patterns -----
// ---------------------------------

// A numeric date. Its numbers are ordered as day, month and year (if written).
export class DatePattern extends Pattern {
  readonly #format: DateFormat;

  constructor(format: DateFormat) {
    super(Kind.DATE);
    this.#format = format;
  }

  match(text: Text, position: number): Match | undefined {
    const format = this.#format;
    const dayEnd = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, format.boundaries) ||
      dayEnd === position ||
      dayEnd > position + 2 ||
      dayEnd >= text.length ||
      !contains(format.separators, text[dayEnd])
    ) {
      return undefined;
    }

    const separator = text[dayEnd];
    const month = format.spaced ? skipSpace(text, dayEnd + 1) : dayEnd + 1;
    const monthEnd = digitsEnd(text, month);
    if (monthEnd === month || monthEnd > month + 2) {
      return undefined;
    }

    const date = match({ kind: Kind.DATE, numbers: [digitsAt(text, position), digitsAt(text, month)], separator });
    const d = smallValue(date.numbers[0].integer);
    const m = smallValue(date.numbers[1].integer);
    if (d < 1 || d > 31 || m < 1 || m > 12) {
      return undefined;
    }
    date.span.begin = position;

    // A closing separator always follows the month (1.5.), otherwise only before a year (12/05/2024)
    const closed = monthEnd < text.length && text[monthEnd] === separator;
    if (format.closingSeparator && !closed) {
      return undefined;
    }

    // With a year first, then (if allowed) without it
    if (closed) {
      const year = monthEnd + 1;
      const length = digitsEnd(text, year) - year;
      if (length === 4 || (length === 2 && format.shortYear)) {
        const dated = { ...date, span: { ...date.span }, numbers: [...date.numbers, digitsAt(text, year)] };
        const result = this.#complete(text, dated, year + length);
        if (result) {
          return result;
        }
      }
    }
    if (format.yearRequired) {
      return undefined;
    }
    return this.#complete(text, date, format.closingSeparator ? monthEnd + 1 : monthEnd);
  }

  // The date without (zero) or with its year, if it ends at a valid position
  #complete(text: Text, date: Match, position: number): Match | undefined {
    const format = this.#format;

    // An abbreviation after the year: 2024 r.
    if (date.numbers.length === 3) {
      const suffix = skipSpace(text, position);
      for (const key of keysAt(text, suffix, format.suffixes)) {
        const end = suffix + size(key);
        if (isClearAfter(text, end) && !isDigitAfter(text, end, format.boundaries)) {
          // A dot closing both the abbreviation and the sentence stays with the sentence
          date.suffix = key;
          date.span.end = key.endsWith(".") && isSentenceEnd(text, end, true) ? end - 1 : end;
          return date;
        }
      }
    }

    if (!isClearAfter(text, position) || isDigitAfter(text, position, format.boundaries)) {
      return undefined;
    }
    date.span.end = position;
    return date;
  }
}

// +48 600 123 456, +1 (555) 123-4567: a phone number with a country code. Its numbers are the code, then the groups.
class InternationalPhonePattern extends Pattern {
  constructor() {
    super(Kind.INTERNATIONAL_PHONE);
  }

  match(text: Text, position: number): Match | undefined {
    if (text[position] !== "+" || !isClearBefore(text, position)) {
      return undefined;
    }

    // 1. The country code: one to three digits
    const country = digitsAt(text, position + 1);
    if (country.integer === "" || country.integer.length > 3) {
      return undefined;
    }
    const phone = match({ kind: Kind.INTERNATIONAL_PHONE, numbers: [country] });
    let end = country.span.end;
    let length = country.integer.length;

    // 2. Groups of digits, each after a space, a hyphen or a dot, or in parentheses: +44 (0)20 7946-0958
    for (let closed = false; end < text.length;) {
      const next = contains(" -.", text[end]) ? end + 1 : end;
      const parenthesized = next < text.length && text[next] === "(";
      const group = digitsAt(text, next + (parenthesized ? 1 : 0));
      const groupEnd = group.span.end;
      if (
        group.integer === "" ||
        (next === end && !parenthesized && !closed) ||
        (parenthesized && (groupEnd >= text.length || text[groupEnd] !== ")"))
      ) {
        break;
      }
      phone.numbers.push(group);
      end = groupEnd + (parenthesized ? 1 : 0);
      length += group.integer.length;
      closed = parenthesized; // (0)20: a group may follow a closing parenthesis directly
    }

    // 3. A whole number: 8 to 15 digits, as numbering plans allow
    if (
      phone.numbers.length < 2 ||
      length < 8 ||
      length > 15 ||
      !isClearAfter(text, end) ||
      isDigitAfter(text, end, ".,")
    ) {
      return undefined;
    }
    phone.span = { begin: position, end };
    return phone;
  }
}

// A phone number. Its numbers are its groups of digits.
export class PhonePattern extends Pattern {
  readonly #format: PhoneFormat;

  constructor(format: PhoneFormat) {
    super(Kind.PHONE);
    this.#format = format;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, this.#format.boundaries)) {
      return undefined;
    }

    // With its optional prefix first (1-555-...), then without it
    if (this.#format.prefix !== "" && startsWith(text, position, this.#format.prefix)) {
      const phone = this.#groupsAt(text, position, position + size(this.#format.prefix));
      if (phone) {
        return phone;
      }
    }
    return this.#groupsAt(text, position, position);
  }

  // The phone number starting with its first group at a position
  #groupsAt(text: Text, start: number, position: number): Match | undefined {
    const format = this.#format;
    const phone = match({ kind: Kind.PHONE });

    // (555) 123-4567: the parenthesis opens the first group
    const parenthesized = format.parenthesized;
    if (parenthesized) {
      if (position >= text.length || text[position] !== "(") {
        return undefined;
      }
      position++;
    }

    for (let i = 0; i < format.groups.length; ++i) {
      // The separator before a group, which a closing parenthesis replaces
      const afterParenthesis = i === 1 && parenthesized;
      if (i > 0 && !afterParenthesis && position < text.length && contains(format.separators, text[position])) {
        phone.separator = phone.separator || text[position];
        position++;
      } else if (i > 0 && !afterParenthesis && !format.optionalSeparators) {
        return undefined;
      }
      if (i === 0 && format.leadingZero && (position >= text.length || text[position] !== "0")) {
        return undefined;
      }

      // Without separators, a run of digits splits into groups of their longest length
      const [shortest, longest] = format.groups[i];
      let length = digitsEnd(text, position) - position;
      length = format.optionalSeparators ? Math.min(length, longest) : length;
      if (length < shortest || length > longest) {
        return undefined;
      }
      phone.numbers.push(
        numeral({ span: { begin: position, end: position + length }, integer: str(text, position, position + length) }),
      );
      position += length;

      if (i === 0 && parenthesized) {
        if (position >= text.length || text[position] !== ")") {
          return undefined;
        }
        position = skipSpace(text, position + 1);
        if (position < text.length && text[position] === "-") {
          position++; // (123)-123-5678
        }
      }
    }

    if (!isClearAfter(text, position, format.closing) || isDigitAfter(text, position, ".,")) {
      return undefined;
    }
    phone.span = { begin: start, end: position };
    return phone;
  }
}

// Digits followed by a suffix from a table.
export class SuffixPattern extends Pattern {
  readonly #suffixes: Keys;
  readonly #format: SuffixFormat;

  constructor(kind: Kind, suffixes: Keys, format: SuffixFormat = suffixFormat()) {
    super(kind);
    this.#suffixes = suffixes;
    this.#format = format;
  }

  match(text: Text, position: number): Match | undefined {
    const format = this.#format;
    if (!isClearBefore(text, position, format.boundaries)) {
      return undefined;
    }

    // '90s: an apostrophe before the digits
    const digits = format.apostrophes && text[position] === "'" ? position + 1 : position;
    const end = digitsEnd(text, digits);
    if (end === digits) {
      return undefined;
    }

    // 90's, 1 º: an apostrophe or a space before the suffix
    let suffix = end;
    if (format.apostrophes && suffix < text.length && text[suffix] === "'") {
      suffix++;
    }
    if (format.space) {
      suffix = skipSpace(text, suffix);
    }

    for (const key of keysAt(text, suffix, this.#suffixes)) {
      let suffixEnd = suffix + size(key);
      if (format.wordEnd && !isClearAfter(text, suffixEnd)) {
        continue;
      }
      // A dot closing both the suffix and the sentence stays with the sentence: 2020 r.
      if (format.sentenceDot && key.endsWith(".") && isSentenceEnd(text, suffixEnd, true)) {
        suffixEnd--;
      }
      return match({
        kind: this.kind,
        span: { begin: position, end: suffixEnd },
        numbers: [digitsAt(text, digits)],
        suffix: key,
      });
    }
    return undefined;
  }
}

// A Roman numeral as a word of its own: VIII, XXI, IV.; one letter only if it is I, V or X. Its number holds its value.
// Whether it is a number, and how it reads, is left to the reading (see romanRole): CD and MX are mostly not.
export class RomanPattern extends Pattern {
  readonly #dot: boolean;
  readonly #suffixes: Keys;

  /**
   * @param dot also takes a dot closing it, as German ordinals do (Ludwig XIV. regierte), unless the text ends there.
   * @param suffixes words which may follow it, after an optional space, as part of it: XIX w.
   */
  constructor(dot = false, suffixes: Keys = []) {
    super(Kind.ROMAN);
    this.#dot = dot;
    this.#suffixes = suffixes;
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, "'.-/")) {
      return undefined;
    }
    let end = position;
    while (end < text.length && contains("IVXLCDM", text[end])) {
      end++;
    }
    // Not a part of a longer word or code (XX-lecie, CD-ROM, MXene), unless a suffix follows: Iº is a letter
    const marked = keysAt(text, end, this.#suffixes).length > 0;
    if (end === position || (!isClearAfter(text, end, "'-/") && !marked)) {
      return undefined;
    }
    const written = str(text, position, end);
    const value = romanValue(written);
    if (value === undefined || (written.length === 1 && written !== "I" && written !== "V" && written !== "X")) {
      return undefined;
    }

    const roman = match({
      kind: Kind.ROMAN,
      span: { begin: position, end },
      numbers: [numeral({ span: { begin: position, end }, integer: toDigits(value) })],
    });
    if (this.#dot && end < text.length && text[end] === "." && !isSentenceEnd(text, end + 1)) {
      roman.span.end = end + 1;
      roman.suffix = ".";
      return roman;
    }

    // XIX w.: a dot closing both the suffix and the sentence stays with the sentence
    const suffix = skipSpace(text, end);
    for (const key of keysAt(text, suffix, this.#suffixes)) {
      const suffixEnd = suffix + size(key);
      if (isClearAfter(text, suffixEnd)) {
        roman.span.end = key.endsWith(".") && isSentenceEnd(text, suffixEnd, true) ? suffixEnd - 1 : suffixEnd;
        roman.suffix = key;
        return roman;
      }
    }
    return roman;
  }
}

// 3. Platz, 55. sesję: an ordinal written as digits with a dot, followed by a word.
export class DotOrdinalPattern extends Pattern {
  readonly #beforePunctuation: boolean;
  readonly #sentenceDot: boolean;

  /**
   * @param beforePunctuation also before a comma or at the end of the text: lat 70., Punkt 3.
   * @param sentenceDot leaves the dot to a sentence it also closes.
   */
  constructor(beforePunctuation = false, sentenceDot = false) {
    super(Kind.ORDINAL);
    this.#beforePunctuation = beforePunctuation;
    this.#sentenceDot = sentenceDot;
  }

  match(text: Text, position: number): Match | undefined {
    const dot = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, NUMBER_BOUNDARIES) ||
      dot === position ||
      dot >= text.length ||
      text[dot] !== "."
    ) {
      return undefined;
    }

    // Followed by a word, or (optionally) by a comma or the end of the text
    let next = dot + 1;
    while (next < text.length && isSpace(text[next])) {
      next++;
    }
    const word = next > dot + 1 && next < text.length;
    const punctuation = this.#beforePunctuation && (dot + 1 === text.length || text[dot + 1] === ",");
    if (!word && !punctuation) {
      return undefined;
    }
    const end = this.#sentenceDot && isSentenceEnd(text, dot + 1, true) ? dot : dot + 1;
    return match({
      kind: Kind.ORDINAL,
      span: { begin: position, end },
      numbers: [digitsAt(text, position)],
      suffix: ".",
    });
  }
}
