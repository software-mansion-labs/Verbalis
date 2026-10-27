import type { Context } from "../../text/context.ts";
import { DatePattern, Pattern, PhonePattern, SuffixPattern, RomanPattern } from "../../numbers/patterns.ts";
import { Kind, match, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  digitsAt,
  fitsClock,
  isOne,
  isSpokenDigits,
  namedFraction,
  parseValue,
  spacedSuffix,
  value,
  romanRole,
  RomanRole,
} from "../../numbers/scanning.ts";
import { isClearAfter, isDigitAfter, join } from "../../text/scan.ts";
import { contains, size, str, toLower, type Text } from "../../text/strings.ts";
import { isAlpha, isDigit, isLower, isSpace, isWord } from "../../text/unicode.ts";
import { PatternLanguage } from "../language.ts";
import {
  CURRENCIES,
  DATE_FORMAT,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  FEMININE_ENDINGS,
  FEMININE_NOUNS,
  FRACTIONS,
  HUNDREDS,
  JOINERS,
  LAWS,
  MASCULINE_IN_A,
  MONTHS,
  NOTATION,
  ONES,
  ORDINAL_HUNDREDS,
  ORDINAL_ONES,
  ORDINAL_SIGN_FORMAT,
  ORDINAL_SIGNS,
  ORDINAL_SUFFIXES,
  ORDINAL_TENS,
  PHONE_FORMATS,
  PHONE_WORDS,
  SCALES,
  STOP_WORDS,
  SYMBOLS,
  TENS,
  TERMS,
  TIME_WORDS,
  UNITS,
  ROMAN_CUES,
  FEMININE_NAMES,
  FEMININE_ROMAN_NOUNS,
} from "./constants.ts";

// How "uno" ends a number: alone (veintiuno), before a masculine noun (veintiún años) or a feminine one (veintiuna)
const One = { UNO: 0, UN: 1, UNA: 2 } as const;
type One = (typeof One)[keyof typeof One];

// ------------------------
// ----- Number words -----
// ------------------------

function belowHundred(n: bigint, one: One): string {
  let words: string = n < 30n ? ONES[Number(n)] : TENS[Number(n / 10n)];
  if (n >= 30n && n % 10n !== 0n) {
    words += ` y ${ONES[Number(n % 10n)]}`;
  }

  // veintiún años, treinta y una personas
  if (words.endsWith("uno") && one !== One.UNO) {
    words = words.slice(0, -3);
    words += one === One.UNA ? "una" : words === "veinti" ? "ún" : "un";
  }
  return words;
}

function belowThousand(n: bigint, one: One): string {
  const h = n / 100n;
  const r = n % 100n;
  if (h === 0n) {
    return belowHundred(r, one);
  }
  if (n === 100n) {
    return "cien";
  }

  // doscientas personas
  let head: string = HUNDREDS[Number(h)];
  if (one === One.UNA && h > 1n) {
    head = `${head.slice(0, -2)}as`;
  }
  return head + (r !== 0n ? ` ${belowHundred(r, one)}` : "");
}

// A cardinal, ending with the given form of "uno": doscientas mil personas, veintiún años
function cardinal(n: bigint, one: One = One.UNO): string {
  if (n < 1000n) {
    return belowThousand(n, one);
  }
  if (n < 1000000n) {
    const t = n / 1000n;
    const r = n % 1000n;
    const head = t === 1n ? "mil" : `${belowThousand(t, one === One.UNA ? One.UNA : One.UN)} mil`;
    return head + (r !== 0n ? ` ${belowThousand(r, one)}` : "");
  }

  const billions = n >= 1000000000000n;
  const scale = billions ? 1000000000000n : 1000000n;
  const q = n / scale;
  const r = n % scale;
  const head =
    q === 1n ? (billions ? "un billón" : "un millón") : cardinal(q, One.UN) + (billions ? " billones" : " millones");
  return head + (r !== 0n ? ` ${cardinal(r, one)}` : "");
}

// centésima trigésima séptima
function ordinal(n: bigint, feminine: boolean): string {
  const words: string[] = [];
  for (const [scale, stems] of [
    [100n, ORDINAL_HUNDREDS],
    [10n, ORDINAL_TENS],
    [1n, ORDINAL_ONES],
  ] as const) {
    const digit = (n / scale) % 10n;
    if (digit !== 0n) {
      words.push(stems[Number(digit)] + (feminine ? "a" : "o"));
    }
  }
  return join(words);
}

function digits(text: string): string {
  return join([...text].filter((c) => isDigit(c)).map((c) => ONES[Number(c)]));
}

