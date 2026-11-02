import { Document } from "../../text/document.ts";
import { contains, startsWith, str, type Text } from "../../text/strings.ts";
import { Language, type LanguageHandle } from "../language.ts";
import {
  AFTERNOON,
  ANNO_DOMINI,
  BEFORE_CHRIST,
  CHARACTERS,
  CUE_LINKS,
  CURRENCIES,
  DATE_MARKERS,
  DAY,
  DIGIT_CUES,
  DIGIT_UNITS,
  DIGITS,
  FRACTION,
  GROUP_UNITS,
  HOUR,
  MAX_DECIMALS,
  MINUTE,
  MONTH,
  MORNING,
  NEGATIVE,
  NOON,
  OPERATORS,
  type Operator,
  PERCENT,
  PRICE_CURRENCIES,
  POINT,
  RANGE_TO,
  RATIO,
  SCALES,
  SCORE_CUES,
  SCORE_RESULTS,
  SECOND,
  TWO,
  TWO_BEFORE,
  type Unit,
  UNITS,
  YEAR,
  YEAR_COUNTS,
  YEAR_WORDS,
  SYMBOLS,
} from "./constants.ts";

function isDigit(c: string | undefined): boolean {
  return c !== undefined && ((c >= "0" && c <= "9") || (c >= "０" && c <= "９"));
}

function isLetter(c: string | undefined): boolean {
  return c !== undefined && ((c >= "a" && c <= "z") || (c >= "A" && c <= "Z"));
}

function digitValue(c: string): number {
  return c <= "9" ? c.codePointAt(0)! - 0x30 : c.codePointAt(0)! - 0xff10;
}

// The character at i, or "" past the end.
function at(s: Text, i: number): string {
  return i >= 0 && i < s.length ? s[i] : "";
}

function before(s: Text, i: number): string {
  return i > 0 ? s[i - 1] : "";
}

// Whether a cue ends at i.
function endsAt(s: Text, i: number, word: string): boolean {
  return i >= word.length && startsWith(s, i - word.length, word);
}

// A number can start at i: no letter, digit, point, or comma after a digit is written right before it.
function freeBefore(s: Text, i: number): boolean {
  const c = before(s, i);
  return !isDigit(c) && !isLetter(c) && c !== "." && !(c === "," && isDigit(before(s, i - 1)));
}

// Length of the digit run starting at i.
function digitsAt(s: Text, i: number): number {
  let j = i;
  while (j < s.length && isDigit(s[j])) {
    j++;
  }
  return j - i;
}

// Length of a number starting at i, with groups of thousands and decimals ("1,234.5"), or 0.
function numberAt(s: Text, i: number): number {
  const n = digitsAt(s, i);
  if (n === 0) {
    return 0;
  }
  let j = i + n;
  if (n <= 3) {
    while (at(s, j) === "," && digitsAt(s, j + 1) === 3) {
      j += 4;
    }
  }
  if (at(s, j) === ".") {
    const fraction = digitsAt(s, j + 1);
    if (fraction > 0) {
      j += 1 + fraction;
    }
  }
  return j - i;
}

// The value of a run of digits, as a uint64_t of the C++ core.
function valueOf(digits: string): bigint {
  let value = 0n;
  for (const c of digits) {
    if (isDigit(c)) {
      value = BigInt.asUintN(64, value * 10n + BigInt(digitValue(c)));
    }
  }
  return value;
}

// Length of the longest word of a table starting at i, or 0.
function longestAt(s: Text, i: number, table: readonly string[]): number {
  let length = 0;
  for (const word of table) {
    if (word.length > length && startsWith(s, i, word)) {
      length = word.length;
    }
  }
  return length;
}

// Reads the digits of a number without its sign and decimal part, e.g. "1024" -> "一千零二十四", by groups of four
// digits: a zero between digits is read once, and a leading 2 of thousands or of the first group is 两 (两千, 两万).
// Digits and words hold only characters below U+10000, so string indexing counts characters.
function integerToWords(digits: string): string | undefined {
  let first = 0;
  while (first + 1 < digits.length && digitValue(digits[first]) === 0) {
    first++;
  }
  digits = digits.slice(first);
  if (digits.length > 4 * GROUP_UNITS.length) {
    return undefined;
  }

  let words = "";
  const groups = Math.floor((digits.length + 3) / 4);
  let zero = false; // A zero is pending: an empty group, or the zeros at the start of a group
  for (let g = 0; g < groups; g++) {
    const end = digits.length - 4 * (groups - 1 - g);
    const group = g === 0 ? digits.slice(0, end) : digits.slice(end - 4, end);
    if (valueOf(group) === 0n) {
      zero = words !== "";
      continue;
    }
    for (let k = 0; k < group.length; k++) {
      const digit = digitValue(group[k]);
      const power = group.length - 1 - k;
      if (digit === 0) {
        zero = words !== "";
        continue;
      }
      if (zero) {
        words += DIGITS[0];
        zero = false;
      }
      const two = digit === 2 && (power === 3 || (g === 0 && group.length === 1 && groups > 1));
      words += two ? TWO : DIGITS[digit];
      words += DIGIT_UNITS[power];
    }
    words += GROUP_UNITS[groups - 1 - g];
    zero = false;
  }

  if (words.startsWith("一十")) {
    words = words.slice(1);
  }
  return words === "" ? DIGITS[0] : words;
}

