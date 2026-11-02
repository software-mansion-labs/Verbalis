import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Context } from "../../text/context.ts";
import { afterName, beforeName, beforeNumber, beforeValue, type Readings } from "../../text/readings.ts";
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

// Months by their names and abbreviations, lowercase: 5 febrero, abr. 22
export const MONTH_NUMBERS: ReadonlyMap<string, bigint> = new Map([
  ["enero", 1n],     ["febrero", 2n],  ["marzo", 3n],      ["abril", 4n],      ["mayo", 5n],   ["junio", 6n],
  ["julio", 7n],     ["agosto", 8n],   ["septiembre", 9n], ["setiembre", 9n],  ["octubre", 10n],
  ["noviembre", 11n], ["diciembre", 12n], ["ene.", 1n],    ["feb.", 2n],       ["mar.", 3n],   ["abr.", 4n],
  ["may.", 5n],      ["jun.", 6n],     ["jul.", 7n],       ["ago.", 8n],       ["sep.", 9n],   ["sept.", 9n],
  ["set.", 9n],      ["oct.", 10n],    ["nov.", 11n],      ["dic.", 12n],
]);

// Currencies by symbol and by code, as written: upper case, or lower case after a number (5 usd)
export const CURRENCIES: Readonly<Record<string, Forms>> = {
  "$": { one: "dólar", many: "dólares" },
  "€": { one: "euro", many: "euros" },
  "£": { one: "libra", many: "libras" },
  "¥": { one: "yen", many: "yenes" },
  "₩": { one: "won", many: "wones" },
  "₡": { one: "colón", many: "colones" },
  "₧": { one: "peseta", many: "pesetas" },
  "R$": { one: "real", many: "reales" },
  "r$": { one: "real", many: "reales" },
  "US$": { one: "dólar", many: "dólares" },
  "us$": { one: "dólar", many: "dólares" },
  USD: { one: "dólar estadounidense", many: "dólares estadounidenses" },
  usd: { one: "dólar estadounidense", many: "dólares estadounidenses" },
  "MEX$": { one: "peso mexicano", many: "pesos mexicanos" },
  "mex$": { one: "peso mexicano", many: "pesos mexicanos" },
  MXN: { one: "peso mexicano", many: "pesos mexicanos" },
  mxn: { one: "peso mexicano", many: "pesos mexicanos" },
  EUR: { one: "euro", many: "euros" },
  eur: { one: "euro", many: "euros" },
  GBP: { one: "libra", many: "libras" },
  JPY: { one: "yen", many: "yenes" },
  jpy: { one: "yen", many: "yenes" },
  CRC: { one: "colón costarricense", many: "colones costarricenses" },
  crc: { one: "colón costarricense", many: "colones costarricenses" },
  NIO: { one: "córdoba", many: "córdobas" },
  nio: { one: "córdoba", many: "córdobas" },
  PAB: { one: "balboa", many: "balboas" },
  pab: { one: "balboa", many: "balboas" },
  HNL: { one: "lempira", many: "lempiras" },
  hnl: { one: "lempira", many: "lempiras" },
  GTQ: { one: "quetzal", many: "quetzales" },
  gtq: { one: "quetzal", many: "quetzales" },
  Q: { one: "quetzal", many: "quetzales" },
  q: { one: "quetzal", many: "quetzales" },
  BRL: { one: "real brasileño", many: "reales brasileños" },
  brl: { one: "real brasileño", many: "reales brasileños" },
  BOP: { one: "peso boliviano", many: "pesos bolivianos" },
  bop: { one: "peso boliviano", many: "pesos bolivianos" },
  CHF: { one: "franco suizo", many: "francos suizos" },
  chf: { one: "franco suizo", many: "francos suizos" },
  HKD: { one: "dólar de Hong Kong", many: "dólares de Hong Kong" },
  hkd: { one: "dólar de Hong Kong", many: "dólares de Hong Kong" },
  TTD: { one: "dólar de Trinidad y Tobago", many: "dólares de Trinidad y Tobago" },
  ttd: { one: "dólar de Trinidad y Tobago", many: "dólares de Trinidad y Tobago" },
  AWG: { one: "florín arubeño", many: "florines arubeños" },
  awg: { one: "florín arubeño", many: "florines arubeños" },
  Rs: { one: "rupia", many: "rupias" },
  rs: { one: "rupia", many: "rupias" },
  kr: { one: "corona", many: "coronas" },
  mk: { one: "marco", many: "marcos" },
  Fr: { one: "franco", many: "francos" },
  fr: { one: "franco", many: "francos" },
  ptas: { one: "peseta", many: "pesetas" },
  pta: { one: "peseta", many: "pesetas" },
  Ptas: { one: "peseta", many: "pesetas" },
  "RD$": { one: "peso dominicano", many: "pesos dominicanos" },
  "U$S": { one: "dólar", many: "dólares" },
  "¢": { one: "colón", many: "colones" },
  "N$": { one: "nuevo peso", many: "nuevos pesos" },
  Bs: { one: "bolívar", many: "bolívares" },
  // Millions of pesos and of dollars, read as a currency: 2 mdp
  mdp: { one: "millón de pesos", many: "millones de pesos" },
  mdd: { one: "millón de dólares", many: "millones de dólares" },
};

