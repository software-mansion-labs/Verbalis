import {
  DatePattern,
  DotOrdinalPattern,
  PhonePattern,
  SuffixPattern,
  Pattern,
  RomanPattern,
  PrimePattern,
} from "../../numbers/patterns.ts";
import { Kind, numeral, type Match, type Numeral, match as matchOf } from "../../numbers/match.ts";
import {
  codeValue,
  fitsClock,
  fractionOf,
  isSpokenDigits,
  parseValue,
  spacedSuffix,
  value,
  digitsAt,
  romanRole,
  romanValue,
  toDigits,
  isYearValue,
  mentions,
} from "../../numbers/scanning.ts";
import { Context, PUNCTUATION, stripped, wordsOf } from "../../text/context.ts";
import type { Span } from "../../text/document.ts";
import {
  digitsEnd,
  isClearBefore,
  join,
  isClearAfter,
  isDigitAfter,
  isSentenceEnd,
  keysAt,
  skipSpace,
  type Keys,
} from "../../text/scan.ts";
import { contains, size, startsWith, str, toLower, type Text } from "../../text/strings.ts";
import { isDigit, isLower, isSpace, isUpper, isWord, isAlpha } from "../../text/unicode.ts";
import { keysOf, primeFormat, type RomanCues } from "../../numbers/formats.ts";
import { Case, Gender, Prime } from "../grammar.ts";
import { PatternLanguage, type LanguageHandle } from "../language.ts";
import {
  ADJECTIVE_ENDINGS,
  AMOUNT_NOUNS,
  BILLION,
  CARDINALS,
  CHARACTERS,
  CLITICS,
  COMBINING,
  CALL_VERBS,
  COMPLEX_PREPOSITIONS,
  CONJUNCTIONS,
  COPULAS,
  COUNT_NOUNS,
  CURRENCIES,
  CURRENCY_WORDS,
  DATE_FORMAT,
  DECADE_WORDS,
  FEMININE_PLURALS,
  GENITIVE_VERBS,
  GROSZ,
  CENT,
  CENTS,
  SAME_IN_BOTH_CASES,
  VIRILE_NOUNS,
  HOUR_WORDS,
  TIME_UNITS,
  HEADINGS,
  JOINERS,
  LOCATIVE_PREPOSITIONS,
  MILLION,
  MONTH_WORDS,
  MONTHS_GENITIVE,
  ORDINAL_NOUNS,
  noun,
  IMPERATIVES,
  STREET_WORDS,
  FRACTION_STOPS,
  NOTATION,
  NOUN_ENDINGS,
  ONE,
  ORDINAL_HEADINGS,
  PARTICLES,
  DEGREE_ADVERBS,
  ORDINAL_STEMS,
  ORDINAL_SUFFIXES,
  PERCENT,
  PHONE_FORMATS,
  PHONE_WORDS,
  PREPOSITION_CASES,
  SCALES,
  SPORT_WORDS,
  SYMBOLS,
  TERMS,
  THOUSAND,
  TIME_WORDS,
  UNITS,
  YEAR_FORMAT,
  YEAR_PREPOSITIONS,
  YEAR_SUFFIXES,
  type Noun,
  CENTURY_ABBREVIATIONS,
  REGNAL_NAMES,
  ROMAN_NOUNS,
  INCH,
  HEIGHT_CUES,
  PRIME_CUES,
  BECOME_STEM,
  LIST_CONJUNCTIONS,
  NUMBER_ABBREVIATION,
  NUMBER_STEM,
  PRIME_UNITS,
  SIGNS,
} from "./constants.ts";

// Whether a number reads as a count or as an ordinal (years, days of a month, headings)
const Form = { CARDINAL: 0, ORDINAL: 1 } as const;
type Form = (typeof Form)[keyof typeof Form];

// Inflection of a number: its form, and the case and gender its words agree with
interface Inflection {
  form: Form; // An ordinal for years, days of a month and headings
  grammaticalCase: Case;
  gender: Gender;
  plural: boolean; // lata 70., 18 urodziny: an ordinal in the plural
}

function inflection(form: Form = Form.CARDINAL, grammaticalCase: Case = Case.NOMINATIVE): Inflection {
  return { form, grammaticalCase, gender: Gender.MASCULINE, plural: false };
}

// What the words around a match say about reading its numbers: the inflection of its first number, and more
interface Cues extends Inflection {
  amountCase: Case; // The case of an amount (5 kg): of its number and its noun
  numbered: boolean; // numerem 3x38: the "x" of a document number stands for a slash
}

function startsWithAny(word: string, stems: readonly string[]): boolean {
  return stems.some((stem) => word.startsWith(stem));
}

// ------------------------
// ----- Number words -----
// ------------------------

// The column of CARDINALS: nominative and accusative, genitive and locative, dative, instrumental
function column(c: Case): number {
  switch (c) {
    case Case.NOMINATIVE:
    case Case.ACCUSATIVE:
      return 0;
    case Case.GENITIVE:
    case Case.LOCATIVE:
      return 1;
    case Case.DATIVE:
      return 2;
    default:
      return 3;
  }
}

// The form of a noun after a number: 1 / 2-4 / 5+ in the nominative, the plural of the case otherwise
function form(noun: Noun, n: bigint, c: Case, fraction = false): string {
  if (fraction) {
    return noun.singular[Case.GENITIVE];
  }
  if (n === 1n) {
    return noun.singular[c];
  }
  if (c === Case.NOMINATIVE || c === Case.ACCUSATIVE) {
    const few = n % 10n >= 2n && n % 10n <= 4n && (n % 100n < 12n || n % 100n > 14n);
    return noun.plural[few ? Case.NOMINATIVE : Case.GENITIVE];
  }
  return noun.plural[c];
}

function belowThousand(n: bigint, c: Case, g: Gender): string {
  if (n === 0n) {
    return CARDINALS.get(0n)![column(c)];
  }

  const r = n % 100n;
  const words: string[] = [];
  for (const part of [n - r, r < 20n ? r : r - (r % 10n), r < 20n ? 0n : r % 10n]) {
    if (part === 1n) {
      words.push("jeden"); // Inside a compound number "jeden" does not inflect
    } else if (
      part === 2n &&
      g === Gender.FEMININE &&
      (c === Case.NOMINATIVE || c === Case.ACCUSATIVE || c === Case.INSTRUMENTAL)
    ) {
      words.push(c === Case.INSTRUMENTAL ? "dwiema" : "dwie");
    } else if (part !== 0n) {
      words.push(CARDINALS.get(part)![column(c)]);
    }
  }
  return join(words);
}

function cardinal(n: bigint, c: Case = Case.NOMINATIVE, g: Gender = Gender.MASCULINE): string {
  if (n === 1n) {
    return ONE[g][c];
  }

  const parts: string[] = [];
  for (const [scale, noun] of [
    [1000000000n, BILLION],
    [1000000n, MILLION],
    [1000n, THOUSAND],
  ] as const) {
    const q = n / scale;
    n %= scale;
    if (q !== 0n) {
      parts.push(q === 1n ? form(noun, q, c) : `${cardinal(q, c)} ${form(noun, q, c)}`);
    }
  }
  if (n !== 0n || parts.length === 0) {
    parts.push(belowThousand(n, c, g));
  }
  return join(parts);
}

// An ordinal stem with an adjective ending: drug + y -> drugi, trzec + a -> trzecia
function adjective(stem: string, ending: string): string {
  let e = ending;
  if (stem.endsWith("g") || stem.endsWith("k")) {
    e = e[0] === "y" ? `i${e.slice(1)}` : e[0] === "e" ? `i${e}` : e;
  } else if (stem.endsWith("c")) {
    e = e[0] === "y" ? `i${e.slice(1)}` : `i${e}`;
  }
  return stem + e;
}

// The form of a number inside a compound: czterdziestopięcio(metrowe), stu(letni)
function combining(n: bigint): string {
  const h = n - (n % 100n);
  const r = n % 100n;
  if (h >= 1000n) {
    return cardinal(n);
  }
  let words = h !== 0n ? COMBINING.get(h)! : "";
  if (r !== 0n) {
    words += r < 20n || r % 10n === 0n ? COMBINING.get(r)! : COMBINING.get(r - (r % 10n))! + COMBINING.get(r % 10n)!;
  }
  return words;
}

