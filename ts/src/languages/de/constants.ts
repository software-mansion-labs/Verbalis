import { dateFormat, keysOf, notation, suffixFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Keys } from "../../text/scan.ts";
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

// Currencies, and prices per unit (12 €/m²): their reading after "pro" follows the whole price
export const CURRENCIES: Readonly<Record<string, string>> = {
  "€": "Euro", "Euro": "Euro", "$": "Dollar", "Dollar": "Dollar", "£": "Pfund",
  "CHF": "Franken",
  "€/m²": "Euro pro Quadratmeter",
  "€/qm": "Euro pro Quadratmeter",
  "€/kWh": "Euro pro Kilowattstunde",
  "€/l": "Euro pro Liter",
  "€/kg": "Euro pro Kilogramm",
  "€/Monat": "Euro pro Monat",
  "€/Jahr": "Euro pro Jahr",
  "€/Tag": "Euro pro Tag",
  "€/Std.": "Euro pro Stunde",
  "€/h": "Euro pro Stunde",
  "€/Stunde": "Euro pro Stunde",
  "€/Nacht": "Euro pro Nacht",
  "€/Person": "Euro pro Person",
};

export const SCALES: Readonly<Record<string, Forms>> = {
  "Mio.": { one: "Million", many: "Millionen" },        "Million": { one: "Million", many: "Millionen" },
  "Millionen": { one: "Million", many: "Millionen" },   "Mrd.": { one: "Milliarde", many: "Milliarden" },
  "Milliarde": { one: "Milliarde", many: "Milliarden" }, "Milliarden": { one: "Milliarde", many: "Milliarden" },
  "Tsd.": { one: "tausend", many: "tausend" },          "k": { one: "tausend", many: "tausend" },
  // Lowercase, as also written: 2,2 millionen €
  "million": { one: "Million", many: "Millionen" },     "millionen": { one: "Million", many: "Millionen" },
  "milliarde": { one: "Milliarde", many: "Milliarden" }, "milliarden": { one: "Milliarde", many: "Milliarden" },
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
  "kwh": { one: "Kilowattstunde", many: "Kilowattstunden" }, "mv": { one: "Millivolt", many: "Millivolt" },
  "MW": { one: "Megawatt", many: "Megawatt" },         "GW": { one: "Gigawatt", many: "Gigawatt" },
  "MWh": { one: "Megawattstunde", many: "Megawattstunden" }, "GWh": { one: "Gigawattstunde", many: "Gigawattstunden" },
  "TWh": { one: "Terawattstunde", many: "Terawattstunden" },
  // Per unit: 42259/m², 56,3 /km²
  "/m²": { one: "pro Quadratmeter", many: "pro Quadratmeter" }, "/km²": { one: "pro Quadratkilometer", many: "pro Quadratkilometer" },
  "/m³": { one: "pro Kubikmeter", many: "pro Kubikmeter" },
  "µm": { one: "Mikrometer", many: "Mikrometer" },
  "μm": { one: "Mikrometer", many: "Mikrometer" },
  "µg": { one: "Mikrogramm", many: "Mikrogramm" },
  "μg": { one: "Mikrogramm", many: "Mikrogramm" },
  "cm²": { one: "Quadratzentimeter", many: "Quadratzentimeter" },
  "mm²": { one: "Quadratmillimeter", many: "Quadratmillimeter" },
  "cm³": { one: "Kubikzentimeter", many: "Kubikzentimeter" },
  "ft": { one: "Fuß", many: "Fuß" },
  "mV": { one: "Millivolt", many: "Millivolt" },
  "kV": { one: "Kilovolt", many: "Kilovolt" },
  "mA": { one: "Milliampere", many: "Milliampere" },
  "°F": { one: "Grad Fahrenheit", many: "Grad Fahrenheit" },
};

export const FRACTIONS: Fractions = {
  "1/2": "ein halb", "1/3": "ein Drittel", "2/3": "zwei Drittel", "1/4": "ein Viertel",
  "3/4": "drei Viertel",
};

// Words after a fraction which mark it as one: 2/3 der
export const FRACTION_WORDS: ReadonlySet<string> = new Set(["der", "des", "eines", "einer", "aller"]);

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

// Words after which an amount stays in the nominative: ist 1 Euro, als 1 Euro, aber 1 Euro ist
export const NOMINATIVE_WORDS: ReadonlySet<string> = new Set([
  "ist", "sind", "war", "waren", "wird", "werden", "wurde", "wurden", "bleibt", "blieb", "sei", "seien",
  "wäre", "als", "wie", "und", "oder", "pro", "je", "mal", "aber", "denn", "sondern", "doch", "dass",
  "weil", "wenn", "ob", "obwohl", "da", "damit", "sodass",
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

// Ludwig XIV. (der Vierzehnte), Kapitel III (drei), im XX. Jahrhundert (zwanzigsten)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "ludwig", "karl", "friedrich", "wilhelm", "heinrich", "otto", "rudolf", "albrecht", "maximilian", "ferdinand",
    "leopold", "joseph", "franz", "georg", "eduard", "richard", "johannes", "paul", "benedikt", "leo", "pius",
    "gregor", "klemens", "innozenz", "elisabeth", "katharina", "maria", "viktoria", "philipp", "napoleon",
    "alexander", "nikolaus", "peter", "iwan", "konstantin", "ramses",
  ]),
  before: new Set([
    "kapitel", "teil", "band", "buch", "akt", "szene", "abschnitt", "artikel", "anhang", "phase", "stufe", "klasse",
    "staffel", "folge", "runde", "typ", "liga",
  ]),
  after: new Set([
    "jahrhundert", "jahrhunderts", "jahrhunderte", "jahrhunderten", "jahrtausend", "jahrtausends", "weltkrieg",
    "weltkriegs", "weltkrieges", "vatikanische", "vatikanischen",
  ]),
};