// Currency codes which also precede an amount: USD 5, q50,10 (Q1 to Q9 are quarters)
export const CURRENCY_WORDS: Keys = [
  "USD", "usd", "EUR", "eur", "MXN", "mxn", "CRC", "crc", "NIO", "nio", "PAB", "pab", "HNL", "hnl", "GTQ", "gtq", "Q", "q", "BRL", "brl", "BOP", "bop", "CHF", "chf", "HKD", "hkd", "TTD", "ttd", "AWG", "awg", "Rs", "rs", "Fr", "fr", "JPY", "jpy", "GBP",
];

// The hundredth of a currency, read alone or after the units: $0,55, cincuenta y cinco centavos
export const CENTS: Readonly<Record<string, Forms>> = {
  "$": { one: "centavo", many: "centavos" },
  "US$": { one: "centavo", many: "centavos" },
  "us$": { one: "centavo", many: "centavos" },
  USD: { one: "centavo", many: "centavos" },
  usd: { one: "centavo", many: "centavos" },
  "MEX$": { one: "centavo", many: "centavos" },
  "mex$": { one: "centavo", many: "centavos" },
  MXN: { one: "centavo", many: "centavos" },
  mxn: { one: "centavo", many: "centavos" },
  "€": { one: "céntimo", many: "céntimos" },
  EUR: { one: "céntimo", many: "céntimos" },
  eur: { one: "céntimo", many: "céntimos" },
  "£": { one: "penique", many: "peniques" },
  GBP: { one: "penique", many: "peniques" },
  "₡": { one: "céntimo", many: "céntimos" },
  Q: { one: "centavo", many: "centavos" },
  q: { one: "centavo", many: "centavos" },
  GTQ: { one: "centavo", many: "centavos" },
  gtq: { one: "centavo", many: "centavos" },
  "R$": { one: "centavo", many: "centavos" },
  "r$": { one: "centavo", many: "centavos" },
};

// An inch after a fraction: 3/4"
export const INCH_MARKS: Keys = ['"', "''", "″", "”"];
export const INCH: Forms = { one: "pulgada", many: "pulgadas" };

// Words of a text about time, which make its 7'11" minutes and seconds, and a lone 25″ seconds
export const PRIME_CUES: ReadonlySet<string> = new Set([
  "tiempo", "carrera", "crono", "cronómetro", "récord", "ganador", "general", "etapa", "ventaja", "vuelta",
  "minuto", "minutos", "segundos", "maratón", "clasificación", "podio",
]);

// Words around a lone 10' which make it minutes: durante más de 10', (36' PT), (49' ST)
export const MINUTE_CUES: ReadonlySet<string> = new Set(["durante", "pt", "st"]);

