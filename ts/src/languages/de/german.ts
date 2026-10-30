import {
  DatePattern,
  DotOrdinalPattern,
  FractionPattern,
  Pattern,
  ScalePattern,
  SuffixPattern,
  RomanPattern,
  SecondsPattern,
  PrimePattern,
} from "../../numbers/patterns.ts";
import { keysOf } from "../../numbers/formats.ts";
import { Kind, match as matchOf, numeral, type Match, type Numeral } from "../../numbers/match.ts";
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
  value,
  romanRole,
  RomanRole,
} from "../../numbers/scanning.ts";
import { PUNCTUATION, stripped, wordsOf, type Context } from "../../text/context.ts";
import {
  digitsEnd,
  isClearAfter,
  isClearBefore,
  isDigitAfter,
  join,
  keysAt,
  skipSpace,
  type Keys,
} from "../../text/scan.ts";
import { contains, str, toLower, type Text } from "../../text/strings.ts";
import { isDigit, isLower, isUpper, toUpper } from "../../text/unicode.ts";
import { Case, Gender } from "../grammar.ts";
import { PatternLanguage, type LanguageHandle } from "../language.ts";
import {
  ACCUSATIVE,
  CURRENCIES,
  DATE_CUES,
  DATE_FORMAT,
  DATIVE,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  FEMININE_ENDINGS,
  FRACTION_WORDS,
  FRACTIONS,
  GENDERS,
  GENITIVE,
  JOINERS,
  MEASURE_WORDS,
  MIXED,
  MONTHS,
  NOMINATIVE_WORDS,
  NEUTER_ENDINGS,
  NOTATION,
  ONES,
  ORDINAL_STEMS,
  SCALES,
  SPORT_WORDS,
  SYMBOLS,
  TIME_WORDS,
  TENS,
  TERMS,
  UNITS,
  WEAK,
  WEAK_NOMINATIVE,
  YEAR_CUES,
  ORDINAL_SUFFIXES,
  ROMAN_CUES,
  SECONDS_SUFFIXES,
  FEMININE_NAMES,
  PRIME_CUES,
  PRIME_UNITS,
  SECOND_FRACTIONS,
} from "./constants.ts";

// How the ending of an ordinal follows the word before it: am dritten (weak), ein zweiter (mixed), dritter (strong)
const Declension = { WEAK: 0, MIXED: 1, STRONG: 2 } as const;
type Declension = (typeof Declension)[keyof typeof Declension];

// What the words around a match say about reading its numbers
interface Cues {
  grammaticalCase: Case; // Given by the word before: mit einer Stimme, am dritten
  gender: Gender; // Of the noun after: eine Stunde, der dritte Platz
  declension: Declension;
  article: boolean; // "1" before a noun is said like an article: eine Stunde
  year: boolean; // A 4-digit number reads as a year (1998 Abi), unless it counts a noun (1945 Einwohner)
  capitalized: boolean; // A decade is a noun (die Neunziger), but an adjective before Jahre (neunziger Jahre)
}

function cues(): Cues {
  return {
    grammaticalCase: Case.NOMINATIVE,
    gender: Gender.MASCULINE,
    declension: Declension.STRONG,
    article: false,
    year: true,
    capitalized: true,
  };
}

// The words with their first character in uppercase
function upperFirst(words: string): string {
  const [first = ""] = words;
  return toUpper(first) + words.slice(first.length);
}

// ------------------------
// ----- Number words -----
// ------------------------

// A cardinal as the first part of a compound: ein, einundzwanzig, einhundertein
function stem(n: bigint): string {
  const words = cardinal(n);
  return words.endsWith("eins") ? words.slice(0, -1) : words;
}

