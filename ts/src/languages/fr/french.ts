import {
  DatePattern,
  GroupedOrdinalPattern,
  Pattern,
  PhonePattern,
  SuffixPattern,
  RomanPattern,
  RomanOrdinalPattern,
} from "../../numbers/patterns.ts";
import { Kind, match, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  digitsAt,
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
} from "../../numbers/scanning.ts";
import { stripped, wordsOf, type Context } from "../../text/context.ts";
import { digitsEnd, isClearAfter, isClearBefore, isDigitAfter, join, keysAt, skipSpace } from "../../text/scan.ts";
import { contains, size, str, toLower, type Text } from "../../text/strings.ts";
import { isAlpha, isLower, isUpper } from "../../text/unicode.ts";
import type { Forms } from "../grammar.ts";
import { PatternLanguage, type LanguageHandle } from "../language.ts";
import {
  CODE_WORDS,
  CURRENCIES,
  DASH_DATE_FORMAT,
  DATE_FORMAT,
  DECADE_WORDS,
  FEMININE_ENDINGS,
  FEMININE_NOUNS,
  FRACTION_WORDS,
  FRACTIONS,
  JOINERS,
  MASCULINE_ENDINGS,
  MASCULINE_IN_E,
  MONTHS,
  NOTATION,
  NUMBER_WORDS,
  ONES,
  ORDINAL_SUFFIXES,
  ORDINAL_WORDS,
  PHONE_FORMAT,
  SCALES,
  STOP_WORDS,
  SYMBOLS,
  TENS,
  TERMS,
  TIME_WORDS,
  UNITS,
  VERSION_WORDS,
  WEEKDAYS,
  ROMAN_CUES,
  ROMAN_ORDINAL_SUFFIXES,
  FEMININE_NAMES,
  FEMININE_ROMAN_NOUNS,
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

  // vingtième, cinquième, neuvième, onzième; millionième, deux millionièmes: without "un" and the plural s
  let words = cardinal(n);
  if (words === "un million" || words === "un milliard") {
    words = words.slice(3);
  }
  for (const plural of ["vingts", "cents", "millions", "milliards"]) {
    if (words.endsWith(plural)) {
      words = words.slice(0, -1);
    }
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
    return n === undefined || isSpokenDigits(number, 8) ? digits(number.integer) : cardinal(n, feminine, cues.pluralS);
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

// A capitalized word after a number starts a title or a name, not a noun it counts: Rennes 1 Situation
function feminine(noun: string): boolean {
  if (noun !== "" && isUpper(String.fromCodePoint(noun.codePointAt(0)!))) {
    return false;
  }
  let word = toLower(noun);
  word = word.slice(word.lastIndexOf("-") + 1);
  if (!FEMININE_NOUNS.has(word) && word.endsWith("s")) {
    word = word.slice(0, -1);
  }
  // eurodéputé is masculine as député is, but propriété is feminine although été is masculine
  const masculine = (noun: string): boolean => word === noun || (noun.length > 4 && word.endsWith(noun));
  const feminineEnding = FEMININE_ENDINGS.some((e) => word.endsWith(e));
  if (FEMININE_NOUNS.has(word) || (feminineEnding && ![...MASCULINE_IN_E].some(masculine))) {
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

// An ungrouped number of five digits or more is an amount (166870 euros, 12500 habitants, 415297) unless something
// marks it as a code: a hyphen or a slash (0-89100-396-7), a word naming a code (numéro 1035836), or a name next to
// it (75005 Paris, Naca 23012) without a noun it counts after it (Environ 12500 personnes)
function isCode(number: Numeral, match: Match): boolean {
  if (number.grouped || number.fraction !== "" || number.integer.length < 5) {
    return false;
  }
  const head = match.context.head();
  const tail = match.context.tail();
  const joined =
    (head.length > 0 && contains("-/", head[head.length - 1])) || (tail.length > 0 && contains("-/", tail[0]));
  const words = wordsOf(head);
  const previous = words.length === 0 ? [] : stripped(words[words.length - 1]);
  const after = match.context.after();
  const name =
    (previous.length > 0 && isUpper(previous[0])) ||
    (after !== "" && isUpper(String.fromCodePoint(after.codePointAt(0)!)));
  return joined || CODE_WORDS.has(match.context.before()) || (name && !counted(match.context));
}

// A noun after a number is singular below 2: un virgule cinq kilomètre, deux kilomètres
function numberForm(number: Numeral, forms: Forms): string {
  return value(number) < 2n ? forms.one : forms.many;
}

// En 1900, vers 1980: a year, whose quatre-vingt and cent take no plural s
function isYear(match: Match): boolean {
  if (match.kind !== Kind.NUMBER || match.numbers.length !== 1) {
    return false;
  }
  const number = match.numbers[0];
  const n = value(number);
  const previous = match.context.before();
  const dimension = previous === "par" || previous === "fois"; // 1920x1080, read before
  return !number.grouped && number.fraction === "" && n >= 1000n && n < 2100n && !counted(match.context) && !dimension;
}

function cuesOf(match: Match): Cues {
  const previous = match.context.before();
  return {
    feminine: counted(match.context) && feminine(match.context.after()),
    pluralS: !ORDINAL_WORDS.has(previous) && !isYear(match),
    equation: match.kind === Kind.DIMENSION && match.context.tail().includes("="),
  };
}

// A noun after "de": de dollars, d'euros
function of(noun: string): string {
  const vowel = contains("aeiouyéh", noun[0]);
  return (vowel ? "d'" : "de ") + noun;
}

// Whether a whole amount ends in millions or billions, which take "de" before a noun: un million de dollars
function roundMillions(number: Numeral): boolean {
  const n = value(number);
  return number.fraction === "" && n >= 1000000n && n % 1000000n === 0n;
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

// 3 1/2 km: a whole number and a half before a masculine unit, read "trois kilomètres et demi" (the whole number by
// readNumber, and "et demi" by the joiners)
class HalfUnitPattern extends Pattern {
  constructor() {
    super(Kind.FRACTION);
  }

  match(text: Text, position: number): Match | undefined {
    const wholeEnd = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, "./-:,") ||
      wholeEnd === position ||
      wholeEnd > position + 3 ||
      str(text, wholeEnd, wholeEnd + 5) !== " 1/2 "
    ) {
      return undefined;
    }
    const unit = wholeEnd + 5;
    for (const key of keysAt(text, unit, NOTATION.units)) {
      const one = UNITS[key].one;
      const space = one.indexOf(" ");
      if (isClearAfter(text, unit + key.length) && !feminine(space === -1 ? one : one.slice(0, space))) {
        return match({
          kind: Kind.FRACTION,
          span: { begin: position, end: unit + key.length },
          numbers: [digitsAt(text, position), digitsAt(text, wholeEnd + 1), digitsAt(text, wholeEnd + 3)],
          unit: key,
        });
      }
    }
    return undefined;
  }
}

// 3 € 50, 12 € 99: euros and cents
class EuroCentsPattern extends Pattern {
  constructor() {
    super(Kind.MONEY);
  }

  match(text: Text, position: number): Match | undefined {
    const euro = skipSpace(text, digitsEnd(text, position));
    const cents = skipSpace(text, euro + 1);
    if (
      !isClearBefore(text, position, ".,-") ||
      euro === position ||
      euro >= text.length ||
      text[euro] !== "€" ||
      digitsEnd(text, cents) !== cents + 2 ||
      !isClearAfter(text, cents + 2) ||
      isDigitAfter(text, cents + 2, ".,")
    ) {
      return undefined;
    }
    const amount = digitsAt(text, position);
    const numbers = [
      { ...amount, span: { begin: amount.span.begin, end: cents + 2 }, fraction: str(text, cents, cents + 2) },
    ];
    return match({ kind: Kind.MONEY, span: { begin: position, end: cents + 2 }, numbers, unit: "€" });
  }
}

// 1m65: a height in meters and centimeters
class MetersPattern extends Pattern {
  constructor() {
    super(Kind.UNIT);
  }

  match(text: Text, position: number): Match | undefined {
    const m = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, ".,-") ||
      m === position ||
      m > position + 1 ||
      m >= text.length ||
      text[m] !== "m" ||
      digitsEnd(text, m + 1) !== m + 3 ||
      !isClearAfter(text, m + 3)
    ) {
      return undefined;
    }
    const length = digitsAt(text, position);
    const numbers = [{ ...length, span: { begin: length.span.begin, end: m + 3 }, fraction: str(text, m + 1, m + 3) }];
    return match({ kind: Kind.UNIT, span: { begin: position, end: m + 3 }, numbers, unit: "m" });
  }
}

// les 17/18 juin, les 17-18-19 juin: days of a month, listed
class DayListPattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,/-")) {
      return undefined;
    }
    const numbers: Numeral[] = [];
    let separator = "";
    let daysEnd = position;
    for (let p = position; ; p++) {
      const end = digitsEnd(text, p);
      const day = parseValue(str(text, p, end)) ?? 0n;
      if (end > p + 2 || day < 1n || day > 31n) {
        return undefined;
      }
      numbers.push(digitsAt(text, p));
      daysEnd = end;
      const next = end < text.length && (text[end] === "/" || text[end] === "-");
      if (!next || (separator !== "" && text[end] !== separator)) {
        break;
      }
      separator = text[end];
      p = end;
    }

    // Two days or more with a slash, or three with a hyphen (17-18 juin is a range), then a month
    const least = separator === "/" ? 2 : 3;
    const month = daysEnd + 1;
    let monthEnd = month;
    while (monthEnd < text.length && isAlpha(text[monthEnd])) {
      monthEnd++;
    }
    const beforeMonth =
      daysEnd < text.length &&
      text[daysEnd] === " " &&
      (MONTHS as readonly string[]).includes(str(text, month, monthEnd));
    if (numbers.length < least || !beforeMonth) {
      return undefined;
    }
    return match({ kind: Kind.DATE, span: { begin: position, end: daysEnd }, numbers, separator });
  }
}

