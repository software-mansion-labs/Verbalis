import { dateFormat, keysOf, notation, phoneFormat, type PhoneFormat, type RomanCues } from "../../numbers/formats.ts";
import type { Characters } from "../../steps/normalize.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Context } from "../../text/context.ts";
import type { Keys } from "../../text/scan.ts";
import { afterAny, type Readings } from "../../text/readings.ts";
import { isAlpha, isDigit } from "../../text/unicode.ts";
import type { Joiners } from "../grammar.ts";

// Echo's classes, but the zero-width non-joiner and joiner stay: they decide how Devanagari letters join
export const CHARACTERS: Characters = [
  { characters: "…",            canonical: "..." }, // Ellipsis
  { characters: "«»„“”‟˝",      canonical: '"' },   // Quotes
  { characters: "‘’‚‛´`ʼ",      canonical: "'" },   // Apostrophes
  { characters: "​﻿", canonical: "" },    // Invisible: zero-width space, byte order mark
  // Devanagari digits, read as numbers: ५४ -> 54
  { characters: "०", canonical: "0" }, { characters: "१", canonical: "1" }, { characters: "२", canonical: "2" },
  { characters: "३", canonical: "3" }, { characters: "४", canonical: "4" }, { characters: "५", canonical: "5" },
  { characters: "६", canonical: "6" }, { characters: "७", canonical: "7" }, { characters: "८", canonical: "8" },
  { characters: "९", canonical: "9" },
  // Subscript digits, as in formulas: H₂O -> H2O
  { characters: "₀", canonical: "0" }, { characters: "₁", canonical: "1" }, { characters: "₂", canonical: "2" },
  { characters: "₃", canonical: "3" }, { characters: "₄", canonical: "4" }, { characters: "₅", canonical: "5" },
  { characters: "₆", canonical: "6" }, { characters: "₇", canonical: "7" }, { characters: "₈", canonical: "8" },
  { characters: "₉", canonical: "9" },
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
  [1000000000n,   "अरब"],
  [100000000000n, "खरब"],
  [10000000000000n,     "नील"],
  [1000000000000000n,   "पद्म"],
  [100000000000000000n, "शंख"],
];

// Currencies. Hindi loanword currencies are invariable in form, so only the
// singular noun is stored.
export const CURRENCIES: Readonly<Record<string, string>> = {
  "$": "डॉलर", "€": "यूरो", "£": "पाउंड", "₹": "रुपये", "¥": "येन", "₺": "लीरा",
  "₩": "वॉन",  "৳": "टका",  "₦": "नाइरा",
};

// The smaller unit of a currency, read after the whole amount: ₹9.50 -> नौ रुपये पचास पैसे
export const CENTS: Readonly<Record<string, string>> = {
  "$": "सेंट", "€": "सेंट", "£": "पेंस", "₹": "पैसे", "¥": "सेन", "₺": "कुरस",
  "₩": "जिओन", "৳": "पैसे", "₦": "कोबो",
};

// Currencies written as words after an amount: 888.60 रुपये
export const CURRENCY_WORDS: ReadonlySet<string> = new Set([
  "रुपये", "रुपए", "रुपया", "रूपये", "रूपए", "डॉलर", "यूरो", "पाउंड",
]);

// Scales written as words after an amount: ₹ 5 लाख
const SCALES: Keys = ["हज़ार", "लाख", "करोड़", "अरब", "खरब", "मिलियन", "बिलियन", "ट्रिलियन"];

