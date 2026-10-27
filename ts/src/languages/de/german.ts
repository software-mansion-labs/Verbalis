import { DatePattern, DotOrdinalPattern, Pattern, SuffixPattern, RomanPattern } from "../../numbers/patterns.ts";
import { Kind, match as matchOf, numeral, type Match, type Numeral } from "../../numbers/match.ts";
import {
  codeValue,
  digitsAt,
  fitsClock,
  isSpokenDigits,
  namedFraction,
  parseValue,
  value,
  romanRole,
  RomanRole,
} from "../../numbers/scanning.ts";
import { stripped, wordsOf, type Context } from "../../text/context.ts";
import { digitsEnd, isClearAfter, isClearBefore, isDigitAfter, join } from "../../text/scan.ts";
import { contains, str, toLower, type Text } from "../../text/strings.ts";
import { isDigit, isUpper, toUpper } from "../../text/unicode.ts";
import { Case, Gender } from "../grammar.ts";
import { PatternLanguage } from "../language.ts";
import {
  ACCUSATIVE,
  CURRENCIES,
  DATE_CUES,
  DATE_FORMAT,
  DATIVE,
  DECADE_FORMAT,
  DECADE_SUFFIXES,
  FEMININE_ENDINGS,
  FRACTIONS,
  GENDERS,
  GENITIVE,
  JOINERS,
  MEASURE_WORDS,
  MIXED,
  MONTHS,
  NEUTER_ENDINGS,
  NOTATION,
  ONES,
  ORDINAL_STEMS,
  SCALES,
  SYMBOLS,
  TENS,
  TERMS,
  UNITS,
  WEAK,
  WEAK_NOMINATIVE,
  YEAR_CUES,
  ROMAN_CUES,
  FEMININE_NAMES,
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
  result.grammaticalCase = caseAfter(context.before());
  result.gender = genderOf(noun);

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

// 030 1234567, 0800-123 45 67: an area code starting with 0, then the number, optionally in groups.
class AreaCodePattern extends Pattern {
  constructor() {
    super(Kind.PHONE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,") || text[position] !== "0") {
      return undefined;
    }

    // The area code and the number: written together (0301234567), or apart (030 1234567, 030/1234567)
    let end = digitsEnd(text, position);
    const length = end - position;
    if (length < 6) {
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

    // Then any further groups: 0800-123 45 67. Each could end the number, if nothing continues it there.
    const ends = [end];
    while (
      ends[ends.length - 1] + 1 < text.length &&
      (text[ends[ends.length - 1]] === " " || text[ends[ends.length - 1]] === "-") &&
      isDigit(text[ends[ends.length - 1] + 1])
    ) {
      ends.push(digitsEnd(text, ends[ends.length - 1] + 1));
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
    this.addPattern(new AreaCodePattern());
    this.addPattern(new DotOrdinalPattern());
    this.addPattern(new SuffixPattern(Kind.DECADE, DECADE_SUFFIXES, DECADE_FORMAT));
    this.addPattern(new RomanPattern(true));
  }

  protected override readNumber(number: Numeral, match: Match): string {
    if (number.fraction !== "") {
      const head = number.integer === "" ? "" : `${cardinal(value(number))} `;
      return `${head}Komma ${digits(number.fraction)}`;
    }

    const n = parseValue(number.integer);
    if (n === undefined || isSpokenDigits(number, 5)) {
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
    return (
      namedFraction(FRACTIONS, match) ??
      `${cardinal(value(match.numbers[0]))} durch ${cardinal(value(match.numbers[1]))}`
    );
  }

  protected override readTime(match: Match): string | undefined {
    if (match.numbers.length === 2) {
      const decimal = match.separator === "." && match.suffix === "";
      if (!fitsClock(match) || decimal) {
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
    // "Platz 3. Danach", "Platz 1. 2 Tore": a sentence end, not an ordinal
    const words = wordsOf(match.context.tail());
    const next = words.length === 0 ? "" : str(words[0]);
    const month = (MONTHS as readonly string[]).includes(next);
    const sentence = capitalized(next) && !month && !INFLECTING.has(match.context.before());
    if (next === "" || isDigit(String.fromCodePoint(next.codePointAt(0) as number)) || sentence) {
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
    const noun = CURRENCIES[match.unit];
    const one = article(cuesOf(match).grammaticalCase, genderOf(noun));
    if (match.scale !== "") {
      return `${this.#scaled(number, match)} ${noun}`;
    }

    // A price: drei Euro fünfzig, neunundneunzig Cent
    if (number.fraction.length === 2) {
      const cents = parseValue(number.fraction) ?? 0n;
      if (value(number) === 0n) {
        return `${cardinal(cents)} Cent`;
      }
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      const words = number.integer === "1" ? one : this.readNumber(whole, match);
      return `${words} ${noun} ${cardinal(cents)}`;
    }

    const single = number.integer === "1" && number.fraction === "";
    return `${single ? one : this.readNumber(number, match)} ${noun}`;
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    return this.#scaled(number, match);
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

export function language(): PatternLanguage {
  return (instance ??= new German());
}