// Words around a 5'5" which make it feet and inches: mide 5'5"
export const HEIGHT_CUES: ReadonlySet<string> = new Set(["mide", "altura", "estatura", "alto", "alta"]);
export const FOOT: Forms = { one: "pie", many: "pies" };

// A time written with prime marks: hours, minutes, and seconds
export const PRIME_UNITS: readonly Forms[] = [
  { one: "hora", many: "horas" }, { one: "minuto", many: "minutos" }, { one: "segundo", many: "segundos" },
];

// Each scale by its singular: millones -> millón
export const SCALES: Readonly<Record<string, string>> = {
  millones: "millón", millón: "millón", billones: "billón", billón: "billón",
  mil: "mil",         k: "mil",         K: "mil",         mn: "millón",
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
  "°C":   { one: "grado Celsius", many: "grados Celsius" }, "°F": { one: "grado Fahrenheit", many: "grados Fahrenheit" }, "°":    { one: "grado", many: "grados" },
  "° C": { one: "grado Celsius", many: "grados Celsius" }, "° F": { one: "grado Fahrenheit", many: "grados Fahrenheit" },
  "°c": { one: "grado centígrado", many: "grados centígrados" }, "° c": { one: "grado centígrado", many: "grados centígrados" },
  K: { one: "kelvin", many: "kelvin" },                 "°K": { one: "kelvin", many: "kelvin" },  "° K": { one: "kelvin", many: "kelvin" },
  "°k": { one: "grado kelvin", many: "grados kelvin" },   "° k": { one: "grado kelvin", many: "grados kelvin" },
  s: { one: "segundo", many: "segundos" },              seg: { one: "segundo", many: "segundos" },
  min: { one: "minuto", many: "minutos" },              "/s": { one: "por segundo", many: "por segundo" },
  kph: { one: "kilómetro por hora", many: "kilómetros por hora" }, psi: { one: "psi", many: "psi" },
  rpm: { one: "revolución por minuto", many: "revoluciones por minuto" },
  Km: { one: "kilómetro", many: "kilómetros" },         mts: { one: "metro", many: "metros" },
  cms: { one: "centímetro", many: "centímetros" },      m2: { one: "metro cuadrado", many: "metros cuadrados" },
  oz: { one: "onza", many: "onzas" },                   kWh: { one: "kilovatio hora", many: "kilovatios hora" },
  hp: { one: "caballo de fuerza", many: "caballos de fuerza" }, "℃": { one: "grado centígrado", many: "grados centígrados" },
  gb: { one: "gigabyte", many: "gigabytes" },           px: { one: "píxel", many: "píxeles" },
  MP: { one: "megapíxel", many: "megapíxeles" },        Mpx: { one: "megapíxel", many: "megapíxeles" },
  pts: { one: "punto", many: "puntos" },
};

export const FRACTION_WORDS: ReadonlySet<string> = new Set(["de", "del"]);

// Words before a day and month which mark them as a date: el 3/5, del 3/5 al 7/5 (a bare 3/5 is a fraction)
export const DATE_WORDS: ReadonlySet<string> = new Set([
  "el", "del", "al", "desde", "hasta", "día", "fecha", "lunes", "martes", "miércoles", "jueves",
  "viernes", "sábado", "domingo",
]);

// Words before the number of a law with its year: Ley 2/2018
export const LAWS: ReadonlySet<string> = new Set([
  "ley", "decreto", "legislativo", "sentencia", "resolución", "declaración", "orden", "acuerdo",
  "circular", "reglamento", "directiva", "constitucionalidad", "expediente", "norma", "instrucción",
]);

// Words before a time with a dot, which make it a time: a las 18.30 (and "a" before a quarter hour: de 9.00 a 13.30)
export const TIME_WORDS: ReadonlySet<string> = new Set(["las", "la"]);

