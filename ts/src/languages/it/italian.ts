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
import { isClearAfter, isClearBefore, join } from "../../text/scan.ts";
import { split, toLower, type Text } from "../../text/strings.ts";
import { PatternLanguage } from "../language.ts";
import {
  CURRENCIES,
  DATE_FORMAT,
  FRACTIONS,
  JOINERS,
  MONTHS,
  NOTATION,
  ONES,
  ORDINAL_SIGN_FORMAT,
  ORDINAL_SIGNS,
  ORDINAL_SUFFIXES,
  ORDINALS,
  PHONE_FORMATS,
  SCALES,
  SYMBOLS,
  TENS,
  TERMS,
  TIME_WORDS,
  UNITS,
  ROMAN_CUES,
  ROMAN_MARKERS,
  FEMININE_NAMES,
  FEMININE_ROMAN_NOUNS,
} from "./constants.ts";

// ------------------------
// ----- Number words -----
// ------------------------

// ventuno, ventotto: a ten drops its vowel before uno and otto
function belowHundred(n: bigint): string {
  if (n < 20n) {
    return ONES[Number(n)];
  }
  let tens: string = TENS[Number(n / 10n)];
  const unit = n % 10n;
  if (unit === 1n || unit === 8n) {
    tens = tens.slice(0, -1);
  }
  return tens + (unit ? ONES[Number(unit)] : "");
}

// centotto, novecentottanta: a hundred drops its vowel before otto and ottanta
function belowThousand(n: bigint): string {
  const h = n / 100n;
  const r = n % 100n;
  let head = h === 0n ? "" : h === 1n ? "cento" : `${ONES[Number(h)]}cento`;
  if (head !== "" && (r === 8n || (r >= 80n && r <= 89n))) {
    head = head.slice(0, -1);
  }
  return head + (r || head === "" ? belowHundred(r) : "");
}

// ventitré, centotré, milletré: a compound ending in tre takes the accent
function accented(words: string): string {
  const out: string[] = [];
  for (const word of split(words, " ")) {
    let w = word;
    if (w.length > 3 && w.endsWith("tre")) {
      w = `${w.slice(0, -3)}tré`;
    }
    out.push(w);
  }
  return join(out);
}

// One word below a million (duemilaventidue), then un milione duecentomila
function cardinal(n: bigint): string {
  const words: string[] = [];
  for (const [scale, one, many] of [
    [1000000000n, "un miliardo", "miliardi"],
    [1000000n, "un milione", "milioni"],
  ] as const) {
    const q = n / scale;
    if (q) {
      words.push(q === 1n ? one : `${cardinal(q)} ${many}`);
    }
    n %= scale;
  }

  const thousands = n / 1000n;
  n %= 1000n;
  const head = thousands === 0n ? "" : thousands === 1n ? "mille" : `${belowThousand(thousands)}mila`;
  if (head !== "" || n || words.length === 0) {
    words.push(head + (n || head === "" ? belowThousand(n) : ""));
  }
  return accented(join(words));
}

// primo ... decimo, then the cardinal with -esimo: undicesimo, ventitreesimo, millesimo
function ordinal(n: bigint, ending: string): string {
  if (n <= 10n) {
    const word: string = ORDINALS[Number(n)];
    return word.slice(0, -1) + ending;
  }

  let words = cardinal(n);
  if (words.endsWith("tré")) {
    words = `${words.slice(0, -1)}e`;
  }
  if (words.endsWith("tre") || words.endsWith("sei")) {
    return `${words}esim${ending}`;
  }
  if (words.endsWith("mila")) {
    return `${words.slice(0, -4)}millesim${ending}`;
  }
  return `${words.slice(0, -1)}esim${ending}`;
}

function digits(text: string): string {
  return join([...text].map((c) => ONES[Number(c)]));
}

// --------------------
// ----- Patterns -----
// --------------------

// '80: a decade written as an apostrophe and its two digits.
class DecadePattern extends Pattern {
  constructor() {
    super(Kind.DECADE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position) || text[position] !== "'") {
      return undefined;
    }

    // Two digits: a ten other than 00
    const decade = digitsAt(text, position + 1);
    if (
      decade.integer.length !== 2 ||
      decade.integer[0] === "0" ||
      decade.integer[1] !== "0" ||
      !isClearAfter(text, decade.span.end)
    ) {
      return undefined;
    }
    return match({ kind: Kind.DECADE, span: { begin: position, end: decade.span.end }, numbers: [decade] });
  }
}

// --------------------
// ----- Language -----
// --------------------

