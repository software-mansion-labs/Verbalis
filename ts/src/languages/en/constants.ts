import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Keys } from "../../text/scan.ts";
import { wordsOf, type Context } from "../../text/context.ts";
import { beforeName, beforeNumber, type Readings } from "../../text/readings.ts";
import { contains, str } from "../../text/strings.ts";
import { isDigit, isUpper, isWord } from "../../text/unicode.ts";
import type { Forms, Joiners } from "../grammar.ts";

export const ONES = [
  "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
  "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
] as const;

export const TENS = ["", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"] as const;

// Scale words, largest first
export const SCALE_WORDS: ReadonlyArray<readonly [bigint, string]> = [
  [1000000000000000000n, "quintillion"], [1000000000000000n, "quadrillion"], [1000000000000n, "trillion"],
  [1000000000n, "billion"], [1000000n, "million"], [1000n, "thousand"],
];

// The scales of groups of three digits, from the thousands: 124,444,234,854,823,834,553
export const GROUP_SCALES = [
  "thousand", "million", "billion", "trillion", "quadrillion", "quintillion", "sextillion", "septillion",
] as const;

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
  "₩": { one: "won", many: "won" },        "US$": { one: "US dollar", many: "US dollars" },
  "$US": { one: "US dollar", many: "US dollars" },
  "C$": { one: "Canadian dollar", many: "Canadian dollars" }, "A$": { one: "Australian dollar", many: "Australian dollars" },
  "AU$": { one: "Australian dollar", many: "Australian dollars" },
  "NZ$": { one: "New Zealand dollar", many: "New Zealand dollars" },
  "S$": { one: "Singapore dollar", many: "Singapore dollars" }, "HK$": { one: "Hong Kong dollar", many: "Hong Kong dollars" },
};
export const CURRENCY_KEYS: Keys = keysOf(CURRENCIES);

// What an amount is counted by after a slash: $20/mo, twenty dollars per month; $12/kg
export const PER: Readonly<Record<string, string>> = {
  mo: "month", month: "month", yr: "year", year: "year", wk: "week",
  week: "week", d: "day",     day: "day",   hr: "hour",  h: "hour",
  hour: "hour", min: "minute", kg: "kilogram", lb: "pound", g: "gram",
  person: "person", night: "night", share: "share", barrel: "barrel", bbl: "barrel",
  oz: "ounce", ton: "ton", tonne: "tonne", gallon: "gallon", kWh: "kilowatt-hour",
  MWh: "megawatt-hour", s: "second", sec: "second",
};
export const PER_KEYS: Keys = keysOf(PER);

// The hundredth of a currency: $4.99, four dollars and ninety-nine cents
export const CENTS: Readonly<Record<string, Forms>> = {
  "$": { one: "cent", many: "cents" }, "€": { one: "cent", many: "cents" }, "£": { one: "penny", many: "pence" },
  "US$": { one: "cent", many: "cents" }, "$US": { one: "cent", many: "cents" }, "C$": { one: "cent", many: "cents" },
  "A$": { one: "cent", many: "cents" }, "AU$": { one: "cent", many: "cents" }, "NZ$": { one: "cent", many: "cents" },
  "S$": { one: "cent", many: "cents" }, "HK$": { one: "cent", many: "cents" },
};

export const SCALES: Readonly<Record<string, string>> = {
  thousand: "thousand", million: "million", billion: "billion", trillion: "trillion",
  k: "thousand",        K: "thousand",      M: "million",       bn: "billion",
  Million: "million",   Billion: "billion", Trillion: "trillion",
  // Only after a currency: $30b, £7.45m
  b: "billion",         B: "billion",       m: "million",
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
  "°":    { one: "degree", many: "degrees" }, "″": { one: "inch", many: "inches" },
  "m2":   { one: "square meter", many: "square meters" },     "km2":  { one: "square kilometer", many: "square kilometers" },
  "cm2":  { one: "square centimeter", many: "square centimeters" }, "cm²": { one: "square centimeter", many: "square centimeters" },
  "mm²":  { one: "square millimeter", many: "square millimeters" }, "m3": { one: "cubic meter", many: "cubic meters" },
  "cm3":  { one: "cubic centimeter", many: "cubic centimeters" }, "cm³": { one: "cubic centimeter", many: "cubic centimeters" },
  "km³":  { one: "cubic kilometer", many: "cubic kilometers" }, "cc":   { one: "cubic centimeter", many: "cubic centimeters" },
  "μm":   { one: "micrometer", many: "micrometers" },         "µm":   { one: "micrometer", many: "micrometers" },
  "Mbps": { one: "megabit per second", many: "megabits per second" }, "mbps": { one: "megabit per second", many: "megabits per second" },
  "Gbps": { one: "gigabit per second", many: "gigabits per second" }, "kbps": { one: "kilobit per second", many: "kilobits per second" },
  "KG":   { one: "kilogram", many: "kilograms" },             "Kg":   { one: "kilogram", many: "kilograms" },
};

