import { dateFormat, keysOf, notation } from "../../numbers/formats.ts";
import type { Characters } from "../../steps/normalize.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Keys } from "../../text/scan.ts";
import type { Readings } from "../../text/readings.ts";
import type { Joiners } from "../grammar.ts";

// Echo's classes, but the zero-width non-joiner and joiner stay: they decide how Devanagari letters join
export const CHARACTERS: Characters = [
  { characters: "…",            canonical: "..." }, // Ellipsis
  { characters: "«»„“”‟˝",      canonical: '"' },   // Quotes
  { characters: "‘’‚‛´`ʼ",      canonical: "'" },   // Apostrophes
  { characters: "​﻿", canonical: "" },    // Invisible: zero-width space, byte order mark
];

// Hindi has unique cardinal words for every number 0-99 — they are not
// compositionally derivable from tens + units like in most European
// languages. Above 99 the construction becomes regular: hundreds use
// "<digit> सौ", and Indian-numbering scales (हज़ार/लाख/करोड़) compose
// multiplicatively.
export const CARDINALS: readonly string[] = [
  "शून्य",      "एक",          "दो",          "तीन",         "चार",
  "पाँच",       "छह",          "सात",         "आठ",          "नौ",
  "दस",         "ग्यारह",       "बारह",        "तेरह",         "चौदह",
  "पंद्रह",      "सोलह",        "सत्रह",        "अठारह",       "उन्नीस",
  "बीस",        "इक्कीस",      "बाईस",        "तेईस",         "चौबीस",
  "पच्चीस",     "छब्बीस",      "सत्ताईस",      "अट्ठाईस",     "उनतीस",
  "तीस",        "इकतीस",       "बत्तीस",       "तैंतीस",        "चौंतीस",
  "पैंतीस",      "छत्तीस",       "सैंतीस",        "अड़तीस",      "उनतालीस",
  "चालीस",      "इकतालीस",    "बयालीस",      "तैंतालीस",      "चवालीस",
  "पैंतालीस",   "छियालीस",     "सैंतालीस",     "अड़तालीस",     "उनचास",
  "पचास",       "इक्यावन",     "बावन",        "तिरेपन",       "चौवन",
  "पचपन",       "छप्पन",       "सत्तावन",      "अट्ठावन",      "उनसठ",
  "साठ",        "इकसठ",       "बासठ",        "तिरेसठ",       "चौंसठ",
  "पैंसठ",       "छियासठ",     "सड़सठ",        "अड़सठ",       "उनहत्तर",
  "सत्तर",       "इकहत्तर",     "बहत्तर",       "तिहत्तर",       "चौहत्तर",
  "पचहत्तर",    "छिहत्तर",      "सतहत्तर",     "अठहत्तर",      "उनासी",
  "अस्सी",      "इक्यासी",     "बयासी",       "तिरासी",       "चौरासी",
  "पचासी",      "छियासी",     "सत्तासी",      "अट्ठासी",      "नवासी",
  "नब्बे",       "इक्यानवे",    "बानवे",        "तिरानवे",      "चौरानवे",
  "पंचानवे",    "छियानवे",    "सत्तानवे",     "अट्ठानवे",     "निन्यानवे",
];

// Indian-style large scales: thousand (हज़ार), lakh (लाख = 100k),
// crore (करोड़ = 10M). Hindi pluralization is invariant for these nouns
// when used as scale words after a numeral.
export const LARGE_CARDINALS: ReadonlyArray<readonly [bigint, string]> = [
  [1000n,     "हज़ार"],
  [100000n,   "लाख"],
  [10000000n, "करोड़"],
];

// Currencies. Hindi loanword currencies are invariable in form, so only the
// singular noun is stored.
export const CURRENCIES: Readonly<Record<string, string>> = {
  "$": "डॉलर",
  "€": "यूरो",
  "£": "पाउंड",
};

// Months: standard Gregorian month names in Hindi (used as-is in dates).
export const MONTHS: readonly string[] = [
  "",          "जनवरी",     "फ़रवरी",    "मार्च",      "अप्रैल",     "मई",         "जून",
  "जुलाई",      "अगस्त",      "सितंबर",    "अक्तूबर",    "नवंबर",      "दिसंबर",
];

// Hindi ordinals have three gender/number forms. Values 1-4 and 6 take
// fully irregular forms; the rest of the ordinals are derived by appending
// a regular ending to the cardinal word at verbalization time.
// Index order matches OrdinalForm: { masculine, feminine, plural }.
export type OrdinalForms = readonly [string, string, string];
export const ORDINALS_SPECIAL: ReadonlyMap<number, OrdinalForms> = new Map<number, OrdinalForms>([
  [1, ["पहला",   "पहली",   "पहले"]],
  [2, ["दूसरा",  "दूसरी",  "दूसरे"]],
  [3, ["तीसरा",  "तीसरी",  "तीसरे"]],
  [4, ["चौथा",   "चौथी",   "चौथे"]],
  [6, ["छठा",    "छठी",    "छठे"]],
]);

// Regular ordinal endings appended directly to the last cardinal token.
// Order matches OrdinalForm: masculine / feminine / plural.
export const ORDINAL_ENDINGS = ["वाँ", "वीं", "वें"] as const;

// Ordinal suffixes after digits, in the order of OrdinalForms: 5वाँ, 5वीं, 5वें
export const ORDINAL_SUFFIXES: Keys = ["वाँ", "वीं", "वें"];

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ",", decimalSeparators: ".", timeSeparators: ":", currencies: keysOf(CURRENCIES),
  fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "दशमलव", dot: "दशमलव", range: " से ", by: "गुणा", score: " ", minus: "माइनस", plus: "प्लस",
};

// 27.03.2026, 27-03-2026: the year has four digits
export const DATE_FORMAT = dateFormat({ separators: ".-/", shortYear: false });

// TODO: not verified against a reference verbalizer yet
export const SYMBOLS: Readings = {
  "&": ["और"],       "+": ["प्लस"],     "=": ["बराबर"],    "%": ["प्रतिशत"],  "@": ["एट"],
  "°": ["डिग्री"],    "$": ["डॉलर"],     "€": ["यूरो"],      "£": ["पाउंड"],    "₹": ["रुपये"],
};

export const TERMS: Terms = {
  abbreviations: {},
  web: { dot: "डॉट", at: "एट", slash: "स्लैश" },
};