// Units, invariable like the currencies: 5 km, 5 किमी -> पाँच किलोमीटर
export const UNITS: Readonly<Record<string, string>> = {
  "km": "किलोमीटर",       "किमी": "किलोमीटर",     "m": "मीटर",             "cm": "सेंटीमीटर",
  "सेमी": "सेंटीमीटर",     "mm": "मिलीमीटर",       "मिमी": "मिलीमीटर",       "km²": "वर्ग किलोमीटर",
  "m²": "वर्ग मीटर",       "kg": "किलोग्राम",       "किग्रा": "किलोग्राम",     "g": "ग्राम",
  "mg": "मिलीग्राम",       "l": "लीटर",            "ml": "मिलीलीटर",        "km/h": "किलोमीटर प्रति घंटा",
  "km/hr": "किलोमीटर प्रति घंटा", "°C": "डिग्री सेल्सियस", "°F": "डिग्री फारेनहाइट", "ft": "फीट",
  "MW": "मेगावाट",         "kW": "किलोवाट",        "GB": "जीबी",            "MB": "एमबी",
  "ha": "हेक्टेयर",         "ac": "एकड़",           "mi": "मील",             "in": "इंच",
  "yd": "यार्ड",            "µm": "माइक्रोमीटर",      "mi/hr": "मील प्रति घंटा", "mph": "मील प्रति घंटा",
  "qt": "क्वार्ट",          "gal": "गैलन",          "pt": "पिंट",            "cg": "सेंटीग्राम",
  "dg": "डेसीग्राम",        "t": "टन",             "st": "स्टोन",           "lb": "पाउंड",
  "oz": "आउन्स",           "q": "क्विंटल",          "yr": "साल",
  "W": "वाट",              "Km/h": "किलोमीटर प्रति घंटा", "x": "गुना",
  // Abbreviations with ० (read as 0) or ॰ as their dot: 34 किमी०, 20 कि॰मी॰, 6 मी॰, 3,000 कि॰
  "किमी0": "किलोमीटर",     "कि॰मी॰": "किलोमीटर",     "मी॰": "मीटर",           "कि॰": "किलोग्राम",
  "रु.": "रुपये",
};