export const FRACTIONS: Fractions = {
  "1/2": "one half", "1/3": "one third", "2/3": "two thirds", "1/4": "one quarter", "3/4": "three quarters",
};

// Words after a fraction which mark it as one: 1/3 of
export const FRACTION_WORDS: ReadonlySet<string> = new Set(["of"]);

const TIME_SUFFIXES = [
  "a.m.", "p.m.", "a.m", "p.m", "am.", "pm.", "am", "pm", "AM", "PM",
  "EST", "EDT", "CST", "CDT", "MST", "MDT", "PST", "PDT", "GMT", "UTC",
];

// Words before a number read digit by digit: PIN 1234, card ending in 8876
export const SPELLING_WORDS: ReadonlySet<string> = new Set([
  "pin", "otp", "code", "passcode", "password", "cvv", "cvc", "zip",
]);

// Months, also abbreviated (lowercase)
export const MONTH_WORDS: ReadonlySet<string> = new Set([
  "january", "february", "march", "april", "may", "june", "july", "august", "september", "october",
  "november", "december", "jan", "feb", "mar", "apr", "jun", "jul", "aug", "sep", "sept", "oct",
  "nov", "dec",
]);

// Words after a number which make it a year: 340 AD, 44 BC
export const ERAS: ReadonlySet<string> = new Set([
  "ad", "a.d", "a.d.", "bc", "b.c", "b.c.", "bce", "b.c.e", "b.c.e.", "ce", "c.e", "c.e.",
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
  "deep", "old", "later", "earlier", "overall", "last", "next", "this",
  // 6.6 mm thick, 20 km northwest, £12 upfront, £209,474 made up
  "thick", "thin", "far", "near", "nearby", "north", "south", "east", "west", "northwest", "northeast", "southwest", "southeast", "upfront", "worldwide", "pa", "apiece", "total", "made", "paid", "spent", "lost", "won", "left", "raised",
]);

// Henry VIII (the Eighth), Chapter IV (four), the XXI century (twenty-first)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "henry", "francis", "edward", "george", "william", "richard", "charles", "james", "elizabeth", "mary", "victoria", "anne",
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
  fraction: " and ", half: " and a half", rating: "out of",
};

// 27.03.2026, 27-03-2026: the year has four digits (3.8.10 is a version)
export const DATE_FORMAT = dateFormat({ separators: ".-", boundaries: ".,/-", shortYear: false });

// 1-555-123-4567, (555) 123-4567, 555.123.4567, 555 123 4567, 555-1234: the longest first, each its own pass
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4], [1, 4]] }), // With an extension: 123-123-5678-1111
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], prefix: "1-" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], parenthesized: true }),
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], separators: "." }),
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], separators: " " }),
  phoneFormat({ groups: [[3, 3], [4, 4]] }),
  phoneFormat({ groups: [[3, 3], [2, 2], [4, 4]] }), // A Social Security number: 123-45-6789
];

// 21st, 2nd, 3rd, 4th
export const ORDINAL_SUFFIXES = ["st", "nd", "rd", "th"];

// 1990s, '90s, 90's
export const DECADE_SUFFIXES = ["s"];
export const DECADE_FORMAT = suffixFormat({ apostrophes: true, boundaries: ".,-" });