function cardinal(n: bigint): string {
  if (n < 20n) {
    return ONES[Number(n)];
  }
  if (n < 100n) {
    return (n % 10n ? `${stem(n % 10n)}und` : "") + TENS[Number(n / 10n)];
  }
  if (n < 1000n) {
    return `${stem(n / 100n)}hundert` + (n % 100n ? cardinal(n % 100n) : "");
  }
  if (n < 1000000n) {
    return `${stem(n / 1000n)}tausend` + (n % 1000n ? cardinal(n % 1000n) : "");
  }
  for (const [scale, one, many] of [
    [1000000000000000000n, "Trillion", "Trillionen"],
    [1000000000000000n, "Billiarde", "Billiarden"],
    [1000000000000n, "Billion", "Billionen"],
    [1000000000n, "Milliarde", "Milliarden"],
    [1000000n, "Million", "Millionen"],
  ] as const) {
    if (n >= scale) {
      const q = n / scale;
      const r = n % scale;
      const head = q === 1n ? `eine ${one}` : `${cardinal(q)} ${many}`;
      return head + (r ? ` ${cardinal(r)}` : "");
    }
  }
  return "";
}

// 1100-1999 in hundreds (neunzehnhundertneunzig); later years read like cardinals
function year(n: bigint): string {
  if (n >= 1100n && n <= 1999n) {
    return `${cardinal(n / 100n)}hundert` + (n % 100n ? cardinal(n % 100n) : "");
  }
  return cardinal(n);
}

function ordinal(n: bigint, ending: string): string {
  const words = cardinal(n);
  const r = n % 100n;
  if (n === 0n) {
    return `${words}t${ending}`; // nullter
  }
  if (r > 0n && r < 20n) {
    const one = ONES[Number(r)];
    const stem = ORDINAL_STEMS[one];
    const base = words.slice(0, words.length - one.length);
    return `${base + (stem !== undefined ? stem : one)}t${ending}`;
  }
  return `${words}st${ending}`;
}

function digits(text: string): string {
  return join([...text].map((c) => ONES[Number(c)]));
}

// "1" said like an article, by case and gender: eine Stunde, mit einem Mal
function article(c: Case, g: Gender): string {
  // prettier-ignore
  const forms = [
    ["ein", "eine", "ein"],      // Nominative
    ["eines", "einer", "eines"], // Genitive
    ["einem", "einer", "einem"], // Dative
    ["einen", "eine", "ein"],    // Accusative
  ] as const;
  const row = c === Case.GENITIVE ? 1 : c === Case.DATIVE ? 2 : c === Case.ACCUSATIVE ? 3 : 0;
  return forms[row][g];
}

// "halb" after "ein", by case and gender: ein halber Meter, einen halben Liter, eine halbe Stunde
function half(c: Case, g: Gender): string {
  if (c === Case.DATIVE || c === Case.GENITIVE || (c === Case.ACCUSATIVE && g === Gender.MASCULINE)) {
    return "halben";
  }
  return g === Gender.MASCULINE ? "halber" : g === Gender.FEMININE ? "halbe" : "halbes";
}

// The adjective ending of an ordinal: am dritten, der dritte, ein zweiter, dritter
function ending(cues: Cues): string {
  // prettier-ignore
  const strong = [
    ["er", "e", "es"],  // Nominative
    ["en", "er", "en"], // Genitive
    ["em", "er", "em"], // Dative
    ["en", "e", "es"],  // Accusative
  ] as const;
  switch (cues.declension) {
    case Declension.WEAK:
      return cues.grammaticalCase === Case.NOMINATIVE ? "e" : "en";
    case Declension.MIXED:
      return cues.gender === Gender.NEUTER ? "es" : "er";
    default: {
      const c = cues.grammaticalCase;
      const row = c === Case.GENITIVE ? 1 : c === Case.DATIVE ? 2 : c === Case.ACCUSATIVE ? 3 : 0;
      return strong[row][cues.gender];
    }
  }
}

// -------------------
// ----- Context -----
// -------------------

function genderOf(noun: string): Gender {
  const word = toLower(noun);

  // The longest known ending decides, then the regular ones
  let longest = 0;
  let gender: Gender = Gender.MASCULINE;
  for (const [key, keyGender] of GENDERS) {
    if (key.length > longest && word.endsWith(key)) {
      longest = key.length;
      gender = keyGender;
    }
  }
  if (longest > 0) {
    return gender;
  }

  const endsWithAny = (endings: readonly string[]) => endings.some((e) => word.endsWith(e));
  return endsWithAny(FEMININE_ENDINGS)
    ? Gender.FEMININE
    : endsWithAny(NEUTER_ENDINGS)
      ? Gender.NEUTER
      : Gender.MASCULINE;
}

