import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Keys } from "../../text/scan.ts";
import { beforeNumber, type Readings } from "../../text/readings.ts";
import type { Forms, Joiners } from "../grammar.ts";

export const ONES = [
  "zero", "uno", "due", "tre", "quattro", "cinque", "sei",
  "sette", "otto", "nove", "dieci", "undici", "dodici", "tredici",
  "quattordici", "quindici", "sedici", "diciassette", "diciotto", "diciannove",
] as const;

export const TENS = [
  "", "", "venti", "trenta", "quaranta", "cinquanta", "sessanta", "settanta", "ottanta", "novanta",
] as const;

// Ordinals up to ten; later ones are the cardinal with -esimo
export const ORDINALS = [
  "", "primo", "secondo", "terzo", "quarto", "quinto", "sesto", "settimo", "ottavo", "nono", "decimo",
] as const;

export const MONTHS: readonly string[] = [
  "gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno",
  "luglio", "agosto", "settembre", "ottobre", "novembre", "dicembre",
];

export const CURRENCIES: Readonly<Record<string, Forms>> = {
  "€": { one: "euro", many: "euro" }, "$": { one: "dollaro", many: "dollari" }, "£": { one: "sterlina", many: "sterline" },
};

// Most scales are written as the word they are read with: 3 milioni
export const SCALES: Readonly<Record<string, Forms>> = {
  "mila":     { one: "mila", many: "mila" },         "k": { one: "mila", many: "mila" }, "K": { one: "mila", many: "mila" },
  "milione":  { one: "milione", many: "milione" },   "milioni":  { one: "milioni", many: "milioni" },
  "miliardo": { one: "miliardo", many: "miliardo" }, "miliardi": { one: "miliardi", many: "miliardi" },
  "mln":      { one: "milione", many: "milioni" },   "mld":      { one: "miliardo", many: "miliardi" },
};

export const UNITS: Readonly<Record<string, Forms>> = {
  "km":   { one: "chilometro", many: "chilometri" },           "m":   { one: "metro", many: "metri" },
  "cm":   { one: "centimetro", many: "centimetri" },           "mm":  { one: "millimetro", many: "millimetri" },
  "m²":   { one: "metro quadro", many: "metri quadri" },       "m2":  { one: "metro quadro", many: "metri quadri" },
  "km²":  { one: "chilometro quadrato", many: "chilometri quadrati" }, "m³": { one: "metro cubo", many: "metri cubi" },
  "kg":   { one: "chilo", many: "chili" },                     "g":   { one: "grammo", many: "grammi" },
  "mg":   { one: "milligrammo", many: "milligrammi" },         "l":   { one: "litro", many: "litri" },
  "ml":   { one: "millilitro", many: "millilitri" },           "TB":  { one: "terabyte", many: "terabyte" },
  "GB":   { one: "giga", many: "giga" },                       "MB":  { one: "mega", many: "mega" },
  "Hz":   { one: "hertz", many: "hertz" },                     "kHz": { one: "kilohertz", many: "kilohertz" },
  "MHz":  { one: "megahertz", many: "megahertz" },             "GHz": { one: "gigahertz", many: "gigahertz" },
  "W":    { one: "watt", many: "watt" },                       "kW":  { one: "kilowatt", many: "kilowatt" },
  "kWh":  { one: "kilowattora", many: "kilowattora" },         "V":   { one: "volt", many: "volt" },
  "km/h": { one: "chilometro orario", many: "chilometri orari" }, "°C": { one: "grado", many: "gradi" },
  "°":    { one: "grado", many: "gradi" },
};

export const FRACTIONS: Fractions = {
  "1/2": "un mezzo", "1/3": "un terzo", "2/3": "due terzi", "1/4": "un quarto", "3/4": "tre quarti",
};

// Words before a time with a dot, which make it a time: alle 21.30
export const TIME_WORDS: ReadonlySet<string> = new Set(["alle", "ore", "dalle", "le", "verso", "entro", "per"]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ".", decimalSeparators: ",.", timeSeparators: ":.",
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "punto", dot: "punto", range: "-", by: "per", score: " a ", minus: "meno", plus: "più",
};

// 24/03/2022
export const DATE_FORMAT = dateFormat({ boundaries: ".,/-" });

// 333 1234567, 06 1234 5678, 06 12345678
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [6, 7]], separators: " " }),
  phoneFormat({ groups: [[2, 4], [3, 4], [4, 4]], separators: " ", leadingZero: true }),
  phoneFormat({ groups: [[2, 4], [6, 8]], separators: " ", leadingZero: true }),
];

// 1º, 2 ª: a sign, possibly after a space
export const ORDINAL_SIGNS: Keys = ["º", "ª"];
export const ORDINAL_SIGN_FORMAT = suffixFormat({ space: true });

// 21esimo, 33esima
export const ORDINAL_SUFFIXES: Keys = ["esimo", "esima", "esimi", "esime"];

// "&" only joins English names: AT&T, R&B
export const SYMBOLS: Readings = {
  "&": ["and"],     "+": ["più"],     "=": ["uguale a"],    "@": ["chiocciola"], "%": ["per cento"],
  "°": ["gradi"],   "#": ["hashtag"], "²": ["al quadrato"], "³": ["al cubo"],    "π": ["pi greco"],
  "×": ["per"],     "€": ["euro"],    "$": ["dollari"],     "£": ["sterline"],   "§": ["paragrafo"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: n. 5
    "n.": [{ words: "numero", when: beforeNumber }],
    "N.": [{ words: "numero", when: beforeNumber }],
    "nr.": [{ words: "numero", when: beforeNumber }],
    "n°": [{ words: "numero", when: beforeNumber }],
    "N°": [{ words: "numero", when: beforeNumber }],
    "art.": [{ words: "articolo", when: beforeNumber }],
    "Art.": [{ words: "articolo", when: beforeNumber }],
    "pag.": [{ words: "pagina", when: beforeNumber }],
    "cap.": [{ words: "capitolo", when: beforeNumber }],
    "§": [{ words: "paragrafo", when: beforeNumber }],
  },
  web: { dot: "punto", at: "chiocciola", slash: "slash" },
};