class Italian extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SIGNS, ORDINAL_SIGN_FORMAT));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new DecadePattern());
    this.addPattern(new RomanPattern(false, ROMAN_MARKERS));
  }

  protected override readNumber(number: Numeral, match: Match): string {
    if (number.fraction === "") {
      const n = parseValue(number.integer);
      if (n === undefined || isSpokenDigits(number, 7)) {
        return digits(number.integer);
      }
      // 1 maggio: "1" before a month is "primo"
      const month = MONTHS.includes(toLower(match.context.after()));
      return n === 1n && month ? "primo" : cardinal(n);
    }

    // The separator is read as written: due virgola cinque, due punto cinque
    const fraction: string[] = [];
    const zeros = number.fraction.search(/[^0]/);
    const count = zeros === -1 ? number.fraction.length : zeros;
    for (let i = 0; i < count; i++) {
      fraction.push("zero");
    }
    if (zeros !== -1) {
      fraction.push(cardinal(parseValue(number.fraction) ?? 0n));
    }
    const head = number.integer === "" ? "" : `${cardinal(value(number))} `;
    return head + (number.decimalMark === "," ? "virgola " : "punto ") + join(fraction);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 6);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    const day = value(match.numbers[0]);
    return (
      `${day === 1n ? "primo" : cardinal(day)} ${MONTHS[Number(value(match.numbers[1])) - 1]} ` +
      cardinal(value(match.numbers[2]))
    );
  }

  protected override readPhone(match: Match): string | undefined {
    // Digit by digit: tre tre tre, uno due tre ...
    return join(match.numbers.map((group) => digits(group.integer)));
  }

  protected override readFraction(match: Match): string | undefined {
    return namedFraction(FRACTIONS, match);
  }

  protected override readTime(match: Match): string | undefined {
    // With a dot, a time comes after "alle" or from 13 on: alle 9.30, 21.30, but versione 2.79
    const hours = value(match.numbers[0]);
    const minutes = value(match.numbers[1]);
    const decimal = match.separator === "." && !TIME_WORDS.has(match.context.before()) && (hours < 13n || hours > 23n);
    if (!fitsClock(match) || decimal) {
      return undefined;
    }

    // alle 21.30: ventuno e trenta
    const hour = hours === 1n ? "una" : cardinal(hours);
    return minutes ? `${hour} e ${cardinal(minutes)}` : hour;
  }

  protected override readRoman(match: Match): string | undefined {
    // Giovanni ventitreesimo, Elisabetta seconda, capitolo quarto, parte seconda, la seconda guerra
    const role = romanRole(match, ROMAN_CUES);
    if (role === undefined) {
      return undefined;
    }
    const previous = match.context.before();
    const feminine =
      role === RomanRole.NAME
        ? FEMININE_NAMES.has(previous)
        : role === RomanRole.BEFORE
          ? FEMININE_ROMAN_NOUNS.has(previous)
          : FEMININE_ROMAN_NOUNS.has(toLower(match.context.after()));
    return ordinal(value(match.numbers[0]), feminine ? "a" : "o");
  }

  protected override readOrdinal(match: Match): string | undefined {
    if (value(match.numbers[0]) === 0n) {
      return undefined;
    }
    const ending = match.suffix === "ª" ? "a" : match.suffix === "º" ? "o" : match.suffix.slice(-1);
    return ordinal(value(match.numbers[0]), ending);
  }

  protected override readDecade(match: Match): string | undefined {
    return cardinal(value(match.numbers[0])); // gli anni '80: gli anni ottanta
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];

    // 3 mln €: tre milioni di euro, but 10 mila €: diecimila euro
    if (match.scale !== "") {
      const thousands = SCALES[match.scale].one === "mila";
      return this.#scaled(number, match) + (thousands ? " " : " di ") + currency.many;
    }

    // 10,50 €: dieci euro e cinquanta
    if (number.fraction.length === 2 && number.decimalMark === ",") {
      const whole = numeral({
        span: { begin: 0, end: 0 },
        integer: number.integer === "" ? "0" : number.integer,
        grouped: number.grouped,
      });
      const cents = parseValue(number.fraction) ?? 0n;
      return (
        `${this.readNumber(whole, match)} ${number.integer === "1" ? currency.one : currency.many}` +
        (cents ? ` e ${cardinal(cents)}` : "")
      );
    }
    return `${this.readNumber(number, match)} ${isOne(number) ? currency.one : currency.many}`;
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    // 4K: a resolution
    if ((match.scale === "k" || match.scale === "K") && value(match.numbers[match.numbers.length - 1]) < 10n) {
      return undefined;
    }
    return this.#scaled(number, match);
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${this.readNumber(number, match)} per cento${spacedSuffix(match)}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    const unit = UNITS[match.unit];
    return `${this.readNumber(number, match)} ${isOne(number) ? unit.one : unit.many}`;
  }

  // An amount with its scale: settantaquattromila, tre milioni
  #scaled(number: Numeral, match: Match): string {
    const scale = SCALES[match.scale];

    // 74 mila: settantaquattromila
    if (scale.one === "mila" && number.fraction === "") {
      // uint64_t in C++: an 18-digit amount wraps around
      return cardinal(BigInt.asUintN(64, value(number) * 1000n));
    }
    const singular = value(number) < 2n && number.fraction === "";
    return `${this.readNumber(number, match)} ${singular ? scale.one : scale.many}`;
  }
}

let instance: Italian | undefined;

export function language(): PatternLanguage {
  return (instance ??= new Italian());
}
