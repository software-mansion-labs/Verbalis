import type { Context } from "../../text/context.ts";
import { DatePattern, PhonePattern, SuffixPattern } from "../../numbers/patterns.ts";
import { Kind, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  fitsClock,
  isOne,
  isSpokenDigits,
  namedFraction,
  parseValue,
  spacedSuffix,
  value,
} from "../../numbers/scanning.ts";
import { join } from "../../text/scan.ts";
import { size, toLower } from "../../text/strings.ts";
import { isAlpha, isLower } from "../../text/unicode.ts";
import { Gender } from "../grammar.ts";
import { PatternLanguage } from "../language.ts";
import {
  CURRENCIES,
  DATE_FORMAT,
  DAY_PARTS,
  FEMININE_ENDINGS,
  FEMININE_NOUNS,
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
  return `${numberWith(number, Gender.MASCULINE)} ${value(number) < 2n ? forms.one : forms.many}`;
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

// --------------------
// ----- Language -----
// --------------------

class Portuguese extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SIGNS, ORDINAL_SIGN_FORMAT));
  }

  protected override readNumber(number: Numeral, match: Match): string {
    const cues = cuesOf(match);
    if (number.fraction === "" && cues.day && number.integer === "1") {
      return "primeiro";
    }
    return numberWith(number, number.fraction === "" ? cues.gender : Gender.MASCULINE);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 6);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    const day = value(match.numbers[0]);
    return (
      (day === 1n ? "primeiro" : cardinal(day)) +
      ` de ${MONTHS[Number(value(match.numbers[1]) - 1n)]} de ${cardinal(value(match.numbers[2]))}`
    );
  }

  protected override readPhone(match: Match): string | undefined {
    // 2021-2022: a range of years; 213 456 789 is a count, unless a word says it is a phone number
    if (match.kind === Kind.PHONE) {
      const years = match.numbers.every(
        (group) => group.integer.length === 4 && value(group) >= 1900n && value(group) <= 2100n,
      );
      if (years || (match.separator === " " && !PHONE_WORDS.has(match.context.before()))) {
        return undefined;
      }
    }

    const groups = match.numbers.map((group) => (group.integer === "0800" ? "zero oitocentos" : digits(group.integer)));
    return join(groups, ", ");
  }

  protected override readFraction(match: Match): string | undefined {
    return namedFraction(FRACTIONS, match);
  }

  protected override readTime(match: Match): string | undefined {
    if (!fitsClock(match)) {
      return undefined;
    }

    // 14h30: catorze e trinta; 21h: vinte e uma horas
    const hours = value(match.numbers[0]);
    const minutes = match.numbers.length > 1 ? value(match.numbers[1]) : 0n;
    const hour = cardinal(hours, Gender.FEMININE);
    return minutes !== 0n ? `${hour} e ${cardinal(minutes)}` : hour + (hours === 1n ? " hora" : " horas");
  }

  protected override readOrdinal(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    if (n === 0n) {
      return undefined;
    }
    return n >= 10n && cuesOf(match).heading ? cardinal(n) : ordinal(n, match.suffix === "ª");
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

    if (cuesOf(match).named) {
      return match.scale === "" ? numberWith(amount, gender) : scaled(amount, match.scale, gender);
    }
    if (match.scale !== "") {
      return `${withDe(scaled(amount, match.scale, gender))} ${currency.many}`;
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
    const unit = UNITS[match.unit];
    const firstWord = unit.one.split(" ")[0];
    const gender = feminine(firstWord) ? Gender.FEMININE : Gender.MASCULINE;
    return `${numberWith(number, gender)} ${value(number) < 2n ? unit.one : unit.many}`;
  }

  protected override readRange(from: Numeral, to: Numeral, match: Match): string {
    // 2016-2022 and 10-15 read "a"; scores and formations (4-3-3, 60-40) keep "-"
    const plain = from.fraction === "" && to.fraction === "" && !from.grouped && !to.grouped;
    const rising = from.integer.length === to.integer.length && value(to) > value(from);
    return plain && rising && !cuesOf(match).lettered ? this.joiners().range : "-";
  }
}

let instance: Portuguese | undefined;

export function language(): PatternLanguage {
  return (instance ??= new Portuguese());
}