// Only the last two parts inflect: dwa tysiące dwudziestym drugim, tysiąc osiemsetnym
function ordinal(n: bigint, c: Case = Case.NOMINATIVE, g: Gender = Gender.MASCULINE, plural = false): string {
  const e = ADJECTIVE_ENDINGS[plural ? 3 : g][c];

  const r = n % 100n;
  if (r !== 0n) {
    const words = [n - r !== 0n ? cardinal(n - r) : ""];
    if (r < 20n || r % 10n === 0n) {
      words.push(adjective(ORDINAL_STEMS.get(r)!, e));
    } else {
      words.push(adjective(ORDINAL_STEMS.get(r - (r % 10n))!, e));
      words.push(adjective(ORDINAL_STEMS.get(r % 10n)!, e));
    }
    return join(words);
  }

  const h = n % 1000n;
  if (h !== 0n) {
    const words = [n - h !== 0n ? cardinal(n - h) : "", adjective(ORDINAL_STEMS.get(h)!, e)];
    return join(words);
  }

  const t = n / 1000n;
  return adjective(`${t === 1n ? "" : combining(t)}tysięczn`, e);
}

function digits(text: string): string {
  const words: string[] = [];
  for (const c of text) {
    words.push(c === "1" ? "jeden" : CARDINALS.get(BigInt(c))![0]);
  }
  return join(words);
}

// -------------------
// ----- Context -----
// -------------------

// A noun's case, gender and number, guessed from its ending (as seen after an ordinal): sesji, sesję, roku
interface NounForm {
  grammaticalCase: Case;
  gender: Gender;
  plural: boolean;
  pluralOnly?: boolean; // urodziny: never a form a count takes
}

// Whether a word is made only of given characters (and is not empty).
function madeOf(word: string, characters: string): boolean {
  return word !== "" && [...word].every((c) => contains(characters, c));
}

const DIGITS = "0123456789";

let genitivePluralForms: ReadonlySet<string> | undefined;

function genitivePlurals(): ReadonlySet<string> {
  // Not one which is also a nominative (procent, euro): trzydzieści cztery procent
  return (genitivePluralForms ??= new Set(
    [UNITS, CURRENCIES, SCALES].flatMap((table) =>
      Object.values(table)
        .filter((noun) => noun.plural[Case.GENITIVE] !== noun.plural[Case.NOMINATIVE])
        .map((noun) => noun.plural[Case.GENITIVE]),
    ),
  ));
}

// A noun that a number counts: osób, metrów, złotych (as opposed to a verb, or nothing after a year)
function counted(word: string): boolean {
  return (
    COUNT_NOUNS.has(word) ||
    genitivePlurals().has(word) ||
    word.endsWith("ów") ||
    word.endsWith("ych") ||
    word.endsWith("ich")
  );
}

/**
 * The words before a number within its clause (lowercased), and the first word after it, skipping a range
 * or coordination it belongs to (20-30, 3 a 368) and the złote of an amount of grosze (39 532 złotych 20 groszy).
 */
function surroundings(context: Context): [string[], string] {
  const head = context.head();
  let clause = 0;
  for (let i = 0; i + 1 < head.length; ++i) {
    if (contains(",;:.!?", head[i]) && isSpace(head[i + 1])) {
      clause = i + 1;
    }
  }

  const before: string[] = [];
  for (const word of wordsOf(head.slice(clause))) {
    before.push(toLower(str(stripped(word))));
  }
  const rangeStart = (word: string) => {
    const w = [...word];
    return w.length > 1 && isDigit(w[0]) && w.at(-1) === "-" && madeOf(str(w, 0, w.length - 1), "0123456789,. ");
  };
  while (before.length > 0 && rangeStart(before[before.length - 1])) {
    before.pop();
  }
  while (
    before.length >= 2 &&
    CONJUNCTIONS.has(before[before.length - 1]) &&
    madeOf(before[before.length - 2], "0123456789,.")
  ) {
    before.length -= 2;
  }
  if (before.length > 0 && SIGNS.has(before[before.length - 1])) {
    before.pop(); // do minus siedmiu stopni, od +12°C
  }
  // do nawet siedmiuset, przez około cztery: the preposition before a particle governs the number
  if (
    before.length >= 2 &&
    PARTICLES.has(before[before.length - 1]) &&
    PREPOSITION_CASES.has(before[before.length - 2])
  ) {
    before.pop();
  }
  if (before.length > 0 && CURRENCY_WORDS.has(before[before.length - 1])) {
    before.pop();
    while (before.length > 0 && madeOf(before[before.length - 1], DIGITS)) {
      before.pop();
    }
  }

  // The first word after, beyond the end of a range (-30)
  let next = "";
  const tail = context.tail();
  if (tail.length === 0 || !contains(PUNCTUATION, tail[0])) {
    for (const word of wordsOf(tail)) {
      const w = stripped(word);
      if (!(w.length > 1 && w[0] === "-" && madeOf(str(w, 1), "0123456789,."))) {
        next = toLower(str(w));
        break;
      }
    }
  }
  return [before, next];
}

// The context of the number this one continues after a comma (140 g mąki, 130 g mąki): both share a case.
function enumerated(context: Context): Context | undefined {
  const head = context.head();
  let end = head.length - 1;
  while (end >= 0 && contains(" \t\n\r", head[end])) {
    end--;
  }
  if (end < 0) {
    return undefined;
  }
  // spośród 510 ocen i 68 recenzji: a conjunction, as a comma
  const w = context.before();
  if (LIST_CONJUNCTIONS.has(w) && isAlpha(head[end])) {
    end = end + 1 - size(w);
  } else if (head[end] !== ",") {
    return undefined;
  }

  // The last number before the comma, at most four words before it
  const text: Text = head.slice(0, end);
  let number: Span | undefined;
  for (let i = 0; i < text.length;) {
    if (isDigit(text[i]) && isClearBefore(text, i, ".,")) {
      number = { begin: i, end: digitsEnd(text, i) };
      i = number.end;
    } else {
      i++;
    }
  }
  if (!number || wordsOf(text.slice(number.end)).length > 4) {
    return undefined;
  }
  // A score or a range, which reads "6 do 0" by now, ends in the genitive of its "do", which the next one doesn't
  // share: 6:0, 7:6 (sześć do zera, siedem do sześciu)
  const earlier = new Context(text, number);
  if (earlier.before() === "do" && madeOf(earlier.before(2), DIGITS)) {
    return undefined;
  }
  return earlier;
}

// The case of a number governed by an amount noun, a genitive verb or a negation right before it.
function governed(before: readonly string[]): Case | undefined {
  const w1 = before.length === 0 ? "" : before[before.length - 1];
  for (const stem of AMOUNT_NOUNS) {
    if (w1.startsWith(stem) && NOUN_ENDINGS.has(w1.slice(stem.length))) {
      return Case.GENITIVE; // od kwoty trzystu tysięcy
    }
  }

  const window = before.slice(before.length - Math.min(before.length, 3));
  const verbs = window.filter((w) => !CLITICS.has(w));
  if (verbs.length > 0 && startsWithAny(verbs[verbs.length - 1], GENITIVE_VERBS)) {
    return Case.GENITIVE; // potrzebowali stu czterdziestu gramów, brakuje nam trzech osób
  }

  // nie powinno przekraczać pięciu gramów, but not: nie jest to pięć, nie więcej niż pięć
  const no = window.indexOf("nie");
  if (no !== -1 && !window.slice(no + 1).some((w) => COPULAS.has(w) || w === "niż" || w === "tylko")) {
    return Case.GENITIVE;
  }
  return undefined;
}

// The forms of the nouns of ORDINAL_NOUNS: each form with its possible case, gender, and number
let ordinalNounForms: Map<string, NounForm[]> | undefined;
function ordinalNouns(): Map<string, NounForm[]> {
  if (ordinalNounForms === undefined) {
    ordinalNounForms = new Map();
    for (const [gender, list] of ORDINAL_NOUNS) {
      const n = noun(list);
      const pluralOnly = n.singular.every((form, i) => form === n.plural[i]); // urodziny
      for (let c = 0; c < 6; ++c) {
        for (const [form, plural] of [
          [n.singular[c], false],
          [n.plural[c], true],
        ] as const) {
          if (pluralOnly && !plural) {
            continue;
          }
          const forms = ordinalNounForms.get(form) ?? [];
          forms.push({ grammaticalCase: c as Case, gender, plural, pluralOnly });
          ordinalNounForms.set(form, forms);
        }
      }
    }
  }
  return ordinalNounForms;
}

