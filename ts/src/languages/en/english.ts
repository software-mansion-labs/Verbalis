import { DatePattern, PhonePattern, SuffixPattern, RomanPattern } from "../../numbers/patterns.ts";
import { Kind, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  fitsClock,
  isSpokenDigits,
  namedFraction,
  parseValue,
  spacedSuffix,
  value,
  romanRole,
  RomanRole,
} from "../../numbers/scanning.ts";
import { join } from "../../text/scan.ts";
import { contains, size, toLower } from "../../text/strings.ts";
import { isAlpha, isUpper } from "../../text/unicode.ts";
import { PatternLanguage } from "../language.ts";
import {
  CURRENCIES,
  DATE_FORMAT,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  FRACTIONS,
  IRREGULAR_ORDINALS,
  IRREGULAR_PLURALS,
  JOINERS,
  MONTH_WORDS,
  MONTHS,
  NOTATION,
  ONES,
  ORDINAL_SUFFIXES,
  PHONE_FORMATS,
  SCALE_WORDS,
  SCALES,
  STOP_WORDS,
  SYMBOLS,
  TENS,
  TERMS,
  TIME_WORDS,
  UNITS,
  YEAR_CUES,
  ROMAN_CUES,
} from "./constants.ts";

// ------------------------
// ----- Number words -----
// ------------------------

function cardinal(n: bigint): string {
  if (n < 20n) {
    return ONES[Number(n)];
  }
  if (n < 100n) {
    return TENS[Number(n / 10n)] + (n % 10n ? `-${ONES[Number(n % 10n)]}` : "");
  }
  if (n < 1000n) {
    return `${ONES[Number(n / 100n)]} hundred` + (n % 100n ? ` ${cardinal(n % 100n)}` : "");
  }
  for (const [scale, word] of SCALE_WORDS) {
    if (n >= scale) {
      return `${cardinal(n / scale)} ${word}` + (n % scale ? ` ${cardinal(n % scale)}` : "");
    }
  }
  return "";
}

function ordinal(n: bigint): string {
  // Only the last word changes: twenty-first, one hundred and twelfth
  const words = cardinal(n);
  const start = Math.max(words.lastIndexOf(" "), words.lastIndexOf("-")) + 1;
  const last = words.slice(start);
  const head = words.slice(0, start);

  const irregular = IRREGULAR_ORDINALS[last];
  if (irregular !== undefined) {
    return head + irregular;
  }
  return head + (last.endsWith("y") ? `${last.slice(0, -1)}ieth` : `${last}th`);
}

// Year-style reading, also of 3-4 digit codes: nineteen oh five, two thousand seven, nine fifty-six
function pairs(n: bigint): string {
  const high = n / 100n;
  const low = n % 100n;
  if (n >= 1000n && n % 1000n < 10n) {
    return cardinal(n);
  }
  if (low === 0n) {
    return `${cardinal(high)} hundred`;
  }
  return `${cardinal(high)} ` + (low < 10n ? `oh ${ONES[Number(low)]}` : cardinal(low));
}

function digits(text: string): string {
  return join([...text].map((c) => ONES[Number(c)]));
}

// a.m. and p.m. as their lowercase abbreviations are said; AM and PM stay as written
function ampm(suffix: string): string {
  const base = toLower(suffix.replace(/\.+$/, ""));
  const lowercase = ![...suffix].some((c) => isUpper(c));
  if (lowercase && (base === "am" || base === "pm")) {
    return base === "am" ? "a.m." : "p.m.";
  }
  return suffix;
}

// What the words around a match say about reading its numbers
interface Cues {
  afterMonth: boolean; // March 3: a day of the month, read as an ordinal
  counted: boolean; // 1999 people: a count rather than a year
  attributive: boolean; // a $5 bill: before a noun, the unit stays singular
}

function cuesOf(match: Match): Cues {
  const previous = match.context.before();
  const next = match.context.after();
  const nextLower = toLower(next);

  // 1999 people: a plural noun after a 4-digit number makes it a count, unless a year is expected (in 1999)
  const pluralNoun =
    IRREGULAR_PLURALS.has(nextLower) ||
    (size(nextLower) > 3 && nextLower.endsWith("s") && !nextLower.endsWith("ss") && !STOP_WORDS.has(nextLower));
  const yearCue = YEAR_CUES.has(previous) || MONTH_WORDS.has(previous);

  return {
    afterMonth: MONTH_WORDS.has(previous),
    counted: pluralNoun && !yearCue,
    attributive:
      next !== "" && isAlpha(String.fromCodePoint(next.codePointAt(0) as number)) && !STOP_WORDS.has(nextLower),
  };
}

// --------------------
// ----- Language -----
// --------------------