function caseAfter(word: string): Case {
  return DATIVE.has(word)
    ? Case.DATIVE
    : ACCUSATIVE.has(word)
      ? Case.ACCUSATIVE
      : GENITIVE.has(word)
        ? Case.GENITIVE
        : Case.NOMINATIVE;
}

function capitalized(word: string): boolean {
  return word !== "" && isUpper(String.fromCodePoint(word.codePointAt(0) as number));
}

function allDigits(word: string): boolean {
  return word !== "" && [...word].every((c) => isDigit(c));
}

// A number right after a capitalized noun is its label: Seite 1, Platz 1
function labelled(context: Context): boolean {
  const head = wordsOf(context.head());
  return (
    head.length > 0 &&
    capitalized(str(head[head.length - 1])) &&
    !(head.length === 1 || contains(".!?", head[head.length - 2].at(-1)))
  );
}

// The ending of an ordinal from the words before it, and the noun it qualifies.
function resolveEnding(context: Context, noun: string, cues: Cues): void {
  const before = wordsOf(context.head()).map((word) => toLower(str(stripped(word))));

  // "in 2. und 3. Lesung", "vom 1. 5. bis 3. 5.": both take the ending of the first
  const joining = ["und", "oder", "bis", "sowie"];
  while (before.length >= 2 && joining.includes(before[before.length - 1]) && allDigits(before[before.length - 2])) {
    before.length -= 2;
    while (before.length > 0 && allDigits(before[before.length - 1])) {
      before.pop();
    }
  }
  const w1 = before.length === 0 ? "" : before[before.length - 1];
  const w2 = before.length < 2 ? "" : before[before.length - 2];

  cues.gender = genderOf(noun);
  if (WEAK.has(w1) || (w1 === "der" && (DATIVE.has(w2) || GENITIVE.has(w2)))) {
    cues.declension = Declension.WEAK;
    cues.grammaticalCase = Case.DATIVE;
  } else if (WEAK_NOMINATIVE.has(w1)) {
    cues.declension = Declension.WEAK;
    cues.grammaticalCase = Case.NOMINATIVE;
  } else if (MIXED.has(w1)) {
    cues.declension = Declension.MIXED;
  } else {
    cues.declension = Declension.STRONG;
    cues.grammaticalCase = caseAfter(w1);
  }
}

function cuesOf(match: Match): Cues {
  const context = match.context;
  const result = cues();

  switch (match.kind) {
    case Kind.DATE:
      resolveEnding(context, "Monat", result);
      return result;

    case Kind.ORDINAL: {
      // The noun is the first capitalized word after it: der 3. Platz
      let noun = "";
      for (const word of wordsOf(context.tail())) {
        if (capitalized(str(word))) {
          let end = word.length;
          while (end > 0 && contains(".,;:!?", word[end - 1])) {
            end--;
          }
          noun = str(word, 0, end);
          break;
        }
      }
      resolveEnding(context, noun, result);
      return result;
    }

    case Kind.DECADE: {
      const next = context.after();
      result.capitalized = next !== "Jahre" && next !== "Jahren" && next !== "Jahrgang";
      return result;
    }

    default:
      break;
  }

  // "1" before a noun is said like an article, in the case of the word before it: mit einer Stimme
  const next = context.after();
  let noun = next;
  if ((next === "bis" || next === "oder") && allDigits(context.after(2))) {
    // "1 bis 2 Werktage": gendered by the plural noun
    noun = context.after(3);
    if (noun.endsWith("en")) {
      noun = noun.slice(0, -1);
    } else if (noun.endsWith("e")) {
      noun = noun.slice(0, -1);
    }
  }
  result.article = capitalized(noun) && noun !== "Quadrat" && (noun !== next || !labelled(context));
  // über 1/4, unter 3 Euro: "more than", "less than" before an amount, not prepositions
  const before = context.before();
  result.grammaticalCase = before === "über" || before === "unter" ? Case.NOMINATIVE : caseAfter(before);
  result.gender = genderOf(noun);

  // kostet 1 Euro, lief 1 km: after a verb, an amount is its object (einen Euro); but ist 1 Euro, als 1 Euro
  const head = wordsOf(context.head());
  const last = head.length === 0 ? "" : str(stripped(head[head.length - 1]));
  const amount = match.kind === Kind.MONEY || match.kind === Kind.UNIT || match.kind === Kind.FRACTION;
  const lowercaseWord = last !== "" && [...last].every((c) => isLower(c));
  if (amount && result.grammaticalCase === Case.NOMINATIVE && lowercaseWord && !NOMINATIVE_WORDS.has(last)) {
    result.grammaticalCase = Case.ACCUSATIVE;
  }

  // 1998 Abi gemacht, im Jahr 1945; but 1945 Einwohner, 1920 mal 1080
  const month = (MONTHS as readonly string[]).includes(next);
  const measure =
    next.endsWith("e") || next.endsWith("n") || next.endsWith("er") || next.endsWith("s") || MEASURE_WORDS.has(next);
  const counted = capitalized(next) && measure && !month;
  const previous = context.before();
  const times = next === "mal" || previous === "mal";
  result.year = !times && (YEAR_CUES.has(previous) || !counted);
  return result;
}

