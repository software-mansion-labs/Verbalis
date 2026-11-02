import {
  DatePattern,
  Pattern,
  PhonePattern,
  PrimePattern,
  RomanOrdinalPattern,
  RomanPattern,
  SecondsPattern,
  SuffixPattern,
} from "../../numbers/patterns.ts";
import { Kind, match, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  digitsAt,
  fitsClock,
  fractionOf,
  isSpokenDigits,
  numbersAt,
  parseValue,
  romanRole,
  value,
  unpaddedDay,
  isYearValue,
} from "../../numbers/scanning.ts";
import { isClearAfter, isClearBefore, isDigitAfter, join } from "../../text/scan.ts";
import { type Text } from "../../text/strings.ts";
import { isAlnum, isAlpha, isDigit, isSpace } from "../../text/unicode.ts";
import { primeFormat } from "../../numbers/formats.ts";
import type { Prime } from "../grammar.ts";
import { PatternLanguage, type LanguageHandle } from "../language.ts";
import {
  CARDINALS,
  CENTS,
  CHARACTERS,
  CODE_CUES,
  CODE_WORDS,
  COUNT_CUES,
  SIGN_CUES,
  CURRENCIES,
  CURRENCY_WORDS,
  DATE_FORMAT,
  HEIGHT_CUES,
  DAY_TIMES,
  ENGLISH_ORDINAL_SUFFIXES,
  ENGLISH_ORDINALS,
  ENGLISH_TENS,
  ENGLISH_TENTHS,
  JOINERS,
  LARGE_CARDINALS,
  LETTERS,
  MONTHS,
  NOTATION,
  ORDINAL_ENDINGS,
  ORDINAL_SUFFIXES,
  ORDINALS_SPECIAL,
  PHONE_CUES,
  PHONE_FORMATS,
  PRIME_CUES,
  PRIME_UNITS,
  ROMAN_CUES,
  ROMAN_ORDINAL_SUFFIXES,
  SYMBOLS,
  TERMS,
  UNITS,
  YEARS_AFTER,
  YEARS_BEFORE,
} from "./constants.ts";

// ------------------------
// ----- Number words -----
// ------------------------

// Every number up to 99 is its own word; then "<digit> सौ", and the Indian scales (हज़ार, लाख, करोड़, अरब, ...)
function cardinal(n: bigint): string {
  if (n < 100n) {
    return CARDINALS[Number(n)];
  }
  if (n < 1000n) {
    const rest = n % 100n;
    return `${CARDINALS[Number(n / 100n)]} सौ` + (rest ? ` ${cardinal(rest)}` : "");
  }

  // Hindi keeps the "एक" of every scale: एक हज़ार, एक लाख
  for (let i = LARGE_CARDINALS.length - 1; i >= 0; i--) {
    const [base, word] = LARGE_CARDINALS[i];
    if (n >= base) {
      const low = n % base;
      return `${cardinal(n / base)} ${word}` + (low ? ` ${cardinal(low)}` : "");
    }
  }
  return "";
}

// A year in hundreds: 1999 -> उन्नीस सौ निन्यानवे, 2101 -> इक्कीस सौ एक; but 2024 -> दो हज़ार चौबीस
function year(n: bigint): string {
  const hundreds = (n >= 1100n && n < 2000n) || (n >= 2100n && n < 10000n && (n / 100n) % 10n !== 0n);
  if (!hundreds) {
    return cardinal(n);
  }
  const rest = n % 100n;
  return `${cardinal(n / 100n)} सौ` + (rest ? ` ${cardinal(rest)}` : "");
}

function digits(text: string): string {
  return join([...text].map((c) => CARDINALS[Number(c)]));
}

// An English ordinal up to 99, as said in Hindi (7th -> सेवंथ); empty above
function englishOrdinal(n: bigint): string {
  if (n < 20n) {
    return ENGLISH_ORDINALS[Number(n)];
  }
  if (n < 100n) {
    const tens = Number(n / 10n);
    return n % 10n ? `${ENGLISH_TENS[tens]} ${ENGLISH_ORDINALS[Number(n % 10n)]}` : ENGLISH_TENTHS[tens];
  }
  return "";
}

// An amount and a half: 1.5 -> डेढ़, 2.5 -> ढाई, 3.5 -> साढ़े तीन
function andAHalf(number: Numeral): string | undefined {
  const n = value(number);
  if (number.fraction.replace(/0+$/, "") !== "5" || n === 0n) {
    return undefined;
  }
  return n === 1n ? "डेढ़" : n === 2n ? "ढाई" : `साढ़े ${cardinal(n)}`;
}