export const SYMBOLS: Readings = {
  "&": ["and"],      "+": ["plus"],    "=": ["equals"],  "@": ["at"],      "%": ["percent"],
  "°": ["degrees"],  "#": ["hash"],  "²": ["squared"], "³": ["cubed"],   "π": ["pi"],
  "×": ["times"],    "±": ["plus or minus"], "½": ["one half"],
  "§": ["section"],    "$": ["dollar"], "€": ["euro"],
  "£": ["pound"],      "¥": ["yen"],      "₩": ["won"],
};

// After the name of a street: 1428 Elm St., on 5th St.; on Main St. unless a name follows (Ascension St. Joseph),
// and not at the start of a sentence (Visit St. Louis)
function afterStreet(context: Context): boolean {
  const words = wordsOf(context.head());
  if (words.length < 2) {
    return false;
  }
  const name = str(words[words.length - 1]);
  const before = words[words.length - 2];
  const last = before[before.length - 1];
  const ordinal = name.length > 2 && isDigit(name[0]) && /(st|nd|rd|th)$/.test(name);
  const numbered = isDigit(last) && isUpper(name[0]);
  const named = isUpper(name[0]) && !contains(".!?", last) && !beforeName(context);
  return ordinal || numbered || named;
}

// A digit right before, possibly before a space: 25 Sept., 25 jul 2012
function afterNumber(context: Context): boolean {
  let head = context.head();
  if (head.length > 0 && head[head.length - 1] === " ") {
    head = head.slice(0, -1);
  }
  return head.length > 0 && isDigit(head[head.length - 1]);
}

// Between a house number and a capitalized name: 708 N 1st St, 1211 E Arques Ave
function betweenNumberAndStreet(context: Context): boolean {
  const tail = context.tail();
  let next = 0;
  while (next < tail.length && tail[next] === " ") {
    next++;
  }
  const street = next > 0 && next < tail.length && (isUpper(tail[next]) || isDigit(tail[next]));
  return afterNumber(context) && street;
}

// Before a ZIP code, possibly after a comma: Santa Clara, CA 95051
function beforeZip(context: Context): boolean {
  const tail = context.tail();
  const start = tail[0] === "," ? 1 : 0;
  if (start >= tail.length || tail[start] !== " ") {
    return false;
  }
  const digits = start + 1;
  let end = digits;
  while (end < tail.length && isDigit(tail[end])) {
    end++;
  }
  return end - digits === 5 && (end === tail.length || !isWord(tail[end]));
}

// At the end of an address, after its town: 123 Main St, Springfield, IL.
function afterTown(context: Context): boolean {
  const head = context.head();
  const tail = context.tail();
  if (str(head, head.length - 2) !== ", " || !(tail.length === 0 || contains(".,;)!?", tail[0]))) {
    return false;
  }
  let comma = head.length - 3;
  while (comma >= 0 && !(head[comma] === "," && head[comma + 1] === " ")) {
    comma--;
  }
  if (comma < 0 || !isUpper(head[comma + 2])) {
    return false;
  }
  return head.slice(0, comma).some((c) => isDigit(c));
}

// Two-letter codes of the US states, read before a ZIP code
const STATES: ReadonlyArray<readonly [string, string]> = [
  ["AL", "Alabama"], ["AK", "Alaska"], ["AZ", "Arizona"], ["AR", "Arkansas"], ["CA", "California"],
  ["CO", "Colorado"], ["CT", "Connecticut"], ["DE", "Delaware"], ["FL", "Florida"], ["GA", "Georgia"],
  ["HI", "Hawaii"], ["ID", "Idaho"], ["IL", "Illinois"], ["IN", "Indiana"], ["IA", "Iowa"], ["KS", "Kansas"],
  ["KY", "Kentucky"], ["LA", "Louisiana"], ["ME", "Maine"], ["MD", "Maryland"], ["MA", "Massachusetts"],
  ["MI", "Michigan"], ["MN", "Minnesota"], ["MS", "Mississippi"], ["MO", "Missouri"], ["MT", "Montana"],
  ["NE", "Nebraska"], ["NV", "Nevada"], ["NH", "New Hampshire"], ["NJ", "New Jersey"], ["NM", "New Mexico"],
  ["NY", "New York"], ["NC", "North Carolina"], ["ND", "North Dakota"], ["OH", "Ohio"], ["OK", "Oklahoma"],
  ["OR", "Oregon"], ["PA", "Pennsylvania"], ["RI", "Rhode Island"], ["SC", "South Carolina"], ["SD", "South Dakota"],
  ["TN", "Tennessee"], ["TX", "Texas"], ["UT", "Utah"], ["VT", "Vermont"], ["VA", "Virginia"], ["WA", "Washington"],
  ["WV", "West Virginia"], ["WI", "Wisconsin"], ["WY", "Wyoming"], ["DC", "D C"],
];