// The cases a preposition gives a noun, in order of likelihood: w rundzie (locative), z miejsca (genitive)
function prepositionCases(preposition: string): Case[] {
  if (LOCATIVE_PREPOSITIONS.has(preposition)) {
    return [Case.LOCATIVE, Case.ACCUSATIVE];
  }
  if (["przed", "nad", "pod", "między", "pomiędzy", "za"].includes(preposition)) {
    return [Case.INSTRUMENTAL, Case.ACCUSATIVE];
  }
  const given = PREPOSITION_CASES.get(preposition);
  if (given !== undefined) {
    return [given === Case.INSTRUMENTAL ? Case.GENITIVE : given, given]; // z miejsca, ze stron
  }
  return [Case.NOMINATIVE, Case.ACCUSATIVE, Case.GENITIVE, Case.LOCATIVE, Case.DATIVE, Case.INSTRUMENTAL];
}

function nounForm(word: string, preposition: string): NounForm {
  // A known noun: the singular form its preposition makes likeliest, then the plural (w ramach fazy, not fazy)
  const known = ordinalNouns().get(word);
  if (known !== undefined) {
    for (const plural of [false, true]) {
      for (const c of prepositionCases(preposition)) {
        const form = known.find((f) => f.grammaticalCase === c && f.plural === plural);
        if (form !== undefined) {
          return form;
        }
      }
    }
    return known[0];
  }

  const locative = LOCATIVE_PREPOSITIONS.has(preposition);
  const instrumental =
    preposition === "z" ||
    preposition === "ze" ||
    preposition === "przed" ||
    preposition === "nad" ||
    preposition === "pod";

  const endings: ReadonlyArray<readonly [string, NounForm]> = [
    ["ami", { grammaticalCase: Case.INSTRUMENTAL, gender: Gender.MASCULINE, plural: true }],
    ["ach", { grammaticalCase: Case.LOCATIVE, gender: Gender.MASCULINE, plural: true }],
    ["ych", { grammaticalCase: Case.GENITIVE, gender: Gender.MASCULINE, plural: true }],
    ["ów", { grammaticalCase: Case.GENITIVE, gender: Gender.MASCULINE, plural: true }],
    ["iem", { grammaticalCase: Case.INSTRUMENTAL, gender: Gender.MASCULINE, plural: false }],
    ["em", { grammaticalCase: Case.INSTRUMENTAL, gender: Gender.MASCULINE, plural: false }],
    ["ym", { grammaticalCase: locative ? Case.LOCATIVE : Case.INSTRUMENTAL, gender: Gender.MASCULINE, plural: false }],
    [
      "ą",
      { grammaticalCase: instrumental ? Case.INSTRUMENTAL : Case.ACCUSATIVE, gender: Gender.FEMININE, plural: false },
    ],
    ["ę", { grammaticalCase: Case.ACCUSATIVE, gender: Gender.FEMININE, plural: false }],
    ["a", { grammaticalCase: Case.NOMINATIVE, gender: Gender.FEMININE, plural: false }],
    ["i", { grammaticalCase: locative ? Case.LOCATIVE : Case.GENITIVE, gender: Gender.FEMININE, plural: false }],
    ["y", { grammaticalCase: locative ? Case.LOCATIVE : Case.GENITIVE, gender: Gender.FEMININE, plural: false }],
    ["u", { grammaticalCase: locative ? Case.LOCATIVE : Case.GENITIVE, gender: Gender.MASCULINE, plural: false }],
    ["ie", { grammaticalCase: Case.LOCATIVE, gender: Gender.MASCULINE, plural: false }],
    ["e", { grammaticalCase: Case.NOMINATIVE, gender: Gender.NEUTER, plural: false }],
    ["o", { grammaticalCase: Case.NOMINATIVE, gender: Gender.NEUTER, plural: false }],
  ];
  for (const [ending, nounForm] of endings) {
    if (word.endsWith(ending)) {
      return nounForm;
    }
  }
  return { grammaticalCase: Case.NOMINATIVE, gender: Gender.MASCULINE, plural: false };
}

// The form, case and gender of a number from its context: an ordinal for years, days and headings.
function inflectionOf(context: Context, n: bigint, decimal: boolean, yearAbbreviation = false): Inflection {
  const [before, next] = surroundings(context);
  const w1 = before.length === 0 ? "" : before[before.length - 1];
  const w2 = before.length < 2 ? "" : before[before.length - 2];
  const w3 = before.length < 3 ? "" : before[before.length - 3];
  const noun = yearAbbreviation ? "roku" : next; // 2020 r.

  // do lat 3, od lat 5: an age in the case of the preposition before "lat" (but z lat 1991-2020 are years)
  if (w1 === "lat" && PREPOSITION_CASES.has(w2) && n < 100n) {
    return inflection(Form.CARDINAL, PREPOSITION_CASES.get(w2)!);
  }

  // 1990-1994: the end of a range of years stays a cardinal, as its start (but od 2020 do 2022 reads ordinals)
  const year = (w: string): boolean => w.length === 4 && madeOf(w, DIGITS);
  if (w1 === "do" && year(w2) && isYearValue(n) && w3 !== "od" && noun !== "roku" && noun !== "r") {
    return inflection(Form.CARDINAL, Case.NOMINATIVE);
  }

  // 1. Years, days and headings are ordinals
  const locative = LOCATIVE_PREPOSITIONS.has(w1);
  if ((noun === "roku" || noun === "rok" || noun === "rokiem") && !decimal) {
    // prettier-ignore
    const c =
      noun === "rok" ? Case.NOMINATIVE
      : noun === "rokiem" || (yearAbbreviation && w1 === "przed") ? Case.INSTRUMENTAL
      : locative ? Case.LOCATIVE
      : Case.GENITIVE;
    return inflection(Form.ORDINAL, c);
  }
  if (w1 === "dnia") {
    return inflection(Form.ORDINAL, Case.GENITIVE); // z dnia 1.
  }
  const hour = HOUR_WORDS.get(w1);
  if (hour !== undefined && n <= 24n && !decimal && !counted(noun)) {
    return { ...inflection(Form.ORDINAL, hour), gender: Gender.FEMININE }; // o godzinie piętnastej
  }
  // w (samym) roku 2016; but roku 76 pracowników counts
  if ((w1 === "roku" || w1 === "rok") && !counted(noun)) {
    const w2Locative =
      LOCATIVE_PREPOSITIONS.has(w2) || (w2.endsWith("ym") && LOCATIVE_PREPOSITIONS.has(w3) && n >= 1000n);
    return inflection(Form.ORDINAL, w1 === "rok" ? Case.NOMINATIVE : w2Locative ? Case.LOCATIVE : Case.GENITIVE);
  }
  // od pierwszego do piątego lipca: the first day of a range, before "do" and a day of a month
  const after = wordsOf(context.tail());
  const monthAfterRange =
    noun === "do" &&
    after.length >= 3 &&
    madeOf(str(stripped(after[1])), DIGITS) &&
    (MONTHS_GENITIVE as readonly string[]).includes(toLower(str(stripped(after[2]))));
  if (((MONTHS_GENITIVE as readonly string[]).includes(noun) || monthAfterRange) && n >= 1n && n <= 31n) {
    // 11 listopada to Święto: a date opening its sentence before a copula is its subject
    const subject = before.length === 0 && after.length >= 2 && COPULAS.has(toLower(str(stripped(after[1]))));
    // od pierwszego kwietnia, po pierwszym maja, przed dwudziestym czwartym grudnia, na szóstego grudnia, był
    // dwudziestego października
    // prettier-ignore
    const c =
      subject ? Case.NOMINATIVE
      : w1 === "po" ? Case.LOCATIVE
      : w1 === "przed" ? Case.INSTRUMENTAL
      : Case.GENITIVE;
    return inflection(Form.ORDINAL, c);
  }
  if (MONTH_WORDS.has(w1) && isYearValue(n)) {
    return inflection(Form.ORDINAL, Case.GENITIVE);
  }
  // października 18, 2017: a day after its month, and the year after them
  const head = context.head();
  const written = wordsOf(head);
  const monthAt = (back: number) =>
    written.length >= back &&
    (MONTHS_GENITIVE as readonly string[]).includes(toLower(str(stripped(written[written.length - back]))));
  const dayAfterMonth = monthAt(1) && n >= 1n && n <= 31n && context.tail()[0] === ",";
  const yearAfterDay =
    head.length >= 2 &&
    head[head.length - 2] === "," &&
    head[head.length - 1] === " " &&
    isYearValue(n) &&
    monthAt(2) &&
    madeOf(str(stripped(written[written.length - 1])), DIGITS);
  if (dayAfterMonth || yearAfterDay) {
    return inflection(Form.ORDINAL, Case.GENITIVE);
  }
  if ((w1 === "w" || w1 === "we") && isYearValue(n) && !genitivePlurals().has(noun)) {
    return inflection(Form.ORDINAL, Case.LOCATIVE);
  }
  const yearCase = YEAR_PREPOSITIONS.get(w1);
  if (yearCase !== undefined && isYearValue(n, 1900n) && !counted(noun)) {
    return inflection(Form.ORDINAL, yearCase); // od dwa tysiące dwudziestego do dwa tysiące dwudziestego drugiego
  }
  if (startsWithAny(w1, ORDINAL_HEADINGS) && n < 100n && !counted(noun)) {
    const heading = nounForm(w1, w2);
    return { ...inflection(Form.ORDINAL, heading.grammaticalCase), gender: heading.gender }; // w punkcie drugim, część druga
  }
  // w 4 lidze, na 6 kroku, 22 urodziny: before a form of a noun no count takes (a singular, or a noun with no
  // singular), a number is an ordinal agreeing with it; but nr 4 turnieju is a label
  const forms = ordinalNouns().get(noun);
  if (
    forms !== undefined &&
    n >= 2n &&
    !decimal &&
    !w1.startsWith(NUMBER_STEM) &&
    w1 !== NUMBER_ABBREVIATION &&
    !forms.some((form) => form.plural && !form.pluralOnly)
  ) {
    const form = nounForm(noun, w1);
    return { form: Form.ORDINAL, grammaticalCase: form.grammaticalCase, gender: form.gender, plural: form.plural };
  }

  // 2. The case of a cardinal: from a preposition, a governing word or a number it continues
  const result = inflection();
  const prepositionCase = PREPOSITION_CASES.get(w1);
  let governing: Case | undefined;
  let earlier: Context | undefined;
  if ((w1 === "pod" || w1 === "na") && CALL_VERBS.has(w2)) {
    result.grammaticalCase = Case.NOMINATIVE; // zadzwoń pod sto dwanaście: a number to call
  } else if (prepositionCase !== undefined) {
    result.grammaticalCase = prepositionCase;
  } else if (COMPLEX_PREPOSITIONS.has(w1) && COMPLEX_PREPOSITIONS.get(w1) === w2) {
    result.grammaticalCase = Case.GENITIVE;
  } else if ((governing = decimal || COPULAS.has(noun) ? undefined : governed(before)) !== undefined) {
    result.grammaticalCase = governing;
  } else if ((earlier = enumerated(context)) !== undefined) {
    result.grammaticalCase = inflectionOf(earlier, 0n, false).grammaticalCase;
  }

  // 3. The noun after it, as it agrees with the number, or with the end of the range it starts: 4-5 godzin agrees
  // with pięć, so cztery-pięć godzin
  const tail = context.tail();
  if (tail.length > 1 && tail[0] === "-" && isDigit(tail[1])) {
    n = parseValue(str(tail, 1, digitsEnd(tail, 1))) ?? n;
  }
  const few = n % 10n >= 2n && n % 10n <= 4n && (n % 100n < 12n || n % 100n > 14n);
  const nominative = result.grammaticalCase === Case.NOMINATIVE || result.grammaticalCase === Case.ACCUSATIVE;
  if ((noun.endsWith("ami") || noun.endsWith("mi")) && size(noun) > 4) {
    result.grammaticalCase = Case.INSTRUMENTAL;
  } else if (noun.endsWith("ach") && size(noun) > 4) {
    result.grammaticalCase = Case.LOCATIVE;
  } else if (noun.endsWith("om") && size(noun) > 4 && !decimal) {
    result.grammaticalCase = Case.DATIVE; // pomogliśmy czterem rodzinom
  } else if (nominative && n >= 2n && n % 1000n !== 0n && VIRILE_NOUNS.has(noun)) {
    result.grammaticalCase = Case.GENITIVE; // piętnastu pracowników, but trzydzieści osiem milionów mieszkańców
  } else if (nominative && few && counted(noun) && !SAME_IN_BOTH_CASES.has(noun)) {
    result.grammaticalCase = Case.GENITIVE; // trzech osób: the noun is genitive, so the number is too
  }

  // jedna godzina, jednej godziny, but jednego stopnia (a masculine genitive in -a)
  const nominativeFeminine = noun.endsWith("a") && result.grammaticalCase === Case.NOMINATIVE;
  const genitiveFeminine = (noun.endsWith("y") || noun.endsWith("i")) && result.grammaticalCase === Case.GENITIVE;
  // na 2 równe części: the noun after an adjective
  const adjective = noun.endsWith("e") || noun.endsWith("ych") || noun.endsWith("ich");
  const headNoun = adjective && after.length >= 2 ? toLower(str(stripped(after[1]))) : noun;
  if (((nominativeFeminine || genitiveFeminine) && n === 1n) || startsWithAny(headNoun, FEMININE_PLURALS)) {
    result.gender = Gender.FEMININE;
  } else if (noun === "euro") {
    result.gender = Gender.NEUTER;
  }
  return result;
}

