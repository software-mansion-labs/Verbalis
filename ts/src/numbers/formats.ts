import type { Keys } from "../text/scan.ts";

// How a numeric date is written in a language: 12.05.2024, 12/05/24, 1. 5.
export interface DateFormat {
  separators: string; // Between its parts: the same one twice in a date
  boundaries: string; // Besides letters, what may not precede it (or a digit after it)
  yearRequired: boolean;
  shortYear: boolean; // Two-digit years: 12/05/24
  closingSeparator: boolean; // A separator also closes the month: 1.5. and 1.5.2024
  spaced: boolean; // An optional space after the day: 12. 5.
  suffixes: Keys; // Abbreviations after the year: 2024 r.
}

export function dateFormat(fields: Partial<DateFormat> = {}): DateFormat {
  return {
    separators: "/",
    boundaries: ".,/",
    yearRequired: true,
    shortYear: true,
    closingSeparator: false,
    spaced: false,
    suffixes: [],
    ...fields,
  };
}

// How a phone number is written: its groups of digits and what separates them.
export interface PhoneFormat {
  groups: ReadonlyArray<readonly [number, number]>; // Length range of each group of digits
  separators: string;
  optionalSeparators: boolean; // 0612345678 as well as 06 12 34 56 78
  prefix: string; // An optional literal start: "1-"
  parenthesized: boolean; // The first group in parentheses, then an optional space: (555) 123-4567
  leadingZero: boolean;
  boundaries: string; // Besides letters, what may not stand right before it
  closing: string; // Besides letters, what may not stand right after it
}

export function phoneFormat(fields: Partial<PhoneFormat> & Pick<PhoneFormat, "groups">): PhoneFormat {
  return {
    separators: "-",
    optionalSeparators: false,
    prefix: "",
    parenthesized: false,
    leadingZero: false,
    boundaries: "-",
    closing: "-",
    ...fields,
  };
}

// How a suffix marks a number: 21st, 1 º, 90's, 2020 r.
export interface SuffixFormat {
  space: boolean; // An optional space before the suffix
  wordEnd: boolean; // No word character may follow the suffix
  apostrophes: boolean; // '90s, 90's: optional apostrophes before the number and the suffix
  boundaries: string; // Besides letters, what may not stand right before the number
  sentenceDot: boolean; // A dot closing the suffix is left to a sentence it also closes
}

export function suffixFormat(fields: Partial<SuffixFormat> = {}): SuffixFormat {
  return { space: false, wordEnd: true, apostrophes: false, boundaries: ".,", sentenceDot: false, ...fields };
}

// Fractions with their own words, keyed "numerator/denominator": "1/2" -> half
export type Fractions = Readonly<Record<string, string>>;

// The words which mark a Roman numeral as a number in a language (lowercase, as Context reads them).
export interface RomanCues {
  readonly names: ReadonlySet<string>; // A name before it takes a regnal number: Henry VIII, Jan Paweł II
  readonly before: ReadonlySet<string>; // A noun before it, which it numbers: chapter IV, siglo XXI
  readonly after: ReadonlySet<string>; // A noun after it, which it numbers: XXI wieku, II Guerra Mundial
}

// How numbers are written in a language, read by the generic patterns.
export interface Notation {
  groupSeparators: string; // 1.000, 1 000, 1,000
  decimalSeparators: string; // 1,5, 1.5
  timeSeparators: string; // 18:30, 18.30, 18h30
  timeSuffixes: Keys; // pm, Uhr, h
  currencies: Keys; // $, zł, EUR
  scales: Keys; // million, mln, k
  units: Keys; // km, kg, °C
  fractions: boolean; // Whether 3/4 reads as a fraction
}

export function notation(fields: Partial<Notation> = {}): Notation {
  return {
    groupSeparators: "",
    decimalSeparators: "",
    timeSeparators: "",
    timeSuffixes: [],
    currencies: [],
    scales: [],
    units: [],
    fractions: false,
    ...fields,
  };
}

// The keys of a table, for a Notation built out of a language's constants.
export function keysOf(table: object): Keys {
  return Object.keys(table);
}