function isDevanagari(c: string | undefined): boolean {
  const code = c?.codePointAt(0) ?? 0;
  return code >= 0x0900 && code < 0x0980 && isAlpha(c);
}

// Whether any of the few words before a match is one of given words: नंबर पे कॉल करो 1374-309988
function afterAnyOf(match: Match, words: ReadonlySet<string>, k: number): boolean {
  for (let i = 1; i <= k; i++) {
    if (words.has(match.context.before(i))) {
      return true;
    }
  }
  return false;
}

// --------------------
// ----- Patterns -----
// --------------------

// टी20, 5जी, है1: a number joined to a Devanagari word, which the generic patterns leave as written
class JoinedNumberPattern extends Pattern {
  constructor() {
    super(Kind.NUMBER);
  }

  match(text: Text, position: number): Match | undefined {
    const before = position > 0 && isDevanagari(text[position - 1]);
    if (!isDigit(text[position]) || (!before && !isClearBefore(text, position, ".,"))) {
      return undefined;
    }
    for (const number of numbersAt(text, position, NOTATION.groupSeparators, NOTATION.decimalSeparators)) {
      // A zero after a letter marks an abbreviation, as a dot: डॉ0, ए0 जे0
      const end = number.span.end;
      const after = end < text.length && isDevanagari(text[end]);
      if (before && number.integer === "0" && number.fraction === "") {
        return undefined;
      }
      if (after || (before && isClearAfter(text, end))) {
        return match({ kind: Kind.NUMBER, span: number.span, numbers: [number] });
      }
    }
    return undefined;
  }
}

// चंद्रयान-III, भाग-XCIX: a Roman numeral joined to the word it numbers by a hyphen
class HyphenRomanPattern extends Pattern {
  readonly #roman = new RomanPattern();

  constructor() {
    super(Kind.ROMAN);
  }

  match(text: Text, position: number): Match | undefined {
    if (position < 2 || text[position - 1] !== "-" || !isAlpha(text[position - 2])) {
      return undefined;
    }
    // The numeral as if it started the text, which the hyphen before it would otherwise rule out
    const roman = this.#roman.match(text.slice(position), 0);
    if (roman === undefined) {
      return undefined;
    }
    for (const span of [roman.span, roman.numbers[0].span]) {
      span.begin += position;
      span.end += position;
    }
    return roman;
  }
}

// 2^3: the power of a number (only the caret and the exponent are matched)
class PowerPattern extends Pattern {
  constructor() {
    super(Kind.SCORE);
  }

  match(text: Text, position: number): Match | undefined {
    if (position === 0 || text[position] !== "^" || !isDigit(text[position - 1]) || !isDigitAfter(text, position + 1)) {
      return undefined;
    }
    const exponent = digitsAt(text, position + 1);
    if (!isClearAfter(text, exponent.span.end)) {
      return undefined;
    }
    return match({
      kind: Kind.SCORE,
      span: { begin: position, end: exponent.span.end },
      numbers: [exponent],
      separator: "^",
    });
  }
}

// --------------------
// ----- Language -----
// --------------------

