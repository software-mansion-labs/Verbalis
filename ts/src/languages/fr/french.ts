import { DatePattern, Pattern, PhonePattern, SuffixPattern } from "../../numbers/patterns.ts";
import { Kind, match, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  fitsClock,
  isSpokenDigits,
  namedFraction,
  parseValue,
  spacedSuffix,
  value,
} from "../../numbers/scanning.ts";
import type { Context } from "../../text/context.ts";
import { isClearAfter, isClearBefore, join, keysAt } from "../../text/scan.ts";
import { contains, size, str, toLower, type Text } from "../../text/strings.ts";
import { isLower } from "../../text/unicode.ts";
import type { Forms } from "../grammar.ts";
import { PatternLanguage } from "../language.ts";
import {
  CURRENCIES,
  DATE_FORMAT,
  FEMININE_NOUNS,
  FRACTIONS,
  JOINERS,
  MASCULINE_ENDINGS,
  MASCULINE_IN_E,
  MONTHS,
  NOTATION,
  NUMBER_WORDS,
  ONES,
  ORDINAL_SUFFIXES,
  PHONE_FORMAT,
  SCALES,
  STOP_WORDS,
  SYMBOLS,
  TENS,
  TERMS,
  TIME_WORDS,
  UNITS,
  WEEKDAYS,
} from "./constants.ts";

// What the words around a match say about reading its numbers
interface Cues {
  feminine: boolean; // vingt et une heures: before a feminine noun
  pluralS: boolean; // quatre-vingts, deux cents; but les années quatre-vingt
  equation: boolean; // 12 x 4 = 48: "x" is "fois" rather than "par"
}

// ------------------------
// ----- Number words -----
// ------------------------

function belowHundred(n: bigint, feminine: boolean, pluralS: boolean): string {
  if (n < 20n) {
    return feminine && n === 1n ? "une" : ONES[Number(n)];
  }

  // soixante-dix, quatre-vingt-dix
  let t = n / 10n;
  let u = n % 10n;
  if (t === 7n || t === 9n) {
    t--;
    u += 10n;
  }
  if (u === 0n) {
    return TENS[Number(t)] + (t === 8n && pluralS ? "s" : "");
  }
  const unit = feminine && u === 1n ? "une" : ONES[Number(u)];
  return TENS[Number(t)] + ((u === 1n || u === 11n) && t < 8n ? " et " : "-") + unit;
}

function belowThousand(n: bigint, feminine: boolean, pluralS: boolean): string {
  const h = n / 100n;
  const r = n % 100n;
  if (h === 0n) {
    return belowHundred(r, feminine, pluralS);
  }
  const head = h === 1n ? "cent" : `${ONES[Number(h)]} cent` + (r === 0n && pluralS ? "s" : "");
  return head + (r ? ` ${belowHundred(r, feminine, pluralS)}` : "");
}

// A cardinal; `pluralS` is false when "mille" follows (deux cent mille, quatre-vingt mille).
function cardinal(n: bigint, feminine = false, pluralS = true): string {
  if (n < 1000n) {
    return belowThousand(n, feminine, pluralS);
  }
  if (n < 1000000n) {
    const t = n / 1000n;
    const r = n % 1000n;
    const head = t === 1n ? "mille" : `${belowThousand(t, false, false)} mille`;
    return head + (r ? ` ${belowThousand(r, feminine, pluralS)}` : "");
  }
  for (const [scale, word] of [
    [1000000000n, "milliard"],
    [1000000n, "million"],
  ] as const) {
    if (n >= scale) {
      const q = n / scale;
      const r = n % scale;
      const head = `${cardinal(q)} ${word}` + (q > 1n ? "s" : "");
      return head + (r ? ` ${cardinal(r, feminine, pluralS)}` : "");
    }
  }
  return "";
}

function ordinal(n: bigint, feminine = false): string {
  if (n === 1n) {
    return feminine ? "première" : "premier";
  }

  // vingtième, cinquième, neuvième, onzième
  let words = cardinal(n);
  if (words.endsWith("vingts") || words.endsWith("cents")) {
    words = words.slice(0, -1);
  }
  if (words.endsWith("neuf")) {
    words = `${words.slice(0, -1)}v`;
  } else if (words.endsWith("cinq")) {
    words += "u";
  }
  if (words.endsWith("e")) {
    words = words.slice(0, -1);
  }
  return `${words}ième`;
}

function digits(text: string): string {
  return join([...text].map((c) => ONES[Number(c)]));
}

