import { Document } from "../../text/document.ts";
import { startsWith, str, type Text } from "../../text/strings.ts";
import { Language } from "../language.ts";
import {
  CELSIUS,
  CHARACTERS,
  DIGITS,
  FRACTION,
  MAX_DECIMALS,
  MEASURE_WORDS,
  NEGATIVE,
  PERCENT,
  POINT,
  RANGE_TO,
  UNITS,
} from "./constants.ts";

function isDigit(c: string | undefined): boolean {
  return c !== undefined && ((c >= "0" && c <= "9") || (c >= "０" && c <= "９"));
}

function digitValue(c: string): number {
  return c <= "9" ? c.codePointAt(0)! - 0x30 : c.codePointAt(0)! - 0xff10;
}

function isAt(s: Text, i: number, c: string): boolean {
  return i < s.length && s[i] === c;
}

// Length of the digit run starting at i.
function digitsAt(s: Text, i: number): number {
  let j = i;
  while (j < s.length && isDigit(s[j])) {
    j++;
  }
  return j - i;
}

// Length of a "\d+(\.\d+)?" number starting at i, or 0.
function decimalAt(s: Text, i: number): number {
  const integer = digitsAt(s, i);
  if (integer > 0 && isAt(s, i + integer, ".")) {
    const fraction = digitsAt(s, i + integer + 1);
    if (fraction > 0) {
      return integer + 1 + fraction;
    }
  }
  return integer;
}

function measureWordAt(s: Text, i: number): boolean {
  return MEASURE_WORDS.some((word) => startsWith(s, i, word));
}

// One pass of cn2an: replaces every match left to right, like re.sub(). `match` returns the length of the match
// at a position (0 for none), and `convert` its replacement, or undefined to keep it.
function substitute(
  draft: Document,
  match: (s: Text, i: number) => number,
  convert: (matched: Text) => string | undefined,
): void {
  const text = draft.text();
  for (let i = 0; i < text.length;) {
    const length = match(text, i);
    if (length === 0) {
      i++;
      continue;
    }
    const words = convert(text.slice(i, i + length));
    if (words !== undefined) {
      draft.replace({ begin: i, end: i + length }, words);
    }
    i += length;
  }
  draft.commit();
}

// Reads the digits of a number without its sign and decimal part, e.g. "1024" -> "一千零二十四".
// Digits and words hold only characters below U+10000, so string indexing counts characters.
function integerToWords(digits: string): string | undefined {
  let first = 0;
  while (first + 1 < digits.length && digitValue(digits[first]) === 0) {
    first++;
  }
  digits = digits.slice(first);
  if (digits.length > UNITS.length) {
    return undefined;
  }

  let words = "";
  for (let i = 0; i < digits.length; i++) {
    const digit = digitValue(digits[i]);
    const power = digits.length - i - 1;
    if (digit !== 0) {
      words += DIGITS[digit];
      words += UNITS[power];
    } else {
      if (power % 4 === 0) {
        words += DIGITS[0];
        words += UNITS[power];
      }
      if (i > 0 && words.at(-1) !== DIGITS[0]) {
        words += DIGITS[0];
      }
    }
  }

  // Python's str.replace(), as replaceAll(): every non-overlapping occurrence, left to right
  words = words.replaceAll("零零", "零");
  words = words.replaceAll("零万", "万");
  words = words.replaceAll("零亿", "亿");
  words = words.replaceAll("亿万", "亿");

  let start = 0;
  while (start < words.length && words[start] === DIGITS[0]) {
    start++;
  }
  let end = words.length;
  while (end > start && words[end - 1] === DIGITS[0]) {
    end--;
  }
  words = words.slice(start, end);

  // "万零三千" -> "万三千"
  let compact = "";
  for (let i = 0; i < words.length;) {
    if (
      i + 3 < words.length &&
      (words[i] === "万" || words[i] === "亿") &&
      words[i + 1] === DIGITS[0] &&
      words[i + 2] !== DIGITS[0] &&
      (DIGITS as readonly string[]).includes(words[i + 2]) &&
      words[i + 3] === "千"
    ) {
      compact += words[i];
      compact += words.slice(i + 2, i + 4);
      i += 4;
    } else {
      compact += words[i++];
    }
  }
  words = compact;

  if (words.startsWith("一十")) {
    words = words.slice(1);
  }
  if (words === "") {
    words = DIGITS[0];
  }
  return words;
}

// Reads a number such as "-12.5" ("负十二点五"); undefined if its integer part has more than 16 digits.
function toCardinal(number: string): string | undefined {
  let sign = "";
  if (number.startsWith("-")) {
    sign = NEGATIVE;
    number = number.slice(1);
  }

  const point = number.indexOf(".");
  const integer = integerToWords(point === -1 ? number : number.slice(0, point));
  if (integer === undefined) {
    return undefined;
  }

  let words = sign + integer;
  if (point !== -1 && point + 1 < number.length) {
    words += POINT;
    for (const c of number.slice(point + 1, point + 1 + MAX_DECIMALS)) {
      words += DIGITS[digitValue(c)];
    }
  }
  return words;
}