// A number ending with a given form of "uno", as before a currency or a unit
function numberWith(number: Numeral, one: One): string {
  if (number.fraction === "") {
    const n = parseValue(number.integer);
    return n === undefined || isSpokenDigits(number, 7) ? digits(number.integer) : cardinal(n, one);
  }

  // The separator is read as written: tres coma cinco, tres punto cinco
  let fraction: string;
  if (number.fraction.length > 2) {
    fraction = digits(number.fraction);
  } else {
    const words: string[] = [];
    const zeros = number.fraction.search(/[^0]/);
    words.push(...Array<string>(zeros === -1 ? number.fraction.length : zeros).fill("cero"));
    if (zeros !== -1) {
      words.push(cardinal(parseValue(number.fraction) ?? 0n, one));
    }
    fraction = join(words);
  }
  const head = number.integer === "" ? "" : `${cardinal(value(number), one === One.UN ? One.UNO : one)} `;
  return head + (number.decimalMark === "," ? "coma " : "punto ") + fraction;
}

// -------------------
// ----- Context -----
// -------------------

function feminine(noun: string): boolean {
  const word = toLower(noun);
  let singular = word;
  if (word.endsWith("iones")) {
    singular = `${word.slice(0, -5)}ión`;
  } else if (word.endsWith("as") || word.endsWith("es")) {
    singular = singular.slice(0, -1);
  }

  if (FEMININE_NOUNS.has(word) || FEMININE_NOUNS.has(singular)) {
    return true;
  }
  const feminineEnding = FEMININE_ENDINGS.some((e) => singular.endsWith(e));
  return !MASCULINE_IN_A.has(singular) && feminineEnding;
}

// The form of "uno" from the noun after a number: veintiún años, veintiuna personas; a la 1
function oneOf(context: Context): One {
  const word = context.after();
  const previous = context.before();
  if (previous === "la" || previous === "las") {
    return One.UNA;
  }
  if (word === "" || !isAlpha(String.fromCodePoint(word.codePointAt(0) as number)) || STOP_WORDS.has(toLower(word))) {
    return One.UNO;
  }
  return feminine(word) ? One.UNA : One.UN;
}

// A lowercase noun follows: 10k seguidores
function counted(context: Context): boolean {
  const word = context.after();
  const lowercaseWord = size(word) > 2 && [...word].every((c) => isLower(c));
  return lowercaseWord && !STOP_WORDS.has(word);
}

// What the words around a match say about reading its numbers
interface Cues {
  one: One; // From the noun after the number
  named: boolean; // $5.000 dólares: the currency is already named after the amount
}

function cuesOf(match: Match): Cues {
  // A decade or a compound stands alone: los 80s, 4-carriles
  const alone = match.kind === Kind.DECADE || match.kind === Kind.COMPOUND;
  const cues: Cues = { one: alone ? One.UNO : oneOf(match.context), named: false };

  // $5.000 dólares
  if (match.kind === Kind.MONEY) {
    const currency = CURRENCIES[match.unit];
    const next = toLower(match.context.after());
    cues.named = next === currency.one || next === currency.many || next === "pesos";
  }
  return cues;
}

// --------------------
// ----- Patterns -----
// --------------------

// Ley 2/2018: the number of a law with its year. The law word stays, its numbers are the number and the year.
class LawPattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    // A law word, then spaces
    let wordEnd = position;
    while (wordEnd > 0 && isSpace(text[wordEnd - 1])) {
      wordEnd--;
    }
    let wordBegin = wordEnd;
    while (wordBegin > 0 && isWord(text[wordBegin - 1])) {
      wordBegin--;
    }
    if (wordEnd === position || !LAWS.has(toLower(str(text, wordBegin, wordEnd)))) {
      return undefined;
    }

    // Its number, then a year of 19xx or 20xx
    const number = digitsAt(text, position);
    const year = number.span.end + 1;
    if (
      number.span.end === position ||
      number.integer.length > 3 ||
      year >= text.length ||
      !contains("./-", text[year - 1])
    ) {
      return undefined;
    }
    const yearNumber = digitsAt(text, year);
    if (
      yearNumber.integer.length !== 4 ||
      !(yearNumber.integer.startsWith("19") || yearNumber.integer.startsWith("20"))
    ) {
      return undefined;
    }
    const end = yearNumber.span.end;
    if (!isClearAfter(text, end) || isDigitAfter(text, end, ".,/")) {
      return undefined;
    }
    return match({ kind: Kind.DATE, span: { begin: position, end }, numbers: [number, yearNumber] });
  }
}

// --------------------
// ----- Language -----
// --------------------