// --------------------
// ----- Patterns -----
// --------------------

// 2.3, 02.03: a day and a month without a dot after them, as the whole text
class BareDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const dot = digitsEnd(text, position);
    if (position > 0 || dot === position || dot > 2 || dot >= text.length || text[dot] !== ".") {
      return undefined;
    }
    const end = digitsEnd(text, dot + 1);
    if (end !== text.length || end - dot - 1 > 2 || end === dot + 1) {
      return undefined;
    }
    const day = digitsAt(text, 0);
    const month = digitsAt(text, dot + 1);
    if (value(day) < 1n || value(day) > 31n || value(month) < 1n || value(month) > 12n) {
      return undefined;
    }
    return matchOf({ kind: Kind.DATE, span: { begin: 0, end }, numbers: [day, month], separator: "." });
  }
}

// 2003-03: a year and its month; not a range of years, whose end is later (2003-04)
class YearMonthPattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const hyphen = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, ".,-/") ||
      hyphen !== position + 4 ||
      hyphen >= text.length ||
      text[hyphen] !== "-"
    ) {
      return undefined;
    }
    const end = digitsEnd(text, hyphen + 1);
    if (end !== hyphen + 3 || !isClearAfter(text, end) || isDigitAfter(text, end, ".-/")) {
      return undefined;
    }
    const year = digitsAt(text, position);
    const month = digitsAt(text, hyphen + 1);
    if (value(month) < 1n || value(month) > 12n || value(month) > value(year) % 100n) {
      return undefined;
    }
    return matchOf({ kind: Kind.DATE, span: { begin: position, end }, numbers: [year, month], separator: "-" });
  }
}

// 030 1234567, 0800-123 45 67, (0411) 1234-1234: an area code starting with 0, then the number, optionally in groups.
class AreaCodePattern extends Pattern {
  constructor() {
    super(Kind.PHONE);
  }

  match(text: Text, position: number): Match | undefined {
    const parenthesized = text[position] === "(";
    const area = parenthesized ? position + 1 : position;
    if (!isClearBefore(text, position, ".,") || area >= text.length || text[area] !== "0") {
      return undefined;
    }

    // The area code and the number: written together (0301234567), or apart (030 1234567, 030/1234567, (030) 1234567)
    let end = digitsEnd(text, area);
    const length = end - area;
    if (parenthesized) {
      const number = end < text.length && text[end] === ")" ? skipSpace(text, end + 1) : end;
      if (length < 3 || number === end || digitsEnd(text, number) < number + 2) {
        return undefined;
      }
      end = digitsEnd(text, number);
    } else if (length < 6) {
      const apart =
        length >= 3 &&
        end < text.length &&
        (text[end] === " " || text[end] === "/" || text[end] === "-") &&
        digitsEnd(text, end + 1) >= end + 4;
      if (!apart) {
        return undefined;
      }
      end = digitsEnd(text, end + 1);
    }

    // Then any further groups: 0800-123 45 67, (030) 4664 - 911300. Each could end the number, if nothing continues it.
    const ends = [end];
    for (;;) {
      const last = ends[ends.length - 1];
      const spaced = text[last] === " " && text[last + 1] === "-" && text[last + 2] === " ";
      const next = last + (spaced ? 3 : 1);
      const separator = last < text.length && (text[last] === " " || text[last] === "-");
      if (!separator || next >= text.length || !isDigit(text[next])) {
        break;
      }
      ends.push(digitsEnd(text, next));
    }
    for (let k = ends.length - 1; k >= 0; k--) {
      const at = ends[k];
      // Not followed by a digit, even after a word character, a dot or a comma
      const continued = isDigitAfter(text, at) || (!isClearAfter(text, at, ".,") && isDigitAfter(text, at + 1));
      if (!continued) {
        const phone = matchOf({ kind: Kind.PHONE, span: { begin: position, end: at } });
        for (let i = position; i < at;) {
          const digits = digitsEnd(text, i);
          if (digits > i) {
            phone.numbers.push(digitsAt(text, i));
            i = digits;
          } else {
            i++;
          }
        }
        return phone;
      }
    }
    return undefined;
  }
}