// Months: standard Gregorian month names in Hindi (used as-is in dates).
export const MONTHS: readonly string[] = [
  "",          "जनवरी",     "फ़रवरी",    "मार्च",      "अप्रैल",     "मई",         "जून",
  "जुलाई",      "अगस्त",      "सितंबर",    "अक्टूबर",    "नवंबर",      "दिसंबर",
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

// Ordinal suffixes after digits, by their form in ORDINAL_ENDINGS: 5वाँ, 5वां, 5वीं, 5वें, and the endings of the
// irregular ordinals: 1ला, 2री, 4थे, 6ठा
export const ORDINAL_SUFFIXES: ReadonlyMap<string, number> = new Map([
  ["वाँ", 0], ["वां", 0], ["वा", 0], ["वॉ", 0], ["वीं", 1], ["वी", 1], ["वें", 2], ["ला", 0], ["ली", 1],
  ["ले", 2],  ["रा", 0],  ["री", 1], ["रे", 2],  ["था", 0], ["थी", 1], ["थे", 2],  ["ठा", 0], ["ठी", 1],
  ["ठे", 2],
]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ",", decimalSeparators: ".", timeSeparators: ":.", currencies: keysOf(CURRENCIES),
  scales: SCALES, units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "दशमलव", dot: "डॉट", range: " से ", by: "बाई", score: " ", minus: "माइनस", plus: "प्लस",
  fraction: " और ",
};

// 27.03.2026, 27-03-2026: the year has four digits. Without it (06-05), the reading decides.
export const DATE_FORMAT = dateFormat({ separators: ".-/", yearRequired: false, shortYear: false });

// TODO: not verified against a reference verbalizer yet
export const SYMBOLS: Readings = {
  "&": ["और"],       "+": ["प्लस"],     "=": ["बराबर"],    "%": ["प्रतिशत"],  "@": ["एट"],
  "°": ["डिग्री"],    "$": ["डॉलर"],     "€": ["यूरो"],      "£": ["पाउंड"],    "₹": ["रुपये"],
};

// Words which mark the number after them as a year: सन 1999, साल 1971, 31 मई 1990
export const YEARS_AFTER: ReadonlySet<string> = new Set([
  "सन", "सन्", "साल", "वर्ष", "जनवरी", "फ़रवरी", "फरवरी", "मार्च", "अप्रैल", "मई", "जून", "जुलाई", "अगस्त",
  "सितंबर", "सितम्बर", "अक्टूबर", "अक्तूबर", "नवंबर", "नवम्बर", "दिसंबर", "दिसम्बर",
]);

// Words which mark the number before them as a year: 1999 में, 1990 ईसवी, 120 ईसा पूर्व
export const YEARS_BEFORE: ReadonlySet<string> = new Set([
  "में", "का", "की", "के", "से", "तक", "ईसवी", "ईस्वी", "ईसा",
]);

// Words before a time written with a dot: सुबह 8.30
export const DAY_TIMES: ReadonlySet<string> = new Set(["सुबह", "सवेरे", "दोपहर", "शाम", "रात", "तड़के"]);

// Words before a number which make it a code, read digit by digit from four digits on: नंबर 1124, पिन कोड 110023
export const CODE_CUES: ReadonlySet<string> = new Set([
  "नंबर", "नम्बर", "नं", "कोड", "पिन", "पिनकोड", "oclc", "isbn", "issn",
]);

// Phone numbers, read digit by digit: 03862-351791, (02229) 411128, 022.29411128, 0 (80) 26411128; and without a trunk
// zero, only after a word of phones: 1374-309988, 98765 43210
export const PHONE_FORMATS: readonly PhoneFormat[] = [
  phoneFormat({ groups: [[3, 7], [4, 8]], separators: "-. ", leadingZero: true }),
  phoneFormat({ groups: [[3, 5], [6, 8]], separators: " ", parenthesized: true, leadingZero: true }),
  phoneFormat({ groups: [[2, 4], [6, 8]], separators: " ", prefix: "0 ", parenthesized: true }),
  phoneFormat({ groups: [[4, 4], [6, 6]], separators: "- " }),
  phoneFormat({ groups: [[5, 5], [5, 5]], separators: "- " }),
];
export const PHONE_CUES: ReadonlySet<string> = new Set([
  "नंबर", "नम्बर", "फोन", "फ़ोन", "मोबाइल", "कॉल", "संपर्क", "हेल्पलाइन", "फैक्स", "फ़ैक्स", "टेलीफोन", "टेलीफ़ोन",
]);

// Words right before a number which make it a count, not the postal code ending an address: मंगलवार को 51340.
export const COUNT_CUES: ReadonlySet<string> = new Set([
  "को", "से", "में", "के", "की", "का", "ने", "पर", "तक", "संख्या", "लगभग", "करीब", "कुल", "और",
]);

// English ordinals as said in Hindi: 7th क्रॉस -> सेवंथ क्रॉस. By number up to 19; then the tens and their ordinals.
export const ENGLISH_ORDINALS: readonly string[] = [
  "",          "फ़र्स्ट",     "सेकंड",      "थर्ड",       "फ़ोर्थ",      "फ़िफ्थ",     "सिक्स्थ",
  "सेवंथ",     "एटथ",       "नाइंथ",      "टेंथ",       "इलेवंथ",     "ट्वेल्फ्थ",   "थर्टींथ",
  "फ़ोर्टींथ",   "फ़िफ्टींथ",   "सिक्सटींथ",   "सेवनटींथ",   "एटींथ",      "नाइनटींथ",
];
export const ENGLISH_TENS: readonly string[] = [
  "", "", "ट्वेंटी", "थर्टी", "फ़ोर्टी", "फ़िफ्टी", "सिक्सटी", "सेवंटी", "एटी", "नाइंटी",
];
export const ENGLISH_TENTHS: readonly string[] = [
  "", "", "ट्वेंटिएथ", "थर्टिएथ", "फ़ोर्टिएथ", "फ़िफ्टिएथ", "सिक्सटिएथ", "सेवंटिएथ", "एटिएथ", "नाइंटिएथ",
];
export const ENGLISH_ORDINAL_SUFFIXES: Keys = ["st", "nd", "rd", "th"];

// Latin capitals as said in Hindi, spelled in codes: ABC-123 -> ए बी सी एक सौ तेईस
export const LETTERS: readonly string[] = [
  "ए",  "बी",  "सी",  "डी",   "ई",   "एफ",  "जी",  "एच",  "आई",  "जे",  "के",  "एल",  "एम",
  "एन", "ओ",  "पी",  "क्यू",  "आर",  "एस",  "टी",  "यू",  "वी",  "डब्ल्यू", "एक्स", "वाई", "ज़ेड",
];

// Capitalized words of codes which are said as words, not spelled: COVID-19
export const CODE_WORDS: ReadonlySet<string> = new Set(["COVID", "SARS", "NASA", "ISRO", "UEFA", "FIFA"]);

// Nouns numbered by a Roman numeral after them (कक्षा XII, अध्याय IV, विश्व युद्ध II), or before them (XC विद्यार्थी)
export const ROMAN_CUES: RomanCues = {
  names: new Set(),
  before: new Set([
    "कक्षा", "अध्याय", "भाग", "खंड", "खण्ड", "युद्ध", "योजना", "राजमार्ग", "श्रेणी", "मिशन", "चरण", "पाठ",
    "अनुच्छेद", "अनुसूची", "सत्र", "स्तर", "वर्ग", "संस्करण", "सीज़न", "सीजन", "टाइप", "फेज़", "फेज",
  ]),
  after: new Set(["विद्यार्थी", "छात्र"]),
};

// Ordinal suffixes which may follow a Roman numeral after a space: LIII वा गणतंत्र दिवस
export const ROMAN_ORDINAL_SUFFIXES: Keys = ["वाँ", "वां", "वा", "वीं", "वें"];

// A number right before: the year of an era (1990 ई., 297-272 ई. पू.)
function afterNumber(context: Context): boolean {
  return isDigit(context.before().at(-1));
}

// A title before a name: at the start, or after punctuation or a postposition. Not the verb of "रोक ले."
const POSTPOSITIONS = new Set(["", "के", "की", "का", "ने", "को", "से", "और", "व"]);
function beforeName(context: Context): boolean {
  const head = context.head();
  let end = head.length;
  while (end > 0 && head[end - 1] === " ") {
    end--;
  }
  const punctuation = end > 0 && !isAlpha(head[end - 1]);
  return context.after() !== "" && (punctuation || POSTPOSITIONS.has(context.before()));
}

export const TERMS: Terms = {
  abbreviations: {
    // Titles before a name: डॉ. अमित -> डॉक्टर अमित
    "डॉ.": ["डॉक्टर"],
    "डॉ0": ["डॉक्टर"],
    "प्रो.": ["प्रोफेसर"],
    "प्रो॰": ["प्रोफ़ेसर"],
    "इं.": [{ words: "इंजीनियर", when: beforeName }],
    "ले.": [{ words: "लेफ्टिनेंट", when: beforeName }],
    "वै.": [{ words: "वैज्ञानिक", when: beforeName }],
    "कु.": [{ words: "कुमारी", when: beforeName }],
    "मा.": [{ words: "मास्टर", when: beforeName }],
    // Eras after a year: 1990 ई., 120 ई. पू., ७०० ईसा पू.
    "ई.": [{ words: "ईसवी", when: afterNumber }],
    "ई. पू.": [{ words: "ईसा पूर्व", when: afterNumber }],
    "ई.पू.": [{ words: "ईसा पूर्व", when: afterNumber }],
    "पू.": [{ words: "पूर्व", when: afterAny(["ईसा"]) }],
    // With ० (read as 0) or ॰ as the dot: 1825 ई०, १९१३ ई॰, ४००० ई॰पू॰
    "ई0": [{ words: "ईसवी", when: afterNumber }],
    "ई॰": [{ words: "ईसवी", when: afterNumber }],
    "ई॰पू॰": [{ words: "ईसा पूर्व", when: afterNumber }],
    "ईपू": [{ words: "ईसा पूर्व", when: afterNumber }],
    "वि.": [{ words: "विक्रम संवत्", when: afterNumber }],
    // Anywhere: प्रति है०, नियम क्र. ७२
    "है0": ["हेक्टेयर"],
    "कु0": ["कुन्तल"],
    "क्र.": ["क्रमांक"],
    "प्र.": ["प्रतिशत"],
  },
  web: {
    dot: "डॉट", at: "एट", slash: "फॉरवर्ड स्लैश", hyphen: "हाइफ़न", underscore: "अंडरस्कोर", backslash: "बैकवर्ड स्लैश", colon: "कोलन",
    digits: ["शून्य", "एक", "दो", "तीन", "चार", "पाँच", "छह", "सात", "आठ", "नौ"],
  },
};
