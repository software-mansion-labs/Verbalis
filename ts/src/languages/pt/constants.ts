import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { beforeNumber, type Readings } from "../../text/readings.ts";
import type { Keys } from "../../text/scan.ts";
import type { Forms, Joiners } from "../grammar.ts";

// Brazilian forms: dezesseis, bilhão
export const ONES = [
  "zero",  "um",   "dois",  "três",  "quatro",  "cinco",  "seis",      "sete",      "oito",    "nove",
  "dez",   "onze", "doze",  "treze", "catorze", "quinze", "dezesseis", "dezessete", "dezoito", "dezenove",
] as const;

export const TENS = [
  "", "", "vinte", "trinta", "quarenta", "cinquenta", "sessenta", "setenta", "oitenta", "noventa",
] as const;

export const HUNDREDS = [
  "",           "cento",      "duzentos",   "trezentos",  "quatrocentos",
  "quinhentos", "seiscentos", "setecentos", "oitocentos", "novecentos",
] as const;

// Ordinal stems, completed by -o or -a: vigésima primeira
export const ORDINAL_ONES = [
  "", "primeir", "segund", "terceir", "quart", "quint", "sext", "sétim", "oitav", "non",
] as const;
export const ORDINAL_TENS = [
  "",             "décim",     "vigésim",     "trigésim",  "quadragésim",
  "quinquagésim", "sexagésim", "septuagésim", "octogésim", "nonagésim",
] as const;
export const ORDINAL_HUNDREDS = [
  "",             "centésim",    "ducentésim",     "trecentésim",   "quadringentésim",
  "quingentésim", "sexcentésim", "septingentésim", "octingentésim", "noningentésim",
] as const;

export const MONTHS: readonly string[] = [
  "janeiro", "fevereiro", "março",    "abril",   "maio",     "junho",
  "julho",   "agosto",    "setembro", "outubro", "novembro", "dezembro",
];

export const CURRENCIES: Readonly<Record<string, Forms>> = {
  "R$": { one: "real", many: "reais" }, "US$": { one: "dólar", many: "dólares" }, "$": { one: "dólar", many: "dólares" },
  "€": { one: "euro", many: "euros" },  "£": { one: "libra", many: "libras" },
};

// Full words are kept as written; abbreviations take the singular below 2
export const SCALES: Readonly<Record<string, Forms>> = {
  "mil": { one: "mil", many: "mil" },             "k": { one: "mil", many: "mil" },          "K": { one: "mil", many: "mil" },
  "milhão": { one: "milhão", many: "milhão" },    "milhões": { one: "milhões", many: "milhões" },
  "bilhão": { one: "bilhão", many: "bilhão" },    "bilhões": { one: "bilhões", many: "bilhões" },
  "trilhão": { one: "trilhão", many: "trilhão" }, "trilhões": { one: "trilhões", many: "trilhões" },
  "mi": { one: "milhão", many: "milhões" },       "bi": { one: "bilhão", many: "bilhões" },
  "tri": { one: "trilhão", many: "trilhões" },
};

export const UNITS: Readonly<Record<string, Forms>> = {
  "km":   { one: "quilômetro", many: "quilômetros" },        "m":    { one: "metro", many: "metros" },
  "cm":   { one: "centímetro", many: "centímetros" },        "mm":   { one: "milímetro", many: "milímetros" },
  "nm":   { one: "nanômetro", many: "nanômetros" },          "m²":   { one: "metro quadrado", many: "metros quadrados" },
  "km²":  { one: "quilômetro quadrado", many: "quilômetros quadrados" }, "m³": { one: "metro cúbico", many: "metros cúbicos" },
  "ha":   { one: "hectare", many: "hectares" },              "kg":   { one: "quilo", many: "quilos" },
  "g":    { one: "grama", many: "gramas" },                  "mg":   { one: "miligrama", many: "miligramas" },
  "t":    { one: "tonelada", many: "toneladas" },            "l":    { one: "litro", many: "litros" },
  "L":    { one: "litro", many: "litros" },                  "ml":   { one: "mililitro", many: "mililitros" },
  "mL":   { one: "mililitro", many: "mililitros" },          "TB":   { one: "terabyte", many: "terabytes" },
  "GB":   { one: "gigabyte", many: "gigabytes" },            "MB":   { one: "megabyte", many: "megabytes" },
  "KB":   { one: "kilobyte", many: "kilobytes" },            "Hz":   { one: "hertz", many: "hertz" },
  "kHz":  { one: "quilohertz", many: "quilohertz" },         "MHz":  { one: "megahertz", many: "megahertz" },
  "GHz":  { one: "gigahertz", many: "gigahertz" },           "W":    { one: "watt", many: "watts" },
  "kW":   { one: "quilowatt", many: "quilowatts" },          "kWh":  { one: "quilowatt-hora", many: "quilowatts-hora" },
  "V":    { one: "volt", many: "volts" },                    "km/h": { one: "quilômetro por hora", many: "quilômetros por hora" },
  "mph":  { one: "milha por hora", many: "milhas por hora" }, "°C":  { one: "grau Celsius", many: "graus Celsius" },
  "°":    { one: "grau", many: "graus" },
};