// 90 Millionen g, 1 1/2 ft: a scale or a fraction, then a unit after an optional space.
class UnitAfterPattern extends Pattern {
  readonly #base: Pattern;
  readonly #units: Keys = keysOf(UNITS);

  constructor(base: Pattern) {
    super(base.kind);
    this.#base = base;
  }

  match(text: Text, position: number): Match | undefined {
    const found = this.#base.match(text, position);
    if (!found) {
      return undefined;
    }
    const start = skipSpace(text, found.span.end);
    for (const unit of keysAt(text, start, this.#units)) {
      if (isClearAfter(text, start + unit.length)) {
        return { ...found, span: { begin: found.span.begin, end: start + unit.length }, unit };
      }
    }
    return undefined;
  }
}

// --------------------
// ----- Language -----
// --------------------

// Words after which a capitalized word still continues an ordinal: und, oder, bis, and the inflecting words
const INFLECTING: ReadonlySet<string> = new Set([
  "und",
  "oder",
  "bis",
  ...[WEAK, WEAK_NOMINATIVE, MIXED, DATIVE, ACCUSATIVE, GENITIVE].flatMap((set) => [...set]),
]);

class German extends PatternLanguage {
  constructor() {
    super({ terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new BareDatePattern());
    this.addPattern(new YearMonthPattern());
    this.addPattern(new AreaCodePattern());
    this.addPattern(new SecondsPattern(SECONDS_SUFFIXES, true));
    this.addPattern(new PrimePattern(NOTATION, PRIME_CUES));
    this.addPattern(new UnitAfterPattern(new ScalePattern(NOTATION)));
    this.addPattern(new UnitAfterPattern(new FractionPattern()));
    this.addPattern(new DotOrdinalPattern(true));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern(true));
  }

  protected override readNumber(number: Numeral, match: Match): string {
    if (number.fraction !== "") {
      const head = number.integer === "" ? "" : `${cardinal(value(number))} `;
      return `${head}Komma ${digits(number.fraction)}`;
    }

    // The whole number of 1 1/2: ein einhalb
    if (match.kind === Kind.FRACTION && isOne(number)) {
      return "ein";
    }
    // A long number is a code (40474 Düsseldorf, WKN 840400), unless it is an amount: 18880 €, 42259 m²
    const n = parseValue(number.integer);
    const amount =
      match.kind === Kind.MONEY || match.kind === Kind.UNIT || match.kind === Kind.PERCENT || match.kind === Kind.SCALE;
    // 46664 alone, or after its sign: the whole text is a number, not a code
    const head = str(match.context.head());
    const alone = (head === "" || head === `${this.joiners().minus} `) && match.context.tail().length === 0;
    if (n === undefined || (isSpokenDigits(number, 5) && !((amount || alone) && number.integer[0] !== "0"))) {
      return digits(number.integer);
    }
    const cues = cuesOf(match);
    if (n === 1n && cues.article) {
      return article(cues.grammaticalCase, cues.gender);
    }
    if (number.integer.length === 4 && !number.grouped && cues.year) {
      return year(n);
    }
    return cardinal(n);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 4);
    return n !== undefined ? year(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    // 2003-03: März zweitausenddrei
    if (match.separator === "-" && match.numbers.length === 2) {
      return `${MONTHS[Number(value(match.numbers[1])) - 1]} ${year(value(match.numbers[0]))}`;
    }
    // "am 12. 3." is a date, "Platz 1. 2 Tore" is not
    if (match.text.includes(" ") && !DATE_CUES.has(match.context.before())) {
      return undefined;
    }
    const words = `${ordinal(value(match.numbers[0]), ending(cuesOf(match)))} ${MONTHS[Number(value(match.numbers[1])) - 1]}`;
    return match.numbers.length < 3 ? words : `${words} ${year(value(match.numbers[2]))}`;
  }

