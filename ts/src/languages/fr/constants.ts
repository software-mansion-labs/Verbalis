import { dateFormat, keysOf, notation, phoneFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { stripped, wordsOf, type Context } from "../../text/context.ts";
import type { Keys } from "../../text/scan.ts";
import { afterAny, beforeName, beforeNumber, type Readings } from "../../text/readings.ts";
import { size } from "../../text/strings.ts";
import { isUpper } from "../../text/unicode.ts";
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
  "€uros": { one: "euro", many: "euros" },
  "M$": { one: "million de dollars", many: "millions de dollars" }, "G$": { one: "milliard de dollars", many: "milliards de dollars" },
  // A price per unit: 2 000 €/mois
  "€/mois": { one: "euro par mois", many: "euros par mois" }, "€/an": { one: "euro par an", many: "euros par an" },
  "€/jour": { one: "euro par jour", many: "euros par jour" }, "€/semaine": { one: "euro par semaine", many: "euros par semaine" },
  "€/h": { one: "euro de l'heure", many: "euros de l'heure" }, "€/heure": { one: "euro de l'heure", many: "euros de l'heure" },
  "€/nuit": { one: "euro la nuit", many: "euros la nuit" }, "€/personne": { one: "euro par personne", many: "euros par personne" },
  "€/kg": { one: "euro le kilo", many: "euros le kilo" }, "€/m²": { one: "euro le mètre carré", many: "euros le mètre carré" },
  "€/l": { one: "euro le litre", many: "euros le litre" },
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
  "ch": { one: "cheval", many: "chevaux" },
  "ko": { one: "kilooctet", many: "kilooctets" },
  "ms": { one: "milliseconde", many: "millisecondes" },
  "pts": { one: "point", many: "points" },
  "kms": { one: "kilomètre", many: "kilomètres" },
  "Ha": { one: "hectare", many: "hectares" },
  "Kg": { one: "kilogramme", many: "kilogrammes" },
  "mn": { one: "minute", many: "minutes" },
  "sec": { one: "seconde", many: "secondes" },
  "dB": { one: "décibel", many: "décibels" },
  "MVA": { one: "mégavoltampère", many: "mégavoltampères" },
  "kV": { one: "kilovolt", many: "kilovolts" },
  "Mhz": { one: "mégahertz", many: "mégahertz" },
  "Mbits/s": { one: "mégabit par seconde", many: "mégabits par seconde" },
  "Gbit/s": { one: "gigabit par seconde", many: "gigabits par seconde" },
  "° C": { one: "degré Celsius", many: "degrés Celsius" },
  "p.": { one: "page", many: "pages" },
  "pp.": { one: "page", many: "pages" },
  "Gbps": { one: "gigabit par seconde", many: "gigabits par seconde" },
  "gHz": { one: "gigahertz", many: "gigahertz" },
};

export const FRACTIONS: Fractions = {
  "1/2": "un demi", "1/3": "un tiers", "2/3": "deux tiers", "1/4": "un quart", "3/4": "trois quarts",
};

// Words after a fraction which mark it as one: 1/3 d'eau
export const FRACTION_WORDS: ReadonlySet<string> = new Set(["de", "du", "des", "d'"]);