// After a time: 19 h, 16:00 hrs. (horas), 5:02 p. m. (de la tarde)
export const HOUR_SUFFIXES: Keys = ["h", "h.", "hrs", "hrs.", "Hrs", "Hrs.", "hs", "hs."];
export const MORNING_SUFFIXES: Keys = ["a. m.", "a.m.", "a.m", "am"];
export const EVENING_SUFFIXES: Keys = ["p. m.", "p.m.", "p.m", "pm"];

// Nouns in -a, -ión, -dad, ... are feminine unless listed below; other feminine nouns are listed
export const MASCULINE_IN_A: ReadonlySet<string> = new Set([
  "día", "dia", "mapa", "problema", "sistema", "programa", "tema", "idioma", "clima", "planeta", "poema",
  "dilema", "drama", "esquema", "sofá", "papá", "cometa", "fantasma", "panorama", "diagrama", "teorema",
  "córdoba", "lempira", "balboa",
]);
export const FEMININE_NOUNS: ReadonlySet<string> = new Set([
  "vez", "veces", "mano", "foto", "moto", "radio", "mujer", "flor", "noche", "clase", "parte",
  "gente", "leche", "muerte", "suerte", "calle", "carne", "red", "voz", "luz", "paz", "ley", "piel",
  "fase", "base", "frase", "nube", "llave", "fuente", "mente", "torre", "tarde", "serie", "especie",
  "superficie", "imagen", "orden", "edición",
]);
export const FEMININE_ENDINGS = ["a", "ión", "dad", "tad", "tud", "umbre", "sis", "itis"] as const;

// Words before a year: en 1971 publicó
export const YEAR_WORDS: ReadonlySet<string> = new Set(["en", "de", "del", "desde", "hasta", "año"]);

// Words after a number which are not the noun it counts
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "de", "del", "en", "con", "por", "para", "sin", "que", "y", "e", "o", "u", "a", "al", "los",
  "las", "el", "la", "lo", "un", "una", "unos", "unas", "es", "son", "era", "sea", "pero", "como",
  "más", "menos", "hasta", "desde", "entre", "sobre", "contra", "hacia", "tras", "según", "durante",
  "mediante", "cada", "toda", "todas", "todo", "todos", "esta", "estas", "este", "estos", "esa",
  "esas", "ese", "esos", "otra", "otras", "otro", "otros", "nada", "ya", "ahora", "cuando", "donde",
  "mientras", "muy", "casi", "también", "así", "se", "le", "les", "me", "te", "nos", "esquina",
]);