// The word governing a dot ordinal: the one before it, or before the first ordinal of "lat 50. i 60."
// The case of a fraction, from the word before it: około jednej piątej, w jednej czwartej, zjadł jedną szóstą
function fractionCase(context: Context): Case {
  const before = context.before();
  if (before === "w" || before === "we") {
    return Case.LOCATIVE;
  }
  const preposition = PREPOSITION_CASES.get(before);
  if (preposition !== undefined) {
    return preposition;
  }
  // An object after a verb: zjadł, wypił, dodaj, kup
  const past = size(before) > 2 && before.endsWith("ł");
  return past || IMPERATIVES.has(before) ? Case.ACCUSATIVE : Case.NOMINATIVE;
}

// a/b in a case: jedna trzecia, dwie trzecie, pięć ósmych, jednej piątej, dwóch trzecich
function fractionWords(a: bigint, b: bigint, c: Case): string {
  const direct = c === Case.NOMINATIVE || c === Case.ACCUSATIVE;
  const few = a % 10n >= 2n && a % 10n <= 4n && (a % 100n < 12n || a % 100n > 14n);
  const denominator = a === 1n || few || !direct ? c : Case.GENITIVE;
  return `${cardinal(a, c, Gender.FEMININE)} ${ordinal(b, denominator, Gender.FEMININE, a !== 1n)}`;
}

function ordinalCue(context: Context): string {
  const w1 = context.before();
  const head = wordsOf(context.head());
  if (LIST_CONJUNCTIONS.has(w1) && head.length > 1) {
    const previous = head[head.length - 2];
    if (previous.at(-1) === "." && madeOf(str(previous, 0, previous.length - 1), DIGITS)) {
      return context.before(3);
    }
  }
  return w1;
}

// The gender of a counted noun, for the number before it: dwie tony, dwa metry
function genderOf(noun: Noun): Gender {
  const nominative = noun.singular[Case.NOMINATIVE];
  return nominative.endsWith("a") ? Gender.FEMININE : nominative === "euro" ? Gender.NEUTER : Gender.MASCULINE;
}

// A decimal number in a case: dwa przecinek pięć, półtora, dwa i pół
function decimal(number: Numeral, c: Case): string {
  // 801.04, 4.0: a dotted code rather than a decimal (but 0.05 is a decimal)
  if (
    number.decimalMark === "." &&
    number.integer !== "0" &&
    (number.fraction.length === 2 || number.fraction === "0")
  ) {
    const f = parseValue(number.fraction) ?? 0n;
    return `${cardinal(value(number))} ${number.fraction[0] === "0" ? digits(number.fraction) : cardinal(f)}`;
  }

  const zero = number.integer !== ""; // .5 has no zero before its point
  if (number.integer === "1" && number.fraction === "5") {
    return "półtora";
  }
  const head = zero ? `${cardinal(value(number), c)} ` : "";
  if (number.fraction === "5" && value(number) > 0n) {
    return `${head}i pół`; // dwa i pół, but zero przecinek pięć
  }
  if (number.fraction.length > 3) {
    return `${head}przecinek ${digits(number.fraction)}`; // dwa przecinek siedem jeden osiem dwa osiem
  }

  const words = ["przecinek"];
  const zeros = number.fraction.search(/[^0]/);
  words.push(...Array<string>(zeros === -1 ? number.fraction.length : zeros).fill("zero"));
  if (zeros !== -1) {
    words.push(cardinal(parseValue(number.fraction) ?? 0n, c));
  }
  return head + join(words);
}

// A number counting a noun, both in a case: pięćset dwadzieścia cztery kilometry
function amount(number: Numeral, noun: Noun, c: Case): string {
  let words: string;
  if (number.fraction !== "") {
    words = decimal(number, c);
  } else {
    const n = parseValue(number.integer);
    words = n === undefined || isSpokenDigits(number, 7) ? digits(number.integer) : cardinal(n, c, genderOf(noun));
  }
  return `${words} ${form(noun, value(number), c, number.fraction !== "")}`;
}

