/**
 * Scanning numbers: written numbers at a position of a text, on top of the shared scanning in text/scan.ts, and the
 * shared steps of the languages' readings: values of digits, digit-by-digit decisions, and the parts of codes.
 */
import { digitsEnd, isDigitAfter } from "../text/scan.ts";
import { contains, str, textOf, type Text } from "../text/strings.ts";
import { isDigit } from "../text/unicode.ts";
import type { Fractions } from "./formats.ts";
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
    let end = position + run;
    while (end + 3 < text.length && contains(group, text[end]) && digitsEnd(text, end + 1) >= end + 4) {
      end += 4;
    }
    // A group may not be followed by a digit, nor by a separator and a digit
    const complete =
      end > position + run &&
      !isDigitAfter(text, end) &&
      !(end < text.length && contains(group, text[end]) && isDigitAfter(text, end + 1));
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
export function namedFraction(fractions: Fractions, fraction: Match): string | undefined {
  return fractions[`${value(fraction.numbers[0])}/${value(fraction.numbers[1])}`];
}

// The suffix of a match with a space before it, to follow its reading; empty if the match has none.
export function spacedSuffix(match: Match): string {
  return match.suffix === "" ? "" : ` ${match.suffix}`;
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