class Spanish extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new LawPattern());
    this.addPattern(new DatePattern(DATE_FORMAT));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SIGNS, ORDINAL_SIGN_FORMAT));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern());
  }

  protected override readNumber(number: Numeral, match: Match): string {
    return numberWith(number, cuesOf(match).one);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 6);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    // A law: Ley dos de dos mil dieciocho
    if (match.numbers.length === 2) {
      return `${cardinal(value(match.numbers[0]))} de ${cardinal(value(match.numbers[1]))}`;
    }

    const day = value(match.numbers[0]);
    return (
      (day === 1n ? "primero" : cardinal(day)) +
      ` de ${MONTHS[Number(value(match.numbers[1])) - 1]} de ${cardinal(value(match.numbers[2]))}`
    );
  }

  protected override readPhone(match: Match): string | undefined {
    // 600 123 456 is a count, unless a word says it is a phone number
    if (match.kind === Kind.PHONE && match.separator === " " && !PHONE_WORDS.has(match.context.before())) {
      return undefined;
    }
    return digits(match.text);
  }

  protected override readFraction(match: Match): string | undefined {
    return namedFraction(FRACTIONS, match);
  }

  protected override readTime(match: Match): string | undefined {
    // Only with minutes; a dot makes a time only after "a las": a las 18.30, but 1.13
    if (match.numbers.length < 2) {
      return undefined;
    }
    const decimal = match.separator === "." && !TIME_WORDS.has(match.context.before());
    if (!fitsClock(match) || decimal) {
      return undefined;
    }

    const minutes = value(match.numbers[1]);
    return cardinal(value(match.numbers[0]), One.UNA) + (minutes !== 0n ? ` ${cardinal(minutes)}` : "");
  }

  protected override readRoman(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    const role = romanRole(match, ROMAN_CUES);
    if (role === undefined || n >= 1000n) {
      return undefined;
    }
    const previous = match.context.before();
    switch (role) {
      case RomanRole.NAME: // Felipe sexto, Isabel segunda, but Alfonso trece
        return n <= 10n ? ordinal(n, FEMININE_NAMES.has(previous)) : cardinal(n);
      case RomanRole.AFTER: {
        // la Segunda Guerra, el primer congreso, el vigésimo tercer festival
        const feminine = FEMININE_ROMAN_NOUNS.has(toLower(match.context.after()));
        const words = ordinal(n, feminine);
        return !feminine && (words.endsWith("primero") || words.endsWith("tercero")) ? words.slice(0, -1) : words;
      }
      default: // siglo quinto, Vaticano segundo, but siglo veintiuno; capítulo cuatro
        return (previous.startsWith("siglo") || previous === "vaticano") && n <= 10n ? ordinal(n, false) : cardinal(n);
    }
  }

  protected override readOrdinal(match: Match): string | undefined {
    // Ordinal words reach 999
    const n = value(match.numbers[0]);
    if (n === 0n || n >= 1000n) {
      return undefined;
    }

    const suffix = match.suffix;
    if (suffix === "er" && (n === 1n || n === 3n)) {
      return n === 1n ? "primer" : "tercer";
    }
    const feminine =
      suffix === "ª" ||
      suffix === "era" ||
      suffix === "ra" ||
      suffix === "da" ||
      suffix === "ta" ||
      suffix === "va" ||
      suffix === "ma" ||
      suffix === "na";
    return ordinal(n, feminine);
  }

  protected override readDecade(match: Match): string | undefined {
    return `${this.readNumber(match.numbers[0], match)}s`; // los 80s: los ochentas
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];
    const one = currency.one === "libra" ? One.UNA : One.UN;

    // A price: cinco dólares con cincuenta
    if (number.fraction.length === 2 && match.scale === "") {
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      return (
        `${numberWith(whole, one)} ${number.integer === "1" ? currency.one : currency.many} con ` +
        cardinal(parseValue(number.fraction) ?? 0n)
      );
    }

    const words = match.scale === "" ? numberWith(number, one) : this.#scaled(number, match);
    if (cuesOf(match).named) {
      return words;
    }
    // tres millones de dólares, but cinco mil dólares
    const of = match.scale !== "" && SCALES[match.scale] !== "mil";
    return words + (of ? " de " : " ") + (isOne(number) && match.scale === "" ? currency.one : currency.many);
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    // 4K, 2K: a letter, read as a code
    if ((match.scale === "k" || match.scale === "K") && !counted(match.context)) {
      return undefined;
    }
    return this.#scaled(number, match);
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${numberWith(number, One.UNO)} por ciento${spacedSuffix(match)}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    const unit = UNITS[match.unit];
    const firstWord = unit.one.split(" ")[0];
    const one = feminine(firstWord) ? One.UNA : One.UN;
    return `${numberWith(number, one)} ${isOne(number) ? unit.one : unit.many}`;
  }

  // An amount with its scale: dos millones, cinco mil
  #scaled(number: Numeral, match: Match): string {
    const scale = SCALES[match.scale];
    const word = scale === "mil" ? "mil" : isOne(number) ? "millón" : "millones";
    return `${numberWith(number, One.UN)} ${word}`;
  }
}

let instance: Spanish | undefined;

export function language(): PatternLanguage {
  return (instance ??= new Spanish());
}