// Nouns in -e, -té, -tion, -sion, and -aison are feminine unless listed below or ending like a masculine noun; other
// feminine nouns are listed
export const FEMININE_NOUNS: ReadonlySet<string> = new Set([
  "fois", "voix", "nuit", "part", "dent", "main", "fleur", "mort", "photo", "vidéo", "radio", "moto",
  "eau", "loi", "foi", "fin", "faim", "peur", "mer", "cour", "tour", "souris", "noix", "paix", "croix",
  "forêt", "image", "page", "plage", "cage", "nage", "rage", "chanson", "leçon", "façon", "boisson",
  "prison", "cuisson", "sœur", "fourmi", "clé", "paroi", "tribu", "vertu", "brebis", "perdrix",
]);
export const FEMININE_ENDINGS = ["té", "tion", "sion", "aison"] as const;
export const MASCULINE_IN_E: ReadonlySet<string> = new Set([
  "monde", "livre", "groupe", "modèle", "problème", "système", "programme", "exemple", "nombre", "membre",
  "homme", "père", "frère", "rôle", "style", "site", "espace", "service", "commerce", "couple",
  "magazine", "musée", "lycée", "verre", "chiffre", "véhicule", "stade", "poste", "signe", "rythme",
  "volume", "domaine", "vote", "texte", "téléphone", "jeune", "collège", "kilowattheure", "gramme",
  "kilogramme", "milligramme", "pourcentage", "ordre", "type", "genre", "acte", "arbre", "autre",
  "génome", "crime", "trimestre", "semestre", "siècle", "masque", "risque", "casque", "disque", "manque",
  "parc", "vice", "film", "été", "côté", "comité", "traité", "député", "pâté", "arrêté", "comté",
  "karaté", "velouté",
]);
export const MASCULINE_ENDINGS = ["age", "ège", "isme", "ème", "aire", "tre", "cle", "ste", "ome"] as const;

// Words after a number which are not the noun it counts
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "avec", "dans", "pour", "mais", "donc", "puis", "sur", "sous", "par", "chez", "vers", "entre", "sans",
  "contre", "comme", "selon", "alors", "aussi", "déjà", "encore", "très", "plus", "moins", "tout",
  "tous", "toute", "toutes", "des",
]);

export const NUMBER_WORDS: ReadonlySet<string> = new Set([
  "zéro", "un", "une", "deux", "trois", "quatre", "cinq", "six", "sept", "huit", "neuf", "dix", "onze",
  "douze", "treize", "quatorze", "quinze", "seize", "vingt", "vingts", "trente", "quarante", "cinquante",
  "soixante", "cent", "cents", "mille",
]);

// Words before a number which make it an ordinal, without the plural s of quatre-vingts and cents: les années
// quatre-vingt, page deux cent, l'an mille neuf cent
export const ORDINAL_WORDS: ReadonlySet<string> = new Set([
  "années", "année", "an", "l'an", "page", "numéro", "chapitre", "article", "chambre",
]);

// Words before a version, which is not a date: version 3.8.10
export const VERSION_WORDS: ReadonlySet<string> = new Set(["version", "v", "ios", "android"]);

// Words before a decade written with an s: les 80s
export const DECADE_WORDS: ReadonlySet<string> = new Set(["les", "des", "aux", "dans", "années"]);

// Words before a number which make it a code: numéro 1035836, code 75001
export const CODE_WORDS: ReadonlySet<string> = new Set(["numéro", "n°", "nº", "code", "réf", "référence"]);

// Words before a time with a dot, which make it a time: à 18.30
export const TIME_WORDS: ReadonlySet<string> = new Set([
  "à", "vers", "dès", "de", "jusqu'à", "avant", "après", "entre", "et",
]);

// XIXe, Ier, Ire: the suffixes of an ordinal in Roman numerals
export const ROMAN_ORDINAL_SUFFIXES: Keys = ["er", "re", "ère", "e", "ème"];

// Louis XIV (quatorze), chapitre III (trois), le XXI siècle (vingt et unième); XIXe and Ier have their own pattern
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "louis", "henri", "charles", "philippe", "françois", "jean", "paul", "jean-paul", "benoît", "léon", "pie",
    "grégoire", "clément", "innocent", "napoléon", "élisabeth", "édouard", "george", "georges", "guillaume",
    "frédéric", "nicolas", "alexandre", "pierre", "ramsès", "ferdinand", "alphonse", "carlos", "felipe",
    "victor-emmanuel", "emmanuel", "catherine", "marie", "victoria",
  ]),
  before: new Set([
    "chapitre", "tome", "livre", "acte", "scène", "titre", "article", "partie", "volume", "saison", "épisode",
    "annexe", "phase", "vatican", "niveau", "type", "classe", "catégorie", "l'acte",
    "l'article", "l'annexe", "l'épisode",
  ]),
  after: new Set([
    "siècle", "siècles", "millénaire", "république", "empire", "arrondissement", "congrès", "édition", "festival",
    "législature", "régiment", "corps", "armée",
  ]),
};