// Reads a number such as "-1,012.5" ("负一千零一十二点五"); undefined if its integer part has more than 16 digits.
function toCardinal(number: string): string | undefined {
  let sign = "";
  if (number.startsWith("-")) {
    sign = NEGATIVE;
    number = number.slice(1);
  }

  const point = number.indexOf(".");
  const integer = (point === -1 ? number : number.slice(0, point)).replaceAll(",", "");
  let words = integerToWords(integer);
  if (words === undefined) {
    return undefined;
  }

  words = sign + words;
  if (point !== -1 && point + 1 < number.length) {
    words += POINT;
    for (const c of number.slice(point + 1, point + 1 + MAX_DECIMALS)) {
      words += DIGITS[digitValue(c)];
    }
  }
  return words;
}

// A count before a measure: a lone 2 is 两 ("两美元").
function toCount(number: string): string | undefined {
  return number.length === 1 && digitValue(number) === 2 ? TWO : toCardinal(number);
}

// Reads a number digit by digit, such as a year ("2024" -> "二零二四").
function toDigits(number: string): string {
  let words = "";
  for (const c of number) {
    if (c === "-") {
      words += NEGATIVE;
    } else if (c === ".") {
      words += POINT;
    } else if (isDigit(c)) {
      words += DIGITS[digitValue(c)];
    }
  }
  return words;
}

// A match at a position: its length, and the words in its place (undefined to keep it as written).
interface Reading {
  length: number;
  words: string | undefined;
}

// One pass: replaces every reading left to right, like re.sub(). `read` returns the reading at a position, if any.
function substitute(draft: Document, read: (s: Text, i: number) => Reading | undefined): void {
  const text = draft.text();
  for (let i = 0; i < text.length;) {
    const reading = read(text, i);
    if (reading === undefined) {
      i++;
      continue;
    }
    if (reading.words !== undefined) {
      draft.replace({ begin: i, end: i + reading.length }, reading.words);
    }
    i += reading.length;
  }
  draft.commit();
}

// ----- Dates -----

function isYear(digits: string, from = 1000n): boolean {
  const value = valueOf(digits);
  return digits.length === 4 && value >= from && value <= 2099n;
}

function isMonthDay(month: bigint, day: bigint): boolean {
  return month >= 1n && month <= 12n && day >= 1n && day <= 31n;
}

function dateWords(year: string, month: string, day: string): string {
  let words = "";
  if (year !== "") {
    words += toDigits(year) + YEAR;
  }
  if (month !== "") {
    words += toCardinal(month)! + MONTH;
  }
  if (day !== "") {
    words += toCardinal(day)! + DAY;
  }
  return words;
}

// "2008-08-08", "08/2008", "08.08": digit groups with one separator.
function readNumericDate(s: Text, i: number): Reading | undefined {
  const groups = [str(s, i, i + digitsAt(s, i))];
  let j = i + groups[0].length;
  const separator = at(s, j);
  if (!contains("-/.·", separator)) {
    return undefined;
  }
  while (groups.length < 4 && at(s, j) === separator && digitsAt(s, j + 1) > 0) {
    groups.push(str(s, j + 1, j + 1 + digitsAt(s, j + 1)));
    j += 1 + groups.at(-1)!.length;
  }
  if (groups.length < 2 || groups.length > 3 || (at(s, j) === "." && isDigit(at(s, j + 1)))) {
    return undefined;
  }

  const dotted = separator === ".";
  const words = (year: string, month: string, day: string): Reading => ({
    length: j - i,
    words: dateWords(year, month, day),
  });
  if (groups.length === 3) {
    const [a, b, c] = groups;
    if (
      (isYear(a) || (a.length === 2 && (dotted || separator === "/"))) &&
      b.length <= 2 &&
      c.length <= 2 &&
      isMonthDay(valueOf(b), valueOf(c))
    ) {
      return words(a, b, c);
    }
    if (isYear(c) && a.length <= 2 && b.length <= 2) {
      const dayFirst = valueOf(a) > 12n;
      if (isMonthDay(valueOf(dayFirst ? b : a), valueOf(dayFirst ? a : b))) {
        return words(c, dayFirst ? b : a, dayFirst ? a : b);
      }
    }
    return undefined;
  }

  const [a, b] = groups;
  if (
    isYear(a, 1900n) &&
    (b.length === 2 || (b.length === 1 && !dotted)) &&
    isMonthDay(valueOf(b), 1n) &&
    (dotted || valueOf(b) !== (valueOf(a) + 1n) % 100n) &&
    !contains("万亿萬億元%％倍", at(s, j))
  ) {
    return words(a, b, "");
  }
  if (isYear(b, 1900n) && (a.length === 2 || (a.length === 1 && !dotted)) && isMonthDay(valueOf(a), 1n)) {
    return words(b, a, "");
  }
  if (a.length === 2 && b.length === 2 && (a[0] === "0" || b[0] === "0") && isMonthDay(valueOf(a), valueOf(b))) {
    return words("", a, b);
  }
  // (24/10): a day which can't be a month, then the month, in parentheses
  const parenthesized = contains("(（", at(s, i - 1)) && contains(")）", at(s, j));
  if (parenthesized && separator === "/" && valueOf(a) > 12n && isMonthDay(valueOf(b), valueOf(a))) {
    return words("", b, a);
  }
  return undefined;
}