  protected override readPhone(match: Match): string | undefined {
    return digits(match.numbers.map((group) => group.integer).join(""));
  }

  protected override readFraction(match: Match): string | undefined {
    // A half before a unit or a noun: 1/2 l einen halben Liter, 1/2 Stunde eine halbe Stunde
    const cues = cuesOf(match);
    const [a, b] = fractionOf(match);
    if (match.numbers.length === 3 && a >= b) {
      return undefined; // 1 5/3: the whole number and the fraction read apart
    }
    const next = match.context.after();
    const noun = match.unit === "" ? next : UNITS[match.unit].one;
    if (a === 1n && b === 2n && match.numbers.length === 2 && capitalized(noun)) {
      const gender = genderOf(noun);
      const words = `${article(cues.grammaticalCase, gender)} ${half(cues.grammaticalCase, gender)}`;
      return match.unit === "" ? words : `${words} ${noun}`;
    }
    if (match.unit === "") {
      return this.#fraction(match, cues.grammaticalCase);
    }

    // 1 1/2 ft: ein einhalb Fuß; 3/4 m: drei Viertel Meter
    const unit = UNITS[match.unit];
    return `${this.#fraction(match, cues.grammaticalCase)} ${match.numbers.length === 3 ? unit.many : unit.one}`;
  }

  protected override readTime(match: Match): string | undefined {
    if (match.separator === "′") {
      return German.#primes(match);
    }
    if (match.numbers.length === 3) {
      return this.#readSeconds(match);
    }
    if (match.numbers.length === 2) {
      const decimal = match.separator === "." && match.suffix === "";
      // mit 31:32 (15:21): a score, read by its own pattern; but um 15:21, Spiel (Montag, 18:00)
      const score =
        match.separator === ":" &&
        match.suffix === "" &&
        match.numbers[1].integer !== "00" &&
        !TIME_WORDS.has(match.context.before()) &&
        PatternLanguage.mentions(match.context, SPORT_WORDS);
      if (!fitsClock(match) || decimal || score) {
        return undefined;
      }
    }
    let words = `${stem(value(match.numbers[0]))} Uhr`;
    if (match.numbers.length === 2 && value(match.numbers[1]) > 0n) {
      words += ` ${cardinal(value(match.numbers[1]))}`;
    }
    return words;
  }

  protected override readOrdinal(match: Match): string | undefined {
    // 3ten, 121tem, 20ste: the ending as written
    if (match.suffix !== ".") {
      return ordinal(value(match.numbers[0]), match.suffix.slice(match.suffix.startsWith("st") ? 2 : 1));
    }

    // "Platz 3. Danach", "Platz 1. 2 Tore": a sentence end, not an ordinal
    const words = wordsOf(match.context.tail());
    const next = words.length === 0 ? "" : str(words[0]);
    // 14. Juni, (a month before a comma)
    const month = (MONTHS as readonly string[]).includes(words.length === 0 ? "" : str(stripped(words[0])));
    // 100. Geburtstag, 2.: a text doesn't open with a sentence of a bare number
    const start = match.context.head().length === 0;
    const sentence = capitalized(next) && !month && !INFLECTING.has(match.context.before()) && !start;
    // Before a comma or the end: only right after an article (in der 79., am 3.), not "Platz 23.," or "von 1983."
    const first = next === "" ? "" : String.fromCodePoint(next.codePointAt(0) as number);
    if (next === "" || contains(PUNCTUATION, first)) {
      const previous = match.context.before();
      if (!WEAK.has(previous) && !WEAK_NOMINATIVE.has(previous) && !(start && next === "")) {
        return undefined;
      }
      return ordinal(value(match.numbers[0]), ending(cuesOf(match))) + (next === "" ? "." : "");
    }
    if (isDigit(first) || sentence) {
      return undefined;
    }
    return ordinal(value(match.numbers[0]), ending(cuesOf(match)));
  }

