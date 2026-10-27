import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { beforeName, beforeNumber, type Readings } from "../../text/readings.ts";
import type { Forms, Joiners } from "../grammar.ts";

export const ONES = [
  "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
  "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
] as const;

export const TENS = ["", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"] as const;

// Scale words, largest first
export const SCALE_WORDS: ReadonlyArray<readonly [bigint, string]> = [
  [1000000000000n, "trillion"], [1000000000n, "billion"], [1000000n, "million"], [1000n, "thousand"],
];

// Ordinals which are not formed with "th": cardinal word -> ordinal word
export const IRREGULAR_ORDINALS: Readonly<Record<string, string>> = {
  one: "first", two: "second", three: "third", five: "fifth", eight: "eighth", nine: "ninth", twelve: "twelfth",
};

export const MONTHS = [
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December",
] as const;

export const CURRENCIES: Readonly<Record<string, Forms>> = {
  "$": { one: "dollar", many: "dollars" }, "€": { one: "euro", many: "euros" },
  "£": { one: "pound", many: "pounds" },   "¥": { one: "yen", many: "yen" },
};

export const SCALES: Readonly<Record<string, string>> = {
  thousand: "thousand", million: "million", billion: "billion", trillion: "trillion",
  k: "thousand",        K: "thousand",      M: "million",       bn: "billion",
};

export const UNITS: Readonly<Record<string, Forms>> = {
  "km":   { one: "kilometer", many: "kilometers" },           "m":    { one: "meter", many: "meters" },
  "cm":   { one: "centimeter", many: "centimeters" },         "mm":   { one: "millimeter", many: "millimeters" },
  "nm":   { one: "nanometer", many: "nanometers" },           "m²":   { one: "square meter", many: "square meters" },
  "km²":  { one: "square kilometer", many: "square kilometers" }, "m³": { one: "cubic meter", many: "cubic meters" },
  "kg":   { one: "kilogram", many: "kilograms" },             "g":    { one: "gram", many: "grams" },
  "mg":   { one: "milligram", many: "milligrams" },           "lb":   { one: "pound", many: "pounds" },
  "lbs":  { one: "pound", many: "pounds" },                   "oz":   { one: "ounce", many: "ounces" },
  "ft":   { one: "foot", many: "feet" },                      "l":    { one: "liter", many: "liters" },
  "L":    { one: "liter", many: "liters" },                   "ml":   { one: "milliliter", many: "milliliters" },
  "mL":   { one: "milliliter", many: "milliliters" },         "TB":   { one: "terabyte", many: "terabytes" },
  "GB":   { one: "gigabyte", many: "gigabytes" },             "MB":   { one: "megabyte", many: "megabytes" },
  "KB":   { one: "kilobyte", many: "kilobytes" },             "kB":   { one: "kilobyte", many: "kilobytes" },
  "Hz":   { one: "hertz", many: "hertz" },                    "kHz":  { one: "kilohertz", many: "kilohertz" },
  "MHz":  { one: "megahertz", many: "megahertz" },            "GHz":  { one: "gigahertz", many: "gigahertz" },
  "W":    { one: "watt", many: "watts" },                     "kW":   { one: "kilowatt", many: "kilowatts" },
  "kWh":  { one: "kilowatt-hour", many: "kilowatt-hours" },   "V":    { one: "volt", many: "volts" },
  "mph":  { one: "mile per hour", many: "miles per hour" },   "km/h": { one: "kilometer per hour", many: "kilometers per hour" },
  "°C":   { one: "degree Celsius", many: "degrees Celsius" }, "°F":   { one: "degree Fahrenheit", many: "degrees Fahrenheit" },
  "°":    { one: "degree", many: "degrees" },
};

export const FRACTIONS: Fractions = {
  "1/2": "one half", "1/3": "one third", "2/3": "two thirds", "1/4": "one quarter", "3/4": "three quarters",
};

export const TIME_SUFFIXES = ["a.m.", "p.m.", "am.", "pm.", "am", "pm", "AM", "PM"];

// Months, also abbreviated (lowercase)
export const MONTH_WORDS: ReadonlySet<string> = new Set([
  "january", "february", "march", "april", "may", "june", "july", "august", "september", "october",
  "november", "december", "jan", "feb", "mar", "apr", "jun", "jul", "aug", "sep", "sept", "oct",
  "nov", "dec",
]);

// Words before a 4-digit number which make it a year: in 1999, since 1999 (and months)
export const YEAR_CUES: ReadonlySet<string> = new Set([
  "in", "of", "since", "from", "by", "until", "till", "circa", "year", "the", "during", "before",
  "after", "early", "late", "mid",
]);

export const IRREGULAR_PLURALS: ReadonlySet<string> = new Set([
  "people", "children", "men", "women", "feet", "persons",
]);

// Words before a time with a dot, which make it a time: at 10.30
export const TIME_WORDS: ReadonlySet<string> = new Set([
  "at", "from", "until", "till", "to", "by", "around", "about", "before", "after", "since", "between",
  "and", "is", "it's", "or",
]);

// Words after an amount that do not make it an adjective ("$5 a month" vs "a $5 bill")
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "a", "an", "the", "and", "or", "but", "nor", "in", "on", "at", "to", "for", "of", "per", "with",
  "from", "by", "into", "than", "more", "less", "each", "every", "is", "are", "was", "were", "be",
  "been", "will", "would", "can", "could", "should", "may", "might", "must", "plus", "minus", "times",
  "anyway", "now", "then", "so", "that", "this", "these", "those", "it", "if", "when", "which", "who",
  "what", "where", "why", "how", "you", "i", "we", "they", "he", "she", "as", "up", "down", "off",
  "out", "over", "under", "about", "around", "after", "before", "back", "away", "ago", "versus", "vs",
  "too", "also", "only", "just", "even", "still", "yet", "already", "not", "no", "there", "here",
  "all", "both", "while", "because", "since", "until", "though", "although", "unless", "has", "have",
  "had", "do", "does", "did", "get", "got", "instead", "worth", "today", "tomorrow", "tonight",
  "outside", "inside", "apart", "ahead", "behind", "above", "below", "long", "wide", "high", "tall",
  "deep", "old", "later", "earlier", "overall",
]);