export const FRACTIONS: Fractions = {
  "1/2": "meio", "1/3": "um terço", "2/3": "dois terços", "1/4": "um quarto", "3/4": "três quartos",
};

// Scale words which take "de" before a currency: um milhão de reais, but dez mil reais
export const SCALE_WORDS = ["milhão", "milhões", "bilhão", "bilhões", "trilhão", "trilhões"] as const;

// Words before an hour, which make it feminine: às duas, das dez
export const HOUR_WORDS: ReadonlySet<string> = new Set(["às", "à", "das", "pelas"]);

// Parts of the day after "da": duas da tarde
export const DAY_PARTS: ReadonlySet<string> = new Set(["manhã", "tarde", "noite", "madrugada"]);

// Legal headings, after which an ordinal from 10 on reads as a cardinal: artigo 80º, artigo oitenta
export const HEADINGS: ReadonlySet<string> = new Set(["artigo", "art", "parágrafo", "capítulo", "inciso"]);

// Nouns in -a are feminine unless listed below; other feminine nouns are listed or end like one
export const MASCULINE_IN_A: ReadonlySet<string> = new Set([
  "dia", "mapa", "problema", "sistema", "programa", "tema", "idioma", "clima", "planeta", "poema",
  "cometa", "drama", "esquema", "sofá", "diagrama", "teorema", "telefonema", "cinema", "prisma",
  "pijama", "aroma", "dilema", "fantasma", "panorama", "enigma", "dogma", "carisma", "atleta", "guia",
  "pila",
]);
export const FEMININE_NOUNS: ReadonlySet<string> = new Set([
  "vez", "mão", "foto", "moto", "mulher", "flor", "noite", "tarde", "parte", "gente", "lei", "luz",
  "voz", "paz", "rede", "sede", "fase", "base", "frase", "classe", "chave", "fonte", "mente", "morte",
  "sorte", "ponte", "árvore", "nuvem", "ordem", "série", "espécie", "superfície", "cor", "dor", "mãe",
  "pele", "neve", "febre", "carne", "torre", "hora", "semana", "polegada", "lição",
]);
export const FEMININE_ENDINGS = ["a", "ção", "são", "dade", "tude", "gem", "ise", "ite", "ez"] as const;

// Words after a number which are not the noun it counts
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "de", "do", "da", "dos", "das", "em", "no", "na", "nos", "nas", "com", "por", "para", "sem", "que",
  "e", "ou", "a", "o", "os", "as", "um", "uma", "uns", "umas", "é", "são", "era", "foi", "mais",
  "menos", "até", "desde", "entre", "sobre", "contra", "após", "cada", "todo", "toda", "todos", "todas",
  "este", "esta", "estes", "estas", "esse", "essa", "esses", "essas", "isso", "isto", "já", "agora",
  "quando", "onde", "muito", "também", "se", "lhe", "me", "te", "ao", "à", "aos", "às", "pelo",
  "pela", "pelos", "pelas", "não", "sim", "meu", "minha", "seu", "sua",
]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ".", decimalSeparators: ",.", timeSeparators: ":h", timeSuffixes: ["h"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "ponto", dot: "ponto", range: " a ", by: "por", score: " a ", minus: "menos", plus: "mais",
};

// 24/03/2022, 07-11-2018
export const DATE_FORMAT = dateFormat({ separators: "/-", boundaries: ".,/-" });

// (11) 98765-4321, 0800-123456, 3456-7890, 213 456 789 (after a word of PHONE_WORDS): the longest first
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[2, 2], [4, 5], [4, 4]], parenthesized: true, closing: "-" }),
  phoneFormat({ groups: [[4, 5], [4, 6]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: " ", boundaries: ".,-", closing: "-" }),
];

// Words before a phone number written with spaces, which else reads as numbers: ligue para 213 456 789
export const PHONE_WORDS: ReadonlySet<string> = new Set([
  "para", "telefone", "tel", "ligue", "contato", "contacto", "número", "whatsapp", "fax",
]);

// 1º, 2 ª: a sign, possibly after a space
export const ORDINAL_SIGNS: Keys = ["º", "ª"];
export const ORDINAL_SIGN_FORMAT = suffixFormat({
  space: true,
  wordEnd: false,
});

export const SYMBOLS: Readings = {
  "&": ["e"],         "+": ["mais"],    "=": ["igual a"],     "@": ["arroba"],   "%": ["por cento"],
  "°": ["graus"],     "#": ["hashtag"], "²": ["ao quadrado"], "³": ["ao cubo"],  "π": ["pi"],
  "×": ["vezes"],     "€": ["euros"],   "$": ["dólares"],     "£": ["libras"],   "§": ["parágrafo"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: nº 5
    nº: [{ words: "número", when: beforeNumber }],
    Nº: [{ words: "número", when: beforeNumber }],
    "n.º": [{ words: "número", when: beforeNumber }],
    "N.º": [{ words: "número", when: beforeNumber }],
    "núm.": [{ words: "número", when: beforeNumber }],
    "art.": [{ words: "artigo", when: beforeNumber }],
    "Art.": [{ words: "artigo", when: beforeNumber }],
    "pág.": [{ words: "página", when: beforeNumber }],
    "§": [{ words: "parágrafo", when: beforeNumber }],
  },
  web: { dot: "ponto", at: "arroba", slash: "barra" },
};