// --------------------
// ----- Language -----
// --------------------

class French extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DayListPattern());
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new DatePattern(DASH_DATE_FORMAT));
    this.addPattern(new PhonePattern(PHONE_FORMAT));
    this.addPattern(new EuroCentsPattern());
    this.addPattern(new MetersPattern());
    this.addPattern(new HalfUnitPattern());
    this.addPattern(new GroupedOrdinalPattern(ORDINAL_SUFFIXES, " \u00a0\u202f"));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new RomanOrdinalPattern(ROMAN_ORDINAL_SUFFIXES));
    this.addPattern(new RomanPattern());
  }

  protected override readNumber(number: Numeral, match: Match): string {
    if (isCode(number, match)) {
      return digits(number.integer);
    }
    // 3 1/2 km: trois kilomètres (et demi)
    if (match.kind === Kind.FRACTION && match.unit !== "") {
      return `${cardinal(value(number))} ${numberForm(number, UNITS[match.unit])}`;
    }
    const cues = cuesOf(match);
    return gendered(number, cues, number.fraction === "" && cues.feminine);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 4);
    return n !== undefined ? cardinal(n, false, false) : digits(text); // A380: a trois cent quatre-vingt
  }

  protected override readDate(match: Match): string | undefined {
    // les dix-sept, dix-huit juin
    if (
      match.numbers.length > 1 &&
      !match.numbers[0].integer.startsWith("0") &&
      (MONTHS as readonly string[]).includes(match.context.after())
    ) {
      return match.numbers.map((day) => (day.integer === "1" ? ordinal(1n) : cardinal(value(day)))).join(", ");
    }
    // 3.8.10 after "version", or with a one-digit day and month and a short year, is a version
    const numbers = match.numbers;
    const shortVersion =
      match.separator === "." &&
      numbers.length === 3 &&
      numbers[2].integer.length === 2 &&
      numbers[0].integer.length === 1 &&
      numbers[1].integer.length === 1;
    if (shortVersion || VERSION_WORDS.has(match.context.before())) {
      return undefined;
    }
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
    const words = namedFraction(FRACTIONS, match);
    if (words !== undefined) {
      return words;
    }
    // un cinquième, deux tiers, trois quarts; else trois sur sept
    const [a, b] = fractionOf(match);
    if (b < 2n || a >= b || (!isMarkedFraction(match, FRACTION_WORDS) && !French.#isPlainFraction(match))) {
      return `${cardinal(a)} sur ${cardinal(b)}`;
    }
    const denominator = b === 2n ? "demi" : b === 3n ? "tiers" : b === 4n ? "quart" : ordinal(b);
    return `${cardinal(a)} ${denominator}${a > 1n && b !== 3n ? "s" : ""}`;
  }

  // A bare unit fraction of a small denominator: 1/5, 1/10; but 2/9 may be a score (5/9 au tir), 1/6 tirs a count, and
  // (1/6) a part of a series
  static #isPlainFraction(match: Match): boolean {
    const [a, b] = fractionOf(match);
    const head = match.context.head();
    return a === 1n && b <= 10n && !counted(match.context) && head[head.length - 1] !== "(";
  }

  protected override readTime(match: Match): string | undefined {
    if (match.numbers.length === 2) {
      // à 18.30, but à 0.40%
      const decimal =
        match.separator === "." && (!TIME_WORDS.has(match.context.before()) || match.context.tail()[0] === "%");
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

  protected override readRoman(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    const role = romanRole(match, ROMAN_CUES);
    if (role === undefined) {
      return undefined;
    }
    switch (role) {
      case RomanRole.NAME: // Louis quatorze, but François premier, Élisabeth première
        return n === 1n ? (FEMININE_NAMES.has(match.context.before()) ? "première" : "premier") : cardinal(n);
      case RomanRole.AFTER: // le XXI siècle: vingt et unième; la V République: cinquième
        return ordinal(n, FEMININE_ROMAN_NOUNS.has(toLower(match.context.after())));
      default:
        return cardinal(n); // chapitre trois
    }
  }

  protected override readOrdinal(match: Match): string | undefined {
    const suffix = match.suffix;
    let words: string;
    if (suffix.startsWith("nd")) {
      words = suffix.startsWith("nde") ? "seconde" : "second";
    } else {
      const feminine = suffix === "re" || suffix === "ère" || suffix === "ere" || suffix === "res" || suffix === "ères";
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
      return `${words} ${of(currency.many)}`;
    }

    // A price: trois euros cinquante, quatre-vingt-dix-neuf centimes, trois euros cinquante par mois
    const feminine = currency.one === "livre";
    if (number.fraction.length === 2) {
      const cents = parseValue(number.fraction) ?? 0n;
      const per = match.unit.includes("/");
      const unit = per ? currency.one.slice(currency.one.indexOf(" ")) : "";
      if (value(number) === 0n) {
        return `${cardinal(cents)} centimes${unit}`;
      }
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      const words = number.integer === "1" ? currency.one : currency.many;
      return (
        `${gendered(whole, cues, feminine)} ${words.slice(0, words.length - unit.length)} ` + cardinal(cents) + unit
      );
    }
    const noun = numberForm(number, currency);
    return `${gendered(number, cues, feminine)} ${roundMillions(number) ? of(noun) : noun}`;
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
    // les 80s, les 1980s: a decade (les années quatre-vingt), not seconds
    const n = value(number);
    const decade = n % 10n === 0n && ((n >= 10n && n < 100n) || (n >= 1000n && n < 10000n)) && number.fraction === "";
    if (
      match.unit === "s" &&
      decade &&
      match.numbers.length === 1 &&
      !match.text.includes(" ") &&
      DECADE_WORDS.has(match.context.before())
    ) {
      return `années ${cardinal(n, false, false)}`;
    }
    // 1,80 m, 1m65: un mètre quatre-vingts, un mètre soixante-cinq
    const meters = match.unit === "m" && n >= 1n && n < 10n && number.fraction !== "" && number.fraction.length <= 2;
    if (meters && match.numbers.length === 1) {
      const centimeters = (parseValue(number.fraction) ?? 0n) * (number.fraction.length === 1 ? 10n : 1n);
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer });
      return `${cardinal(n)} ${numberForm(whole, UNITS["m"])}` + (centimeters ? ` ${cardinal(centimeters)}` : "");
    }
    const unit = UNITS[match.unit];
    const space = unit.one.indexOf(" ");
    const firstWord = space === -1 ? unit.one : unit.one.slice(0, space);
    return `${gendered(number, cuesOf(match), feminine(firstWord))} ${numberForm(number, unit)}`;
  }

  protected override readJoiner(match: Match): string | undefined {
    if (match.kind === Kind.DIMENSION) {
      return cuesOf(match).equation ? " fois " : " par ";
    }
    // 0-89100-396-7: a hyphen in a code, not a range
    if (
      match.kind === Kind.RANGE &&
      match.numbers.some((number) => !number.grouped && number.integer.length >= 5 && !number.integer.endsWith("000"))
    ) {
      return undefined;
    }
    return super.readJoiner(match);
  }
}

let instance: French | undefined;

export function language(): LanguageHandle {
  return (instance ??= new French());
}