// Henry VIII (the Eighth), Chapter IV (four), the XXI century (twenty-first)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "henry", "edward", "george", "william", "richard", "charles", "james", "elizabeth", "mary", "victoria", "anne",
    "john", "paul", "benedict", "leo", "pius", "gregory", "clement", "innocent", "louis", "philip", "frederick",
    "alexander", "nicholas", "peter", "ivan", "catherine", "napoleon", "ferdinand", "alfonso", "carlos", "felipe",
    "wilhelm", "ludwig", "constantine", "ramesses",
  ]),
  before: new Set([
    "chapter", "part", "volume", "book", "act", "scene", "section", "article", "appendix", "phase", "stage", "level",
    "war", "season", "episode", "bowl", "vatican", "type", "class", "grade", "category", "tier", "mark",
  ]),
  after: new Set([
    "century", "centuries", "millennium",
  ]),
};

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ",", decimalSeparators: ".", timeSeparators: ":.", timeSuffixes: TIME_SUFFIXES,
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "point", dot: "dot", range: " to ", by: "by", score: " to ", minus: "minus", plus: "plus",
};

// 27.03.2026, 27-03-2026: the year has four digits (3.8.10 is a version)
export const DATE_FORMAT = dateFormat({ separators: ".-", boundaries: ".,/-", shortYear: false });

// 1-555-123-4567, (555) 123-4567, 555.123.4567, 555 123 4567, 555-1234: the longest first, each its own pass
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], prefix: "1-" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], parenthesized: true }),
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], separators: "." }),
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], separators: " " }),
  phoneFormat({ groups: [[3, 3], [4, 4]] }),
];

// 21st, 2nd, 3rd, 4th
export const ORDINAL_SUFFIXES = ["st", "nd", "rd", "th"];

// 1990s, '90s, 90's
export const DECADE_SUFFIXES = ["s"];
export const DECADE_FORMAT = suffixFormat({ apostrophes: true, boundaries: ".,-" });

export const SYMBOLS: Readings = {
  "&": ["and"],      "+": ["plus"],    "=": ["equals"],  "@": ["at"],      "%": ["percent"],
  "°": ["degrees"],  "#": ["number"],  "²": ["squared"], "³": ["cubed"],   "π": ["pi"],
  "×": ["times"],    "±": ["plus or minus"], "½": ["one half"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: No. 5
    "No.": [{ words: "number", when: beforeNumber }],
    "no.": [{ words: "number", when: beforeNumber }],
    "Nos.": [{ words: "numbers", when: beforeNumber }],
    "vol.": [{ words: "volume", when: beforeNumber }],
    "Vol.": [{ words: "volume", when: beforeNumber }],
    "p.": [{ words: "page", when: beforeNumber }],
    "pp.": [{ words: "pages", when: beforeNumber }],
    "ch.": [{ words: "chapter", when: beforeNumber }],
    "Ch.": [{ words: "chapter", when: beforeNumber }],
    "fig.": [{ words: "figure", when: beforeNumber }],
    "Fig.": [{ words: "figure", when: beforeNumber }],
    // Anywhere
    "e.g.": ["for example"],
    "i.e.": ["that is"],
    "etc.": ["et cetera"],
    "vs.": ["versus"],
    "Mr.": ["Mister"],
    "Mrs.": ["Missus"],
    "Dr.": [{ words: "Doctor", when: beforeName }],
    "St.": [{ words: "Saint", when: beforeName }, "Street"],
  },
  web: { dot: "dot", at: "at", slash: "slash" },
};