// Felipe VI (sexto), Alfonso XIII (trece), siglo XXI (veintiuno), capítulo IV (cuatro), la II Guerra (Segunda)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "felipe", "juan", "carlos", "alfonso", "fernando", "isabel", "enrique", "pedro", "jaime", "sancho", "luis",
    "benedicto", "pío", "pablo", "león", "gregorio", "clemente", "inocencio", "eduardo", "jorge", "guillermo",
    "napoleón", "ramsés", "juana", "maría", "catalina", "leonor", "victoria",
  ]),
  before: new Set([
    "capítulo", "tomo", "libro", "acto", "título", "parte", "volumen", "temporada", "fase", "siglo", "año", "siglos",
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
// What an amount is counted by after a slash (por): 12 €/kg, doce euros por kilogramo
export const PER: Readonly<Record<string, string>> = {
  h: "por hora", hora: "por hora", min: "por minuto", s: "por segundo", seg: "por segundo", "día": "por día",
  semana: "por semana", mes: "por mes", "año": "por año", a: "por año", kg: "por kilogramo", g: "por gramo",
  t: "por tonelada", l: "por litro", L: "por litro", ml: "por mililitro", km: "por kilómetro", m: "por metro",
  "m²": "por metro cuadrado", m2: "por metro cuadrado", "m³": "por metro cúbico", m3: "por metro cúbico",
  ha: "por hectárea", kWh: "por kilovatio hora", MWh: "por megavatio hora", oz: "por onza", persona: "por persona",
};

export const NOTATION = notation({
  groupSeparators: "., \u00a0\u202f", decimalSeparators: ".,", timeSeparators: ":.",
  timeSuffixes: ["h", "h.", "hrs", "hrs.", "Hrs", "Hrs.", "hs", "hs.", "a. m.", "a.m.", "a.m", "am",
    "p. m.", "p.m.", "p.m", "pm"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true, per: PER,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "punto", dot: "punto", range: "-", by: "por", score: " a ", minus: "menos", plus: "más",
  fraction: " y ", half: " y medio", rating: "sobre",
};

// Starts of words of sport, which make a falling or tied range a score: ganó 2-0, empató 1-1 (dos a cero, uno a uno)
export const SPORT_WORDS: Keys = [
  "partido", "gol", "marcador", "ganó", "ganar", "ganaron", "venci", "vencer", "empat", "derrot",
  "victoria", "triunf", "impus", "impone", "cayó", "superó", "encuentro", "jornada", "torneo", "liga",
  "selección", "canasta", "anotación", "clasificatoria", "duelo", "fútbol",
];

// 14/07/2023, 31/12/99
export const DATE_FORMAT = dateFormat({ separators: "/-.", boundaries: ".,/-", yearRequired: false, months: MONTHS });

// 600-123-456, 600 123 456 (after a word of PHONE_WORDS)
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: " ", boundaries: ".,-", closing: "-" }),
  // 123-123-5678, 1234-5678: Latin American numbers
  phoneFormat({ groups: [[3, 3], [3, 3], [4, 4]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[4, 4], [4, 4]], separators: "-", boundaries: ".,-/", closing: "-/" }),
];

// Words before a phone number written with spaces, which else reads as numbers: llame al 600 123 456
export const PHONE_WORDS: ReadonlySet<string> = new Set([
  "al", "teléfono", "tel", "móvil", "número", "llame", "llamar", "fax", "whatsapp", "contacto",
]);

// 1º, 2 ª, 4.º, 1.ª, 1.ᵉʳ, 1.°: a sign, possibly after a space or a dot
export const ORDINAL_SIGNS: Keys = ["º", "ª", ".º", ".ª", "ᵉʳ", ".ᵉʳ", ".°", "°"];

// Articles before 2° which make it an ordinal (el 2° lugar), not degrees (a 30°), and the feminine ones (la 6°)
export const ORDINAL_ARTICLES: ReadonlySet<string> = new Set([
  "el", "la", "del", "al", "su", "este", "esta", "nuestro",
  "nuestra",
]);
export const FEMININE_ORDINAL_ARTICLES: ReadonlySet<string> = new Set([
  "la", "esta", "nuestra",
]);
// Nouns after 2° which make it an ordinal: 2° lugar, 1° piso
export const ORDINAL_NOUNS: ReadonlySet<string> = new Set([
  "lugar", "puesto", "piso", "fecha", "aniversario", "edición", "festival", "congreso",
  "concurso", "encuentro", "jornada", "bono", "año", "semestre", "trimestre", "cuatrimestre",
  "bimestre", "nivel", "torneo", "campeonato", "capítulo", "lote", "premio", "etapa",
  "vuelta", "ronda", "sección", "distrito", "juzgado", "batallón", "regimiento", "grado",
  "día", "semana", "período", "periodo", "ciclo", "curso", "tiempo", "round",
  "foro", "seminario", "simposio", "salón", "certamen", "convención", "cumbre", "sesión",
  "división", "informe",
]);

export const ORDINAL_SIGN_FORMAT = suffixFormat({ space: true, wordEnd: false });

// 1er, 1.er, 3ero, 2da, 5ta
export const ORDINAL_SUFFIXES: Keys = [
  "er", ".er", "ero", "era", "ro", "ra", "do", "da", "to", "ta", "vo", "va", "mo", "ma", "no", "na",
];

// 80s, 90's
export const DECADE_SUFFIXES: Keys = ["s"];
export const DECADE_FORMAT = suffixFormat({ apostrophes: true, boundaries: ".,-" });

// "+" is "plus" after a name (Google+), "más" otherwise (C++)
export const SYMBOLS: Readings = {
  "&": ["y"],       "=": ["igual a"],     "@": ["arroba"],  "%": ["por ciento"], "°": ["grados"],
  "#": ["numeral"], "²": ["al cuadrado"], "³": ["al cubo"], "π": ["pi"],         "×": ["por"],
  "€": ["euros"],   "$": ["dólares"],     "£": ["libras"],  "+": [{ words: "plus", when: afterName }, "más"],
  "⅓": ["un tercio"],   "⅔": ["dos tercios"],   "¼": ["un cuarto"],
  "¾": ["tres cuartos"], "⅕": ["un quinto"], "⅖": ["dos quintos"], "⅗": ["tres quintos"],
  "⅘": ["cuatro quintos"], "⅙": ["un sexto"], "⅚": ["cinco sextos"], "⅛": ["un octavo"],
  "⅜": ["tres octavos"], "⅝": ["cinco octavos"], "⅞": ["siete octavos"],
  "±": ["más menos"],    "+/-": ["más menos"],   "‰": ["por mil"],
  // Only before a number: p<0.05, ~$145, but <i>, ->, Año~
  "<": [{ words: "menor que", when: beforeValue }],
  ">": [{ words: "mayor que", when: beforeValue }],
  "≤": [{ words: "menor o igual que", when: beforeValue }],
  "≥": [{ words: "mayor o igual que", when: beforeValue }],
  "~": [{ words: "aproximadamente", when: beforeValue }],
};

// After a year or a century, as digits or a Roman numeral: 10.000 a. C., siglo x ec
function afterYear(context: Context): boolean {
  const word = context.before();
  return /^[0-9.]+$/.test(word) || /^[ivxlcdm]+$/.test(word);
}

// After a clock time: 1.00 est, 2:00 pm gmt-4:30, 10 h utc+1:30
const TIME_SUFFIXES_BEFORE_ZONE: ReadonlySet<string> = new Set(["am", "pm", "a.m", "p.m", "a.m.", "p.m.", "h", "hrs", "horas"]);

function afterTime(context: Context): boolean {
  const word = context.before();
  return (word !== "" && /^[0-9.:]+$/.test(word)) || TIME_SUFFIXES_BEFORE_ZONE.has(word);
}

// Time zones, in capitals after a clock time, with the sign of an offset: gmt-3 GMT menos tres
export const TIME_ZONES: readonly string[] = ["gmt", "utc", "est", "pst", "cst", "mst", "edt", "pdt", "cet", "cest"];

// At the start of a sentence: da. ana pérez (not the verb of "se lo da.")
function sentenceStart(context: Context): boolean {
  const head = context.head();
  let end = head.length - 1;
  while (end >= 0 && head[end] === " ") {
    end--;
  }
  return end < 0 || ".!?:;".includes(head[end]);
}

// Before a name or at the start of a sentence: Da. Ana
function beforeTitle(context: Context): boolean {
  return beforeName(context) || sentenceStart(context);
}

const TERMS_BASE: Terms = {
  abbreviations: {
    // Before a number only: Nº 5
    "Nº": [{ words: "número", when: beforeNumber }],
    "ext.": [{ words: "extensión", when: beforeNumber }],
    "Ext.": [{ words: "extensión", when: beforeNumber }],
    "nº": [{ words: "número", when: beforeNumber }],
    "N°": [{ words: "número", when: beforeNumber }],
    "n°": [{ words: "número", when: beforeNumber }],
    "n.°": [{ words: "número", when: beforeNumber }],
    "N.°": [{ words: "número", when: beforeNumber }],
    "n.º": [{ words: "número", when: beforeNumber }],
    "N.º": [{ words: "número", when: beforeNumber }],
    "núm.": [{ words: "número", when: beforeNumber }],
    "pág.": [{ words: "página", when: beforeNumber }],
    "art.": [{ words: "artículo", when: beforeNumber }],
    "Núm.": [{ words: "número", when: beforeNumber }],
    "Pág.": [{ words: "página", when: beforeNumber }],
    pp: [{ words: "páginas", when: beforeNumber }],
    "Km.": [{ words: "kilómetro", when: beforeNumber }],
    "Tel.": [{ words: "teléfono", when: beforeNumber }],
    "No.": [{ words: "número", when: beforeNumber }],
    "p.": [{ words: "página", when: beforeNumber }],
    "Av.": [{ words: "avenida", when: beforeName }, { words: "avenida", when: beforeNumber }],
    "Col.": [{ words: "colonia", when: beforeName }],
    // Before a day: vier. 10 de mayo
    "lun.": [{ words: "lunes", when: beforeNumber }],
    "mié.": [{ words: "miércoles", when: beforeNumber }],
    "miér.": [{ words: "miércoles", when: beforeNumber }],
    "jue.": [{ words: "jueves", when: beforeNumber }],
    "vie.": [{ words: "viernes", when: beforeNumber }],
    "vier.": [{ words: "viernes", when: beforeNumber }],
    "sáb.": [{ words: "sábado", when: beforeNumber }],
    "dom.": [{ words: "domingo", when: beforeNumber }],
    // After a year: 10.000 a. C., siglo X e. c.
    "a. C.": [{ words: "antes de Cristo", when: afterYear }],
    "a.C.": [{ words: "antes de Cristo", when: afterYear }],
    "a. c.": [{ words: "antes de Cristo", when: afterYear }],
    "a.c.": [{ words: "antes de Cristo", when: afterYear }],
    "d. C.": [{ words: "después de Cristo", when: afterYear }],
    "d.C.": [{ words: "después de Cristo", when: afterYear }],
    "d. c.": [{ words: "después de Cristo", when: afterYear }],
    "a. e. c.": [{ words: "antes de la era común", when: afterYear }],
    "e. c.": [{ words: "era común", when: afterYear }],
    ec: [{ words: "era común", when: afterYear }],
    // Titles: Da. Ana
    "Da.": [{ words: "doña", when: beforeTitle }],
    "da.": [{ words: "doña", when: sentenceStart }],
    // Anywhere
    "Sr.": ["señor"],
    "sr.": ["señor"],
    "Sra.": ["señora"],
    "sra.": ["señora"],
    "Srta.": ["señorita"],
    "Dr.": ["doctor"],
    "dr.": ["doctor"],
    "Dra.": ["doctora"],
    "dra.": ["doctora"],
    "Ud.": ["usted"],
    "Uds.": ["ustedes"],
    "Ing.": ["ingeniero"],
    "Prof.": ["profesor"],
    "Avda.": ["avenida"],
    "calz.": ["calzada"],
    "pp.": ["páginas"],
    "esq.": ["esquina"],
    "pza.": ["plaza"],
    "Pza.": ["plaza"],
    "c.º b.": ["camino bajo"],
    "EE. UU.": ["Estados Unidos"],
    "EE.UU.": ["Estados Unidos"],
    "ee. uu.": ["Estados Unidos"],
    "etc.": ["etcétera"],
    "aprox.": ["aproximadamente"],
  },
  web: { dot: "punto", at: "arroba", slash: "barra", hyphen: "guion", underscore: "guion bajo",
    digits: ["cero", "uno", "dos", "tres", "cuatro", "cinco", "seis", "siete", "ocho", "nueve"],
  },
  closing: new Set(["etc."]),
};

export const TERMS: Terms = (() => {
  const abbreviations: Record<string, Readings[string]> = { ...TERMS_BASE.abbreviations };
  for (const zone of TIME_ZONES) {
    const upper = zone.toUpperCase();
    for (const form of [zone, upper]) {
      abbreviations[form] = [{ words: upper, when: afterTime }];
      abbreviations[`${form}-`] = [{ words: `${upper} menos`, when: afterTime }];
      abbreviations[`${form}+`] = [{ words: `${upper} más`, when: afterTime }];
    }
  }
  return { ...TERMS_BASE, abbreviations };
})();