function isZloty(currency: string): boolean {
  return currency === "zł" || currency === "PLN";
}

function cuesOf(match: Match): Cues {
  const context = match.context;
  const cues: Cues = { ...inflection(), plural: false, amountCase: Case.NOMINATIVE, numbered: false };

  switch (match.kind) {
    case Kind.DATE:
      cues.grammaticalCase = COPULAS.has(context.before()) ? Case.NOMINATIVE : Case.GENITIVE;
      return cues;

    case Kind.TIME: {
      // o osiemnastej trzydzieści: the hour is a feminine ordinal (godzina) in the case of the word before, or of the
      // word before the first time of "między 10:00 a 12:00"
      let w1 = context.before();
      const w2 = context.before(2);
      if (CONJUNCTIONS.has(w1) && w2 !== "" && isDigit(w2[w2.length - 1]) && (w2.includes(":") || w2.includes("."))) {
        w1 = context.before(3);
      }
      const hour = HOUR_WORDS.get(w1);
      // prettier-ignore
      cues.grammaticalCase =
        hour !== undefined ? hour
        : w1 === "o" || w1 === "po" ? Case.LOCATIVE
        : w1 === "do" || w1 === "od" || w1 === "około" || w1 === "koło" ? Case.GENITIVE
        : w1 === "przed" || w1 === "między" || w1 === "pomiędzy" ? Case.INSTRUMENTAL
        : w1 === "na" || w1 === "za" ? Case.ACCUSATIVE
        : Case.NOMINATIVE;
      return cues;
    }

    case Kind.ORDINAL: {
      // "29. sesji": an ordinal agreeing with the noun after it, the decade of "lat 70.", or a heading
      cues.form = Form.ORDINAL;
      const w1 = ordinalCue(context);
      const decadeCase = DECADE_WORDS.get(w1);
      if (decadeCase !== undefined) {
        cues.grammaticalCase = decadeCase;
        cues.plural = true;
      } else if (startsWithAny(w1, HEADINGS) && value(match.numbers[0]) < 100n) {
        cues.grammaticalCase = nounForm(w1, context.before(2)).grammaticalCase; // Punkt 3., w punkcie 5.
      } else {
        // 21. najbardziej zakorkowanym: the word after an adverb of degree
        const words = wordsOf(context.tail());
        const i = words.length > 0 && DEGREE_ADVERBS.has(toLower(str(stripped(words[0])))) ? 1 : 0;
        const word = words.length > i ? toLower(str(stripped(words[i]))) : "";
        const next = { ...nounForm(word, w1) };
        // został 11. kierowcą: an instrumental after a form of "zostać" or "być", in the gender of the verb
        const verb = COPULAS.has(w1) || w1.startsWith(BECOME_STEM) ? w1 : context.before(2);
        if (word.endsWith("ą") && (COPULAS.has(verb) || verb.startsWith(BECOME_STEM))) {
          next.grammaticalCase = Case.INSTRUMENTAL;
          // prettier-ignore
          next.gender =
            verb.endsWith("ła") ? Gender.FEMININE
            : verb.endsWith("ło") ? Gender.NEUTER
            : verb.endsWith("ł") ? Gender.MASCULINE
            : next.gender;
        }
        cues.grammaticalCase = next.grammaticalCase;
        cues.gender = next.gender;
        cues.plural = next.plural;
      }
      return cues;
    }

    case Kind.DIMENSION: {
      // numerem 3x38: a document number, where "x" stands for a slash
      const head = context.head();
      let digits = head.length;
      while (digits > 0 && isDigit(head[digits - 1])) {
        digits--;
      }
      let wordEnd = digits;
      while (wordEnd > 0 && isSpace(head[wordEnd - 1])) {
        wordEnd--;
      }
      let wordBegin = wordEnd;
      while (wordBegin > 0 && isWord(head[wordBegin - 1])) {
        wordBegin--;
      }
      const word = str(head, wordBegin, wordEnd);
      cues.numbered =
        digits < head.length && wordEnd < digits && (word.startsWith(NUMBER_STEM) || word === NUMBER_ABBREVIATION);
      return cues;
    }

    default:
      break;
  }

  // A number: its own inflection (the start of a range for amounts), and the case of what it counts
  if (match.numbers.length > 0) {
    const first = match.numbers[0];
    const yearAbbreviation = match.suffix === "r.";
    Object.assign(
      cues,
      first.fraction === ""
        ? inflectionOf(context, value(first), false, yearAbbreviation)
        : inflectionOf(context, 0n, true, yearAbbreviation),
    );

    const last = match.numbers[match.numbers.length - 1];
    const grosze = match.kind === Kind.MONEY && match.scale === "" && last.fraction.length === 2 && isZloty(match.unit);
    cues.amountCase = inflectionOf(context, 0n, last.fraction !== "" && !grosze).grammaticalCase;

    // w 10 m³, w 100 proc.: in an amount, locative; but w 10 min (within ten minutes) stays accusative
    const w1 = context.before();
    const measure = match.kind === Kind.PERCENT || (match.kind === Kind.UNIT && !TIME_UNITS.has(match.unit));
    if ((w1 === "w" || w1 === "we") && measure && cues.amountCase === Case.ACCUSATIVE) {
      cues.amountCase = Case.LOCATIVE;
      // w 10-15 proc.: w dziesięciu do piętnastu procent, the start of the range too
      if (match.numbers.length === 2 && cues.grammaticalCase === Case.ACCUSATIVE) {
        cues.grammaticalCase = Case.LOCATIVE;
      }
    }
  }
  return cues;
}

// 5.VI, 5.VI.2024: a date with its month in Roman numerals
// lat 80.-90., latach 20. - 30.: a range of decades after a word for years. Its numbers are both decades.
class DecadeRangePattern extends Pattern {
  constructor() {
    super(Kind.ORDINAL);
  }

  match(text: Text, position: number): Match | undefined {
    if (
      !isClearBefore(text, position, ".,-") ||
      !DECADE_WORDS.has(toLower(new Context(text, { begin: position, end: position }).before()))
    ) {
      return undefined;
    }
    const first = digitsEnd(text, position);
    if (first !== position + 2 || first >= text.length || text[first] !== ".") {
      return undefined;
    }
    const dash = skipSpace(text, first + 1);
    if (dash >= text.length || (text[dash] !== "-" && text[dash] !== "–")) {
      return undefined;
    }
    const second = skipSpace(text, dash + 1);
    const end = digitsEnd(text, second);
    if (end !== second + 2 || end >= text.length || text[end] !== ".") {
      return undefined;
    }
    const a = digitsAt(text, position);
    const b = digitsAt(text, second);
    if (value(a) % 10n !== 0n || value(b) % 10n !== 0n || value(a) === 0n || value(b) === 0n) {
      return undefined;
    }
    // A dot closing both the decade and the sentence stays with the sentence; a century after it continues the
    // sentence (lat 80.-90. XX wieku)
    const next = skipSpace(text, end + 1);
    let roman = next;
    while (roman < text.length && contains("IVXLCDM", text[roman])) {
      roman++;
    }
    const century = roman > next && isClearAfter(text, roman);
    const spanEnd = isSentenceEnd(text, end + 1, true) && !century ? end : end + 1;
    return matchOf({ kind: Kind.ORDINAL, span: { begin: position, end: spanEnd }, numbers: [a, b], suffix: "." });
  }
}