// Names with a regnal number which are feminine: Elisabeth die Zweite
export const FEMININE_NAMES: ReadonlySet<string> = new Set([
  "elisabeth", "katharina", "maria", "viktoria",
]);

// --- Notation: how numbers are written ---
// Words after hours, minutes, and seconds, which the reading says: 02:02:23 Uhr, 13:46:36 Stunden
export const SECONDS_SUFFIXES: Keys = ["Uhr", "Stunden", "Std.", "h"];

export const NOTATION = notation({
  groupSeparators: ".   ", decimalSeparators: ",.", timeSeparators: ":.", timeSuffixes: ["Uhr"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "Komma", dot: "Punkt", range: " bis ", by: "mal", score: " zu ", minus: "minus", plus: "plus",
  fraction: " ",
};

// 1.5.2024, 1.5., 12. 3.: the year is optional, a dot closes the month
export const DATE_FORMAT = dateFormat({
  separators: ".", boundaries: ".,", yearRequired: false, closingSeparator: true, spaced: true,
});

// 90er, 90ern, 90ers
export const DECADE_SUFFIXES = ["er", "ern", "ers"];

// Ordinal endings written after the digits: am 3ten, zum 20sten, dem 121tem
export const ORDINAL_SUFFIXES: Keys = ["te", "ten", "tem", "ter", "tes", "ste", "sten", "stem", "ster", "stes"];
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

// Days of a month, the word before a month abbreviation in a date: 2. Jan. 2003
export const DAYS: readonly string[] = Array.from({ length: 31 }, (_, i) => [String(i + 1), String(i + 1).padStart(2, "0")]).flat();

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: Abs. 5
    "Abs.": [{ words: "Absatz", when: beforeNumber }],
    "Nr.": [{ words: "Nummer", when: beforeNumber }],
    "S.": [{ words: "Seite", when: beforeNumber }],
    "Art.": [{ words: "Artikel", when: beforeNumber }],
    "Kap.": [{ words: "Kapitel", when: beforeNumber }],
    "Bd.": [{ words: "Band", when: beforeNumber }],
    // After a day: 2. Jan. 2003
    "Jan.": [{ words: "Januar", when: afterAny(DAYS) }],
    "Feb.": [{ words: "Februar", when: afterAny(DAYS) }],
    "Mär.": [{ words: "März", when: afterAny(DAYS) }],
    "Apr.": [{ words: "April", when: afterAny(DAYS) }],
    "Jun.": [{ words: "Juni", when: afterAny(DAYS) }],
    "Jul.": [{ words: "Juli", when: afterAny(DAYS) }],
    "Aug.": [{ words: "August", when: afterAny(DAYS) }],
    "Sep.": [{ words: "September", when: afterAny(DAYS) }],
    "Sept.": [{ words: "September", when: afterAny(DAYS) }],
    "Okt.": [{ words: "Oktober", when: afterAny(DAYS) }],
    "Nov.": [{ words: "November", when: afterAny(DAYS) }],
    "Dez.": [{ words: "Dezember", when: afterAny(DAYS) }],
    "jan.": [{ words: "Januar", when: afterAny(DAYS) }],
    "feb.": [{ words: "Februar", when: afterAny(DAYS) }],
    "mär.": [{ words: "März", when: afterAny(DAYS) }],
    "apr.": [{ words: "April", when: afterAny(DAYS) }],
    "jun.": [{ words: "Juni", when: afterAny(DAYS) }],
    "jul.": [{ words: "Juli", when: afterAny(DAYS) }],
    "aug.": [{ words: "August", when: afterAny(DAYS) }],
    "sep.": [{ words: "September", when: afterAny(DAYS) }],
    "sept.": [{ words: "September", when: afterAny(DAYS) }],
    "okt.": [{ words: "Oktober", when: afterAny(DAYS) }],
    "nov.": [{ words: "November", when: afterAny(DAYS) }],
    "dez.": [{ words: "Dezember", when: afterAny(DAYS) }],
    // Anywhere
    "ca.": ["circa"],
    "z.B.": ["zum Beispiel"],
    "z. B.": ["zum Beispiel"],
    "d.h.": ["das heißt"],
    "d. h.": ["das heißt"],
    "bzw.": ["beziehungsweise"],
    "usw.": ["und so weiter"],
    "u.a.": ["unter anderem"],
    "u. a.": ["unter anderem"],
    "z.T.": ["zum Teil"],
    "z. T.": ["zum Teil"],
    "ggf.": ["gegebenenfalls"],
    "evtl.": ["eventuell"],
    "inkl.": ["inklusive"],
    "vgl.": ["vergleiche"],
    "etc.": ["et cetera"],
    "Dr.": ["Doktor"],
    "Prof.": ["Professor"],
    "Mr.": ["Mister"],
    "Mrs.": ["Misses"],
    "Ms.": ["Miss"],
    "MwSt.": ["Mehrwertsteuer"],
    "Mwst.": ["Mehrwertsteuer"],
  },
  web: { dot: "Punkt", at: "at", slash: "Schrägstrich", hyphen: "Bindestrich", underscore: "Unterstrich",
    digits: ["null", "eins", "zwei", "drei", "vier", "fünf", "sechs", "sieben", "acht", "neun"],
  },
};