// A word of letters and points at i, in any case, that no letter continues. Returns its length, or 0.
function lettersAt(s: Text, i: number, word: string): number {
  for (let k = 0; k < word.length; k++) {
    if (i + k >= s.length || (s[i + k].codePointAt(0)! | 0x20) !== word.charCodeAt(k)) {
      return 0;
    }
  }
  return isLetter(at(s, i + word.length)) ? 0 : word.length;
}

// An era after a year: "ad", "a.d.", "bc", "b.c.", in any case. Returns its length and whether it is before Christ.
function eraAt(s: Text, i: number): { length: number; bc: boolean } | undefined {
  for (const [era, bc] of [
    ["a.d.", false],
    ["b.c.", true],
    ["ad", false],
    ["bc", true],
  ] as const) {
    const length = lettersAt(s, i, era);
    if (length > 0) {
      return { length, bc };
    }
  }
  return undefined;
}

// "2024年5月1日": years digit by digit (a short year of years as a count), months and days as numbers.
function readWrittenDate(s: Text, i: number): Reading | undefined {
  const NONE = -1;
  // The position of a marker after a number ending at k, maybe after a space: "2020 年"
  const marker = (k: number, markers: string): number => {
    const m = at(s, k) === " " ? k + 1 : k;
    return contains(markers, at(s, m)) ? m : NONE;
  };

  let words = "";
  let j = i;
  const n = digitsAt(s, i);
  const to = digitsAt(s, i + n + 1);
  let year = marker(i + 5 + to, "年");
  if (n === 4 && contains("-~～－—到至", at(s, i + 4)) && (to === 2 || to === 4) && year !== NONE) {
    // "2020-2021年", "2009到2012年"
    words = toDigits(str(s, i, i + 4)) + RANGE_TO + toDigits(str(s, i + 5, i + 5 + to)) + YEAR;
    j = year + 1;
  } else if (((year = marker(i + n, "年")), n >= 2 && n <= 4 && year !== NONE)) {
    // A year of two digits is a count of years ("20年后") unless a month follows or it starts with 0; one of three
    // digits is a year unless it is round and a word of time follows ("700年前"); one of four digits is a year unless
    // it is past 2099 outside an era ("5000年"), or a round thousand before a word of time ("1000年前")
    j = year + 1;
    const k = at(s, j) === " " ? j + 1 : j;
    const month = digitsAt(s, k);
    const value = valueOf(str(s, i, i + n));
    const count = longestAt(s, j, YEAR_COUNTS) > 0 && !startsWith(s, j, "前后") && !startsWith(s, j, "前後");
    const era = endsAt(s, i, ANNO_DOMINI) || endsAt(s, i, BEFORE_CHRIST);
    const digits =
      (n === 4 && (value <= 2099n || era) && !(count && value % 1000n === 0n)) ||
      (n === 2 && (s[i] === "0" || (month >= 1 && month <= 2 && marker(k + month, "月") !== NONE))) ||
      (n === 3 && !(count && value % 10n === 0n));
    words = (digits ? toDigits(str(s, i, i + n)) : toCardinal(str(s, i, i + n))!) + YEAR;
  } else if (n === 4 && isYear(str(s, i, i + 4), 1900n)) {
    // "2020款", and a year cited after a name or in brackets: "Price 2008,", "(2003)"
    const next = at(s, i + 4);
    const cited =
      (before(s, i) === "(" || (before(s, i) === " " && isLetter(before(s, i - 1)))) &&
      (next === "" || next.codePointAt(0)! <= 0x7f) &&
      !contains("-~/", next);
    if (cited || longestAt(s, i + 4, YEAR_WORDS) > 0) {
      return { length: 4, words: toDigits(str(s, i, i + 4)) };
    }
  }
  for (const markers of ["月", "日号號"]) {
    const k = j > i && at(s, j) === " " ? j + 1 : j;
    const m = digitsAt(s, k);
    const p = m >= 1 && m <= 2 ? marker(k + m, markers) : NONE;
    if (p !== NONE) {
      words += toCardinal(str(s, k, k + m))! + s[p];
      j = p + 1;
    }
  }

  // "2934bc" -> "公元前二九三四年"
  if (j === i && n <= 4) {
    const era = eraAt(s, i + n);
    if (era !== undefined) {
      return {
        length: n + era.length,
        words: (era.bc ? BEFORE_CHRIST : ANNO_DOMINI) + toDigits(str(s, i, i + n)) + YEAR,
      };
    }
    return undefined;
  }
  if (j === i) {
    return undefined;
  }
  const era = eraAt(s, j);
  if (era !== undefined) {
    return { length: j + era.length - i, words: (era.bc ? BEFORE_CHRIST : ANNO_DOMINI) + words };
  }
  return { length: j - i, words };
}

