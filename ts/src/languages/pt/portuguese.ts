import type { Context } from "../../text/context.ts";
import {
  SecondsPattern,
  DatePattern,
  Pattern,
  PhonePattern,
  SuffixPattern,
  RomanPattern,
} from "../../numbers/patterns.ts";
import { Kind, match, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  digitsAt,
  fitsClock,
  isOne,
  isSpokenDigits,
  fractionOf,
  isMarkedFraction,
  namedFraction,
  parseValue,
  spacedSuffix,
  value,
  romanRole,
  RomanRole,
  isOrdinalDegree,
} from "../../numbers/scanning.ts";
import { digitsEnd, isClearAfter, isClearBefore, isDigitAfter, join } from "../../text/scan.ts";
import { contains, size, startsWith, str, toLower, type Text } from "../../text/strings.ts";
import { isAlpha, isDigit, isLower } from "../../text/unicode.ts";
import { Gender } from "../grammar.ts";
import { PatternLanguage, type LanguageHandle } from "../language.ts";
import {
  CURRENCIES,
  DATE_FORMAT,
  DOT_DATE_FORMAT,
  DATE_WORDS,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  YEAR_WORDS,
  LAW_WORDS,
  DAY_PARTS,
  FEMININE_ENDINGS,
  FEMININE_NOUNS,
  FRACTION_WORDS,
  FRACTIONS,
  HEADINGS,
  HOUR_WORDS,
  HUNDREDS,
  JOINERS,
  MASCULINE_IN_A,
  MONTHS,
  NOTATION,
  ONES,
  ORDINAL_HUNDREDS,
  ORDINAL_ONES,
  ORDINAL_SIGN_FORMAT,
  ORDINAL_SIGNS,
  ORDINAL_TENS,
  PHONE_FORMATS,
  PHONE_WORDS,
  SCALE_WORDS,
  SCALES,
  STOP_WORDS,
  SYMBOLS,
  TENS,
  TERMS,
  UNITS,
  ROMAN_CUES,
  ROMAN_MARKERS,
  FEMININE_NAMES,
  FEMININE_ROMAN_NOUNS,
  ORDINAL_ARTICLES,
  FEMININE_ORDINAL_ARTICLES,
  ORDINAL_NOUNS,
} from "./constants.ts";

// What the words around a match say about reading its numbers
interface Cues {
  gender: Gender; // Of the noun after: duas pessoas; an hour is feminine (às duas)
  day: boolean; // 1 de maio: "1" before "de" and a month is "primeiro"
  heading: boolean; // artigo 80º: an ordinal after a legal heading reads as a cardinal
  lettered: boolean; // A320-200, 3-4x: a range touching letters is a plain hyphen
  named: boolean; // R$ 50 reais: the currency is already named after the amount
}

function isAsciiLetter(c: string | undefined): boolean {
  return c !== undefined && (c.codePointAt(0) as number) < 0x80 && isAlpha(c);
}

// ------------------------
// ----- Number words -----
// ------------------------

function unit(n: bigint, gender: Gender): string {
  if (gender === Gender.FEMININE && (n === 1n || n === 2n)) {
    return n === 1n ? "uma" : "duas";
  }
  return ONES[Number(n)];
}

function belowHundred(n: bigint, gender: Gender): string {
  if (n < 20n) {
    return unit(n, gender);
  }
  return TENS[Number(n / 10n)] + (n % 10n !== 0n ? ` e ${unit(n % 10n, gender)}` : "");
}

function belowThousand(n: bigint, gender: Gender): string {
  const h = n / 100n;
  const r = n % 100n;
  if (h === 0n) {
    return belowHundred(r, gender);
  }
  if (n === 100n) {
    return "cem";
  }

  // duzentas pessoas
  let head: string = HUNDREDS[Number(h)];
  if (gender === Gender.FEMININE && h > 1n) {
    head = `${head.slice(0, -2)}as`;
  }
  return head + (r !== 0n ? ` e ${belowHundred(r, gender)}` : "");
}