// A number of a given gender, as before a currency or a unit
function gendered(number: Numeral, cues: Cues, feminine: boolean): string {
  if (number.fraction === "") {
    const n = parseValue(number.integer);
    return n === undefined || isSpokenDigits(number, 5) ? digits(number.integer) : cardinal(n, feminine, cues.pluralS);
  }

  // Up to three decimals read as a number: zéro virgule zéro cinq, deux virgule soixante-quinze
  let fraction: string;
  if (number.fraction.length > 3) {
    fraction = digits(number.fraction);
  } else {
    const nonZero = number.fraction.search(/[^0]/);
    const zeros = nonZero === -1 ? number.fraction.length : nonZero;
    const words: string[] = Array<string>(zeros).fill("zéro");
    if (nonZero !== -1) {
      words.push(cardinal(parseValue(number.fraction) ?? 0n));
    }
    fraction = join(words);
  }
  const head = number.integer === "" ? "" : `${cardinal(value(number), feminine)} `;
  return `${head}virgule ${fraction}`;
}

// -------------------
// ----- Context -----
// -------------------

function feminine(noun: string): boolean {
  let word = toLower(noun);
  word = word.slice(word.lastIndexOf("-") + 1);
  if (!FEMININE_NOUNS.has(word) && word.endsWith("s")) {
    word = word.slice(0, -1);
  }
  if (FEMININE_NOUNS.has(word)) {
    return true;
  }
  const masculineEnding = MASCULINE_ENDINGS.some((e) => word.endsWith(e));
  const date = (MONTHS as readonly string[]).includes(word) || WEEKDAYS.has(word);
  return word.endsWith("e") && !MASCULINE_IN_E.has(word) && !date && !masculineEnding;
}

// A lowercase noun follows: 10k abonnés
function counted(context: Context): boolean {
  const word = context.after();
  const lowercaseWord = size(word) > 2 && [...word].every((c) => isLower(c));
  return lowercaseWord && !STOP_WORDS.has(word) && !NUMBER_WORDS.has(word);
}

// A noun after a number is singular below 2: un virgule cinq kilomètre, deux kilomètres
function numberForm(number: Numeral, forms: Forms): string {
  return value(number) < 2n ? forms.one : forms.many;
}

function cuesOf(match: Match): Cues {
  const previous = match.context.before();
  return {
    feminine: feminine(match.context.after()),
    pluralS: previous !== "années" && previous !== "année",
    equation: match.kind === Kind.DIMENSION && match.context.tail().includes("="),
  };
}

// An amount with its scale: trois millions, un virgule cinq million, dix mille
function scaled(number: Numeral, scale: string, cues: Cues): string {
  const word = SCALES[scale];
  if (word === "mille") {
    return `${gendered(number, cues, false)} mille`;
  }
  const one = word.endsWith("s") ? word.slice(0, -1) : word;
  return `${gendered(number, cues, false)} ${numberForm(number, { one, many: `${one}s` })}`;
}

// --------------------
// ----- Patterns -----
// --------------------

function romanDigit(c: string | undefined): number {
  switch (c) {
    case "I":
      return 1;
    case "V":
      return 5;
    case "X":
      return 10;
    case "L":
      return 50;
    case "C":
      return 100;
    default:
      return 0;
  }
}

// prettier-ignore
const ROMAN_NUMERALS = [
  [100, "C"], [90, "XC"], [50, "L"], [40, "XL"], [10, "X"], [9, "IX"], [5, "V"], [4, "IV"], [1, "I"],
] as const;

// The value of a Roman numeral written in its canonical form (XIX, not XVIIII).
function romanValue(numeral: string): number | undefined {
  let value = 0;
  for (let i = 0; i < numeral.length; ++i) {
    const digit = romanDigit(numeral[i]);
    value += i + 1 < numeral.length && digit < romanDigit(numeral[i + 1]) ? -digit : digit;
  }

  let canonical = "";
  let rest = value;
  for (const [numeralValue, symbol] of ROMAN_NUMERALS) {
    for (; rest >= numeralValue; rest -= numeralValue) {
      canonical += symbol;
    }
  }
  return canonical === numeral ? value : undefined;
}

// XIXe, Ier, IIIe: an ordinal written in Roman numerals.
class RomanOrdinalPattern extends Pattern {
  constructor() {
    super(Kind.ORDINAL);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position)) {
      return undefined;
    }

    // Two Roman digits or more, or a single I (Ier)
    let end = position;
    while (end < text.length && romanDigit(text[end])) {
      end++;
    }
    const written = str(text, position, end);
    if (written.length < 2 && written !== "I") {
      return undefined;
    }
    const roman = romanValue(written);
    if (roman === undefined) {
      return undefined;
    }

    for (const suffix of keysAt(text, end, ["er", "re", "ère", "e", "ème"])) {
      if (isClearAfter(text, end + suffix.length)) {
        return match({
          kind: Kind.ORDINAL,
          span: { begin: position, end: end + suffix.length },
          numbers: [numeral({ span: { begin: position, end }, integer: String(roman) })],
          suffix,
        });
      }
    }
    return undefined;
  }
}