function readDate(s: Text, i: number): Reading | undefined {
  const afterDate = before(s, i) === "-" && contains(DATE_MARKERS, before(s, i - 1));
  if (!isDigit(at(s, i)) || !freeBefore(s, i) || (contains("-/:", before(s, i)) && !afterDate)) {
    return undefined;
  }
  return readNumericDate(s, i) ?? readWrittenDate(s, i);
}

// ----- Times and ratios -----

// Positions of the parts of a time: [begin, end) of its hours, minutes, and seconds (begin = end without them).
interface Clock {
  length: number;
  hours: [number, number];
  minutes: [number, number];
  seconds: [number, number];
}

// "13:10", "13: 10", "13:10:36"
function clockAt(s: Text, i: number): Clock | undefined {
  const h = digitsAt(s, i);
  if (h === 0 || at(s, i + h) !== ":") {
    return undefined;
  }
  let j = i + h + 1;
  if (at(s, j) === " ") {
    j++;
  }
  const m = digitsAt(s, j);
  if (m === 0) {
    return undefined;
  }
  const clock: Clock = { length: 0, hours: [i, i + h], minutes: [j, j + m], seconds: [0, 0] };
  j += m;
  if (at(s, j) === ":" && digitsAt(s, j + 1) === 2) {
    clock.seconds = [j + 1, j + 3];
    j += 3;
  }
  if (at(s, j) === ":" && isDigit(at(s, j + 1))) {
    return undefined;
  }
  clock.length = j - i;
  return clock;
}

const partOf = (s: Text, [begin, end]: [number, number]): string => str(s, begin, end);
const hasSeconds = (clock: Clock): boolean => clock.seconds[1] > clock.seconds[0];

// A time, not a ratio such as "1:50,000"
function isTime(s: Text, clock: Clock): boolean {
  const [begin, end] = clock.minutes;
  return (
    numberAt(s, begin) === end - begin &&
    clock.hours[1] - clock.hours[0] <= 2 &&
    valueOf(partOf(s, clock.hours)) <= 24n &&
    end - begin === 2 &&
    valueOf(partOf(s, clock.minutes)) <= 59n &&
    (!hasSeconds(clock) || valueOf(partOf(s, clock.seconds)) <= 59n)
  );
}

function timeWords(s: Text, clock: Clock): string {
  const hours = valueOf(partOf(s, clock.hours));
  const minutes = valueOf(partOf(s, clock.minutes));
  let words = (hours === 2n ? TWO : toCardinal(partOf(s, clock.hours))!) + HOUR;
  if (minutes > 0n || hasSeconds(clock)) {
    words += (minutes < 10n && minutes > 0n ? DIGITS[0] : "") + toCardinal(partOf(s, clock.minutes))! + MINUTE;
  }
  if (hasSeconds(clock)) {
    words += toCardinal(partOf(s, clock.seconds))! + SECOND;
  }
  return words;
}

// "am", "a.m.", "pm", "p.m.", in any case. Returns its length and whether it is after noon.
function meridiemAt(s: Text, i: number): { length: number; pm: boolean } | undefined {
  for (const [meridiem, pm] of [
    ["a.m.", false],
    ["p.m.", true],
    ["am", false],
    ["pm", true],
  ] as const) {
    const length = lettersAt(s, i, meridiem);
    if (length > 0) {
      return { length, pm };
    }
  }
  return undefined;
}

// "13:10:36" -> "十三点十分三十六秒", "1:02pm" -> "下午一点零二分", "8:45-9:00" -> "八点四十五分到九点"; a pair of
// numbers that is no time is a ratio or a score: "2:0" -> "二比零".
function readTime(s: Text, i: number): Reading | undefined {
  if (!isDigit(at(s, i)) || !freeBefore(s, i) || before(s, i) === ":") {
    return undefined;
  }
  const clock = clockAt(s, i);
  if (clock === undefined) {
    return undefined;
  }
  let j = i + clock.length;
  if (!isTime(s, clock)) {
    if (hasSeconds(clock) || clock.hours[1] - clock.hours[0] > 3 || clock.minutes[1] - clock.minutes[0] > 3) {
      return undefined;
    }
    const minutes = clock.minutes[0];
    j = minutes + numberAt(s, minutes); // "1:300,000"
    const second = toCardinal(str(s, minutes, j));
    return second === undefined
      ? undefined
      : { length: j - i, words: toCardinal(partOf(s, clock.hours))! + RATIO + second };
  }

  let words = timeWords(s, clock);
  const space = at(s, j) === " " ? 1 : 0;
  const meridiem = meridiemAt(s, j + space);
  if (meridiem !== undefined) {
    const hours = valueOf(partOf(s, clock.hours));
    words = (!meridiem.pm ? MORNING : hours < 12n ? AFTERNOON : hours === 12n ? NOON : "") + words;
    j += space + meridiem.length;
  } else if (contains("-~～", at(s, j)) && isDigit(at(s, j + 1))) {
    const end = clockAt(s, j + 1);
    if (end !== undefined && isTime(s, end)) {
      words += RANGE_TO + timeWords(s, end);
      j += 1 + end.length;
    }
  }
  return { length: j - i, words };
}