class English extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern());
  }

  protected override readNumber(number: Numeral, match: Match): string {
    if (number.fraction === "") {
      return this.#integer(number, cuesOf(match));
    }

    // .5 has no zero before its point
    const head = number.integer === "" ? "" : `${cardinal(value(number))} `;
    return head + (number.fraction === "0" ? "point oh" : `point ${digits(number.fraction)}`);
  }

  protected override readCodeDigits(text: string): string {
    const n = parseValue(text);
    if (text.length > 1 && text[0] === "0") {
      return join([...text].map((c) => (c === "0" ? "oh" : ONES[Number(c)])));
    }
    if (n === undefined || text.length > 4) {
      return digits(text);
    }
    return text.length <= 2 ? cardinal(n) : pairs(n);
  }

  protected override readDate(match: Match): string | undefined {
    const words = `${ordinal(value(match.numbers[0]))} ${MONTHS[Number(value(match.numbers[1])) - 1]}`;
    if (match.numbers.length < 3) {
      return words;
    }
    const year = match.numbers[2];
    return `${words} ` + (year.integer.length === 4 ? pairs(value(year)) : cardinal(value(year)));
  }

  protected override readPhone(match: Match): string | undefined {
    // 1-555-...: the leading one joins the area code
    const groups = match.numbers.map((number) => number.integer);
    if (match.text.startsWith("1-")) {
      groups[0] = `1${groups[0]}`;
    }
    return join(groups.map(digits), ", ");
  }

  protected override readFraction(match: Match): string | undefined {
    return namedFraction(FRACTIONS, match);
  }

  protected override readTime(match: Match): string | undefined {
    const hours = value(match.numbers[0]);
    const suffix = ampm(match.suffix);

    // 7pm: an hour alone needs am or pm
    if (match.numbers.length === 1) {
      if (hours > 12n || !contains("apAP", match.suffix[0])) {
        return undefined;
      }
      return `${cardinal(hours)} ${suffix}`;
    }

    // 1.13 is a decimal, unless a time is expected: at 10.30
    const decimal = match.separator === "." && match.suffix === "" && !TIME_WORDS.has(match.context.before());
    if (!fitsClock(match) || decimal) {
      return undefined;
    }

    // ten thirty, ten oh five, ten o'clock
    const minutes = match.numbers[1].integer;
    let words = cardinal(hours);
    if (minutes !== "00") {
      const m = value(match.numbers[1]);
      words += minutes[0] === "0" ? ` oh ${ONES[Number(m)]}` : ` ${cardinal(m)}`;
    }
    if (suffix !== "") {
      return `${words} ${suffix}`;
    }
    return words + (minutes === "00" ? " o'clock" : "");
  }

  protected override readOrdinal(match: Match): string | undefined {
    return ordinal(value(match.numbers[0]));
  }

  protected override readRoman(match: Match): string | undefined {
    // Henry the Eighth, the twenty-first century, Chapter four
    const n = value(match.numbers[0]);
    const role = romanRole(match, ROMAN_CUES);
    switch (role ?? RomanRole.BEFORE) {
      case RomanRole.NAME:
        return `the ${ordinal(n)}`;
      case RomanRole.AFTER:
        return ordinal(n);
      default:
        return role !== undefined ? cardinal(n) : undefined;
    }
  }

  protected override readDecade(match: Match): string | undefined {
    return this.plural(this.readNumber(match.numbers[0], match));
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];
    const cues = cuesOf(match);

    // A price: four ninety-nine, ninety-nine cents
    if (number.fraction.length === 2 && match.scale === "") {
      const centsValue = parseValue(number.fraction) ?? 0n;
      if (value(number) === 0n) {
        return `${cardinal(centsValue)} cents`;
      }
      const cents = number.fraction[0] === "0" ? `oh ${ONES[Number(centsValue)]}` : cardinal(centsValue);
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      return `${this.#integer(whole, cues)} ${cents}`;
    }

    let words = this.readNumber(number, match);
    if (match.scale !== "") {
      words += ` ${SCALES[match.scale]}`;
    }
    const one = number.integer === "1" && number.fraction === "" && match.scale === "";
    return `${words} ` + (one || cues.attributive ? currency.one : currency.many);
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    if (match.scale === "K") {
      return undefined; // 4K: a resolution
    }
    return `${this.readNumber(number, match)} ${SCALES[match.scale]}`;
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${this.readNumber(number, match)} percent${spacedSuffix(match)}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    const unit = UNITS[match.unit];
    const one = number.fraction === "" && value(number) === 1n;
    return `${this.readNumber(number, match)} ` + (one || cuesOf(match).attributive ? unit.one : unit.many);
  }

  protected override readRange(from: Numeral, to: Numeral, _match: Match): string {
    if (from.fraction !== "" || to.fraction !== "") {
      return this.joiners().range;
    }

    // 10-15 is "ten to fifteen"; but scores and abbreviated seasons keep "-": 51-44, 2016-17, 21-22
    const a = value(from);
    const b = value(to);
    const season = from.integer.length === 2 && b === a + 1n;
    return from.integer.length === to.integer.length && b > a && !season ? this.joiners().range : "-";
  }

  protected override plural(words: string): string | undefined {
    if (words.endsWith("y")) {
      return `${words.slice(0, -1)}ies`;
    }
    return words + (words.endsWith("x") ? "es" : "s");
  }

  // An integer as a year, a day of a month or a count, digit by digit if it is a code
  #integer(number: Numeral, cues: Cues): string {
    const n = parseValue(number.integer);
    if (n === undefined || isSpokenDigits(number, 5)) {
      return digits(number.integer);
    }
    if (cues.afterMonth && n >= 1n && n <= 31n) {
      return ordinal(n);
    }
    if (number.integer.length === 4 && !number.grouped) {
      return cues.counted ? cardinal(n) : pairs(n);
    }
    return cardinal(n);
  }
}

let instance: English | undefined;

export function language(): PatternLanguage {
  return (instance ??= new English());
}