// 04.02., 23.07-22.08, (2.01), 29.06: a day and a month of two digits (the day of one before a padded month),
// without a year: before a closing dot, as a range of two such dates (its numbers are then both days and months), in
// parentheses, or with a two-digit day and a zero-padded month
class YearlessDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    if (!isClearBefore(text, position, ".,-/")) {
      return undefined;
    }
    const dateAt = (at: number): [Numeral, Numeral] | undefined => {
      const dot = digitsEnd(text, at);
      if (
        dot === at ||
        dot > at + 2 ||
        dot + 3 > text.length ||
        text[dot] !== "." ||
        digitsEnd(text, dot + 1) !== dot + 3
      ) {
        return undefined;
      }
      const day = digitsAt(text, at);
      const month = digitsAt(text, dot + 1);
      const padded = day.integer.length === 2 || month.integer[0] === "0"; // 4.02, but not the version 1.10
      if (value(day) < 1n || value(day) > 31n || value(month) < 1n || value(month) > 12n || !padded) {
        return undefined;
      }
      return [day, month];
    };
    const first = dateAt(position);
    if (!first) {
      return undefined;
    }
    const end = first[1].span.end;
    // 23.07-22.08: a range
    if (end < text.length && text[end] === "-") {
      const second = dateAt(end + 1);
      if (second && isClearAfter(text, second[1].span.end) && !isDigitAfter(text, second[1].span.end, ".")) {
        return matchOf({
          kind: Kind.DATE,
          span: { begin: position, end: second[1].span.end },
          numbers: [first[0], first[1], second[0], second[1]],
          separator: "-",
        });
      }
      return undefined;
    }
    // 04.02.: a closing dot, not followed by a year
    if (end < text.length && text[end] === "." && !isDigitAfter(text, end + 1) && isClearAfter(text, end + 1)) {
      const spanEnd = isSentenceEnd(text, end + 1, true) ? end : end + 1;
      return matchOf({ kind: Kind.DATE, span: { begin: position, end: spanEnd }, numbers: first, separator: "." });
    }
    // (21.11), 29.06
    const parenthesized = position > 0 && text[position - 1] === "(" && end < text.length && text[end] === ")";
    const padded = first[0].integer.length === 2 && first[1].integer[0] === "0";
    if ((parenthesized || padded) && isClearAfter(text, end) && !isDigitAfter(text, end, ".,")) {
      return matchOf({ kind: Kind.DATE, span: { begin: position, end }, numbers: first, separator: "." });
    }
    return undefined;
  }
}

// Jean18 listopada, 17stycznia, 24maja 2018: a day glued to the letters before it or to its month, the month in the
// genitive, and the year if written
class GluedDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const dayEnd = digitsEnd(text, position);
    if (dayEnd === position || dayEnd > position + 2 || (position > 0 && isDigit(text[position - 1]))) {
      return undefined;
    }
    const month = skipSpace(text, dayEnd);
    const glued = (position > 0 && isAlpha(text[position - 1])) || month === dayEnd;
    const index = MONTHS_GENITIVE.findIndex(
      (m: string) => startsWith(text, month, m) && isClearAfter(text, month + m.length),
    );
    const day = digitsAt(text, position);
    if (!glued || index < 0 || value(day) < 1n || value(day) > 31n) {
      return undefined;
    }
    const monthEnd = month + MONTHS_GENITIVE[index].length;
    const date = matchOf({
      kind: Kind.DATE,
      span: { begin: position, end: monthEnd },
      numbers: [day, numeral({ span: { begin: 0, end: 0 }, integer: toDigits(BigInt(index + 1)) })],
      separator: " ",
    });
    const year = skipSpace(text, monthEnd);
    if (year > monthEnd && digitsEnd(text, year) === year + 4 && isClearAfter(text, year + 4)) {
      date.numbers.push(digitsAt(text, year));
      date.span.end = year + 4;
    }
    return date;
  }
}

// 1/2 kg, 3/4 l: a fraction of a unit, read with the unit's genitive (pół kilograma, trzy czwarte litra)
class FractionUnitPattern extends Pattern {
  readonly #units: Keys = keysOf(UNITS);

  constructor() {
    super(Kind.FRACTION);
  }

  match(text: Text, position: number): Match | undefined {
    const slash = digitsEnd(text, position);
    if (!isClearBefore(text, position, ".,/") || slash === position || slash >= text.length || text[slash] !== "/") {
      return undefined;
    }
    const end = digitsEnd(text, slash + 1);
    if (end === slash + 1) {
      return undefined;
    }
    const unit = skipSpace(text, end);
    for (const key of keysAt(text, unit, this.#units)) {
      if (isClearAfter(text, unit + size(key))) {
        return matchOf({
          kind: Kind.FRACTION,
          span: { begin: position, end: unit + size(key) },
          numbers: [digitsAt(text, position), digitsAt(text, slash + 1)],
          unit: key,
          separator: "/",
        });
      }
    }
    return undefined;
  }
}

class RomanDatePattern extends Pattern {
  constructor() {
    super(Kind.DATE);
  }

  match(text: Text, position: number): Match | undefined {
    const dayEnd = digitsEnd(text, position);
    if (
      !isClearBefore(text, position, ".,") ||
      dayEnd === position ||
      dayEnd > position + 2 ||
      dayEnd >= text.length ||
      (text[dayEnd] !== "." && text[dayEnd] !== " ")
    ) {
      return undefined;
    }
    const month = dayEnd + 1; // 5.VI, or 15 X
    let monthEnd = month;
    while (monthEnd < text.length && contains("IVX", text[monthEnd])) {
      monthEnd++;
    }
    const monthValue = romanValue(str(text, month, monthEnd));
    const day = value(digitsAt(text, position));
    if (monthValue === undefined || monthValue > 12n || day < 1n || day > 31n) {
      return undefined;
    }
    // 15 X: after a space, not V or I alone (12 V is volts), nor an edition (15 X wyd.)
    const roman = str(text, month, monthEnd);
    if (text[dayEnd] === " " && (roman === "V" || roman === "I" || str(text, monthEnd, monthEnd + 4) === " wyd")) {
      return undefined;
    }

    const date = matchOf({
      kind: Kind.DATE,
      numbers: [
        digitsAt(text, position),
        numeral({ span: { begin: month, end: monthEnd }, integer: toDigits(monthValue) }),
      ],
      separator: ".",
    });
    // With a year first (5.VI.2024), then without it (5.VI)
    if (
      monthEnd + 5 <= text.length &&
      text[monthEnd] === "." &&
      digitsEnd(text, monthEnd + 1) === monthEnd + 5 &&
      isClearAfter(text, monthEnd + 5)
    ) {
      date.numbers.push(digitsAt(text, monthEnd + 1));
      date.span = { begin: position, end: monthEnd + 5 };
      return date;
    }
    if (!isClearAfter(text, monthEnd) || isDigitAfter(text, monthEnd, ".")) {
      return undefined;
    }
    date.span = { begin: position, end: monthEnd };
    return date;
  }
}

// Every form of the regnal names, with its case and gender
const NAME_FORMS: ReadonlyArray<readonly [string, Case, Gender]> = REGNAL_NAMES.flatMap(([line, gender]) =>
  line.split(" ").map((form, c) => [form, c as Case, gender] as const),
);
const NAME_CUES: RomanCues = {
  names: new Set(NAME_FORMS.map(([form]) => form)),
  before: new Set(),
  after: new Set(),
};

// --------------------
// ----- Language -----
// --------------------

/**
 * Polish numbers inflect for case and gender, so most readings first find the case a number takes:
 * from the preposition before it (od pięciu), the noun after it (z trzema kolegami), a governing noun or verb
 * (od kwoty trzystu tysięcy), or the year and date patterns which make it an ordinal (w 1985 roku).
 */
const MATH_WORDS: ReadonlySet<string> = new Set(["oblicz", "policz", "ile", "pomnóż", "wynik"]);

class Polish extends PatternLanguage {
  constructor() {
    super({ characters: CHARACTERS, terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new YearlessDatePattern());
    this.addPattern(new GluedDatePattern());
    this.addPattern(new RomanDatePattern());
    this.addPattern(new FractionUnitPattern());
    this.addPattern(new PrimePattern(NOTATION, primeFormat({ cues: PRIME_CUES, heights: HEIGHT_CUES })));
    for (const format of PHONE_FORMATS) {
      this.addPattern(new PhonePattern(format));
    }
    this.addPattern(new DecadeRangePattern());
    this.addPattern(new DotOrdinalPattern(true, true));
    this.addPattern(new SuffixPattern(Kind.NUMBER, YEAR_SUFFIXES, YEAR_FORMAT));
    this.addPattern(new RomanPattern(false, CENTURY_ABBREVIATIONS));
  }

  protected override readNumber(number: Numeral, match: Match): string {
    const cues = cuesOf(match);
    if (number.fraction !== "") {
      return decimal(number, cues.grammaticalCase);
    }

    const n = parseValue(number.integer);
    if (n === undefined || isSpokenDigits(number, 7)) {
      return digits(number.integer);
    }
    if (cues.form === Form.ORDINAL) {
      return ordinal(n, cues.grammaticalCase, cues.gender, cues.plural);
    }
    return cardinal(n, cues.grammaticalCase, cues.gender);
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 6);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    // 23.07-22.08: dwudziestego trzeciego lipca do dwudziestego drugiego sierpnia
    if (match.numbers.length === 4) {
      return (
        `${ordinal(value(match.numbers[0]), cuesOf(match).grammaticalCase)} ` +
        `${MONTHS_GENITIVE[Number(value(match.numbers[1])) - 1]} do ` +
        `${ordinal(value(match.numbers[2]), Case.GENITIVE)} ${MONTHS_GENITIVE[Number(value(match.numbers[3])) - 1]}`
      );
    }
    const words = [
      ordinal(value(match.numbers[0]), cuesOf(match).grammaticalCase),
      MONTHS_GENITIVE[Number(value(match.numbers[1])) - 1],
    ];
    if (match.numbers.length === 3) {
      // 5.VI has no year
      words.push(ordinal(value(match.numbers[2]), Case.GENITIVE));
    }
    if (match.suffix !== "") {
      words.push("roku");
    }
    return join(words);
  }