// ----- Codes: telephone numbers, addresses, numbers with a leading zero -----

// "415-816-3816", "010-64035547": groups of three digits or more, the last of four or more, and no range of years
// or of round numbers.
function phoneAt(s: Text, i: number): number {
  const groups: string[] = [];
  let j = i;
  for (let n = digitsAt(s, j); n >= 3; n = digitsAt(s, j + 1)) {
    if (groups.length > 0) {
      j++;
    }
    groups.push(str(s, j, j + n));
    j += n;
    if (at(s, j) !== "-") {
      break;
    }
  }
  if (
    groups.length < 2 ||
    groups.at(-1)!.length < 4 ||
    groups.length > 3 ||
    at(s, j) === "-" ||
    (at(s, j) === "." && isDigit(at(s, j + 1)))
  ) {
    return 0;
  }
  if (groups.length === 2 && groups[0].length === 4 && groups[1].length === 4) {
    const next = at(s, j);
    const years = isYear(groups[0], 1500n) && isYear(groups[1], 1500n);
    const round = valueOf(groups[0]) % 100n === 0n && valueOf(groups[1]) % 100n === 0n;
    if (years || round || (next !== "" && (next.codePointAt(0)! > 0x7f || isLetter(next)))) {
      return 0;
    }
  }
  return j - i;
}

function codeWords(code: string): string {
  let words = "";
  for (const c of code) {
    words += c === "-" ? " " : toDigits(c);
  }
  return words;
}

// A cue of a table ends at i, maybe followed by a link: "尾号是"
function cueBefore(s: Text, i: number, cues: readonly string[]): boolean {
  const cueAt = (end: number): boolean => cues.some((cue) => endsAt(s, end, cue));
  return cueAt(i) || CUE_LINKS.some((link) => endsAt(s, i, link) && cueAt(i - link.length));
}

function readCode(s: Text, i: number): Reading | undefined {
  // "(310)726-7887"
  if (at(s, i) === "(" && !isDigit(before(s, i))) {
    const n = digitsAt(s, i + 1);
    const j = i + 2 + n;
    const space = at(s, j) === " " ? 1 : 0;
    if ((n === 3 || n === 4) && at(s, j - 1) === ")") {
      const phone = phoneAt(s, j + space);
      if (phone > 0) {
        return {
          length: j + space + phone - i,
          words:
            "(" +
            toDigits(str(s, i + 1, i + 1 + n)) +
            ")" +
            (space ? " " : "") +
            codeWords(str(s, j + space, j + space + phone)),
        };
      }
    }
    return undefined;
  }
  // After one or two capitals, a code: "CA1234", "G2501"
  let capitals = 0;
  while (capitals < 3 && i > capitals && at(s, i - capitals - 1) >= "A" && at(s, i - capitals - 1) <= "Z") {
    capitals++;
  }
  const digits = digitsAt(s, i);
  if (
    capitals >= 1 &&
    capitals <= 2 &&
    !isLetter(before(s, i - capitals)) &&
    (digits === 3 || digits === 4) &&
    !(at(s, i + digits) === "." && isDigit(at(s, i + digits + 1)))
  ) {
    return { length: digits, words: toDigits(str(s, i, i + digits)) };
  }
  if (
    !isDigit(at(s, i)) ||
    !freeBefore(s, i) ||
    contains("-/", before(s, i)) ||
    (before(s, i) === ":" && isDigit(before(s, i - 1)))
  ) {
    return undefined;
  }
  const n = digitsAt(s, i);
  const next = at(s, i + n);
  const phone = phoneAt(s, i);
  if (phone > 0) {
    return { length: phone, words: codeWords(str(s, i, i + phone)) };
  }
  // A mobile number, "13800138000"
  if (n === 11 && at(s, i) === "1" && digitValue(at(s, i + 1)) >= 3 && !(next === "." && isDigit(at(s, i + n + 1)))) {
    return { length: n, words: toDigits(str(s, i, i + n)) };
  }
  // After a word such as 尾号
  if (n >= 3 && cueBefore(s, i, DIGIT_CUES)) {
    let j = i + n;
    while (at(s, j) === "-" && digitsAt(s, j + 1) > 0) {
      j += 1 + digitsAt(s, j + 1);
    }
    return { length: j - i, words: codeWords(str(s, i, j)) };
  }
  // An address or a section, "127.0.0.1", "8.2.4"
  if (next === "." && digitsAt(s, i + n + 1) > 0) {
    let j = i + n;
    let groups = 1;
    while (at(s, j) === "." && digitsAt(s, j + 1) > 0) {
      j += 1 + digitsAt(s, j + 1);
      groups++;
    }
    return groups >= 3 ? { length: j - i, words: toDigits(str(s, i, j)) } : undefined;
  }
  // A leading zero, "007", and a generation, "80后"
  const generation = n === 2 && digitValue(at(s, i + 1)) === 0 && contains("后後", next);
  const grouped = contains(" ,", before(s, i)) && (isDigit(before(s, i - 1)) || before(s, i - 1) === ",");
  if (
    n >= 2 &&
    (digitValue(at(s, i)) === 0 || generation) &&
    !grouped &&
    !(next === "," && digitsAt(s, i + n + 1) === 3)
  ) {
    return { length: n, words: toDigits(str(s, i, i + n)) };
  }
  return undefined;
}

