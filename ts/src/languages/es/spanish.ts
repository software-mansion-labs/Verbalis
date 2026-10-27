import { Context } from "../../text/context.ts";
import {
  DatePattern,
  FractionPattern as GenericFractionPattern,
  Pattern,
  PhonePattern,
  PrefixedMoneyPattern,
  SuffixPattern,
  RomanPattern,
} from "../../numbers/patterns.ts";
import { Kind, match, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  digitsAt,
  numbersAt,
  fitsClock,
  isOne,
  isSpokenDigits,
  fractionOf,
  parseValue,
  spacedSuffix,
  value,
  romanRole,
  romanValue,
  toDigits,
  RomanRole,
} from "../../numbers/scanning.ts";
import {
  digitsEnd,
  isClearAfter,
  isClearBefore,
  isDigitAfter,
  isSentenceEnd,
  join,
  keysAt,
  skipSpace,
} from "../../text/scan.ts";
import { contains, size, str, toLower, type Text } from "../../text/strings.ts";
import { isAlpha, isDigit, isLower, isSpace, isWord } from "../../text/unicode.ts";
import type { Forms } from "../grammar.ts";
import { PatternLanguage } from "../language.ts";
import {
  CENTS,
  CURRENCIES,
  CURRENCY_WORDS,
  DATE_FORMAT,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  EVENING_SUFFIXES,
  FEMININE_ENDINGS,
  FEMININE_NOUNS,
  FRACTION_WORDS,
  DATE_WORDS,
  HOUR_SUFFIXES,
  HUNDREDS,
  INCH,
  INCH_MARKS,
  JOINERS,
  LAWS,
  MASCULINE_IN_A,
  MONTHS,
  MONTH_NUMBERS,
  MORNING_SUFFIXES,
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
  const ending = feminine ? "a" : "o";
  const words: string[] = [];
  const h = (n / 100n) % 10n;
  if (h !== 0n) {
    words.push(ORDINAL_HUNDREDS[Number(h)] + ending);
  }
  const r = n % 100n;
  if (r === 11n || r === 12n) {
    words.push((r === 11n ? "undécim" : "duodécim") + ending);
  } else if (r > 12n && r < 30n && r % 10n !== 0n) {
    let head = r < 20n ? "decimo" : "vigesimo";
    const unit = ORDINAL_ONES[Number(r % 10n)] + ending;
    if (unit.startsWith("o")) {
      head = head.slice(0, -1);
    }
    words.push(head + unit);
  } else {
    for (const [scale, stems] of [
      [10n, ORDINAL_TENS],
      [1n, ORDINAL_ONES],
    ] as const) {
      const digit = (r / scale) % 10n;
      if (digit !== 0n) {
        words.push(stems[Number(digit)] + ending);
      }
    }
  }
  return join(words);
}

// The denominator of a fraction from 11 to 99: onceavo, dieciseisavo, veintiunavo, treintavo, treintaiunavo
function avo(n: bigint): string {
  let words = cardinal(n).replace(" y ", "i").replaceAll("é", "e").replaceAll("ó", "o");
  if (words.endsWith("uno") || words.endsWith("a")) {
    words = words.slice(0, -1);
  }
  return `${words}avo`;
}

// The first word of a name: dólar of dólar estadounidense
function firstWord(name: string): string {
  return name.split(" ")[0];
}

