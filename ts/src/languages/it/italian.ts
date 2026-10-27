import { DatePattern, Pattern, PhonePattern, SuffixPattern, RomanPattern } from "../../numbers/patterns.ts";
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
} from "../../numbers/scanning.ts";
import { isClearAfter, isClearBefore, isDigitAfter, join } from "../../text/scan.ts";
import { contains, size, split, toLower, type Text } from "../../text/strings.ts";
import { isDigit, isLower, isSpace, isUpper } from "../../text/unicode.ts";
import type { Forms } from "../grammar.ts";
import { PatternLanguage } from "../language.ts";
import {
  CENTS,
  COUNT_WORDS,
  FEMININE_ARTICLES,
  FEMININE_IN_E,
  MASCULINE_IN_A,
  MASCULINE_IN_IONE,
  ORDINAL_ARTICLES,
  STOP_WORDS,
  ELIDED_ARTICLES,
  VERSION_WORDS,
  CURRENCIES,
  DATE_FORMAT,
  FRACTION_WORDS,
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

// An integer as a cardinal, or digit by digit if it reads as a code: 0039, 1034500
function integerWords(number: Numeral): string {
  const n = parseValue(number.integer);
  return n === undefined || isSpokenDigits(number, 7) ? digits(number.integer) : cardinal(n);
}

// -------------------
// ----- Context -----
// -------------------

function isVowel(c: string | undefined): boolean {
  return contains("aeiouàèéìòù", c);
}

function isFeminine(noun: string): boolean {
  const word = toLower(noun);
  if (word.endsWith("a")) {
    return !MASCULINE_IN_A.has(word);
  }
  if (word.endsWith("ione")) {
    return !MASCULINE_IN_IONE.has(word);
  }
  return (
    word.endsWith("tà") || word.endsWith("tù") || word.endsWith("si") || word.endsWith("ie") || FEMININE_IN_E.has(word)
  );
}

// The article form of "uno" before a noun: un anno, una volta, uno studente, un'ora
function oneBefore(noun: string): string {
  const space = noun.indexOf(" ");
  const word = [...toLower(space === -1 ? noun : noun.slice(0, space))];
  if (word.length === 0) {
    return "uno";
  }
  if (isFeminine(word.join(""))) {
    return isVowel(word[0]) ? "un'" : "una";
  }
  const start = word.slice(0, 2).join("");
  const impure =
    (word[0] === "s" && word.length > 1 && !isVowel(word[1])) ||
    word[0] === "z" ||
    word[0] === "x" ||
    word[0] === "y" ||
    start === "gn" ||
    start === "ps" ||
    start === "pn";
  return impure ? "uno" : "un";
}

// One of a noun: un metro, una sterlina, un'ora
function oneOf(noun: string): string {
  const article = oneBefore(noun);
  return article + (article.endsWith("'") ? "" : " ") + noun;
}

// Whether a word after a number may be a noun it counts: figli, chili, but not su, è, dell'anno, maggio
function isNoun(word: string): boolean {
  return (
    word !== "" &&
    isLower(String.fromCodePoint(word.codePointAt(0)!)) &&
    !STOP_WORDS.has(word) &&
    !/['’]/.test(word) &&
    !MONTHS.includes(word)
  );
}

// The end of the text before a match, without the spaces closing it
function headEnd(head: Text): number {
  let end = head.length;
  while (end > 0 && isSpace(head[end - 1])) {
    end--;
  }
  return end;
}

// Whether a decimal with a dot measures the noun after it (pesava 72.3 chili), unlike a version (Python 3.12 introduce)
function measuresNoun(match: Match): boolean {
  const head = match.context.head();
  const end = headEnd(head);
  let word = end;
  while (word > 0 && !contains(" \t\n", head[word - 1])) {
    word--;
  }
  const previous = match.context.before();
  // A capitalized word before it names the version (Python 3.12), unless it is an article opening a sentence (Il 72.3)
  const name =
    word < end &&
    (isUpper(head[word]) || isDigit(head[word])) &&
    !STOP_WORDS.has(previous) &&
    !COUNT_WORDS.has(previous);
  const percent =
    match.context.after() === "percento" || (match.context.after() === "per" && match.context.after(2) === "cento");
  return (isNoun(match.context.after()) || percent) && !name && !VERSION_WORDS.has(previous);
}

// Whether a 1 counts the noun after it: ha 1 figlio, but articolo 1 comma 2, 1 su 3
function countsNoun(match: Match): boolean {
  if (!isNoun(match.context.after())) {
    return false;
  }
  const head = match.context.head();
  const end = headEnd(head);
  const previous = match.context.before();
  const participle =
    size(previous) > 4 &&
    (previous.endsWith("ato") || previous.endsWith("ito") || previous.endsWith("uto") || previous.endsWith("ati"));
  return end === 0 || contains('.,!?:;("', head[end - 1]) || COUNT_WORDS.has(previous) || participle;
}

// A whole number before its noun: un euro, una sterlina, due euro
function counted(number: Numeral, noun: Forms): string {
  return isOne(number) ? oneOf(noun.one) : `${integerWords(number)} ${noun.many}`;
}

function countedValue(n: bigint, noun: Forms): string {
  return n === 1n ? oneOf(noun.one) : `${cardinal(n)} ${noun.many}`;
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

// 17:15:26: hours, minutes, and seconds.
class SecondsPattern extends Pattern {
  constructor() {
    super(Kind.TIME);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,:")) {
      return undefined;
    }
    const numbers: Numeral[] = [];
    for (let start = position; numbers.length < 3; start = numbers[numbers.length - 1].span.end + 1) {
      const part = digitsAt(text, start);
      const length = part.integer.length;
      if (
        length === 0 ||
        length > 2 ||
        (numbers.length > 0 && length !== 2) ||
        (numbers.length < 2 && (part.span.end >= text.length || text[part.span.end] !== ":"))
      ) {
        return undefined;
      }
      numbers.push(part);
    }
    const end = numbers[2].span.end;
    if (!isClearAfter(text, end) || isDigitAfter(text, end, ".,:")) {
      return undefined;
    }
    return match({ kind: Kind.TIME, span: { begin: position, end }, numbers, separator: ":" });
  }
}

// --------------------
// ----- Language -----
// --------------------

class Italian extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new SecondsPattern());
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
      if (number.integer !== "1") {
        return integerWords(number);
      }
      // 1 maggio: primo maggio; ha 1 figlio: un figlio
      if (MONTHS.includes(toLower(match.context.after()))) {
        return "primo";
      }
      return match.kind === Kind.NUMBER && countsNoun(match) ? oneBefore(match.context.after()) : "uno";
    }

    // A decimal comma reads virgola; a dot reads punto (versione 2.1), but virgola in an amount (1.5 kg, 4.3%)
    const amount =
      match.kind === Kind.MONEY ||
      match.kind === Kind.SCALE ||
      match.kind === Kind.PERCENT ||
      match.kind === Kind.UNIT ||
      (match.kind === Kind.NUMBER && measuresNoun(match));
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
    return head + (number.decimalMark === "," || amount ? "virgola " : "punto ") + join(fraction);
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
    // 1/2 litro: mezzo litro, 1/2 porzione: mezza porzione
    const noun = match.context.after();
    const [first, second] = fractionOf(match);
    if (first === 1n && second === 2n && match.numbers.length === 2 && isNoun(noun) && !FRACTION_WORDS.has(noun)) {
      return isFeminine(noun) ? "mezza" : "mezzo";
    }
    const words = namedFraction(FRACTIONS, match);
    if (words !== undefined) {
      return words;
    }
    const [a, b] = fractionOf(match);
    if (b < 2n || a >= b || !isMarkedFraction(match, FRACTION_WORDS)) {
      return undefined;
    }
    // un quinto, due terzi, tre undicesimi
    return `${a === 1n ? "un" : cardinal(a)} ${ordinal(b, a === 1n ? "o" : "i")}`;
  }

  protected override readTime(match: Match): string | undefined {
    // With a dot, a time comes after "alle" or from 13 on: alle 9.30, 21.30, but versione 2.79
    const hours = value(match.numbers[0]);
    const minutes = value(match.numbers[1]);
    const decimal = match.separator === "." && !TIME_WORDS.has(match.context.before()) && (hours < 13n || hours > 23n);
    if (!fitsClock(match) || decimal) {
      return undefined;
    }

    // alle 21.30: ventuno e trenta; 17:15:26: diciassette e quindici minuti e ventisei secondi
    const hour = hours === 1n ? "una" : cardinal(hours);
    if (match.numbers.length === 3) {
      const seconds = value(match.numbers[2]);
      if (seconds > 59n) {
        return undefined;
      }
      return (
        hour +
        (minutes ? ` e ${minutes === 1n ? "un minuto" : `${cardinal(minutes)} minuti`}` : "") +
        (seconds ? ` e ${seconds === 1n ? "un secondo" : `${cardinal(seconds)} secondi`}` : "")
      );
    }
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
    // il 3° posto, la 3° edizione: ° takes the gender of the article
    let ending = match.suffix === "ª" ? "a" : match.suffix === "º" ? "o" : match.suffix.slice(-1);
    const article = match.context.before();
    if (match.suffix === "°") {
      const elided = ELIDED_ARTICLES.has(article);
      ending = (elided ? isFeminine(match.context.after()) : FEMININE_ARTICLES.has(article)) ? "a" : "o";
    }
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

    // 10,50 €: dieci euro e cinquanta centesimi; 0,01 €: un centesimo; but 41,2 €/MWh: quarantuno virgola due euro
    const cents = Object.hasOwn(CENTS, match.unit) ? CENTS[match.unit] : undefined;
    if (number.fraction.length === 2 && cents !== undefined) {
      const whole = numeral({
        span: { begin: 0, end: 0 },
        integer: number.integer === "" ? "0" : number.integer,
        grouped: number.grouped,
      });
      const hundredths = parseValue(number.fraction) ?? 0n;
      const centsWords = countedValue(hundredths, cents);
      if (/^0*$/.test(whole.integer) && hundredths) {
        return centsWords;
      }
      return counted(whole, currency) + (hundredths ? ` e ${centsWords}` : "");
    }
    if (number.fraction === "") {
      return counted(number, currency);
    }
    return `${this.readNumber(number, match)} ${currency.many}`;
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
    // il 1° gennaio, al 90° minuto: an ordinal, read by its own pattern
    if (
      match.unit === "°" &&
      match.numbers.length === 1 &&
      (ORDINAL_ARTICLES.has(match.context.before()) || MONTHS.includes(toLower(match.context.after())))
    ) {
      return undefined;
    }
    // 155 d: giorni, but not Italia/2007 d.17' nor 1 d'oro
    const tail = match.context.tail();
    if (match.unit === "d" && tail.length > 0 && contains(".'’", tail[0])) {
      return undefined;
    }
    const unit = UNITS[match.unit];
    return number.fraction === "" ? counted(number, unit) : `${this.readNumber(number, match)} ${unit.many}`;
  }

  // An amount with its scale: settantaquattromila, tre milioni
  #scaled(number: Numeral, match: Match): string {
    const scale = SCALES[match.scale];

    // 74 mila: settantaquattromila
    if (scale.one === "mila" && number.fraction === "") {
      // uint64_t in C++: an 18-digit amount wraps around
      return cardinal(BigInt.asUintN(64, value(number) * 1000n));
    }
    return number.fraction === "" ? counted(number, scale) : `${this.readNumber(number, match)} ${scale.many}`;
  }
}

let instance: Italian | undefined;

export function language(): PatternLanguage {
  return (instance ??= new Italian());
}
