import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { afterName, beforeNumber, type Readings } from "../../text/readings.ts";
import type { Keys } from "../../text/scan.ts";
import type { Forms, Joiners } from "../grammar.ts";

// Up to 29, each a single word
export const ONES = [
  "cero",       "uno",        "dos",         "tres",        "cuatro",       "cinco",
  "seis",       "siete",      "ocho",        "nueve",       "diez",         "once",
  "doce",       "trece",      "catorce",     "quince",      "dieciséis",    "diecisiete",
  "dieciocho",  "diecinueve", "veinte",      "veintiuno",   "veintidós",    "veintitrés",
  "veinticuatro", "veinticinco", "veintiséis", "veintisiete", "veintiocho", "veintinueve",
] as const;

export const TENS = [
  "", "", "", "treinta", "cuarenta", "cincuenta", "sesenta", "setenta", "ochenta", "noventa",
] as const;

export const HUNDREDS = [
  "",           "ciento",      "doscientos",  "trescientos", "cuatrocientos",
  "quinientos", "seiscientos", "setecientos", "ochocientos", "novecientos",
] as const;

// Ordinal stems, completed by -o or -a: centésima trigésima séptima
export const ORDINAL_ONES = [
  "", "primer", "segund", "tercer", "cuart", "quint", "sext", "séptim", "octav", "noven",
] as const;
export const ORDINAL_TENS = [
  "",           "décim",      "vigésim",     "trigésim",   "cuadragésim",
  "quincuagésim", "sexagésim", "septuagésim", "octogésim", "nonagésim",
] as const;
export const ORDINAL_HUNDREDS = [
  "",             "centésim",      "ducentésim",     "tricentésim",   "cuadringentésim",
  "quingentésim", "sexcentésim",   "septingentésim", "octingentésim", "noningentésim",
] as const;

export const MONTHS = [
  "enero", "febrero", "marzo",      "abril",   "mayo",      "junio",
  "julio", "agosto",  "septiembre", "octubre", "noviembre", "diciembre",
] as const;

export const CURRENCIES: Readonly<Record<string, Forms>> = {
  "$": { one: "dólar", many: "dólares" }, "US$": { one: "dólar", many: "dólares" }, "€": { one: "euro", many: "euros" },
  "£": { one: "libra", many: "libras" },
};

export const SCALES: Readonly<Record<string, string>> = {
  millones: "millones", millón: "millón", mil: "mil", k: "mil", K: "mil",
};

export const UNITS: Readonly<Record<string, Forms>> = {
  "km":   { one: "kilómetro", many: "kilómetros" },         "m":    { one: "metro", many: "metros" },
  "cm":   { one: "centímetro", many: "centímetros" },       "mm":   { one: "milímetro", many: "milímetros" },
  "nm":   { one: "nanómetro", many: "nanómetros" },         "m²":   { one: "metro cuadrado", many: "metros cuadrados" },
  "km²":  { one: "kilómetro cuadrado", many: "kilómetros cuadrados" }, "m³": { one: "metro cúbico", many: "metros cúbicos" },
  "ha":   { one: "hectárea", many: "hectáreas" },           "kg":   { one: "kilogramo", many: "kilogramos" },
  "g":    { one: "gramo", many: "gramos" },                 "mg":   { one: "miligramo", many: "miligramos" },
  "t":    { one: "tonelada", many: "toneladas" },           "l":    { one: "litro", many: "litros" },
  "L":    { one: "litro", many: "litros" },                 "ml":   { one: "mililitro", many: "mililitros" },
  "TB":   { one: "terabyte", many: "terabytes" },           "GB":   { one: "gigabyte", many: "gigabytes" },
  "MB":   { one: "megabyte", many: "megabytes" },           "KB":   { one: "kilobyte", many: "kilobytes" },
  "Hz":   { one: "hercio", many: "hercios" },               "kHz":  { one: "kilohercio", many: "kilohercios" },
  "MHz":  { one: "megahercio", many: "megahercios" },       "GHz":  { one: "gigahercio", many: "gigahercios" },
  "W":    { one: "vatio", many: "vatios" },                 "kW":   { one: "kilovatio", many: "kilovatios" },
  "V":    { one: "voltio", many: "voltios" },               "km/h": { one: "kilómetro por hora", many: "kilómetros por hora" },
  "MPH":  { one: "milla por hora", many: "millas por hora" }, "mph": { one: "milla por hora", many: "millas por hora" },
  "°C":   { one: "grado Celsius", many: "grados Celsius" }, "°":    { one: "grado", many: "grados" },
};

export const FRACTIONS: Fractions = {
  "1/2": "un medio", "1/3": "un tercio", "2/3": "dos tercios", "1/4": "un cuarto", "3/4": "tres cuartos",
};

// Words before the number of a law with its year: Ley 2/2018
export const LAWS: ReadonlySet<string> = new Set([
  "ley", "decreto", "legislativo", "sentencia", "resolución", "declaración", "orden", "acuerdo",
  "circular", "reglamento", "directiva", "constitucionalidad", "expediente", "norma", "instrucción",
]);

// Words before a time with a dot, which make it a time: a las 18.30
export const TIME_WORDS: ReadonlySet<string> = new Set(["las", "la", "a"]);