// "e" joins the last group below 100 or of round hundreds: dois mil e vinte, mil quinhentos e vinte, mil duzentos e um
function cardinal(n: bigint, gender: Gender = Gender.MASCULINE): string {
  const groups: Array<readonly [bigint, string]> = [];
  for (const [scale, one, many] of [
    [1000000000000n, "trilhão", "trilhões"],
    [1000000000n, "bilhão", "bilhões"],
    [1000000n, "milhão", "milhões"],
  ] as const) {
    const q = n / scale;
    if (q !== 0n) {
      groups.push([q, `${cardinal(q)} ${q === 1n ? one : many}`]);
    }
    n %= scale;
  }

  // Thousands agree with the noun: duas mil pessoas
  const q = n / 1000n;
  if (q !== 0n) {
    groups.push([q, q === 1n ? "mil" : `${belowThousand(q, gender)} mil`]);
  }
  n %= 1000n;
  if (n !== 0n || groups.length === 0) {
    groups.push([n, belowThousand(n, gender)]);
  }

  let words = groups[0][1];
  for (let i = 1; i < groups.length; ++i) {
    const last = groups[i][0];
    const joined = i + 1 === groups.length && (last < 100n || last % 100n === 0n);
    words += (joined ? " e " : " ") + groups[i][1];
  }
  return words;
}

// décimo terceiro, vigésima primeira, milésimo
function ordinal(n: bigint, feminine: boolean): string {
  const ending = feminine ? "a" : "o";
  const words: string[] = [];
  const thousands = n / 1000n;
  if (thousands !== 0n) {
    words.push(`${thousands === 1n ? "" : `${cardinal(thousands)} `}milésim${ending}`);
  }
  for (const [scale, stems] of [
    [100n, ORDINAL_HUNDREDS],
    [10n, ORDINAL_TENS],
    [1n, ORDINAL_ONES],
  ] as const) {
    const digit = (n / scale) % 10n;
    if (digit !== 0n) {
      words.push(stems[Number(digit)] + ending);
    }
  }
  return join(words);
}

function digits(text: string): string {
  return join([...text].map((c) => unit(BigInt(c.charCodeAt(0) - 0x30), Gender.MASCULINE)));
}

// um milhão de reais, but dez mil reais
function withDe(words: string): string {
  const scale = SCALE_WORDS.some((w) => words.endsWith(w));
  return scale ? `${words} de` : words;
}

// A number of a given gender, as before a currency or a unit
function numberWith(number: Numeral, gender: Gender): string {
  if (number.fraction === "") {
    const n = parseValue(number.integer);
    return n === undefined || isSpokenDigits(number, 7) ? digits(number.integer) : cardinal(n, gender);
  }

  // The separator is read as written: dois vírgula cinco, dois ponto cinco
  let fraction: string;
  if (number.fraction.length > 2) {
    fraction = digits(number.fraction);
  } else {
    const words: string[] = [];
    const zeros = number.fraction.search(/[^0]/);
    words.push(...Array<string>(zeros === -1 ? number.fraction.length : zeros).fill("zero"));
    if (zeros !== -1) {
      words.push(cardinal(parseValue(number.fraction) ?? 0n));
    }
    fraction = join(words);
  }
  const head = number.integer === "" ? "" : `${cardinal(value(number), gender)} `;
  return head + (number.decimalMark === "," ? "vírgula " : "ponto ") + fraction;
}

// An amount with its scale: vinte e duas mil pessoas, três milhões
function scaled(number: Numeral, scale: string, gender: Gender): string {
  const forms = SCALES[scale];
  if (forms.one === "mil") {
    return `${numberWith(number, gender)} mil`;
  }
  return `${numberWith(number, Gender.MASCULINE)} ${value(number) === 1n ? forms.one : forms.many}`;
}

// -------------------
// ----- Context -----
// -------------------