  protected override readPhone(match: Match): string | undefined {
    // 600 123 456 is a count, unless a word says it is a phone number: numer, tel., zadzwoń pod
    const w1 = match.context.before();
    const called = (w1 === "pod" || w1 === "na") && CALL_VERBS.has(match.context.before(2));
    if (match.kind === Kind.PHONE && match.separator === " " && !PHONE_WORDS.has(w1) && !called) {
      return undefined;
    }
    // 31-123 Kraków: a postal code only before a capitalized town or after "kod"; else a range (20-300 zł)
    const town = match.context.after();
    const postal =
      (town !== "" && isUpper(String.fromCodePoint(town.codePointAt(0)!))) || w1 === "kod" || w1 === "pocztowy";
    if (match.numbers.length === 2 && match.numbers[0].integer.length === 2 && !postal) {
      return undefined;
    }

    // In groups: sześćset sto dwadzieścia trzy, a leading zero apart (020: zero dwadzieścia, 00: zero zero); a
    // longer one digit by digit
    const words: string[] = [];
    for (const group of match.numbers) {
      const g = group.integer;
      if (g.length > 3) {
        words.push(digits(g));
        continue;
      }
      const first = [...g].findIndex((c) => c !== "0");
      const zeros = first === -1 ? g.length : first;
      for (let i = 0; i < zeros; i++) {
        words.push("zero");
      }
      if (zeros < g.length) {
        words.push(cardinal(parseValue(g.slice(zeros)) ?? 0n));
      }
    }
    return join(words);
  }

  protected override readTime(match: Match): string | undefined {
    const minutes = match.numbers[1].integer;
    const round = minutes === "00" || minutes === "15" || minutes === "30" || minutes === "45";
    const decimal = match.separator === "." && !round && !TIME_WORDS.has(match.context.before());
    // prowadzenie 20:16: a score, read by its own pattern; but o 20:16, mecz (poniedziałek, 18:00)
    const score =
      match.separator === ":" &&
      match.suffix === "" &&
      minutes !== "00" &&
      !TIME_WORDS.has(match.context.before()) &&
      mentions(match.context, SPORT_WORDS);
    if (!fitsClock(match) || decimal || score) {
      return undefined;
    }

    const hours = ordinal(value(match.numbers[0]), cuesOf(match).grammaticalCase, Gender.FEMININE);
    if (value(match.numbers[1]) === 0n) {
      return hours;
    }
    return `${hours} ${minutes[0] === "0" ? digits(minutes) : cardinal(value(match.numbers[1]))}`;
  }

  protected override readRoman(match: Match): string | undefined {
    const n = value(match.numbers[0]);
    let previous = match.context.before();
    // w XX czy XXI wieku: the second of two numerals takes the case of the word before the first
    const first = match.context.before(2); // lowercased: xix
    if (CONJUNCTIONS.has(previous) && first !== "" && madeOf(first, "ivxlcdm")) {
      previous = match.context.before(3);
    }

    // XIX w.: dziewiętnastego wieku, w dziewiętnastym wieku; opening its sentence or clause, osiemnasty wiek
    let next = toLower(match.context.after());
    const centuryAfter =
      CONJUNCTIONS.has(next) &&
      romanValue(match.context.after(2)) !== undefined &&
      str(match.context.tail()).includes(" w.") &&
      toLower(match.context.after(3)) === "w";
    if (match.suffix === "w." || centuryAfter) {
      if (previous === "") {
        return ordinal(n, Case.NOMINATIVE) + (centuryAfter ? "" : " wiek");
      }
      const c = LOCATIVE_PREPOSITIONS.has(previous) ? Case.LOCATIVE : Case.GENITIVE;
      return ordinal(n, c) + (centuryAfter ? "" : " wieku"); // XIX i XX w.: the noun once, after the second
    }

    // Jan Paweł drugi, Jana Pawła drugiego, Elżbiety drugiej: the case of the name
    if (romanRole(match, NAME_CUES) !== undefined) {
      for (const [form, c, gender] of NAME_FORMS) {
        if (form === previous) {
          return ordinal(n, c, gender);
        }
      }
    }

    // XXI wieku, II wojna, w II połowie; rozdział czwarty, w rozdziale czwartym: agreeing with its noun, after it first
    const genderOf = (word: string): Gender | undefined => {
      for (const [form, gender] of ROMAN_NOUNS) {
        const prefix = form.endsWith("-");
        if (prefix ? word.startsWith(form.slice(0, -1)) : word === form) {
          return gender;
        }
      }
      return undefined;
    };
    // A locative in -e after its preposition (w rozdziale, na piętrze), and the -ej of an adjective noun (Rzeczypospolitej)
    const formOf = (noun: string, preposition: string): NounForm => {
      const form = nounForm(noun, preposition);
      const locative = LOCATIVE_PREPOSITIONS.has(preposition);
      if (noun.endsWith("ej")) {
        // II Rzeczypospolitej
        return { ...form, grammaticalCase: locative ? Case.LOCATIVE : Case.GENITIVE };
      }
      if (form.grammaticalCase === Case.NOMINATIVE && noun.endsWith("e") && locative) {
        return { ...form, grammaticalCase: Case.LOCATIVE };
      }
      return form;
    };
    // XIX i XX wieku: the first of two numerals shares the noun of the second
    if (CONJUNCTIONS.has(next) && romanValue(match.context.after(2)) !== undefined) {
      next = toLower(match.context.after(3));
    }
    const after = genderOf(next);
    if (after !== undefined) {
      const noun = formOf(next, previous);
      return ordinal(n, noun.grammaticalCase, after, noun.plural);
    }
    const head = match.context.head();
    let last = head.length;
    while (last > 0 && isSpace(head[last - 1])) {
      last--;
    }
    const before = genderOf(previous);
    if (before !== undefined && last > 0 && isAlpha(head[last - 1])) {
      const noun = formOf(previous, match.context.before(2));
      return ordinal(n, noun.grammaticalCase, before, noun.plural);
    }
    return undefined;
  }

  protected override readOrdinal(match: Match): string | undefined {
    // An ordinal agrees with the noun after it: "29. sesji", but "Punkt 3. Dalej" ends a sentence
    const w1 = ordinalCue(match.context);
    const words = wordsOf(match.context.tail());
    const next = words.length === 0 ? [] : stripped(words[0]);
    const heading = startsWithAny(w1, HEADINGS) && value(match.numbers[0]) < 100n;
    // lat 70.: a decade; but "do lat 3." counts years (do lat trzech)
    const n = value(match.numbers[0]);
    const decade = DECADE_WORDS.has(w1) && n % 10n === 0n && n >= 10n && n < 100n;
    if (DECADE_WORDS.has(w1) && !decade) {
      return undefined;
    }
    if (!decade && !heading && (next.length === 0 || !isLower(next[0]))) {
      return undefined;
    }

    const cues = cuesOf(match);
    let reading = ordinal(n, cues.grammaticalCase, cues.gender, cues.plural);
    if (match.numbers.length === 2) {
      reading += `-${ordinal(value(match.numbers[1]), cues.grammaticalCase, cues.gender, cues.plural)}`; // 80.-90.
    }
    return reading;
  }

  protected override readCompound(match: Match): string | undefined {
    const number = match.numbers[0];
    if (number.fraction !== "") {
      return `${this.readNumber(number, match)}-${match.suffix}`;
    }

    // lat 70-tych: siedemdziesiątych - the first gender and case taking the ending of the suffix
    const ending = ORDINAL_SUFFIXES.get(match.suffix);
    if (ending !== undefined) {
      for (let g = 0; g < ADJECTIVE_ENDINGS.length; ++g) {
        for (let c = 0; c < ADJECTIVE_ENDINGS[g].length; ++c) {
          if (ADJECTIVE_ENDINGS[g][c] === ending) {
            const gender = g === 3 ? Gender.MASCULINE : (g as Gender);
            return ordinal(value(number), c as Case, gender, g === 3);
          }
        }
      }
    }

    // 45-metrowe: czterdziestopięciometrowe
    return combining(value(number)) + match.suffix;
  }