// --------------------
// ----- Language -----
// --------------------

class French extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new PhonePattern(PHONE_FORMAT));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new RomanOrdinalPattern());
  }

  protected override readNumber(number: Numeral, match: Match): string {
    const cues = cuesOf(match);
    return gendered(number, cues, number.fraction === "" && cues.feminine);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 4);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    // Without a year only a zero-padded day or month is a date: 05/11, but not 3/4
    if (match.numbers.length !== 3 && match.numbers[0].integer[0] !== "0" && match.numbers[1].integer[0] !== "0") {
      return undefined;
    }
    const day = value(match.numbers[0]);
    const words = `${day === 1n ? ordinal(1n) : cardinal(day)} ${MONTHS[Number(value(match.numbers[1])) - 1]}`;
    return match.numbers.length < 3 ? words : `${words} ${cardinal(value(match.numbers[2]))}`;
  }

  protected override readPhone(match: Match): string | undefined {
    // In pairs: zéro six douze trente-quatre ...
    const words: string[] = [];
    for (const pair of match.numbers) {
      const zero = pair.integer.length === 2 && pair.integer[0] === "0";
      if (pair.integer.length > 3) {
        words.push(digits(pair.integer)); // A longer group of a foreign number
      } else {
        words.push(zero ? `zéro ${ONES[Number(pair.integer[1])]}` : cardinal(value(pair)));
      }
    }
    return join(words);
  }

  protected override readFraction(match: Match): string | undefined {
    return (
      namedFraction(FRACTIONS, match) ?? `${cardinal(value(match.numbers[0]))} sur ${cardinal(value(match.numbers[1]))}`
    );
  }

  protected override readTime(match: Match): string | undefined {
    if (match.numbers.length === 2) {
      const decimal = match.separator === "." && !TIME_WORDS.has(match.context.before());
      if (!fitsClock(match) || decimal) {
        return undefined;
      }
    }
    const hours = value(match.numbers[0]);
    let words = cardinal(hours, true) + (hours < 2n ? " heure" : " heures");
    if (match.numbers.length === 2 && value(match.numbers[1]) > 0n) {
      words += ` ${cardinal(value(match.numbers[1]), true)}`;
    }
    return words;
  }

  protected override readOrdinal(match: Match): string | undefined {
    const suffix = match.suffix;
    let words: string;
    if (suffix.startsWith("nd")) {
      words = suffix.startsWith("nde") ? "seconde" : "second";
    } else {
      const feminine = suffix === "re" || suffix === "ère" || suffix === "res" || suffix === "ères";
      words = ordinal(value(match.numbers[0]), feminine);
    }
    return words + (suffix.endsWith("s") ? "s" : "");
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    let currency = CURRENCIES[match.unit];
    let scale = match.scale;
    if (match.unit === "M€") {
      scale = "millions";
      currency = { one: "euro", many: "euros" };
    }
    const cues = cuesOf(match);

    if (scale !== "") {
      // trois millions d'euros, but dix mille euros
      const words = scaled(number, scale, cues);
      if (SCALES[scale] === "mille") {
        return `${words} ${currency.many}`;
      }
      const vowel = contains("aeiouyéh", currency.many[0]);
      return words + (vowel ? " d'" : " de ") + currency.many;
    }

    // A price: trois euros cinquante, quatre-vingt-dix-neuf centimes
    const feminine = currency.one === "livre";
    if (number.fraction.length === 2) {
      const cents = parseValue(number.fraction) ?? 0n;
      if (value(number) === 0n) {
        return `${cardinal(cents)} centimes`;
      }
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      return (
        `${gendered(whole, cues, feminine)} ${number.integer === "1" ? currency.one : currency.many} ` + cardinal(cents)
      );
    }
    return `${gendered(number, cues, feminine)} ${numberForm(number, currency)}`;
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    // 4K, 3k de Q: a letter, read as a code
    if (SCALES[match.scale] === "mille" && !counted(match.context)) {
      return undefined;
    }
    return scaled(number, match.scale, cuesOf(match));
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${gendered(number, cuesOf(match), false)} pour cent${spacedSuffix(match)}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    const unit = UNITS[match.unit];
    const space = unit.one.indexOf(" ");
    const firstWord = space === -1 ? unit.one : unit.one.slice(0, space);
    return `${gendered(number, cuesOf(match), feminine(firstWord))} ${numberForm(number, unit)}`;
  }

  protected override readJoiner(match: Match): string | undefined {
    if (match.kind === Kind.DIMENSION) {
      return cuesOf(match).equation ? " fois " : " par ";
    }
    return super.readJoiner(match);
  }
}

let instance: French | undefined;

export function language(): PatternLanguage {
  return (instance ??= new French());
}