// ----- Money, units, and percentages -----

// The unit starting at i, if a letter or digit doesn't continue it.
function unitAt(s: Text, i: number): Unit | undefined {
  let found: Unit | undefined;
  for (const unit of UNITS) {
    if ((found === undefined || unit.symbol.length > found.symbol.length) && startsWith(s, i, unit.symbol)) {
      const next = at(s, i + unit.symbol.length);
      if (!isLetter(next) && !isDigit(next)) {
        found = unit;
      }
    }
  }
  return found;
}

// The unit a price is per, after a slash: "/kg" -> "一千克", "/个" -> "一" (the character stays): a price per one unit
function perUnit(s: Text, i: number): Reading | undefined {
  if (at(s, i) !== "/") {
    return undefined;
  }
  const unit = unitAt(s, i + 1);
  if (unit !== undefined && unit.per === "" && unit.word !== "") {
    return { length: 1 + unit.symbol.length, words: "一" + unit.word };
  }
  const c = at(s, i + 1);
  if (c >= "\u4e00" && c <= "\u9fff") {
    return { length: 1, words: "一" };
  }
  return undefined;
}

// "$1.5万" -> "一点五万美元", "$5/kg" -> "五美元一千克"
function readMoney(s: Text, i: number): Reading | undefined {
  for (const { symbol, name } of CURRENCIES) {
    if (!startsWith(s, i, symbol) || (isLetter(symbol[0]) && isLetter(before(s, i)))) {
      continue;
    }
    let j = i + symbol.length;
    if (at(s, j) === " ") {
      j++;
    }
    const n = numberAt(s, j);
    if (n === 0) {
      return undefined;
    }
    const number = toCount(str(s, j, j + n));
    if (number === undefined) {
      return undefined;
    }
    const scale = longestAt(s, j + n, SCALES);
    // "$500元" names its currency once
    const currency = contains("元圓圆块塊", at(s, j + n + scale)) ? "" : name;
    const per = perUnit(s, j + n + scale);
    return {
      length: j + n + scale + (per?.length ?? 0) - i,
      words: number + str(s, j + n, j + n + scale) + currency + (per?.words ?? ""),
    };
  }
  return undefined;
}

// "5元/kg" -> "五元一千克", "10元/个" -> "十元一个"
function readPrice(s: Text, i: number): Reading | undefined {
  const n = numberAt(s, i);
  if (n === 0 || !freeBefore(s, i)) {
    return undefined;
  }
  const j = at(s, i + n) === " " ? i + n + 1 : i + n;
  const currency = longestAt(s, j, PRICE_CURRENCIES);
  const per = currency > 0 ? perUnit(s, j + currency) : undefined;
  const number = toCount(str(s, i, i + n));
  if (per === undefined || number === undefined) {
    return undefined;
  }
  return { length: j + currency + per.length - i, words: number + str(s, j, j + currency) + per.words };
}

