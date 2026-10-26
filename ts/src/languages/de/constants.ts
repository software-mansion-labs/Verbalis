import { dateFormat, keysOf, notation, suffixFormat, type Fractions } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { afterAny, beforeNumber, type Readings } from "../../text/readings.ts";
import { Gender, type Forms, type Joiners } from "../grammar.ts";

export const ONES = [
  "null",   "eins",    "zwei",    "drei",      "vier",      "fünf",      "sechs",    "sieben",    "acht",     "neun",
  "zehn",   "elf",     "zwölf",   "dreizehn",  "vierzehn",  "fünfzehn",  "sechzehn", "siebzehn",  "achtzehn", "neunzehn",
] as const;

export const TENS = ["", "", "zwanzig", "dreißig", "vierzig", "fünfzig", "sechzig", "siebzig", "achtzig", "neunzig"] as const;

export const MONTHS = [
  "Januar", "Februar", "März",      "April",   "Mai",      "Juni",
  "Juli",   "August",  "September", "Oktober", "November", "Dezember",
] as const;

// Ordinal stems which differ from their cardinals: erst, dritt, siebt, acht
export const ORDINAL_STEMS: Readonly<Record<string, string>> = {
  eins: "ers", drei: "drit", sieben: "sieb", acht: "ach",
};

export const CURRENCIES: Readonly<Record<string, string>> = {
  "€": "Euro", "Euro": "Euro", "$": "Dollar", "Dollar": "Dollar", "£": "Pfund",
  "CHF": "Franken",
};

export const SCALES: Readonly<Record<string, Forms>> = {
  "Mio.": { one: "Million", many: "Millionen" },        "Million": { one: "Million", many: "Millionen" },
  "Millionen": { one: "Million", many: "Millionen" },   "Mrd.": { one: "Milliarde", many: "Milliarden" },
  "Milliarde": { one: "Milliarde", many: "Milliarden" }, "Milliarden": { one: "Milliarde", many: "Milliarden" },
  "Tsd.": { one: "tausend", many: "tausend" },          "k": { one: "tausend", many: "tausend" },
};

export const UNITS: Readonly<Record<string, Forms>> = {
  "km": { one: "Kilometer", many: "Kilometer" },       "m": { one: "Meter", many: "Meter" },
  "cm": { one: "Zentimeter", many: "Zentimeter" },     "mm": { one: "Millimeter", many: "Millimeter" },
  "nm": { one: "Nanometer", many: "Nanometer" },       "m²": { one: "Quadratmeter", many: "Quadratmeter" },
  "km²": { one: "Quadratkilometer", many: "Quadratkilometer" }, "m³": { one: "Kubikmeter", many: "Kubikmeter" },
  "ha": { one: "Hektar", many: "Hektar" },             "kg": { one: "Kilogramm", many: "Kilogramm" },
  "g": { one: "Gramm", many: "Gramm" },                "mg": { one: "Milligramm", many: "Milligramm" },
  "t": { one: "Tonne", many: "Tonnen" },               "l": { one: "Liter", many: "Liter" },
  "ml": { one: "Milliliter", many: "Milliliter" },     "TB": { one: "Terabyte", many: "Terabyte" },
  "GB": { one: "Gigabyte", many: "Gigabyte" },         "MB": { one: "Megabyte", many: "Megabyte" },
  "Hz": { one: "Hertz", many: "Hertz" },               "kHz": { one: "Kilohertz", many: "Kilohertz" },
  "MHz": { one: "Megahertz", many: "Megahertz" },      "GHz": { one: "Gigahertz", many: "Gigahertz" },
  "W": { one: "Watt", many: "Watt" },                  "kW": { one: "Kilowatt", many: "Kilowatt" },
  "kWh": { one: "Kilowattstunde", many: "Kilowattstunden" }, "V": { one: "Volt", many: "Volt" },
  "km/h": { one: "Kilometer pro Stunde", many: "Kilometer pro Stunde" }, "°C": { one: "Grad Celsius", many: "Grad Celsius" },
  "°": { one: "Grad", many: "Grad" },                  "h": { one: "Stunde", many: "Stunden" },
  "Std.": { one: "Stunde", many: "Stunden" },          "min": { one: "Minute", many: "Minuten" },
  "Min.": { one: "Minute", many: "Minuten" },          "s": { one: "Sekunde", many: "Sekunden" },
  "Sek.": { one: "Sekunde", many: "Sekunden" },        "ct": { one: "Cent", many: "Cent" },
  "PS": { one: "PS", many: "PS" },
};

export const FRACTIONS: Fractions = {
  "1/2": "ein halb", "1/3": "ein Drittel", "2/3": "zwei Drittel", "1/4": "ein Viertel",
  "3/4": "drei Viertel",
};

// Determiners after which an ordinal takes a weak ending: am dritten, der dritte (nominative)
export const WEAK: ReadonlySet<string> = new Set([
  "den", "dem", "des", "am", "im", "vom", "zum", "zur", "beim", "ins", "diesem", "diesen", "dieses",
  "jedem", "jeden", "einen", "einem", "einer", "eines", "keinen", "keinem", "keiner", "keines",
  "meinen", "meinem", "meiner", "seinen", "seinem", "seiner", "ihren", "ihrem", "ihrer", "unseren",
  "unserem", "unserer",
]);
export const WEAK_NOMINATIVE: ReadonlySet<string> = new Set([
  "der", "die", "das", "diese", "dieser", "jede", "jeder", "jedes", "eine", "keine", "meine", "seine",
  "ihre", "unsere",
]);

// ein-words, after which an ordinal takes a mixed ending: ein zweiter, ein drittes
export const MIXED: ReadonlySet<string> = new Set([
  "ein", "kein", "mein", "dein", "sein", "ihr", "unser", "euer",
]);