  protected override readRoman(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    const role = romanRole(match, ROMAN_CUES);
    if (role === undefined) {
      return undefined;
    }
    switch (role) {
      case RomanRole.NAME: {
        // Ludwig der Vierzehnte, unter Ludwig dem Vierzehnten, für Wilhelm den Zweiten, Elisabeth die Zweite
        const feminine = FEMININE_NAMES.has(match.context.before());
        const dative = DATIVE.has(match.context.before(2));
        const accusative = ACCUSATIVE.has(match.context.before(2));
        const article = dative
          ? feminine
            ? "der"
            : "dem"
          : accusative
            ? feminine
              ? "die"
              : "den"
            : feminine
              ? "die"
              : "der";
        const words = ordinal(n, dative || (accusative && !feminine) ? "en" : "e");
        return `${article} ${upperFirst(words)}`;
      }
      case RomanRole.AFTER:
        // im XX. Jahrhundert: an ordinal, as im 20. Jahrhundert
        return this.readOrdinal({ ...match, kind: Kind.ORDINAL });
      default:
        return cardinal(n); // Kapitel drei
    }
  }

  protected override readDecade(match: Match): string | undefined {
    // 90er, 90ern: neunziger, neunzigern; 1990er: neunzehnhundertneunziger
    const digits = match.numbers[0].integer;
    const number =
      digits.length > 1 && digits[0] === "0"
        ? `null${cardinal(value(match.numbers[0]))}`
        : year(value(match.numbers[0]));
    let words = number + match.suffix;
    if (cuesOf(match).capitalized) {
      words = upperFirst(words);
    }
    return words;
  }

  protected override readCompound(match: Match): string | undefined {
    // 30-jährige: dreißigjährige; 4-Zimmer-Wohnung: Vier-Zimmer-Wohnung
    const number = match.numbers[0];
    let words = number.fraction === "" && !number.grouped ? stem(value(number)) : this.readNumber(number, match);
    if (capitalized(match.suffix)) {
      words = upperFirst(words);
      return `${words}-${match.suffix}`;
    }
    return words + match.suffix;
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    // 12,50 €/m²: zwölf Euro fünfzig pro Quadratmeter
    let noun = CURRENCIES[match.unit];
    let per = "";
    const at = noun.indexOf(" pro ");
    if (at !== -1) {
      if (match.span.begin < match.numbers[0].span.begin) {
        return undefined; // Only after its amount: 19,87 €/l, not €/l 14,90
      }
      per = noun.slice(at);
      noun = noun.slice(0, at);
    }
    return this.#price(number, match, noun) + per;
  }

  // An amount of a currency: drei Euro fünfzig, neunundneunzig Cent
  #price(number: Numeral, match: Match, noun: string): string {
    const one = article(cuesOf(match).grammaticalCase, genderOf(noun));
    if (match.scale !== "") {
      return `${this.#scaled(number, match)} ${noun}`;
    }

    // Decimals as cents, without trailing zeros: 2,2 € and 2,200 € read like 2,20 €
    let fraction = number.fraction;
    while (fraction.length > 2 && fraction.endsWith("0")) {
      fraction = fraction.slice(0, -1);
    }
    if (fraction.length === 1) {
      fraction += "0";
    }
    const amount = { ...number, fraction };