// "25kg" -> "二十五千克", "5-6mm" -> "五到六毫米", "4.4×4.1 cm" -> "四点四乘四点一厘米", "-5%" -> "负百分之五"
function readMeasure(s: Text, i: number): Reading | undefined {
  const negative = at(s, i) === "-" && !isDigit(before(s, i)) && !isLetter(before(s, i));
  const start = negative ? i + 1 : i;
  // After a letter, only a percentage: "GDP10%"
  const afterLetter = !negative && isLetter(before(s, i));
  if (!isDigit(at(s, start)) || (!negative && !afterLetter && !freeBefore(s, i))) {
    return undefined;
  }

  // The numbers of the quantity: one, a range of two, or a product
  const numbers = [str(s, start, start + numberAt(s, start))];
  let j = start + numbers[0].length;
  let joint = "";
  if (contains("-~～×", at(s, j)) && numberAt(s, j + 1) > 0 && !negative) {
    joint = at(s, j);
    while (at(s, j) === joint && numberAt(s, j + 1) > 0 && (joint === "×" || numbers.length < 2)) {
      numbers.push(str(s, j + 1, j + 1 + numberAt(s, j + 1)));
      j += 1 + numbers.at(-1)!.length;
    }
  }

  const unitAfter = (end: number): [Unit | undefined, number] => {
    const unit = unitAt(s, end);
    if (unit !== undefined) {
      return [unit, end + unit.symbol.length];
    }
    if (at(s, end) === " ") {
      // A unit of one letter is a word after a space: "230 W Cermak Road"
      const spaced = unitAt(s, end + 1);
      if (spaced !== undefined && !(spaced.symbol.length === 1 && isLetter(spaced.symbol))) {
        return [spaced, end + 1 + spaced.symbol.length];
      }
    }
    return [undefined, end];
  };
  let [unit, end] = unitAfter(j);
  if (unit === undefined && numbers.length > 1) {
    // Only the first number has a unit: "5-6" before nothing is a range for a later pass
    numbers.length = 1;
    [unit, end] = unitAfter(start + numbers[0].length);
  }
  if (unit === undefined || (afterLetter && unit.word !== "")) {
    return undefined;
  }

  const found = unit;
  const quantity = (number: string): string | undefined => (found.counted ? toCount(number) : toCardinal(number));
  let words = negative ? NEGATIVE : "";
  words += unit.per;
  for (let k = 0; k < numbers.length; k++) {
    const number = quantity(numbers[k]);
    if (number === undefined) {
      return undefined;
    }
    if (k > 0) {
      words += joint === "×" ? "乘" : RANGE_TO;
    }
    words += number;
  }
  words += unit.word;

  // "10%-20%" -> "百分之十到百分之二十"
  if (numbers.length === 1 && contains("-~～", at(s, end))) {
    const n = numberAt(s, end + 1);
    const number = quantity(str(s, end + 1, end + 1 + n));
    if (n > 0 && number !== undefined && unitAt(s, end + 1 + n) === unit) {
      words += RANGE_TO + unit.per + number + unit.word;
      end += 1 + n + unit.symbol.length;
    }
  }
  return { length: end - i, words };
}

// ----- Expressions, fractions, ranges -----

// "1+2=3" -> "一加二等于三"; a minus counts only in an equation.
function readMath(s: Text, i: number): Reading | undefined {
  const start = at(s, i) === "-" && !isDigit(before(s, i)) && !isLetter(before(s, i)) ? i + 1 : i;
  if (!isDigit(at(s, start)) || !freeBefore(s, i)) {
    return undefined;
  }
  let j = start + numberAt(s, start);
  let words = toCardinal(str(s, i, j));
  // Each operation: its symbol, its end, and the words up to it
  const operations: { symbol: string; end: number; words: string }[] = [];
  while (words !== undefined) {
    let k = at(s, j) === " " ? j + 1 : j;
    let op: Operator | undefined;
    for (const o of OPERATORS) {
      if (startsWith(s, k, o.symbol) && o.symbol.length > (op?.symbol.length ?? 0)) {
        op = o;
      }
    }
    if (op === undefined) {
      break;
    }
    k += op.symbol.length;
    if (at(s, k) === " ") {
      k++;
    }
    const n = numberAt(s, k);
    const operand = toCardinal(str(s, k, k + n));
    if (n === 0 || operand === undefined) {
      break;
    }
    j = k + n;
    words += op.word + operand;
    operations.push({ symbol: op.symbol, end: j, words });
  }
  // Without an equals sign, a minus is a dash: keep the operations before the first one
  if (!operations.some((o) => o.symbol === "=")) {
    const minus = operations.findIndex((o) => o.symbol === "-");
    if (minus !== -1) {
      operations.length = minus;
    }
  }
  const last = operations.at(-1);
  return last === undefined ? undefined : { length: last.end - i, words: last.words };
}

// "1/3" -> "三分之一", "-1/3" -> "负三分之一", "2009/10" -> "二零零九到一零"
function readFraction(s: Text, i: number): Reading | undefined {
  const negative = at(s, i) === "-" && !isDigit(before(s, i)) && !isLetter(before(s, i));
  const start = negative ? i + 1 : i;
  if (!isDigit(at(s, start)) || !freeBefore(s, i) || before(s, i) === "/") {
    return undefined;
  }
  const a = digitsAt(s, start);
  let j = start + a;
  const spaced = at(s, j) === " ";
  if (at(s, j + (spaced ? 1 : 0)) !== "/" || (spaced && at(s, j + 2) !== " ")) {
    return undefined;
  }
  j += spaced ? 3 : 1;
  const b = digitsAt(s, j);
  if (b === 0 || (at(s, j + b) === "/" && isDigit(at(s, j + b + 1)))) {
    return undefined;
  }
  const numerator = str(s, start, start + a);
  const denominator = str(s, j, j + b);
  const first = valueOf(numerator);
  const second = valueOf(denominator);
  if (
    !negative &&
    !spaced &&
    isYear(numerator, 1500n) &&
    ((b === 2 && second === (first + 1n) % 100n) || (b === 4 && second === first + 1n))
  ) {
    return { length: j + b - i, words: toDigits(numerator) + RANGE_TO + toDigits(denominator) };
  }
  const top = toCardinal(numerator);
  const bottom = toCardinal(denominator);
  if (top === undefined || bottom === undefined) {
    return { length: j + b - i, words: undefined };
  }
  return { length: j + b - i, words: (negative ? NEGATIVE : "") + bottom + FRACTION + top };
}