function feminine(noun: string): boolean {
  const word = toLower(noun);
  let singular = word;
  if (word.endsWith("ões") || word.endsWith("ães")) {
    singular = `${word.slice(0, -3)}ão`;
  } else if (word.endsWith("res") || word.endsWith("zes") || word.endsWith("les")) {
    singular = word.slice(0, -2);
  } else if (word.endsWith("s")) {
    singular = singular.slice(0, -1);
  }

  if (MASCULINE_IN_A.has(singular) || MASCULINE_IN_A.has(word)) {
    return false;
  }
  const feminineEnding = FEMININE_ENDINGS.some((e) => singular.endsWith(e));
  return FEMININE_NOUNS.has(singular) || FEMININE_NOUNS.has(word) || feminineEnding;
}

// The gender of the noun after a number; an hour is feminine: às duas, duas da tarde
function genderOf(context: Context): Gender {
  const word = context.after();
  if ((word === "da" || word === "de") && DAY_PARTS.has(toLower(context.after(2)))) {
    return Gender.FEMININE;
  }
  if (word === "" || !isAlpha(String.fromCodePoint(word.codePointAt(0) as number)) || STOP_WORDS.has(toLower(word))) {
    return HOUR_WORDS.has(context.before()) ? Gender.FEMININE : Gender.MASCULINE;
  }
  return feminine(word) ? Gender.FEMININE : Gender.MASCULINE;
}

// A lowercase noun follows: 500k seguidores
function counted(context: Context): boolean {
  const word = context.after();
  const lowercaseWord = size(word) > 2 && [...word].every((c) => isLower(c));
  return lowercaseWord && !STOP_WORDS.has(word);
}

// A range touching letters: A320-200, 3-4x
function lettered(range: Match, context: Context): boolean {
  const head = context.head();
  const tail = context.tail();
  const before = range.numbers[0].span.begin;
  const after = range.numbers[1].span.end - range.span.end;
  return (before > 0 && isAsciiLetter(head[before - 1])) || (after < tail.length && isAsciiLetter(tail[after]));
}

// A proper fraction with a small denominator, read as one without a word after it: 3/8, 5/13, 1/100
function smallFraction(a: bigint, b: bigint): boolean {
  return a < b && (b < 20n || b === 100n || b === 1000n);
}

// The numbers of a slashed pair which a text starts with (6/2), or ends with if fromEnd; undefined if none
function slashed(text: Text, fromEnd: boolean): [bigint, bigint] | undefined {
  let start = 0;
  if (fromEnd) {
    const end = text.length;
    start = end;
    while (start > 0 && isDigit(text[start - 1])) {
      start--;
    }
    if (start === end || start < 2 || text[start - 1] !== "/" || !isDigit(text[start - 2])) {
      return undefined;
    }
    start -= 2;
    while (start > 0 && isDigit(text[start - 1])) {
      start--;
    }
  }
  const slash = digitsEnd(text, start);
  const end = digitsEnd(text, slash + 1);
  if (slash === start || slash >= text.length || text[slash] !== "/" || end === slash + 1) {
    return undefined;
  }
  const a = parseValue(str(text, start, slash));
  const b = parseValue(str(text, slash + 1, end));
  if (a === undefined || b === undefined) {
    return undefined;
  }
  return [a, b];
}

// A phone number written with spaces only: after a word for one (ligue para 213 456 789), or nine digits that start
// as a Portuguese number does (2, 3, 7, 8, 9) and count nothing after them (através do 707 246 707 (dias úteis)), but
// not 213 456 789 pessoas or 300 000 000 €
function spacedPhone(phone: Match): boolean {
  const context = phone.context;
  if (PHONE_WORDS.has(context.before())) {
    return true;
  }
  const tail = context.tail();
  const next = tail[0] === " " ? 1 : 0;
  const word = toLower(context.after());
  const counts = next < tail.length && tail[next] !== "(" && word !== "" && !STOP_WORDS.has(word);
  const numbers = phone.numbers;
  const national = numbers.length === 3 && contains("23789", numbers[0].integer[0]);
  return national && numbers[numbers.length - 1].integer !== "000" && !counts;
}