  protected override readPlain(match: Match): string | undefined {
    if (match.suffix !== "r.") {
      return this.readNumber(match.numbers[0], match);
    }
    // przed 1999 r.: przed tysiąc dziewięćset dziewięćdziesiątym dziewiątym rokiem
    const instrumental = cuesOf(match).grammaticalCase === Case.INSTRUMENTAL;
    return this.readNumber(match.numbers[0], match) + (instrumental ? " rokiem" : " roku");
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    const currency = CURRENCIES[match.unit];
    const c = this.#amountCase(match);
    if (match.scale !== "") {
      return `${amount(number, SCALES[match.scale], c)} ${currency.plural[Case.GENITIVE]}`;
    }

    // dwanaście złotych dziewięćdziesiąt dziewięć groszy; 5,00 zł pięć złotych; 0,99 zł dziewięćdziesiąt dziewięć
    // groszy; 3,50 € trzy euro pięćdziesiąt centów
    // A price per unit keeps its unit for the end: 5,96 zł/m² pięć złotych dziewięćdziesiąt sześć groszy za metr
    const slash = match.unit.indexOf("/");
    const unit = slash === -1 ? match.unit : match.unit.slice(0, slash);
    const per = currency.singular[0].slice(CURRENCIES[unit].singular[0].length);
    const hundredth = isZloty(unit) ? GROSZ : CENTS.has(unit) ? CENT : undefined;
    if (number.fraction.length === 2 && hundredth !== undefined) {
      const cents = parseValue(number.fraction) ?? 0n;
      const whole = numeral({ span: { begin: 0, end: 0 }, integer: number.integer, grouped: number.grouped });
      const small = `${cardinal(cents, c)} ${form(hundredth, cents, c)}`;
      if (value(whole) === 0n && cents !== 0n) {
        return small + per;
      }
      return amount(whole, CURRENCIES[unit], c) + (cents !== 0n ? ` ${small}` : "") + per;
    }
    return amount(number, currency, c);
  }

  protected override readScale(number: Numeral, match: Match): string | undefined {
    return amount(number, SCALES[match.scale], this.#amountCase(match));
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return amount(number, PERCENT, this.#amountCase(match)) + spacedSuffix(match);
  }

  protected override readUnit(number: Numeral, match: Match): string | undefined {
    const unit = match.unit === "″" ? INCH : UNITS[match.unit];
    return amount(number, unit, this.#amountCase(match));
  }

  protected override readFraction(match: Match): string | undefined {
    // Only where it divides a noun after it (2/3 głosów), after "ułamek", or after a whole number: 10/12 may be a
    // house number, and 7/10 a score
    const [a, b] = fractionOf(match);
    // 1/2 kg pół kilograma, 3/4 l trzy czwarte litra: a unit's genitive after the fraction
    if (match.unit !== "") {
      if (b < 2n || a === 0n || a >= b) {
        return undefined;
      }
      const c = fractionCase(match.context);
      const direct = c === Case.NOMINATIVE || c === Case.ACCUSATIVE;
      const unit = UNITS[match.unit].singular[Case.GENITIVE];
      return `${a === 1n && b === 2n && direct ? "pół" : fractionWords(a, b, c)} ${unit}`;
    }
    // ulica Marszałkowska 10/12: a house number, dziesięć łamane przez dwanaście
    // (ulica Nowy Świat 15/17: a name of up to three words)
    const street = [2, 3, 4].some((k) => STREET_WORDS.has(match.context.before(k)));
    if (match.numbers.length === 2 && street) {
      return `${cardinal(a)} łamane przez ${cardinal(b)}`;
    }
    const next = toLower(match.context.after());
    const month = (MONTHS_GENITIVE as readonly string[]).includes(next); // 3/4 maja: two days
    const noun =
      next !== "" && isAlpha(String.fromCodePoint(next.codePointAt(0)!)) && !FRACTION_STOPS.has(next) && !month;
    const compared = match.context.before() === "niż" && !FRACTION_STOPS.has(next); // większy niż 1/2
    const marked = match.numbers.length === 3 || match.context.before().startsWith("ułam") || compared || noun;
    const head = match.context.head();
    const decimal = head.length >= 2 && head[head.length - 1] === "," && isDigit(head[head.length - 2]); // 10,8/12 V
    const common = b <= 10n || b === 12n || b === 16n || b === 20n || b === 100n || b === 1000n;
    if (a === 0n || a >= b || !common || !marked || decimal) {
      return undefined;
    }

    // pół szklanki; jedna trzecia, dwie trzecie, pięć ósmych; około jednej piątej, w jednej czwartej finału
    const c = fractionCase(match.context);
    const direct = c === Case.NOMINATIVE || c === Case.ACCUSATIVE;
    if (a === 1n && b === 2n && noun && direct) {
      return "pół";
    }
    return fractionWords(a, b, c);
  }

  protected override readMixedFraction(match: Match): string | undefined {
    if (match.numbers.length === 2) {
      return this.readFraction(match);
    }
    // 1 1/2: półtorej godziny, półtora roku (a feminine noun's genitive ends in -y or -i); 2 1/2 roku: dwa i pół
    // roku; 3 3/4 mili: trzy i trzy czwarte mili
    const [a, b] = fractionOf(match);
    const whole = value(match.numbers[0]);
    if (a >= b) {
      return undefined;
    }
    if (whole === 1n && a === 1n && b === 2n) {
      const next = match.context.after();
      return next.endsWith("y") || next.endsWith("i") ? "półtorej" : "półtora";
    }
    const c = fractionCase(match.context);
    return `${cardinal(whole, c)} i ${a === 1n && b === 2n ? "pół" : fractionWords(a, b, c)}`;
  }

  protected override readJoiner(match: Match): string | undefined {
    if (match.kind === Kind.DIMENSION) {
      // 5 × 3 = 15, oblicz 5 × 3: a product; 60 × 35 cm, wymiary 20 × 30: a dimension
      const product = match.context.after(2) === "=" || MATH_WORDS.has(match.context.before(2));
      if (product && match.text.includes("×")) {
        return " razy ";
      }
      return cuesOf(match).numbered ? " przez " : " na ";
    }
    // 1990 - 1994: a spaced dash between numbers is a range, unless an equation follows (5 - 3 = 2)
    if (match.kind === Kind.DIFFERENCE && match.text === " - " && match.context.after(2) !== "=") {
      return " do ";
    }
    return super.readJoiner(match);
  }

  // 15-20%, 20-300 zł, 1990-1994: piętnaście do dwudziestu procent ("do" gives the end its case). But a range
  // of hours (w godzinach 12-16: dwunasta-szesnasta), or before a noun written to agree with its end (2-3 minuty:
  // dwie-trzy minuty), keeps its hyphen.
  protected override readRange(from: Numeral, to: Numeral, match: Match): string {
    // A range of hours, or of clock times (9:00-17:00): dziewiąta-siedemnasta
    const head = match.context.head();
    let start = head.length;
    while (start > 0 && isDigit(head[start - 1])) {
      start--;
    }
    if (HOUR_WORDS.has(match.context.before(2)) || (start > 0 && head[start - 1] === ":")) {
      return "-";
    }
    const year = (n: Numeral): boolean => n.integer.length === 4 && isYearValue(value(n));
    const after = wordsOf(match.context.tail());
    const noun =
      after.length >= 2 && madeOf(str(stripped(after[0])), "0123456789,.") && str(after[0]) === str(stripped(after[0]))
        ? str(stripped(after[1]))
        : "";
    const counted = noun !== "" && isLower([...noun][0]) && !(noun in UNITS) && !(noun in CURRENCIES);
    return counted && !(year(from) && year(to)) ? "-" : " do ";
  }

  // The case of an amount, or of the end of its range after "do": 4-5 proc. cztery do pięciu procent
  // 8'22": osiem minut dwadzieścia dwie sekundy; 5'5": pięć stóp pięć cali
  protected override readPrime(number: Numeral, unit: Prime, _match: Match): string {
    const whole = numeral({ span: { begin: 0, end: 0 }, integer: toDigits(value(number)) });
    return amount(whole, PRIME_UNITS[unit], Case.NOMINATIVE);
  }
  protected override readPrimeFraction(digits: string, _match: Match): string | undefined {
    return cardinal(parseValue(digits) ?? 0n);
  }

  #amountCase(match: Match): Case {
    if (match.numbers.length === 2 && this.readRange(match.numbers[0], match.numbers[1], match) === " do ") {
      return Case.GENITIVE;
    }
    return cuesOf(match).amountCase;
  }
}

let instance: Polish | undefined;

export function language(): LanguageHandle {
  return (instance ??= new Polish());
}