// Nouns in -a, -ión, -dad, ... are feminine unless listed below; other feminine nouns are listed
export const MASCULINE_IN_A: ReadonlySet<string> = new Set([
  "día", "mapa", "problema", "sistema", "programa", "tema", "idioma", "clima", "planeta", "poema",
  "dilema", "drama", "esquema", "sofá", "papá", "cometa", "fantasma", "panorama", "diagrama", "teorema",
]);
export const FEMININE_NOUNS: ReadonlySet<string> = new Set([
  "vez", "veces", "mano", "foto", "moto", "radio", "mujer", "flor", "noche", "clase", "parte",
  "gente", "leche", "muerte", "suerte", "calle", "carne", "red", "voz", "luz", "paz", "ley", "piel",
  "fase", "base", "frase", "nube", "llave", "fuente", "mente", "torre", "tarde", "serie", "especie",
  "superficie", "imagen", "orden", "edición",
]);
export const FEMININE_ENDINGS = ["a", "ión", "dad", "tad", "tud", "umbre", "sis", "itis"] as const;

// Words after a number which are not the noun it counts
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "de", "del", "en", "con", "por", "para", "sin", "que", "y", "e", "o", "u", "a", "al", "los",
  "las", "el", "la", "lo", "un", "una", "unos", "unas", "es", "son", "era", "sea", "pero", "como",
  "más", "menos", "hasta", "desde", "entre", "sobre", "contra", "hacia", "tras", "según", "durante",
  "mediante", "cada", "toda", "todas", "todo", "todos", "esta", "estas", "este", "estos", "esa",
  "esas", "ese", "esos", "otra", "otras", "otro", "otros", "nada", "ya", "ahora", "cuando", "donde",
  "mientras", "muy", "casi", "también", "así", "se", "le", "les", "me", "te", "nos",
]);

// Felipe VI (sexto), Alfonso XIII (trece), siglo XXI (veintiuno), capítulo IV (cuatro), la II Guerra (Segunda)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "felipe", "juan", "carlos", "alfonso", "fernando", "isabel", "enrique", "pedro", "jaime", "sancho", "luis",
    "benedicto", "pío", "pablo", "león", "gregorio", "clemente", "inocencio", "eduardo", "jorge", "guillermo",
    "napoleón", "ramsés", "juana", "maría", "catalina", "leonor", "victoria",
  ]),
  before: new Set([
    "capítulo", "tomo", "libro", "acto", "título", "parte", "volumen", "temporada", "fase", "siglo", "siglos",
    "vaticano", "escena", "episodio", "nivel", "número", "distrito", "región", "tipo", "clase",
  ]),
  after: new Set([
    "guerra", "congreso", "encuentro", "festival", "edición", "jornada", "jornadas", "bienal", "feria", "cumbre",
    "conferencia", "asamblea", "concurso", "foro", "simposio", "seminario", "campeonato", "olimpiada", "república",
    "legislatura", "aniversario", "centenario", "reunión", "convención", "gira", "coloquio", "censo", "plan",
    "informe", "premio", "torneo", "salón", "semana", "carrera", "marcha",
  ]),
};

// Names with a regnal number, and nouns after a numeral, which are feminine: Isabel II (segunda), la II Guerra
export const FEMININE_NAMES: ReadonlySet<string> = new Set([
  "isabel", "juana", "maría", "catalina", "leonor", "victoria",
]);


export const FEMININE_ROMAN_NOUNS: ReadonlySet<string> = new Set([
  "guerra", "edición", "jornada", "jornadas", "bienal", "feria", "cumbre", "conferencia", "asamblea", "olimpiada",
  "república", "legislatura", "reunión", "convención", "gira", "semana", "carrera", "marcha",
]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ".,", decimalSeparators: ".,", timeSeparators: ":.", timeSuffixes: ["h", "hrs"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "punto", dot: "punto", range: "-", by: "por", score: " a ", minus: "menos", plus: "más",
};

// 14/07/2023, 31/12/99
export const DATE_FORMAT = dateFormat();

// 600-123-456, 600 123 456 (after a word of PHONE_WORDS)
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: " ", boundaries: ".,-", closing: "-" }),
];

// Words before a phone number written with spaces, which else reads as numbers: llame al 600 123 456
export const PHONE_WORDS: ReadonlySet<string> = new Set([
  "al", "teléfono", "tel", "móvil", "número", "llame", "llamar", "fax", "whatsapp", "contacto",
]);

// 1º, 2 ª: a sign, possibly after a space
export const ORDINAL_SIGNS: Keys = ["º", "ª"];
export const ORDINAL_SIGN_FORMAT = suffixFormat({ space: true, wordEnd: false });

// 1er, 3ero, 2da, 5ta
export const ORDINAL_SUFFIXES: Keys = [
  "er", "ero", "era", "ro", "ra", "do", "da", "to", "ta", "vo", "va", "mo", "ma", "no", "na",
];

// 80s, 90's
export const DECADE_SUFFIXES: Keys = ["s"];
export const DECADE_FORMAT = suffixFormat({ apostrophes: true, boundaries: ".,-" });

// "+" is "plus" after a name (Google+), "más" otherwise (C++)
export const SYMBOLS: Readings = {
  "&": ["y"],       "=": ["igual a"],     "@": ["arroba"],  "%": ["por ciento"], "°": ["grados"],
  "#": ["numeral"], "²": ["al cuadrado"], "³": ["al cubo"], "π": ["pi"],         "×": ["por"],
  "€": ["euros"],   "$": ["dólares"],     "£": ["libras"],  "+": [{ words: "plus", when: afterName }, "más"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: Nº 5
    "Nº": [{ words: "número", when: beforeNumber }],
    "nº": [{ words: "número", when: beforeNumber }],
    "N°": [{ words: "número", when: beforeNumber }],
    "núm.": [{ words: "número", when: beforeNumber }],
    "pág.": [{ words: "página", when: beforeNumber }],
    "art.": [{ words: "artículo", when: beforeNumber }],
  },
  web: { dot: "punto", at: "arroba", slash: "barra" },
};