    // A price: drei Euro fünfzig, neunundneunzig Cent, ein Cent; 2,00 €: zwei Euro
    if (amount.fraction.length === 2) {
      const cents = parseValue(amount.fraction) ?? 0n;
      if (value(amount) === 0n) {
        return `${cents === 1n ? article(cuesOf(match).grammaticalCase, Gender.MASCULINE) : cardinal(cents)} Cent`;
      }
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: amount.integer, grouped: amount.grouped });
      const words = `${amount.integer === "1" ? one : this.readNumber(whole, match)} ${noun}`;
      return cents !== 0n ? `${words} ${cardinal(cents)}` : words;
    }

    const single = amount.integer === "1" && amount.fraction === "";
    return `${single ? one : this.readNumber(amount, match)} ${noun}`;
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    // 90 Millionen g: neunzig Millionen Gramm
    const words = this.#scaled(number, match);
    return match.unit === "" ? words : `${words} ${UNITS[match.unit].many}`;
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    const words = this.readNumber(number, match);
    // 8%ige: achtprozentige
    return match.suffix === "" ? `${words} Prozent` : `${words}prozent${match.suffix}`;
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    const unit = UNITS[match.unit];
    if (number.integer === "1" && number.fraction === "") {
      return `${article(cuesOf(match).grammaticalCase, genderOf(unit.one))} ${unit.one}`;
    }
    return `${this.readNumber(number, match)} ${unit.many}`;
  }

  // ein Fünftel, drei Achtel, ein Zwanzigstel, ein Hundertstel; else drei durch sieben. After a whole number, a half is
  // einhalb: zweieinhalb. The "ein" is the article of a neuter noun (das Drittel): von einem Achtel.
  #fraction(match: Match, grammaticalCase: Case): string {
    const [a, b] = fractionOf(match);
    if (a === 1n && b === 2n && match.numbers.length === 3) {
      return "einhalb";
    }
    const one = article(grammaticalCase, Gender.NEUTER);
    const words = namedFraction(FRACTIONS, match);
    if (words !== undefined) {
      return a === 1n && b > 2n && match.numbers.length === 2 ? one + words.slice(3) : words;
    }

    // Seasons and school years: 2023/24, 2011/2012 read as two years
    const [first, second] = match.numbers.slice(-2).map((number) => number.integer);
    const years =
      first.length === 4 &&
      a >= 1900n &&
      a < 2100n &&
      ((second.length === 2 && b === (a + 1n) % 100n) || (second.length === 4 && b === a + 1n));
    if (years) {
      return `${year(a)} ${second.length === 2 ? cardinal(b) : year(b)}`;
    }

    if (b < 3n || a >= b || !isMarkedFraction(match, FRACTION_WORDS)) {
      return `${cardinal(a)} durch ${cardinal(b)}`;
    }
    let denominator = ordinal(b, "el");
    if (b === 100n || b === 1000n) {
      denominator = denominator.slice(3); // einhundertstel
    }
    return `${a === 1n ? one : cardinal(a)} ${toUpper(denominator[0])}${denominator.slice(1)}`;
  }

  // A clock time before "Uhr": zwei Uhr zwei Minuten dreiundzwanzig Sekunden; else a duration: zwei Stunden acht
  // Minuten neunundzwanzig Sekunden
  #readSeconds(match: Match): string | undefined {
    const [hours, minutes, seconds] = match.numbers.map(value);
    const clock = match.suffix === "Uhr";
    if (minutes > 59n || seconds > 59n || (clock && hours > 24n)) {
      return undefined;
    }
    const count = (n: bigint, one: string, many: string) => (n === 1n ? `eine ${one}` : `${cardinal(n)} ${many}`);
    const head = clock ? `${stem(hours)} Uhr` : count(hours, "Stunde", "Stunden");
    return `${head} ${count(minutes, "Minute", "Minuten")} ${count(seconds, "Sekunde", "Sekunden")}`;
  }

  // 1'27"23: eine Minute siebenundzwanzig Sekunden und dreiundzwanzig Hundertstel
  static #primes(match: Match): string | undefined {
    const first = match.unit === "h" ? 0 : match.unit === "min" ? 1 : 2;
    const words = match.numbers.map((number, i) => {
      const n = value(number);
      const unit = PRIME_UNITS[first + i];
      return n === 1n ? `eine ${unit.one}` : `${cardinal(n)} ${unit.many}`;
    });
    const fraction = match.numbers[match.numbers.length - 1].fraction;
    if (fraction !== "") {
      words.push(`und ${cardinal(parseValue(fraction) ?? 0n)} ${SECOND_FRACTIONS[fraction.length - 1]}`);
    }
    return join(words);
  }

  // An amount with its scale: drei Millionen, zehntausend
  #scaled(number: Numeral, match: Match): string {
    const scale = SCALES[match.scale];
    if (scale.many === "tausend") {
      return `${this.readNumber(number, match)}tausend`;
    }
    const single = number.integer === "1" && number.fraction === "";
    return single ? `eine ${scale.one}` : `${this.readNumber(number, match)} ${scale.many}`;
  }
}

let instance: German | undefined;

export function language(): LanguageHandle {
  return (instance ??= new German());
}
