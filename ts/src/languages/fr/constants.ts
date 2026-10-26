import { dateFormat, keysOf, notation, phoneFormat, type Fractions } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { beforeNumber, type Readings } from "../../text/readings.ts";
import type { Forms, Joiners } from "../grammar.ts";

export const ONES = [
  "zéro", "un",   "deux",  "trois",  "quatre",   "cinq",   "six",   "sept",     "huit",     "neuf",
  "dix",  "onze", "douze", "treize", "quatorze", "quinze", "seize", "dix-sept", "dix-huit", "dix-neuf",
] as const;

// Tens, where 70 and 90 are built on 60 and 80: soixante-dix, quatre-vingt-dix
export const TENS = ["", "dix", "vingt", "trente", "quarante", "cinquante", "soixante", "", "quatre-vingt"] as const;

export const MONTHS = [
  "janvier", "février", "mars",      "avril",   "mai",      "juin",
  "juillet", "août",    "septembre", "octobre", "novembre", "décembre",
] as const;

export const WEEKDAYS: ReadonlySet<string> = new Set([
  "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi", "dimanche",
]);

export const CURRENCIES: Readonly<Record<string, Forms>> = {
  "€": { one: "euro", many: "euros" },    "$": { one: "dollar", many: "dollars" }, "£": { one: "livre", many: "livres" },
  "CHF": { one: "franc suisse", many: "francs suisses" }, "M€": { one: "million d'euros", many: "millions d'euros" },
};

export const SCALES: Readonly<Record<string, string>> = {
  millions: "millions", million: "million", milliards: "milliards", milliard: "milliard",
  k: "mille",           K: "mille",
};

export const UNITS: Readonly<Record<string, Forms>> = {
  "km":     { one: "kilomètre", many: "kilomètres" },           "m":    { one: "mètre", many: "mètres" },
  "cm":     { one: "centimètre", many: "centimètres" },         "mm":   { one: "millimètre", many: "millimètres" },
  "nm":     { one: "nanomètre", many: "nanomètres" },           "m²":   { one: "mètre carré", many: "mètres carrés" },
  "km²":    { one: "kilomètre carré", many: "kilomètres carrés" }, "m³": { one: "mètre cube", many: "mètres cubes" },
  "ha":     { one: "hectare", many: "hectares" },               "kg":   { one: "kilogramme", many: "kilogrammes" },
  "g":      { one: "gramme", many: "grammes" },                 "mg":   { one: "milligramme", many: "milligrammes" },
  "t":      { one: "tonne", many: "tonnes" },                   "l":    { one: "litre", many: "litres" },
  "L":      { one: "litre", many: "litres" },                   "cl":   { one: "centilitre", many: "centilitres" },
  "ml":     { one: "millilitre", many: "millilitres" },         "mL":   { one: "millilitre", many: "millilitres" },
  "km/h":   { one: "kilomètre par heure", many: "kilomètres par heure" }, "°C": { one: "degré Celsius", many: "degrés Celsius" },
  "°":      { one: "degré", many: "degrés" },                   "To":   { one: "téraoctet", many: "téraoctets" },
  "Go":     { one: "gigaoctet", many: "gigaoctets" },           "Mo":   { one: "mégaoctet", many: "mégaoctets" },
  "Ko":     { one: "kilooctet", many: "kilooctets" },           "GHz":  { one: "gigahertz", many: "gigahertz" },
  "MHz":    { one: "mégahertz", many: "mégahertz" },            "Hz":   { one: "hertz", many: "hertz" },
  "kW":     { one: "kilowatt", many: "kilowatts" },             "kWh":  { one: "kilowattheure", many: "kilowattheures" },
  "W":      { one: "watt", many: "watts" },                     "V":    { one: "volt", many: "volts" },
  "Mbit/s": { one: "mégabit par seconde", many: "mégabits par seconde" }, "min": { one: "minute", many: "minutes" },
  "s":      { one: "seconde", many: "secondes" },
};

export const FRACTIONS: Fractions = {
  "1/2": "un demi", "1/3": "un tiers", "2/3": "deux tiers", "1/4": "un quart", "3/4": "trois quarts",
};