// Names with a regnal number which are feminine (Élisabeth Ire), and nouns after a numeral which make it feminine
export const FEMININE_NAMES: ReadonlySet<string> = new Set([
  "élisabeth", "catherine", "marie", "victoria",
]);


export const FEMININE_ROMAN_NOUNS: ReadonlySet<string> = new Set([
  "république", "édition", "législature", "armée",
]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: "   .'’", decimalSeparators: ",.", timeSeparators: ":.h", timeSuffixes: ["h"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "point", dot: "point", range: " à ", by: "par", score: " à ", minus: "moins", plus: "plus",
  fraction: " et ", half: " et demi", rating: "sur",
};

// 05/11, 05/11/2024, 1.5.2024: the year is optional
export const DATE_FORMAT = dateFormat({ separators: "./", boundaries: ".,/", yearRequired: false });

// 12-05-2024, but not 12-05, which may be a range or a score
export const DASH_DATE_FORMAT = dateFormat({ separators: "-", boundaries: ".,/" });

// 06 12 34 56 78, 06.12.34.56.78, 0612345678
export const PHONE_FORMAT = phoneFormat({
  groups: [[2, 2], [2, 2], [2, 2], [2, 2], [2, 2]], separators: " .", optionalSeparators: true,
  leadingZero: true, boundaries: ".,", closing: "",
});

// 1er, 1re, 2e, 2ème, 2nd, 3èmes
export const ORDINAL_SUFFIXES = [
  "ères", "èmes", "ère", "ere", "ème", "eme", "ers", "er", "res", "re", "es", "ndes", "nde", "nds", "nd",
  "e",    "è",
];

export const SYMBOLS: Readings = {
  "&": ["et"],       "+": ["plus"],    "=": ["égale"],   "@": ["arobase"], "%": ["pour cent"],
  "°": ["degrés"],   "§": ["paragraphe"], "#": ["hashtag"], "²": ["carré"], "³": ["cube"],
  "π": ["pi"],       "×": ["fois"],    "½": ["un demi"], "€": ["euros"],   "$": ["dollars"],
  "£": ["livres"],   "±": ["plus ou moins"], "+/-": ["plus ou moins"], "‰": ["pour mille"],
  "≤": ["inférieur ou égal à"],
  "°C": ["degrés Celsius"], "M€": ["millions d'euros"],
};

// Days of a month, the word before a month abbreviation in a date: 2 janv. 2003, 1er févr.
const DAYS: readonly string[] = [
  "1er", ...Array.from({ length: 31 }, (_, i) => [String(i + 1), String(i + 1).padStart(2, "0")]).flat(),
];

// XXe s., Ve s.: the century after a Roman ordinal (not after Le, De, Ce, Me)
function afterRomanOrdinal(context: Context): boolean {
  const words = wordsOf(context.head());
  const word = words.length === 0 ? "" : stripped(words[words.length - 1]).join("");
  return /^([IVX]|[IVXLCDM]{2,})e$/.test(word);
}

// St-Étienne: a name after a hyphen
function beforeHyphenatedName(context: Context): boolean {
  const tail = context.tail();
  return tail.length > 1 && tail[0] === "-" && isUpper(tail[1]);
}

// M. et Mme Durand
function beforeAndMadame(context: Context): boolean {
  return context.tail().join("").startsWith(" et Mme");
}

// M. Dupont, but not the M. of J. M. Ayuso
function beforeNameNotInitial(context: Context): boolean {
  return beforeName(context) && size(context.before()) !== 1;
}

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: n° 5
    "n°": [{ words: "numéro", when: beforeNumber }],
    "N°": [{ words: "numéro", when: beforeNumber }],
    "nº": [{ words: "numéro", when: beforeNumber }],
    "n.°": [{ words: "numéro", when: beforeNumber }],
    "N.°": [{ words: "numéro", when: beforeNumber }],
    "Nº": [{ words: "numéro", when: beforeNumber }],
    "n.º": [{ words: "numéro", when: beforeNumber }],
    "N.º": [{ words: "numéro", when: beforeNumber }],
    "p.": [{ words: "page", when: beforeNumber }],
    "art.": [{ words: "article", when: beforeNumber }],
    "Art.": [{ words: "article", when: beforeNumber }],
    "pp.": [{ words: "pages", when: beforeNumber }],
    "chap.": [{ words: "chapitre", when: beforeNumber }],
    "vol.": [{ words: "volume", when: beforeNumber }],
    "fig.": [{ words: "figure", when: beforeNumber }],
    "s.": [{ words: "siècle", when: afterRomanOrdinal }],
    // Before a name: M. Dupont, Dr Martin, St-Étienne
    "M.": [
      { words: "monsieur", when: beforeNameNotInitial },
      { words: "monsieur", when: beforeAndMadame },
    ],
    "Mr": [{ words: "monsieur", when: beforeName }],
    "Dr": [{ words: "docteur", when: beforeName }],
    "Dr.": [{ words: "docteur", when: beforeName }],
    "Pr": [{ words: "professeur", when: beforeName }],
    "St": [{ words: "saint", when: beforeName }, { words: "saint", when: beforeHyphenatedName }],
    "Ste": [{ words: "sainte", when: beforeName }, { words: "sainte", when: beforeHyphenatedName }],
    "Mgr": [{ words: "monseigneur", when: beforeName }],
    // After a day: 2 janv. 2003
    "janv.": [{ words: "janvier", when: afterAny(DAYS) }],
    "févr.": [{ words: "février", when: afterAny(DAYS) }],
    "fév.": [{ words: "février", when: afterAny(DAYS) }],
    "avr.": [{ words: "avril", when: afterAny(DAYS) }],
    "juil.": [{ words: "juillet", when: afterAny(DAYS) }],
    "sept.": [{ words: "septembre", when: afterAny(DAYS) }],
    "oct.": [{ words: "octobre", when: afterAny(DAYS) }],
    "nov.": [{ words: "novembre", when: afterAny(DAYS) }],
    "déc.": [{ words: "décembre", when: afterAny(DAYS) }],
    // Anywhere
    "MM.": ["messieurs"],
    "Mme": ["madame"],
    "Mmes": ["mesdames"],
    "Mlle": ["mademoiselle"],
    "Mlles": ["mesdemoiselles"],
    "Mᵐᵉ": ["madame"],
    "Mᵐᵉˢ": ["mesdames"],
    "Mˡˡᵉ": ["mademoiselle"],
    "Mˡˡᵉˢ": ["mesdemoiselles"],
    "Dʳ": ["docteur"],
    "Dʳˢ": ["docteurs"],
    "Dʳᵉ": ["docteure"],
    "Dʳᵉˢ": ["docteures"],
    "Pʳ": ["professeur"],
    "etc.": ["et cetera"],
    "Tél.": ["téléphone"],
    "tél.": ["téléphone"],
    "cf.": ["confer"],
    "env.": ["environ"],
    "c.-à-d.": ["c'est-à-dire"],
    "p. ex.": ["par exemple"],
    "J.-C.": ["Jésus-Christ"],
    "av. J.-C.": ["avant Jésus-Christ"],
    "apr. J.-C.": ["après Jésus-Christ"],
  },
  web: { dot: "point", at: "arobase", slash: "slash", hyphen: "tiret", underscore: "tiret bas",
    digits: ["zéro", "un", "deux", "trois", "quatre", "cinq", "six", "sept", "huit", "neuf"],
  },
  closing: new Set(["etc."]),
};