// A fraction: un quinto, dos tercios, tres medios, un onceavo, un ducentésimo, un dosmilésimo, un millonésimo;
// uno sobre ciento uno. Before "partes", feminine: cinco sextas, una tercera
function fractionWords(a: bigint, b: bigint, parts = false): string {
  // The ordinal of a round thousand or million is one word: dosmilésimo, cienmillonésimo
  const round = (n: bigint, ending: string) => (n === 1n ? "" : cardinal(n, One.UN).replaceAll(" ", "")) + ending;
  let denominator: string;
  if (b <= 10n) {
    denominator = b === 2n ? "medio" : b === 3n ? "tercio" : ordinal(b, false);
  } else if (b < 100n) {
    denominator = avo(b);
  } else if (b < 1000n && b % 100n === 0n) {
    denominator = ordinal(b, false);
  } else if (b % 1000000n === 0n) {
    denominator = round(b / 1000000n, "millonésimo");
  } else if (b % 1000n === 0n && b < 1000000n) {
    denominator = round(b / 1000n, "milésimo");
  } else {
    return `${cardinal(a)} sobre ${cardinal(b)}`;
  }
  if (parts) {
    denominator = denominator === "tercio" ? "tercera" : `${denominator.slice(0, -1)}a`;
  }
  return `${cardinal(a, parts ? One.UNA : One.UN)} ${denominator}${a === 1n ? "" : "s"}`;
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

// A word before a numbered item, whose number is not a fraction or a date: nº 12/45, expediente 3/5
function isNumberWord(word: string): boolean {
  return word === "número" || word === "expediente" || LAWS.has(word);
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

  if (match.kind === Kind.MONEY) {
    // $5.000 dólares, $3 millones de dólares
    const currency = CURRENCIES[match.unit];
    const next = toLower(match.context.after(match.scale !== "" && match.context.after() === "de" ? 2 : 1));
    cues.named = next === firstWord(currency.one) || next === firstWord(currency.many) || next === "pesos";
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

// A fraction which the generic pattern leaves: with a minus (-1/3), a grouped denominator (1/100.000), or the unit
// it measures after it (1/2 km, 3 3/5 m). Its unit is the unit's key; a minus starts its text.
class FractionPattern extends Pattern {
  readonly #fractions = new GenericFractionPattern();

  constructor() {
    super(Kind.FRACTION);
  }

  match(text: Text, position: number): Match | undefined {
    const minus = text[position] === "-" && isClearBefore(text, position, ".,") && isDigitAfter(text, position + 1);
    // After a minus, the generic pattern reads the text from the number on, where a mixed number may start: -2 1/3
    const start = minus ? position + 1 : position;
    const offset = minus ? start : 0;
    let fraction = this.#fractions.match(text.slice(offset), start - offset);
    if (fraction !== undefined) {
      fraction.span.end += offset;
      for (const number of fraction.numbers) {
        number.span.begin += offset;
        number.span.end += offset;
      }
    }
    const grouped = fraction === undefined;
    fraction ??= groupedAt(text, start);
    if (fraction === undefined) {
      return undefined;
    }
    fraction.span.begin = position;

    // 3/4": tres cuartos de pulgada
    for (const mark of keysAt(text, fraction.span.end, INCH_MARKS)) {
      fraction.unit = mark;
      fraction.span.end += size(mark);
      return fraction;
    }
    const unitStart = skipSpace(text, fraction.span.end);
    for (const unit of keysAt(text, unitStart, NOTATION.units)) {
      if (isClearAfter(text, unitStart + size(unit))) {
        fraction.unit = unit;
        fraction.span.end = unitStart + size(unit);
        return fraction;
      }
    }
    return minus || grouped ? fraction : undefined;
  }
}

// 1/1.000.000
function groupedAt(text: Text, position: number): Match | undefined {
  const slash = digitsEnd(text, position);
  if (slash === position || slash + 1 >= text.length || text[slash] !== "/" || !isClearBefore(text, position, "./")) {
    return undefined;
  }
  const numbers = numbersAt(text, slash + 1, ".", "");
  if (
    numbers.length === 0 ||
    !numbers[0].grouped ||
    !isClearAfter(text, numbers[0].span.end, "/") ||
    isDigitAfter(text, numbers[0].span.end, ".,")
  ) {
    return undefined;
  }
  return match({
    kind: Kind.FRACTION,
    span: { begin: position, end: numbers[0].span.end },
    numbers: [digitsAt(text, position), numbers[0]],
    separator: "/",
  });
}

// The month written as a word at a position (abril, abr.): its number and end
function monthAt(text: Text, position: number): [bigint, number] | undefined {
  let end = position;
  while (end < text.length && isAlpha(text[end])) {
    end++;
  }
  if (end === position || !isClearBefore(text, position)) {
    return undefined;
  }
  const word = toLower(str(text, position, end));
  const month = MONTH_NUMBERS.get(word);
  if (month !== undefined && isClearAfter(text, end)) {
    return [month, end];
  }
  const abbreviation = MONTH_NUMBERS.get(`${word}.`);
  if (end < text.length && text[end] === "." && abbreviation !== undefined) {
    return [abbreviation, end + 1];
  }
  return undefined;
}

// A year of four digits at a position, after a space or a comma and a space: 10 mayo 1990, junio 11, 2000
function yearAt(text: Text, position: number): Numeral | undefined {
  const start = skipSpace(text, position < text.length && text[position] === "," ? position + 1 : position);
  if (
    start === position ||
    digitsEnd(text, start) !== start + 4 ||
    !isClearAfter(text, start + 4) ||
    isDigitAfter(text, start + 4, ".,")
  ) {
    return undefined;
  }
  return digitsAt(text, start);
}

function isDay(day: Numeral): boolean {
  const n = value(day);
  return day.integer.length <= 2 && n >= 1n && n <= 31n;
}

// 5 febrero, 10 mayo 1990, abril 30, junio 11 2000, abr. 22: a date with its month as a word and no "de" before it.
// Its numbers are the day, the month, and the year if written; its separator is a space.
class MonthDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const date = match({ kind: Kind.DATE, span: { begin: position, end: position }, separator: " " });
    if (isDigit(text[position])) {
      // The day, then the month
      const day = digitsAt(text, position);
      const month = isClearBefore(text, position, ".,/-") ? monthAt(text, skipSpace(text, day.span.end)) : undefined;
      if (month === undefined || skipSpace(text, day.span.end) === day.span.end || !isDay(day)) {
        return undefined;
      }
      date.numbers = [day, numeral({ span: { begin: 0, end: 0 }, integer: toDigits(month[0]) })];
      date.span.end = month[1];
    } else {
      // The month, then the day: the end of a phrase, or before a year (not "en junio 2 personas", nor the time of
      // "21 de octubre 12:00")
      const month =
        new Context(text, { begin: position, end: position }).before() === "de" ? undefined : monthAt(text, position);
      const start = month !== undefined ? skipSpace(text, month[1]) : 0;
      if (month === undefined || start === month[1] || start >= text.length || !isDigit(text[start])) {
        return undefined;
      }
      const day = digitsAt(text, start);
      const closed =
        isSentenceEnd(text, day.span.end) || (day.span.end < text.length && contains(",.;)", text[day.span.end]));
      if (!isDay(day) || !isClearAfter(text, day.span.end) || (!closed && yearAt(text, day.span.end) === undefined)) {
        return undefined;
      }
      date.numbers = [day, numeral({ span: { begin: 0, end: 0 }, integer: toDigits(month[0]) })];
      date.span.end = day.span.end;
    }
    const year = yearAt(text, date.span.end);
    if (year !== undefined) {
      date.numbers.push(year);
      date.span.end = year.span.end;
    }
    return date;
  }
}

// siglo xix, año mcmlxxxix, maría vii: a lowercase Roman numeral, after a century or a year, or of three letters or
// more after a name ("parte vi" may be "I saw")
class LowerRomanPattern extends Pattern {
  constructor() {
    super(Kind.ROMAN);
  }

  match(text: Text, position: number): Match | undefined {
    let end = position;
    while (end < text.length && contains("ivxlcdm", text[end])) {
      end++;
    }
    if (end === position || !isClearBefore(text, position, "'.-/") || !isClearAfter(text, end, "'-/")) {
      return undefined;
    }
    const letters = str(text, position, end).toUpperCase();
    const n = romanValue(letters);
    const previous = new Context(text, { begin: position, end }).before();
    const year = previous.startsWith("siglo") || previous === "año";
    const name = ROMAN_CUES.names.has(previous) && letters.length >= 3;
    if (
      n === undefined ||
      (letters.length === 1 && letters !== "I" && letters !== "V" && letters !== "X") ||
      !(year || name)
    ) {
      return undefined;
    }
    return match({
      kind: Kind.ROMAN,
      span: { begin: position, end },
      numbers: [numeral({ span: { begin: position, end }, integer: toDigits(n) })],
    });
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
    this.addPattern(new MonthDatePattern());
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new FractionPattern());
    this.addPattern(new PrefixedMoneyPattern(NOTATION, CURRENCY_WORDS));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SIGNS, ORDINAL_SIGN_FORMAT));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern());
    this.addPattern(new LowerRomanPattern());
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
    if (match.separator === "") {
      return `${cardinal(value(match.numbers[0]))} de ${cardinal(value(match.numbers[1]))}`;
    }

    // 30-2-1990 and 30.2.1990 need a full year (3.8.10 is a version)
    const day = value(match.numbers[0]);
    const month = value(match.numbers[1]);
    const slash = match.separator === "/";
    if (!slash && match.separator !== " " && (match.numbers.length < 3 || match.numbers[2].integer.length !== 4)) {
      return undefined;
    }

    // A day and month: 15/06, el 3/5, but 3/4 and el 1/3 de are fractions, 24/7 is a phrase, nº 3/5 a number
    if (slash && match.numbers.length === 2) {
      const previous = match.context.before();
      const padded = match.numbers[0].integer.startsWith("0") || match.numbers[1].integer.startsWith("0");
      const cue = DATE_WORDS.has(previous) && !FRACTION_WORDS.has(toLower(match.context.after()));
      const tail = match.context.tail();
      const inch = tail[0] === '"' || (tail[0] === "'" && tail[1] === "'") || tail[0] === "″" || tail[0] === "”";
      if ((day < month && !padded && !cue) || (day === 24n && month === 7n) || inch || isNumberWord(previous)) {
        return undefined;
      }
    }

    const words = `${day === 1n ? "primero" : cardinal(day)} de ${MONTHS[Number(month) - 1]}`;
    return match.numbers.length === 2 ? words : `${words} de ${cardinal(value(match.numbers[2]))}`;
  }

  protected override readPhone(match: Match): string | undefined {
    // 600 123 456 is a count, unless a word says it is a phone number
    if (match.kind === Kind.PHONE && match.separator === " " && !PHONE_WORDS.has(match.context.before())) {
      return undefined;
    }
    return digits(match.text);
  }

  protected override readFraction(match: Match): string | undefined {
    // un quinto, dos tercios; 24/7, but nº 12/45 is a number
    const [a, b] = fractionOf(match);
    if (a === 24n && b === 7n) {
      return "veinticuatro siete";
    }
    // Not a number with its year (117/2004) or a code (023/2006, 8/12-2K, SUP-REC-15/2011)
    const previous = match.context.before();
    const year = a > 1n && b >= 1900n && b < 2100n;
    const head = match.context.head();
    const code =
      match.numbers[match.numbers.length - 2].integer.startsWith("0") ||
      match.context.tail()[0] === "-" ||
      head[head.length - 1] === "-";
    if (b < 2n || a >= b || year || code || isNumberWord(previous)) {
      return undefined;
    }
    // medio limón, media taza; el tercio de, but un tercio
    const one = cuesOf(match).one;
    if (a === 1n && b === 2n && one !== One.UNO) {
      return one === One.UNA ? "media" : "medio";
    }
    const next = toLower(match.context.after());
    const words = fractionWords(a, b, next === "parte" || next === "partes");
    const article = previous === "el" || previous === "del" || previous === "al" || previous === "un";
    return a === 1n && article && words.startsWith("un ") ? words.slice(3) : words;
  }

  protected override readMixedFraction(match: Match): string | undefined {
    // -1/3: menos un tercio
    if (match.text.startsWith("-")) {
      const words = this.readMixedFraction({ ...match, text: match.text.slice(1) });
      return words === undefined ? undefined : `menos ${words}`;
    }

    const mixed = match.numbers.length === 3;
    if (!mixed && match.unit === "") {
      return this.readFraction(match);
    }
    const [a, b] = fractionOf(match);
    if (b < 2n || a >= b) {
      return undefined;
    }

    // medio milímetro, tres quintos de milla por hora; un metro y medio, tres y tres quintos de metro, dos y media
    const unit = match.unit === "" ? undefined : Object.hasOwn(UNITS, match.unit) ? UNITS[match.unit] : INCH;
    const one = unit !== undefined ? (feminine(firstWord(unit.one)) ? One.UNA : One.UN) : cuesOf(match).one;
    const half = a === 1n && b === 2n;
    const fraction = half ? (one === One.UNA ? "media" : "medio") : fractionWords(a, b);
    if (!mixed) {
      // 1/2"Para: a word right after an inch mark gets a space
      const glued = unit === INCH && isAlpha(match.context.tail()[0] ?? " ");
      return fraction + (half ? " " : " de ") + (unit as Forms).one + (glued ? " " : "");
    }
    const whole = numberWith(match.numbers[0], one);
    if (half && unit !== undefined) {
      const words = isOne(match.numbers[0]) ? unit.one : unit.many;
      const per = words.indexOf(" por ");
      const at = per === -1 ? words.length : per;
      return `${whole} ${words.slice(0, at)} y ${fraction}${words.slice(at)}`;
    }
    return `${whole} y ${fraction}${unit !== undefined ? ` de ${unit.one}` : ""}`;
  }

  protected override readTime(match: Match): string | undefined {
    const hour = HOUR_SUFFIXES.includes(match.suffix);
    const morning = MORNING_SUFFIXES.includes(match.suffix);
    const evening = EVENING_SUFFIXES.includes(match.suffix);
    const hours = value(match.numbers[0]);
    let words = cardinal(hours, One.UNA);

    // 19 h, 80hrs: diecinueve horas; 7 p. m. needs a clock hour
    if (match.numbers.length < 2) {
      if (hour) {
        words += hours === 1n ? " hora" : " horas";
      } else if (hours === 0n || hours > 12n) {
        return undefined;
      }
    } else {
      // A dot makes a time only after "a las" or before a suffix: a las 18.30, 18.30 h, a 13.30, but 1.13, a 3.28
      const minutes = value(match.numbers[1]);
      const previous = match.context.before();
      const cue = TIME_WORDS.has(previous) || (previous === "a" && minutes % 15n === 0n);
      const decimal = match.separator === "." && match.suffix === "" && !cue;
      if (!fitsClock(match) || decimal || ((morning || evening) && hours > 12n)) {
        return undefined;
      }
      // cinco y dos, doce y cuarto, trece y media; 16:00 h: dieciséis horas
      if (minutes === 0n && hour) {
        words += hours === 1n ? " hora" : " horas";
      } else if (minutes !== 0n) {
        words += ` y ${minutes === 15n ? "cuarto" : minutes === 30n ? "media" : cardinal(minutes)}`;
      }
    }

    // 1 a. m.: una de la mañana; 12 p. m.: doce del mediodía; 9 p. m.: nueve de la noche
    if (morning) {
      words += hours === 12n ? " de la noche" : " de la mañana";
    } else if (evening) {
      words += hours === 12n ? " del mediodía" : hours < 8n ? " de la tarde" : " de la noche";
    }
    // The dot of "a. m." or "hrs." also closes a sentence: a las 2:43 a. m. La mayoría
    if (match.suffix.endsWith(".") && isSentenceEnd(match.context.tail(), 0, true)) {
      words += ".";
    }
    return words;
  }

  protected override readRoman(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    const previous = match.context.before();
    let role = romanRole(match, ROMAN_CUES);
    if (isLower(match.text[0])) {
      role = ROMAN_CUES.names.has(previous) ? RomanRole.NAME : RomanRole.BEFORE; // As LowerRomanPattern found
    }

    // el XI apóstol: after an article and before a noun, a numeral of I, V and X is an ordinal
    const next = match.context.after();
    const article = previous === "el" || previous === "la" || previous === "los" || previous === "las";
    const noun = next !== "" && isLower(String.fromCodePoint(next.codePointAt(0) as number)) && !STOP_WORDS.has(next);
    if (role === undefined && article && noun && /^[IVX]+$/.test(match.text)) {
      const words = ordinal(n, previous === "la" || previous === "las");
      return words.endsWith("primero") || words.endsWith("tercero") ? words.slice(0, -1) : words;
    }
    // Years reach 3999: año MCMLXXXIX
    if (role === undefined || (n >= 1000n && role !== RomanRole.BEFORE)) {
      return undefined;
    }
    switch (role) {
      case RomanRole.NAME: // Felipe sexto, Isabel segunda, but Alfonso trece
        return n <= 10n ? ordinal(n, FEMININE_NAMES.has(previous)) : cardinal(n);
      case RomanRole.AFTER: {
        // la Segunda Guerra, el primer congreso, el vigesimotercer festival
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

    // 1.er, 21ᵉʳ: vigesimoprimer; 4.ª, 2da: feminine
    const suffix = match.suffix;
    let words = ordinal(n, suffix.endsWith("a") || suffix.endsWith("ª"));
    if ((suffix.endsWith("er") || suffix.endsWith("ᵉʳ")) && (words.endsWith("primero") || words.endsWith("tercero"))) {
      words = words.slice(0, -1);
    }
    return words;
  }

  protected override readDecade(match: Match): string | undefined {
    return `${this.readNumber(match.numbers[0], match)}s`; // los 80s: los ochentas
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];
    const one = feminine(firstWord(currency.one)) ? One.UNA : One.UN;
    const named = cuesOf(match).named;

    // Q1 to Q9 are quarters
    if ((match.unit === "Q" || match.unit === "q") && number.integer.length === 1 && number.fraction === "") {
      return undefined;
    }

    // A price: cinco dólares con cincuenta, cincuenta y cinco centavos, un euro con cinco céntimos, diez dólares
    const places = number.fraction.length;
    if (match.scale === "" && (places === 1 || places === 2) && Object.hasOwn(CENTS, match.unit)) {
      const cents = CENTS[match.unit];
      const n = (parseValue(number.fraction) ?? 0n) * (places === 1 ? 10n : 1n);
      const centWords = `${cardinal(n, One.UN)} ${n === 1n ? cents.one : cents.many}`;
      if (value(number) === 0n && n > 0n) {
        return centWords;
      }
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      let words = numberWith(whole, one);
      if (!named) {
        words += ` ${isOne(whole) ? currency.one : currency.many}`;
      }
      if (n === 0n) {
        return words;
      }
      return `${words} con ${n < 10n && !named ? centWords : cardinal(n)}`;
    }

    const words = match.scale === "" ? numberWith(number, one) : this.#scaled(number, match);
    if (named) {
      // un millón de pesos
      const of = (words.endsWith("llón") || words.endsWith("llones")) && match.context.after() !== "de";
      return of ? `${words} de` : words;
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
    // 4 s, but 6s and los 60s
    if (match.unit === "s" && !match.text.includes(" ")) {
      return undefined;
    }
    const unit = UNITS[match.unit];
    // 4,51/s: cuatro coma cincuenta y uno por segundo
    const one = unit.one.startsWith("por ") ? One.UNO : feminine(firstWord(unit.one)) ? One.UNA : One.UN;
    return `${numberWith(number, one)} ${isOne(number) ? unit.one : unit.many}`;
  }

  // An amount with its scale: dos millones, cinco mil
  #scaled(number: Numeral, match: Match): string {
    const scale = SCALES[match.scale];
    const word = scale === "mil" || isOne(number) ? scale : `${scale.slice(0, -2)}ones`;
    return `${numberWith(number, One.UN)} ${word}`;
  }
}

let instance: Spanish | undefined;

export function language(): PatternLanguage {
  return (instance ??= new Spanish());
}