// Nouns in -e are feminine unless listed below or ending like a masculine noun; other feminine nouns are listed
export const FEMININE_NOUNS: ReadonlySet<string> = new Set([
  "fois", "voix", "nuit", "part", "dent", "main", "fleur", "mort", "photo", "vidéo", "radio", "moto",
  "eau", "loi", "foi", "fin", "faim", "peur", "mer", "cour", "tour", "souris", "noix", "paix", "croix",
  "forêt", "image", "page", "plage", "cage", "nage", "rage",
]);
export const MASCULINE_IN_E: ReadonlySet<string> = new Set([
  "monde", "livre", "groupe", "modèle", "problème", "système", "programme", "exemple", "nombre", "membre",
  "homme", "père", "frère", "rôle", "style", "site", "espace", "service", "commerce", "couple",
  "magazine", "musée", "lycée", "verre", "chiffre", "véhicule", "stade", "poste", "signe", "rythme",
  "volume", "domaine", "vote", "texte", "téléphone", "jeune", "collège", "kilowattheure", "gramme",
  "kilogramme", "milligramme", "pourcentage", "ordre", "type", "genre", "acte", "arbre", "autre",
  "génome", "crime", "trimestre", "semestre", "siècle", "masque", "risque", "casque", "disque", "manque",
  "parc", "vice", "film",
]);
export const MASCULINE_ENDINGS = ["age", "ège", "isme", "ème", "aire", "tre", "cle", "ste", "ome"] as const;

// Words after a number which are not the noun it counts
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "avec", "dans", "pour", "mais", "donc", "puis", "sur", "sous", "par", "chez", "vers", "entre", "sans",
  "contre", "comme", "selon", "alors", "aussi", "déjà", "encore", "très", "plus", "moins", "tout",
  "tous", "toute", "toutes",
]);

export const NUMBER_WORDS: ReadonlySet<string> = new Set([
  "zéro", "un", "une", "deux", "trois", "quatre", "cinq", "six", "sept", "huit", "neuf", "dix", "onze",
  "douze", "treize", "quatorze", "quinze", "seize", "vingt", "vingts", "trente", "quarante", "cinquante",
  "soixante", "cent", "cents", "mille",
]);

// Words before a time with a dot, which make it a time: à 18.30
export const TIME_WORDS: ReadonlySet<string> = new Set([
  "à", "vers", "dès", "de", "jusqu'à", "avant", "après", "entre", "et",
]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: "   .", decimalSeparators: ",.", timeSeparators: ":.h", timeSuffixes: ["h"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "virgule", dot: "point", range: " à ", by: "par", score: " à ", minus: "moins", plus: "plus",
};

// 05/11, 05/11/2024, 1.5.2024: the year is optional
export const DATE_FORMAT = dateFormat({ separators: "./", boundaries: ".,/", yearRequired: false });

// 06 12 34 56 78, 06.12.34.56.78, 0612345678
export const PHONE_FORMAT = phoneFormat({
  groups: [[2, 2], [2, 2], [2, 2], [2, 2], [2, 2]], separators: " .", optionalSeparators: true,
  leadingZero: true, boundaries: ".,", closing: "",
});

// 1er, 1re, 2e, 2ème, 2nd, 3èmes
export const ORDINAL_SUFFIXES = [
  "ères", "èmes", "ère", "ème", "eme", "ers", "er", "res", "re", "es", "ndes", "nde", "nds", "nd",
  "e",    "è",
];

export const SYMBOLS: Readings = {
  "&": ["et"],       "+": ["plus"],    "=": ["égale"],   "@": ["arobase"], "%": ["pour cent"],
  "°": ["degrés"],   "§": ["paragraphe"], "#": ["hashtag"], "²": ["carré"], "³": ["cube"],
  "π": ["pi"],       "×": ["fois"],    "½": ["un demi"], "€": ["euros"],   "$": ["dollars"],
  "£": ["livres"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: n° 5
    "n°": [{ words: "numéro", when: beforeNumber }],
    "N°": [{ words: "numéro", when: beforeNumber }],
    "nº": [{ words: "numéro", when: beforeNumber }],
    "p.": [{ words: "page", when: beforeNumber }],
    "art.": [{ words: "article", when: beforeNumber }],
    "Art.": [{ words: "article", when: beforeNumber }],
  },
  web: { dot: "point", at: "arobase", slash: "slash" },
};