// A round number before "s" which is a decade: anos 90s, década de 80s, '80s, 1980s
function isDecade(match: Match): boolean {
  const number = match.numbers[match.numbers.length - 1];
  const n = value(number);
  const word = match.context.before();
  const head = match.context.head();
  const marked =
    word === "anos" ||
    word === "década" ||
    (word === "de" && match.context.before(2) === "década") ||
    match.text.startsWith("'") ||
    head[head.length - 1] === "'";
  return (
    n % 10n === 0n &&
    ((number.integer.length === 2 && marked) || (number.integer.length === 4 && n >= 1000n && n <= 2100n))
  );
}

// A day and a month marked as a date by the word before them, or by parentheses: dia 15/9, quinta-feira (16/3);
// after "de" if they are no fraction: de 31/12, but de 1/3
function dated(date: Match): boolean {
  const word = date.context.before();
  const head = date.context.head();
  const tail = date.context.tail();
  const parenthesized = head[head.length - 1] === "(" && tail[0] === ")";
  const afterDe = word === "de" && !smallFraction(value(date.numbers[0]), value(date.numbers[1]));
  return DATE_WORDS.has(word) || word.endsWith("-feira") || parenthesized || afterDe;
}

// A fraction without a word after it: 3/8, 5/13, 1/100. Not a law number (Lei 15/98), a grade (18/20), or a score in
// a list of them (3/6, 6/2 e 7/6; 22/24 e 12/15).
function bareFraction(fraction: Match): boolean {
  const numbers = fraction.numbers;
  const leadingZero = numbers[0].integer[0] === "0" || numbers[numbers.length - 1].integer[0] === "0";
  const [a, b] = fractionOf(fraction);
  if (numbers.length !== 2 || leadingZero || !smallFraction(a, b) || LAW_WORDS.has(fraction.context.before())) {
    return false;
  }
  const head = fraction.context.head();
  const tail = fraction.context.tail();
  for (const joiner of [", ", " e "]) {
    const before = head.length - joiner.length;
    const next = startsWith(tail, 0, joiner) ? slashed(tail.slice(joiner.length), false) : undefined;
    const previous = before >= 0 && startsWith(head, before, joiner) ? slashed(head.slice(0, before), true) : undefined;
    for (const pair of [next, previous]) {
      if (pair !== undefined && !smallFraction(pair[0], pair[1])) {
        return false;
      }
    }
  }
  return true;
}

function cuesOf(match: Match): Cues {
  const context = match.context;
  const cues: Cues = {
    gender: genderOf(context),
    day: toLower(context.after()) === "de" && MONTHS.includes(toLower(context.after(2))),
    heading: HEADINGS.has(context.before()),
    lettered: false,
    named: false,
  };
  if (match.kind === Kind.RANGE) {
    cues.lettered = lettered(match, context);
  }

  // R$ 50 reais
  if (match.kind === Kind.MONEY) {
    const currency = CURRENCIES[match.unit];
    const next = toLower(context.after());
    cues.named = next === currency.one || next === currency.many;
  }
  return cues;
}

// ---------------------
// ----- Patterns -----
// ---------------------

// 2024/03/15, 2024.03.15, 03/15/2024: a date written year first, or month first where the day can't be a month.
// Its numbers are ordered as day, month, year.
class ReorderedDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const firstEnd = digitsEnd(text, position);
    if (!isClearBefore(text, position, ".,/-") || firstEnd === position || firstEnd >= text.length) {
      return undefined;
    }
    const parts = [digitsAt(text, position)];
    const separator = text[firstEnd];
    if (separator !== "/" && separator !== ".") {
      return undefined;
    }
    for (let end = parts[0].span.end; parts.length < 3; end = parts[parts.length - 1].span.end) {
      if (end + 1 >= text.length || text[end] !== separator || !isDigit(text[end + 1])) {
        return undefined;
      }
      parts.push(digitsAt(text, end + 1));
    }
    const end = parts[2].span.end;
    if (!isClearAfter(text, end) || isDigitAfter(text, end, ".,/-")) {
      return undefined;
    }

    const fits = (i: number, low: bigint, high: bigint) =>
      parts[i].integer.length <= 2 && value(parts[i]) >= low && value(parts[i]) <= high;
    const date = match({ kind: Kind.DATE, span: { begin: position, end }, separator });
    if (parts[0].integer.length === 4 && fits(1, 1n, 12n) && fits(2, 1n, 31n)) {
      date.numbers = [parts[2], parts[1], parts[0]];
    } else if (separator === "/" && fits(0, 1n, 12n) && fits(1, 13n, 31n) && parts[2].integer.length === 4) {
      date.numbers = [parts[1], parts[0], parts[2]];
    } else {
      return undefined;
    }
    return date;
  }
}