// Hindi reads its numbers without the gender of the counted noun so far.
class Hindi extends PatternLanguage {
  constructor() {
    super({ characters: CHARACTERS, terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new SecondsPattern([], true));
    this.addPattern(new PrimePattern(NOTATION, primeFormat({ cues: PRIME_CUES, heights: HEIGHT_CUES })));
    this.addPattern(new PowerPattern());
    this.addPattern(new SuffixPattern(Kind.ORDINAL, [...ORDINAL_SUFFIXES.keys()]));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ENGLISH_ORDINAL_SUFFIXES));
    this.addPattern(new RomanOrdinalPattern([...ORDINAL_SUFFIXES.keys()]));
    this.addPattern(new HyphenRomanPattern());
    this.addPattern(new RomanPattern(false, ROMAN_ORDINAL_SUFFIXES));
    this.addPattern(new JoinedNumberPattern());
  }

  protected override readNumber(number: Numeral, match: Match): string {
    // An address of four parts digit by digit: 192.168.1.1 -> एक नौ दो डॉट एक छह आठ डॉट एक डॉट एक
    const address =
      match.kind === Kind.VERSION && match.numbers.length === 4 && match.numbers.every((part) => value(part) <= 255n);
    if (number.fraction === "" && (address || this.#isSpokenDigits(number, match))) {
      return digits(number.integer);
    }
    // The fraction is read digit by digit: as written in a number (97.0 -> सत्तानवे दशमलव शून्य), without its
    // trailing zeros in an amount (5.50 लाख -> पाँच दशमलव पाँच लाख, 888.60 रुपये)
    let fraction = number.fraction;
    if (match.kind !== Kind.NUMBER || CURRENCY_WORDS.has(match.context.after())) {
      fraction = fraction.replace(/0+$/, "");
    }
    const integer = cardinal(value(number));
    return fraction === "" ? integer : `${integer} दशमलव ${digits(fraction)}`;
  }

  protected override readCodeDigits(text: string): string {
    // A380: तीन सौ अस्सी, 5000mAh: पाँच हज़ार, census2011: दो हज़ार ग्यारह, but ABCDE1234F digit by digit
    const n = codeValue(text, 4);
    const number = n !== undefined && (n < 1000n || n % 100n === 0n || isYearValue(n, 1900n, 2099n));
    return number ? cardinal(n) : digits(text);
  }

  protected override readCodeLetters(letters: string): string {
    // Capitals are spelled (ABC-123: ए बी सी), unless they make a word (COVID-19); other letters stay
    const capitals = letters.length <= 5 && [...letters].every((c) => c >= "A" && c <= "Z");
    if (!capitals || CODE_WORDS.has(letters)) {
      return letters;
    }
    return join([...letters].map((c) => LETTERS[c.charCodeAt(0) - 65]));
  }

  protected override readCountryCode(text: string): string {
    return digits(text); // +91: प्लस नौ एक
  }

  protected override readPhone(match: Match): string | undefined {
    // Digit by digit. Without a trunk zero (1374 309988), only after a word of phones: नंबर पे कॉल करो
    const zero = match.text.startsWith("0") || match.text.startsWith("(0");
    if (match.kind === Kind.PHONE && !zero && !afterAnyOf(match, PHONE_CUES, 6)) {
      return undefined;
    }
    // The digits of a prefix too: 0 (80) 26411128
    const words: string[] = [];
    const prefix =
      match.kind === Kind.PHONE ? [...match.text].slice(0, match.numbers[0].span.begin - match.span.begin) : [];
    for (const c of prefix) {
      if (isDigit(c)) {
        words.push(CARDINALS[Number(c)]);
      }
    }
    for (const group of match.numbers) {
      words.push(digits(group.integer));
    }
    return join(words);
  }

  protected override readDate(match: Match): string | undefined {
    // 06-05: a day and a month only with a leading zero; 10-15 is a range
    if (match.numbers.length === 2) {
      const day = match.numbers[0].integer;
      const month = match.numbers[1].integer;
      if (match.separator === "." || day.length !== 2 || month.length !== 2 || (day[0] !== "0" && month[0] !== "0")) {
        return undefined;
      }
      return `${cardinal(value(match.numbers[0]))} ${MONTHS[Number(value(match.numbers[1]))]}`;
    }
    // सत्ताईस मार्च दो हज़ार छब्बीस, दो जुलाई उन्नीस सौ सत्तर
    return (
      `${cardinal(value(match.numbers[0]))} ${MONTHS[Number(value(match.numbers[1]))]} ` + year(value(match.numbers[2]))
    );
  }

  protected override readFraction(match: Match): string | undefined {
    // Numbers with a leading zero are codes: गाड़ी संख्या 03281/03282
    const coded = (number: Numeral) => number.integer.length > 1 && number.integer[0] === "0";
    if (coded(match.numbers[match.numbers.length - 2]) || coded(match.numbers[match.numbers.length - 1])) {
      return undefined;
    }
    // Common proper fractions have their own nouns, others are "<a> बटा <b>": 14/3 -> चौदह बटा तीन
    const [a, b] = fractionOf(match);
    if (a === 1n && b === 2n) {
      return "आधा";
    }
    if ((b === 3n || b === 4n) && a < b) {
      return cardinal(a) + (b === 3n ? " तिहाई" : " चौथाई");
    }
    return `${cardinal(a)} बटा ${cardinal(b)}`;
  }

  protected override readOrdinal(match: Match): string | undefined {
    return this.#ordinal(value(match.numbers[0]), match.suffix);
  }

  protected override readRoman(match: Match): string | undefined {
    // LIII वा: an ordinal; कक्षा XII, चंद्रयान-III: a number
    const n = value(match.numbers[0]);
    if (match.suffix !== "") {
      return this.#ordinal(n, match.suffix);
    }
    if (match.context.head().at(-1) !== "-" && romanRole(match, ROMAN_CUES) === undefined) {
      return undefined;
    }
    return cardinal(n);
  }

  protected override readTime(match: Match): string | undefined {
    // आठ बजे, सात बजकर इक्कीस मिनट; 10:30 बजे: दस तीस बजे; 03:50 मिनट: तीन बजकर पचास मिनट
    if (match.numbers.length < 2 || !fitsClock(match)) {
      return undefined;
    }
    // 8.30 only before बजे or after सुबह, शाम: otherwise a decimal
    const next = match.context.after();
    if (match.separator === "." && next !== "बजे" && !DAY_TIMES.has(match.context.before())) {
      return undefined;
    }
    const minutes = value(match.numbers[1]);
    const hours = cardinal(value(match.numbers[0]));
    if (match.numbers.length === 3) {
      // बारह बजकर दस मिनट दस सेकंड
      const seconds = value(match.numbers[2]);
      if (seconds > 59n) {
        return undefined;
      }
      return `${hours} बजकर ` + (minutes ? `${cardinal(minutes)} मिनट ` : "") + `${cardinal(seconds)} सेकंड`;
    }
    if (next === "बजे") {
      return minutes !== 0n ? `${hours} ${cardinal(minutes)}` : hours;
    }
    if (minutes === 0n) {
      return `${hours} बजे`;
    }
    return `${hours} बजकर ${cardinal(minutes)}${next === "मिनट" ? "" : " मिनट"}`;
  }

  protected override readPlain(match: Match): string | undefined {
    // A year after सन or a month, or before में, का, तक: 1999 में -> उन्नीस सौ निन्यानवे में. Not 3200 से ज्यादा.
    const number = match.numbers[0];
    const dated = YEARS_AFTER.has(match.context.before()) || YEARS_BEFORE.has(match.context.after());
    const hundreds = value(number) >= 1100n && value(number) < 2000n && number.integer.length === 4;
    const words =
      hundreds && number.fraction === "" && !number.grouped && dated
        ? year(value(number))
        : this.readNumber(number, match);

    // A number joined to a word gets spaces: टी20 -> टी बीस
    const before = isDevanagari(match.context.head().at(-1));
    const after = isDevanagari(match.context.tail()[0]);
    return (before ? " " : "") + words + (after ? " " : "");
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    // एक रुपया, पाँच रुपये, एक लाख रुपये; $50 अरब डॉलर: the currency is already named
    const currency = CURRENCIES[match.unit];
    const one = match.unit === "₹" && number.integer === "1" && match.scale === "";

    // ₹9.50: नौ रुपये पचास पैसे; ₹5.00: पाँच रुपये. Three decimals or more stay a decimal: ₹2.123
    const fraction = number.fraction.replace(/0+$/, "");
    const cents = Object.hasOwn(CENTS, match.unit) ? CENTS[match.unit] : undefined;
    if (number.fraction !== "" && fraction.length <= 2 && match.scale === "" && cents !== undefined) {
      const n = fraction === "" ? 0n : (parseValue(fraction) ?? 0n) * (fraction.length === 1 ? 10n : 1n);
      const whole = `${cardinal(value(number))} ${one ? "रुपया" : currency}`;
      const small = `${cardinal(n)} ${n === 1n && match.unit === "₹" ? "पैसा" : cents}`;
      return value(number) === 0n && n ? small : whole + (n ? ` ${small}` : "");
    }

    const words = this.readNumber(number, match) + (match.scale === "" ? "" : ` ${match.scale}`);
    if (match.context.after() === currency || match.context.after() === "रुपए") {
      return words;
    }
    return `${words} ${one && number.fraction === "" ? "रुपया" : currency}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    // 1st: an ordinal, not a stone
    if (match.unit === "st" && !match.text.includes(" ")) {
      return undefined;
    }
    // 1.5 yr: डेढ़ साल
    const half = andAHalf(number);
    if (half !== undefined && match.unit === "yr") {
      return `${half} ${UNITS["yr"]}`;
    }
    return `${this.readNumber(number, match)} ${UNITS[match.unit]}`;
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${this.readNumber(number, match)} प्रतिशत`;
  }

  protected override readJoiner(match: Match): string | undefined {
    // 3^2: तीन स्क्वेर्ड, 2^3: दो क्यूब, 4^5: चार टु द पावर पाँच
    if (match.separator === "^") {
      const n = value(match.numbers[0]);
      return n === 2n ? " स्क्वेर्ड" : n === 3n ? " क्यूब" : ` टु द पावर ${cardinal(n)}`;
    }
    // कोविड -19, डीजेएम -800: a hyphen after a word joins it to the number, unless the word cues a minus (तापमान -7)
    const head = match.context.head();
    const before = match.context.before();
    if (
      match.kind === Kind.SIGN &&
      head.length >= 2 &&
      head[head.length - 1] === " " &&
      isAlpha(head[head.length - 2]) &&
      !COUNT_CUES.has(before) &&
      !SIGN_CUES.has(before)
    ) {
      return undefined;
    }
    return super.readJoiner(match);
  }

  protected override readRange(from: Numeral, to: Numeral, match: Match): string {
    // 10-20-30, 1-800-555: a chain of numbers, and 66-4: no range down; but 297-272 ई. पू. is one, and 1920-26, with a
    // shortened year
    if (match.kind === Kind.RANGE) {
      const head = match.context.head();
      const tail = match.context.tail();
      const a = from.integer.length;
      const b = to.integer.length;
      const chain =
        (head.length > a + 1 && head[head.length - a - 1] === "-" && isDigit(head[head.length - a - 2])) ||
        (tail.length > b + 1 && tail[b] === "-" && isDigit(tail[b + 1]));
      const down = value(to) < value(from) && b < a && !(a === 4 && b === 2);
      const tied = value(to) === value(from); // 12-12 अंक: a tie, बारह-बारह
      if (chain || down || tied) {
        return "-";
      }
    }
    return super.readRange(from, to, match);
  }

  // 5'9": पाँच फीट नौ इंच; 1'23"45: एक मिनट तेईस दशमलव चार पाँच सेकंड
  protected override readPrime(number: Numeral, unit: Prime, match: Match): string {
    const forms = PRIME_UNITS[unit];
    const one = number.fraction === "" && value(number) === 1n;
    const count = number.fraction === "" ? cardinal(value(number)) : this.readNumber(number, match);
    return `${count} ${one ? forms.one : forms.many}`;
  }

  // Whether a number is read digit by digit: a leading zero, twelve digits or more, a mobile number (98765
  // 43210), a code after its word (पिन कोड 110023), or the postal code ending an address (रांची, झारखंड 736557)
  #isSpokenDigits(number: Numeral, match: Match): boolean {
    const i = number.integer;
    // 04 जून: a day before a month, despite its leading zero
    if (!unpaddedDay(number, match, MONTHS) && isSpokenDigits(number, 12)) {
      return true;
    }
    if (match.kind !== Kind.NUMBER || number.grouped || i.length < 4) {
      return false;
    }
    const mobile = i.length === 10 && i[0] >= "6";
    const before = match.context.before();
    const code =
      CODE_CUES.has(before) ||
      (before === "अंक" && (match.context.before(2) === "आखिरी" || match.context.before(2) === "अंतिम"));
    const last = match.context.tail().every((c) => isSpace(c) || !isAlnum(c));
    const postal =
      (i.length === 5 || i.length === 6) &&
      !i.endsWith("000") &&
      last &&
      before !== "" &&
      isAlpha([...before].at(-1)) &&
      !COUNT_CUES.has(before);
    return mobile || code || postal;
  }

  // The ordinal of a number by its suffix: 5वीं -> पाँचवीं, 2री -> दूसरी, 7th -> सेवंथ
  #ordinal(n: bigint, suffix: string): string {
    if (ENGLISH_ORDINAL_SUFFIXES.includes(suffix)) {
      const words = englishOrdinal(n);
      return words === "" ? cardinal(n) + ORDINAL_ENDINGS[0] : words;
    }
    // The suffix gives the form: masculine (वाँ, रा), feminine (वीं, री) or plural (वें, रे)
    const form = ORDINAL_SUFFIXES.get(suffix)!;

    // पहला, दूसरा, तीसरा, चौथा, छठा: irregular
    const special = n < 10n ? ORDINALS_SPECIAL.get(Number(n)) : undefined;
    if (special !== undefined) {
      return special[form];
    }
    // The regular ending as written (5वां, 5वाँ), or by the form of an irregular one
    const regular = suffix.startsWith("व");
    return cardinal(n) + (regular ? suffix : ORDINAL_ENDINGS[form]);
  }
}

let instance: Hindi | undefined;

export function language(): LanguageHandle {
  return (instance ??= new Hindi());
}