// Street types after the name of a street: Laguna Ct, San Tomas Expy
const STREET_TYPES: ReadonlyArray<readonly [string, string]> = [
  ["Ave", "Avenue"], ["Blvd", "Boulevard"], ["Rd", "Road"], ["Ct", "Court"], ["Ln", "Lane"], ["Pkwy", "Parkway"],
  ["Hwy", "Highway"], ["hwy", "Highway"], ["Expy", "Expressway"], ["Dr", "Drive"], ["St", "Street"],
  ["st", "Street"], ["Pl", "Place"], ["Sq", "Square"], ["Ter", "Terrace"],
];

// Directions between a house number and its street
const DIRECTIONS: ReadonlyArray<readonly [string, string]> = [
  ["N", "North"], ["S", "South"], ["E", "East"], ["W", "West"], ["NE", "Northeast"], ["NW", "Northwest"],
  ["SE", "Southeast"], ["SW", "Southwest"],
];

// Month abbreviations and their names
const MONTH_ABBREVIATIONS: ReadonlyArray<readonly [string, string]> = [
  ["Jan", "January"], ["Feb", "February"], ["Mar", "March"], ["Apr", "April"],
  ["Jun", "June"],    ["Jul", "July"],     ["Aug", "August"], ["Sep", "September"],
  ["Sept", "September"], ["Oct", "October"], ["Nov", "November"], ["Dec", "December"],
];

const TERMS_BASE: Terms = {
  abbreviations: {
    // Before a number only: No. 5, #5
    "#": [{ words: "number", when: beforeNumber }],
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
    "401(k)": ["four oh one k"],
    "e.g.": ["for example"],
    "i.e.": ["that is"],
    "etc.": ["et cetera"],
    "vs.": ["versus"],
    "Mr.": ["Mister"],
    "Mrs.": ["Missus"],
    "Dr.": [{ words: "Doctor", when: beforeName }],
    "St.": [{ words: "Street", when: afterStreet }, { words: "Saint", when: beforeName }, "Street"],
  },
  web: { dot: "dot", at: "at", slash: "slash", hyphen: "dash", underscore: "underscore", backslash: "backslash", colon: "colon",
    digits: ["zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"],
  },
};

// With the month abbreviations around a number: Jan 15, jan. 15, JAN 15 2020, Jan-15; 25 Sept., 25 jul 2012 (but not
// "mar", a verb, in lowercase)
export const TERMS: Terms = (() => {
  const abbreviations: Record<string, Readings[string]> = { ...TERMS_BASE.abbreviations };
  for (const [abbreviation, name] of MONTH_ABBREVIATIONS) {
    for (const form of [abbreviation, abbreviation.toLowerCase(), abbreviation.toUpperCase()]) {
      if (form === "mar") {
        continue;
      }
      for (const key of [form, `${form}.`]) {
        abbreviations[key] = [{ words: name, when: beforeNumber }, { words: name, when: afterNumber }];
      }
      abbreviations[`${form}-`] = [{ words: name, when: beforeNumber }];
    }
  }
  for (const [code, state] of STATES) {
    abbreviations[code] = [
      { words: state, when: beforeZip },
      { words: state, when: afterTown },
    ];
  }
  for (const [abbreviation, name] of STREET_TYPES) {
    for (const key of [abbreviation, `${abbreviation}.`]) {
      abbreviations[key] = [{ words: name, when: afterStreet }, ...(abbreviations[key] ?? [])];
    }
  }
  for (const [abbreviation, name] of DIRECTIONS) {
    for (const key of [abbreviation, `${abbreviation}.`]) {
      abbreviations[key] = [{ words: name, when: betweenNumberAndStreet }];
    }
  }
  const closing = new Set(["etc.", "St.", ...STREET_TYPES.map(([abbreviation]) => `${abbreviation}.`)]);
  return { abbreviations, web: TERMS_BASE.web, closing };
})();