// "3-5" -> "三到五", "1995-2002" -> "一九九五到二零零二"; a second number below the first is no range.
function readRange(s: Text, i: number): Reading | undefined {
  if (!isDigit(at(s, i)) || !freeBefore(s, i) || before(s, i) === "-") {
    return undefined;
  }
  const a = numberAt(s, i);
  const j = i + a;
  if (!contains("-~～－—", at(s, j))) {
    return undefined;
  }
  const b = numberAt(s, j + 1);
  const end = j + 1 + b;
  if (b === 0 || isLetter(at(s, end)) || (at(s, end) === "-" && isDigit(at(s, end + 1)))) {
    return undefined;
  }
  const from = str(s, i, j);
  const to = str(s, j + 1, end);
  const integerOf = (number: string): string => (number.includes(".") ? number.slice(0, number.indexOf(".")) : number);
  const first = valueOf(integerOf(from));
  const second = valueOf(integerOf(to));
  if (
    (isYear(from, 1500n) && isYear(to, 1500n) && second > first && !(first % 100n === 0n && second % 100n === 0n)) ||
    (isYear(from) && b === 2 && (second > first % 100n || second === (first + 1n) % 100n))
  ) {
    return { length: end - i, words: toDigits(from) + RANGE_TO + toDigits(to) };
  }
  // A score: "23-0", "比分为1-2"
  const address = contains("号號室座楼樓", at(s, end)) || (b > 1 && to[0] === "0");
  const cued = cueBefore(s, i, SCORE_CUES) || longestAt(s, end, SCORE_RESULTS) > 0;
  if (!address && a <= 3 && b <= 3 && ((second < first && a <= 2 && b <= 2) || cued)) {
    return { length: end - i, words: toCardinal(from)! + RATIO + toCardinal(to)! };
  }
  if (second < first) {
    return undefined;
  }
  const low = toCardinal(from);
  const high = toCardinal(to);
  if (low === undefined || high === undefined) {
    return undefined;
  }
  return { length: end - i, words: low + RANGE_TO + high };
}

// "2个" -> "两个"
function readTwo(s: Text, i: number): Reading | undefined {
  if (
    !isDigit(at(s, i)) ||
    digitValue(at(s, i)) !== 2 ||
    !freeBefore(s, i) ||
    contains("第-:/", before(s, i)) ||
    isDigit(at(s, i + 1)) ||
    (contains(".,", at(s, i + 1)) && isDigit(at(s, i + 2)))
  ) {
    return undefined;
  }
  const word = longestAt(s, i + 1, TWO_BEFORE);
  if (word === 0 || (str(s, i + 1, i + 1 + word) === "年" && contains("级級", at(s, i + 2)))) {
    return undefined;
  }
  return { length: 1, words: TWO };
}

// Any other number: "-12.5" -> "负十二点五"
function readNumber(s: Text, i: number): Reading | undefined {
  if (at(s, i) === "-" && contains("万萬亿億", before(s, i)) && isDigit(at(s, i + 1))) {
    return { length: 1, words: RANGE_TO }; // "3万-5万"
  }
  const sign =
    at(s, i) === "-" && !isDigit(before(s, i)) && !isLetter(before(s, i)) && !contains(DATE_MARKERS, before(s, i));
  if (sign && startsWith(s, i + 1, PERCENT)) {
    return { length: 1, words: NEGATIVE }; // "-百分之1"
  }
  const start = sign ? i + 1 : i;
  const n = numberAt(s, start);
  if (n === 0) {
    return undefined;
  }
  return { length: start + n - i, words: toCardinal(str(s, i, start + n)) };
}

// --------------------
// ----- Language -----
// --------------------

/**
 * Spells out Arabic numbers in Chinese, after cn2an.transform(text, "an2cn"). In order, it reads dates (years digit
 * by digit), times and scores, codes (telephone numbers, addresses, leading zeros) digit by digit, money, units and
 * percentages, expressions, fractions, ranges, a 2 before a measure word as 两, and plain numbers.
 * Full-width digits count as digits.
 */
class Mandarin extends Language {
  constructor() {
    super({ characters: CHARACTERS, symbols: SYMBOLS });
  }

  readNumbers(document: Document): void {
    // Each pass rewrites a draft in turn: later passes see the words of earlier ones
    const draft = new Document(document.text());
    for (const read of [
      readDate,
      readTime,
      readCode,
      readMoney,
      readPrice,
      readMeasure,
      readMath,
      readFraction,
      readRange,
      readTwo,
      readNumber,
    ]) {
      substitute(draft, read);
    }

    // The changes of the draft are the edits of the document
    for (const { span, text } of draft.changes()) {
      document.replace(span, text);
    }
  }
}

let instance: Mandarin | undefined;

export function language(): LanguageHandle {
  return (instance ??= new Mandarin());
}