// Prepositions by the case they govern
export const DATIVE: ReadonlySet<string> = new Set([
  "mit", "nach", "bei", "seit", "von", "zu", "aus", "gegenüber", "in", "an", "auf", "unter", "über",
  "vor", "hinter", "neben", "zwischen", "ab",
]);
export const ACCUSATIVE: ReadonlySet<string> = new Set(["für", "durch", "gegen", "ohne", "um", "bis"]);
export const GENITIVE: ReadonlySet<string> = new Set([
  "wegen", "während", "trotz", "statt", "innerhalb", "außerhalb",
]);

// Words before a date (am 12. 3.), and before a year (im Jahr 1945, and months)
export const DATE_CUES: ReadonlySet<string> = new Set(["am", "vom", "zum", "bis", "ab", "seit", "den", "der"]);
export const YEAR_CUES: ReadonlySet<string> = new Set([
  "jahr", "jahre", "jahren", "im", "seit", "bis", "von", "ab", "anfang", "ende", "mitte", "sommer",
  "winter", "frühjahr", "herbst", "frühling", "anno", "januar", "februar", "märz", "april", "mai",
  "juni", "juli", "august", "september", "oktober", "november", "dezember",
]);

// Nouns which stay singular after a number: 1500 Stück. Like plural nouns, they make a 4-digit number a count.
export const MEASURE_WORDS: ReadonlySet<string> = new Set([
  "Stück", "Mal", "Prozent", "Grad", "Euro", "Dollar", "Pfund", "Kilo", "Gramm", "Paar", "Cent",
  "Franken",
]);

// Noun gender by ending: the exceptions that matter after numbers (matched as compound endings), then the rules
export const GENDERS: ReadonlyArray<readonly [string, Gender]> = [
  ["jahrhundert", Gender.NEUTER], ["jahrtausend", Gender.NEUTER], ["semester", Gender.NEUTER],
  ["mitglied", Gender.NEUTER],    ["kapitel", Gender.NEUTER],     ["quartal", Gender.NEUTER],
  ["prozent", Gender.NEUTER],     ["zimmer", Gender.NEUTER],      ["rennen", Gender.NEUTER],
  ["person", Gender.FEMININE],    ["gramm", Gender.NEUTER],       ["stück", Gender.NEUTER],
  ["spiel", Gender.NEUTER],       ["leben", Gender.NEUTER],       ["liter", Gender.MASCULINE],
  ["meter", Gender.MASCULINE],    ["monat", Gender.MASCULINE],    ["kunde", Gender.MASCULINE],
  ["junge", Gender.MASCULINE],    ["kilo", Gender.NEUTER],        ["jahr", Gender.NEUTER],
  ["kind", Gender.NEUTER],        ["glas", Gender.NEUTER],        ["haus", Gender.NEUTER],
  ["land", Gender.NEUTER],        ["buch", Gender.NEUTER],        ["bild", Gender.NEUTER],
  ["ende", Gender.NEUTER],        ["pfund", Gender.NEUTER],       ["euro", Gender.MASCULINE],
  ["name", Gender.MASCULINE],     ["käse", Gender.MASCULINE],     ["grad", Gender.MASCULINE],
  ["mal", Gender.NEUTER],         ["tor", Gender.NEUTER],         ["ei", Gender.NEUTER],
];
export const FEMININE_ENDINGS = [
  "e", "ung", "heit", "keit", "schaft", "ion", "tät", "ik", "ur", "enz", "anz", "ei", "in", "ie",
];
export const NEUTER_ENDINGS = ["chen", "lein", "ment", "um", "tum", "ma", "o"];

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ".   ", decimalSeparators: ",.", timeSeparators: ":.", timeSuffixes: ["Uhr"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "Komma", dot: "Punkt", range: " bis ", by: "mal", score: " zu ", minus: "minus", plus: "plus",
};

// 1.5.2024, 1.5., 12. 3.: the year is optional, a dot closes the month
export const DATE_FORMAT = dateFormat({
  separators: ".", boundaries: ".,", yearRequired: false, closingSeparator: true, spaced: true,
});

// 90er, 90ern, 90ers
export const DECADE_SUFFIXES = ["er", "ern", "ers"];
export const DECADE_FORMAT = suffixFormat({ boundaries: ".,-" });

// Words before "§" which take its plural: im § 5 (in dem Paragrafen)
export const PARAGRAPH_PLURALS: ReadonlySet<string> = new Set([
  "im", "vom", "zum", "beim", "dem", "des", "den",
]);

export const SYMBOLS: Readings = {
  "&": ["und"],       "€": ["Euro"],      "$": ["Dollar"],    "£": ["Pfund"],    "+": ["plus"],
  "=": ["gleich"],    "@": ["at"],        "%": ["Prozent"],   "°": ["Grad"],     "#": ["Hashtag"],
  "²": ["Quadrat"],   "³": ["hoch drei"], "¹": ["hoch eins"], "π": ["Pi"],       "×": ["mal"],
  "½": ["ein halb"],  "§§": ["Paragrafen"], "§": [{ words: "Paragrafen", when: afterAny([...PARAGRAPH_PLURALS]) }, "Paragraf"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: Abs. 5
    "Abs.": [{ words: "Absatz", when: beforeNumber }],
    "Nr.": [{ words: "Nummer", when: beforeNumber }],
    "S.": [{ words: "Seite", when: beforeNumber }],
    "Art.": [{ words: "Artikel", when: beforeNumber }],
    "Kap.": [{ words: "Kapitel", when: beforeNumber }],
    "ca.": [{ words: "circa", when: beforeNumber }],
    "Bd.": [{ words: "Band", when: beforeNumber }],
  },
  web: { dot: "Punkt", at: "at", slash: "Schrägstrich" },
};