// Reads a number digit by digit, such as a year ("2024" -> "二零二四").
function toDigits(number: string): string {
  let words = "";
  for (const c of number) {
    if (c === "-") {
      words += NEGATIVE;
    } else if (c === ".") {
      words += POINT;
    } else {
      words += DIGITS[digitValue(c)];
    }
  }
  return words;
}

// --------------------
// ----- Language -----
// --------------------

/**
 * Spells out Arabic numbers in Chinese, as cn2an.transform(text, "an2cn") does.
 * In order, it reads ranges before a measure word ("3-5个" -> "三到五个"), dates
 * (years digit by digit), fractions, percentages, degrees Celsius and plain numbers.
 * Full-width digits count as digits.
 */
class Mandarin extends Language {
  constructor() {
    super({ characters: CHARACTERS });
  }

  readNumbers(document: Document): void {
    // Each pass rewrites a draft in turn, as cn2an's regular expressions do: later passes see the words of earlier ones
    const draft = new Document(document.text());

    // 1. Ranges before a measure word: "3-5个" -> "三到五个"
    substitute(
      draft,
      (s, i) => {
        const from = decimalAt(s, i);
        if (from === 0 || !isAt(s, i + from, "-")) {
          return 0;
        }
        const to = decimalAt(s, i + from + 1);
        const end = i + from + 1 + to;
        return to > 0 && measureWordAt(s, end) ? end - i : 0;
      },
      (range) => {
        const dash = range.indexOf("-");
        const from = toCardinal(str(range, 0, dash));
        const to = toCardinal(str(range, dash + 1));
        if (from === undefined || to === undefined) {
          return undefined;
        }
        return from + RANGE_TO + to;
      },
    );

    // 2. Dates: years digit by digit, months and days as numbers
    substitute(
      draft,
      (s, i) => {
        let j = i;
        let n = digitsAt(s, j);
        if (n >= 2 && n <= 4 && isAt(s, j + n, "年")) {
          j += n + 1;
        }
        n = digitsAt(s, j);
        if (n >= 1 && n <= 2 && isAt(s, j + n, "月")) {
          j += n + 1;
        }
        n = digitsAt(s, j);
        if (n >= 1 && n <= 2 && isAt(s, j + n, "日")) {
          j += n + 1;
        }
        return j - i;
      },
      (date) => {
        let words = "";
        for (let i = 0; i < date.length;) {
          const n = digitsAt(date, i);
          if (n === 0) {
            words += date[i++];
            continue;
          }
          const digits = str(date, i, i + n);
          words += isAt(date, i + n, "年") ? toDigits(digits) : toCardinal(digits)!;
          i += n;
        }
        return words;
      },
    );

    // 3. Fractions: "1/3" -> "三分之一"
    substitute(
      draft,
      (s, i) => {
        const numerator = digitsAt(s, i);
        if (numerator === 0 || !isAt(s, i + numerator, "/")) {
          return 0;
        }
        const denominator = digitsAt(s, i + numerator + 1);
        return denominator > 0 ? numerator + 1 + denominator : 0;
      },
      (fraction) => {
        const slash = fraction.indexOf("/");
        const numerator = toCardinal(str(fraction, 0, slash));
        const denominator = toCardinal(str(fraction, slash + 1));
        if (numerator === undefined || denominator === undefined) {
          return undefined;
        }
        return denominator + FRACTION + numerator;
      },
    );

    // 4. Percentages: "-5%" -> "百分之负五"
    substitute(
      draft,
      (s, i) => {
        const j = isAt(s, i, "-") ? i + 1 : i;
        const integer = digitsAt(s, j);
        if (integer === 0) {
          return 0;
        }
        // "1.%" is no percentage: a point must be followed by digits.
        const number = decimalAt(s, j);
        if (number === integer && isAt(s, j + integer, ".")) {
          return 0;
        }
        return isAt(s, j + number, "%") ? j + number + 1 - i : 0;
      },
      (percent) => {
        const number = toCardinal(str(percent, 0, percent.length - 1));
        if (number === undefined) {
          return undefined;
        }
        return PERCENT + number;
      },
    );

    // 5. Degrees Celsius: "25℃" -> "二十五摄氏度"
    substitute(
      draft,
      (s, i) => {
        const n = digitsAt(s, i);
        return n > 0 && isAt(s, i + n, "℃") ? n + 1 : 0;
      },
      (degrees) => {
        const number = toCardinal(str(degrees, 0, degrees.length - 1));
        if (number === undefined) {
          return undefined;
        }
        return number + CELSIUS;
      },
    );

    // 6. Remaining numbers
    substitute(
      draft,
      (s, i) => {
        const j = isAt(s, i, "-") ? i + 1 : i;
        const n = decimalAt(s, j);
        return n > 0 ? j + n - i : 0;
      },
      (number) => toCardinal(str(number)),
    );

    // The changes of the draft are the edits of the document
    for (const { span, text } of draft.changes()) {
      document.replace(span, text);
    }
  }
}

let instance: Mandarin | undefined;

export function language(): Language {
  return (instance ??= new Mandarin());
}