// --------------------
// ----- Language -----
// --------------------

class Portuguese extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new DatePattern(DOT_DATE_FORMAT));
    this.addPattern(new ReorderedDatePattern());
    this.addPattern(new SecondsPattern());
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SIGNS, ORDINAL_SIGN_FORMAT));
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern(false, ROMAN_MARKERS));
  }

  protected override readNumber(number: Numeral, match: Match): string {
    const cues = cuesOf(match);
    if (number.fraction === "" && cues.day && number.integer === "1") {
      return "primeiro";
    }
    // 1234567890 alone: the whole text is a number, not a code
    const n = parseValue(number.integer);
    if (
      n !== undefined &&
      number.fraction === "" &&
      number.integer[0] !== "0" &&
      match.context.head().length === 0 &&
      match.context.tail().length === 0
    ) {
      return cardinal(n, Gender.MASCULINE);
    }
    // Em 2020 havia: a year counts nothing after it
    const year =
      YEAR_WORDS.has(match.context.before()) &&
      number.integer.length === 4 &&
      !number.grouped &&
      value(number) >= 1000n &&
      value(number) <= 2100n;
    return numberWith(number, number.fraction === "" && !year ? cues.gender : Gender.MASCULINE);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 6);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    const day = value(match.numbers[0]);
    const words = `${day === 1n ? "primeiro" : cardinal(day)} de ${MONTHS[Number(value(match.numbers[1]) - 1n)]}`;
    if (match.numbers.length === 3) {
      return `${words} de ${cardinal(value(match.numbers[2]))}`;
    }

    // Without a year, a zero-padded or marked date: 03/04, dia 15/9, quinta-feira (16/3); but 1/4 do salário, 3/4
    const padded = match.numbers[0].integer[0] === "0" || match.numbers[1].integer[0] === "0";
    if (match.separator !== "/" || (!padded && (isMarkedFraction(match, FRACTION_WORDS) || !dated(match)))) {
      return undefined;
    }
    return words;
  }

  protected override readPhone(match: Match): string | undefined {
    // 2021-2022, 20 2021-2022, 1685-1750: a range of years; 500-1000: a range of round numbers; 213 456 789 pessoas:
    // a count
    if (match.kind === Kind.PHONE) {
      const year = (group: Numeral) => group.integer.length === 4 && value(group) >= 1000n && value(group) <= 2100n;
      const numbers = match.numbers;
      const n = numbers.length;
      const from = value(numbers[n - 2]);
      const to = value(numbers[n - 1]);
      const years = year(numbers[n - 2]) && year(numbers[n - 1]) && (from >= 1900n || to > from);
      const round = n === 2 && numbers[0].integer.endsWith("0") && numbers[1].integer.endsWith("0");
      const spaced = match.separator === " " && !match.text.includes("-");
      if (years || round || (spaced && !spacedPhone(match))) {
        return undefined;
      }
    }

    const groups = match.numbers.map((group) => (group.integer === "0800" ? "zero oitocentos" : digits(group.integer)));
    return join(groups, ", ");
  }

  protected override readFraction(match: Match): string | undefined {
    // 1/2 litro: meio litro; 1/2 xícara, 1 1/2 xícara: meia xícara, uma e meia xícara
    const words = namedFraction(FRACTIONS, match);
    if (words !== undefined) {
      return words === "meio" && cuesOf(match).gender === Gender.FEMININE ? "meia" : words;
    }
    const [a, b] = fractionOf(match);
    if (b < 2n || a >= b || !(isMarkedFraction(match, FRACTION_WORDS) || bareFraction(match))) {
      return undefined;
    }
    // um quinto, dois terços, um centésimo, três onze avos
    if (b > 10n && b !== 100n && b !== 1000n) {
      return `${cardinal(a)} ${cardinal(b)} avos`;
    }
    const denominator = b === 2n ? "meio" : b === 3n ? "terço" : ordinal(b, false);
    return `${cardinal(a)} ${denominator}${a === 1n ? "" : "s"}`;
  }

  protected override readTime(match: Match): string | undefined {
    // A dot makes a time only after an hour word or before "h": às 21.30, 21.30h, but 1.71
    const decimal = match.separator === "." && match.suffix === "" && !HOUR_WORDS.has(match.context.before());
    if (!fitsClock(match) || decimal) {
      return undefined;
    }

    // 15h da tarde: três horas da tarde; 21h da noite: nove horas da noite
    let hours = value(match.numbers[0]);
    const part = match.context.after() === "da" ? toLower(match.context.after(2)) : "";
    if ((part === "tarde" && hours >= 13n && hours <= 18n) || (part === "noite" && hours >= 19n && hours <= 23n)) {
      hours -= 12n;
    }

    // 14h30: catorze horas e trinta; 14:30:05: catorze horas e trinta minutos e cinco segundos
    const words = cardinal(hours, Gender.FEMININE) + (hours === 1n ? " hora" : " horas");
    const minutes = match.numbers.length > 1 ? value(match.numbers[1]) : 0n;
    if (match.numbers.length === 3) {
      const seconds = value(match.numbers[2]);
      return (
        `${words} e ${cardinal(minutes)}${minutes === 1n ? " minuto e " : " minutos e "}${cardinal(seconds)}` +
        (seconds === 1n ? " segundo" : " segundos")
      );
    }
    return minutes !== 0n ? `${words} e ${cardinal(minutes)}` : words;
  }

  protected override readRoman(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    const role = romanRole(match, ROMAN_CUES);
    if (role === undefined) {
      return undefined;
    }
    const previous = match.context.before();
    switch (role) {
      case RomanRole.NAME: {
        // Dom Pedro segundo, Dona Maria segunda, but Bento dezesseis
        const feminine = FEMININE_NAMES.has(previous);
        return n <= 10n ? ordinal(n, feminine) : cardinal(n, feminine ? Gender.FEMININE : Gender.MASCULINE);
      }
      case RomanRole.AFTER: // a Segunda Guerra, o terceiro congresso
        return ordinal(n, FEMININE_ROMAN_NOUNS.has(toLower(match.context.after())));
      default: // século quinto, Vaticano segundo, but século vinte e um; capítulo quatro
        return (previous.startsWith("século") || previous === "vaticano") && n <= 10n ? ordinal(n, false) : cardinal(n);
    }
  }

  protected override readOrdinal(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    if (n === 0n || (match.suffix === "°" && !isOrdinalDegree(match, ORDINAL_ARTICLES, ORDINAL_NOUNS))) {
      return undefined; // 39° 2' are degrees
    }
    // 2° takes the gender of its article or noun: na 5° edição
    const before = match.context.before();
    const feminineDegree =
      match.suffix.endsWith("°") &&
      (ORDINAL_ARTICLES.has(before) ? FEMININE_ORDINAL_ARTICLES.has(before) : feminine(match.context.after()));
    return n >= 10n && cuesOf(match).heading ? cardinal(n) : ordinal(n, match.suffix.endsWith("ª") || feminineDegree);
  }

  // anos 90s: anos noventa; anos 1980s: anos mil novecentos e oitenta
  protected override readDecade(match: Match): string | undefined {
    if (!isDecade(match)) {
      return undefined;
    }
    return cardinal(value(match.numbers[0]));
  }

  protected override readCompound(match: Match): string | undefined {
    return `${numberWith(match.numbers[0], Gender.MASCULINE)}-${match.suffix}`;
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];
    const gender = currency.one === "libra" ? Gender.FEMININE : Gender.MASCULINE;

    // R$ 37,517: prices have two decimals, so three digits are a thousands group
    let amount = number;
    if (
      number.decimalMark === "," &&
      number.integer !== "" &&
      number.integer.length <= 3 &&
      number.fraction.length === 3
    ) {
      amount = numeral({
        span: { begin: 0, end: 0 },
        integer: number.integer + number.fraction,
      });
    }
    // R$ 1811605: an amount, however long, is not a code
    amount = { ...amount, grouped: amount.grouped || !amount.integer.startsWith("0") };

    if (cuesOf(match).named) {
      return match.scale === "" ? numberWith(amount, gender) : scaled(amount, match.scale, gender);
    }
    if (match.scale !== "") {
      return `${withDe(scaled(amount, match.scale, gender))} ${currency.many}`;
    }
    // 66,23 euros: a written word keeps the decimal reading, without a zero fraction (3,00 euros: três euros)
    if (match.unit === "euros") {
      if (/^0*$/.test(amount.fraction)) {
        amount = numeral({ span: { begin: 0, end: 0 }, integer: amount.integer, grouped: amount.grouped });
      }
      return `${withDe(numberWith(amount, gender))} ${currency.many}`;
    }

    // A price: vinte e nove reais e noventa centavos
    if (amount.fraction.length === 2 && amount.decimalMark === ",") {
      const cents = parseValue(amount.fraction) ?? 0n;
      const centavos = cardinal(cents) + (cents === 1n ? " centavo" : " centavos");
      if (value(amount) === 0n) {
        return centavos;
      }
      const whole = numeral({
        span: { begin: 0, end: 0 },
        integer: amount.integer,
        grouped: amount.grouped,
      });
      const words = `${withDe(numberWith(whole, gender))} ${amount.integer === "1" ? currency.one : currency.many}`;
      return cents !== 0n ? `${words} e ${centavos}` : words;
    }
    return `${withDe(numberWith(amount, gender))} ${isOne(amount) ? currency.one : currency.many}`;
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    // 4K, 2K: a resolution; but 500k is quinhentos mil
    if (
      (match.scale === "k" || match.scale === "K") &&
      value(match.numbers[match.numbers.length - 1]) < 10n &&
      !counted(match.context)
    ) {
      return undefined;
    }
    return scaled(number, match.scale, cuesOf(match).gender);
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${numberWith(number, Gender.MASCULINE)} por cento${spacedSuffix(match)}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    // o 2° lugar, na 5° edição: an ordinal, read by its own pattern
    if (isOrdinalDegree(match, ORDINAL_ARTICLES, ORDINAL_NOUNS)) {
      return undefined;
    }
    // 45 s, 30s, but anos 90s
    if (match.unit === "s" && !match.text.endsWith(" s") && isDecade(match)) {
      return undefined;
    }
    const unit = UNITS[match.unit];
    const firstWord = unit.one.split(" ")[0];
    const gender = feminine(firstWord) ? Gender.FEMININE : Gender.MASCULINE;
    return `${numberWith(number, gender)} ${value(number) === 1n ? unit.one : unit.many}`;
  }

  protected override readRange(from: Numeral, to: Numeral, match: Match): string {
    // 2016-2022 and 10-15 read "a"; scores and formations (4-3-3, 60-40) keep "-"
    const plain = from.fraction === "" && to.fraction === "" && !from.grouped && !to.grouped;
    const rising = from.integer.length === to.integer.length && value(to) > value(from);
    return plain && rising && !cuesOf(match).lettered ? this.joiners().range : "-";
  }
}

let instance: Portuguese | undefined;

export function language(): LanguageHandle {
  return (instance ??= new Portuguese());
}
