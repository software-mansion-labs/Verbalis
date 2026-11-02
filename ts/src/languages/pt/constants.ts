import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { beforeName, beforeNumber, beforeValue, type Readings } from "../../text/readings.ts";
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
  "€": { one: "euro", many: "euros" },  "£": { one: "libra", many: "libras" }, EUR: { one: "euro", many: "euros" },
  // A currency written as a word after a number, read as a decimal: 66,23 euros
  euros: { one: "euro", many: "euros" },
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
  "mph":  { one: "milha por hora", many: "milhas por hora" }, "°C":  { one: "grau Celsius", many: "graus Celsius" }, "°F": { one: "grau Fahrenheit", many: "graus Fahrenheit" },
  "°":    { one: "grau", many: "graus" },                    "min":  { one: "minuto", many: "minutos" },
  "s":    { one: "segundo", many: "segundos" },              "mmol": { one: "milimole", many: "milimoles" },
  "Kb":   { one: "quilobyte", many: "quilobytes" },          "ton":  { one: "tonelada", many: "toneladas" },
  "kgs":  { one: "quilo", many: "quilos" },                  "Km":   { one: "quilômetro", many: "quilômetros" },
  "cv":   { one: "cavalo", many: "cavalos" },
};

export const FRACTIONS: Fractions = {
  "1/2": "meio", "1/3": "um terço", "2/3": "dois terços", "1/4": "um quarto", "3/4": "três quartos",
};

// Words after a fraction which mark it as one: 1/3 da
export const FRACTION_WORDS: ReadonlySet<string> = new Set(["de", "do", "da", "dos", "das"]);

// Words before a day and a month without a year which make them a date: dia 15/9, quinta-feira (16/3)
export const DATE_WORDS: ReadonlySet<string> = new Set([
  "dia", "dias", "em", "até", "desde", "hoje", "ontem", "amanhã", "data", "sábado", "domingo",
]);

// Words before a number and a year after a slash, which is not a fraction: Lei 15/98, nº 913/2010
export const LAW_WORDS: ReadonlySet<string> = new Set([
  "lei", "decreto", "decreto-lei", "portaria", "processo", "proc", "resolução", "despacho", "emenda",
  "parecer", "acórdão", "número", "nº", "n.º",
]);

// Scale words which take "de" before a currency: um milhão de reais, but dez mil reais
export const SCALE_WORDS = ["milhão", "milhões", "bilhão", "bilhões", "trilhão", "trilhões"] as const;

// Words of a text about time, which make its 2'14" minutes and seconds, and a lone 25″ seconds
export const PRIME_CUES: ReadonlySet<string> = new Set([
  "tempo", "corrida", "prova", "cronómetro", "cronômetro", "recorde", "vantagem", "etapa", "classificação",
  "maratona", "minutos", "segundos", "pódio", "vídeo", "filme", "duração",
]);

// Words around a lone 10' which make it minutes: aos 6′ do segundo tempo
export const MINUTE_CUES: ReadonlySet<string> = new Set(["tempo"]);

// A time written with prime marks: hours, minutes, seconds, and tenths to thousandths of a second; and an inch
export const PRIME_UNITS: readonly Forms[] = [
  { one: "hora", many: "horas" }, { one: "minuto", many: "minutos" }, { one: "segundo", many: "segundos" },
];
export const SECOND_FRACTIONS: readonly Forms[] = [
  { one: "décimo", many: "décimos" }, { one: "centésimo", many: "centésimos" }, { one: "milésimo", many: "milésimos" },
];
export const INCH: Forms = { one: "polegada", many: "polegadas" };

// Words around a 5'5" which make it feet and inches: ele mede 5'5"
export const HEIGHT_CUES: ReadonlySet<string> = new Set(["mede", "altura", "alto", "alta"]);
export const FOOT: Forms = { one: "pé", many: "pés" };

// Words before an hour, which make it feminine: às duas, das dez
export const HOUR_WORDS: ReadonlySet<string> = new Set(["às", "à", "as", "das", "pelas"]);

// Parts of the day after "da": duas da tarde
export const DAY_PARTS: ReadonlySet<string> = new Set(["manhã", "tarde", "noite", "madrugada"]);

// Legal headings, after which an ordinal from 10 on reads as a cardinal: artigo 80º, artigo oitenta
export const HEADINGS: ReadonlySet<string> = new Set(["artigo", "art", "parágrafo", "capítulo", "inciso"]);

// Nouns in -a are feminine unless listed below; other feminine nouns are listed or end like one
export const MASCULINE_IN_A: ReadonlySet<string> = new Set([
  "dia", "mapa", "problema", "sistema", "programa", "tema", "idioma", "clima", "planeta", "poema",
  "cometa", "drama", "esquema", "sofá", "diagrama", "teorema", "telefonema", "cinema", "prisma",
  "pijama", "aroma", "dilema", "fantasma", "panorama", "enigma", "dogma", "carisma", "atleta", "guia",
  "pila", "grama", "quilograma", "miligrama", "micrograma", "motorista", "regista", "jornalista", "turista",
  "artista", "ciclista", "especialista", "analista", "economista",
]);
export const FEMININE_NOUNS: ReadonlySet<string> = new Set([
  "vez", "mão", "foto", "moto", "mulher", "flor", "noite", "tarde", "parte", "gente", "lei", "luz",
  "voz", "paz", "rede", "sede", "fase", "base", "frase", "classe", "chave", "fonte", "mente", "morte",
  "sorte", "ponte", "árvore", "nuvem", "ordem", "série", "espécie", "superfície", "cor", "dor", "mãe",
  "pele", "neve", "febre", "carne", "torre", "hora", "semana", "polegada", "lição", "colher",
]);
export const FEMININE_ENDINGS = ["a", "ã", "ção", "são", "dade", "tude", "gem", "ise", "ite", "ez"] as const;

// Words before a year, which agrees with no noun after it: em 2020 havia
export const YEAR_WORDS: ReadonlySet<string> = new Set(["em", "de", "desde", "até", "ano"]);

// Words after a number which are not the noun it counts
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "de", "do", "da", "dos", "das", "em", "no", "na", "nos", "nas", "com", "por", "para", "sem", "que",
  "e", "ou", "a", "o", "os", "as", "um", "uma", "uns", "umas", "é", "são", "era", "foi", "mais",
  "menos", "até", "desde", "entre", "sobre", "contra", "após", "cada", "todo", "toda", "todos", "todas",
  "este", "esta", "estes", "estas", "esse", "essa", "esses", "essas", "isso", "isto", "já", "agora",
  "quando", "onde", "muito", "também", "se", "lhe", "me", "te", "ao", "à", "aos", "às", "pelo",
  "pela", "pelos", "pelas", "não", "sim", "meu", "minha", "seu", "sua",
  "nada", "ainda", "apenas", "embora", "fora", "cerca",
]);

// D. João Iº: an ordinal marker on a Roman numeral
export const ROMAN_MARKERS: Keys = ["º", "ª", "°"];

// D. Pedro II (segundo), Bento XVI (dezesseis), século XXI (vinte e um), capítulo IV (quatro), a II Guerra (Segunda)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "pedro", "joão", "paulo", "bento", "leão", "pio", "manuel", "afonso", "sebastião", "josé", "luís", "carlos",
    "filipe", "henrique", "dinis", "maria", "isabel", "jorge", "guilherme", "gregório", "clemente", "napoleão",
    "fernando", "duarte", "sancho", "eduardo", "elizabeth", "beatriz", "leonor", "teresa",
  ]),
  before: new Set([
    "capítulo", "tomo", "livro", "ato", "título", "parte", "volume", "temporada", "fase", "século", "séculos",
    "vaticano", "cena", "episódio", "nível", "tipo", "classe",
  ]),
  after: new Set([
    "guerra", "congresso", "encontro", "festival", "edição", "bienal", "feira", "cimeira", "conferência", "jornada",
    "jornadas", "seminário", "simpósio", "fórum", "campeonato", "república", "olimpíada", "assembleia",
    "aniversário", "centenário", "reunião", "plano", "prémio", "prêmio", "torneio", "semana", "mostra", "corrida",
    "marcha", "salão", "colóquio",
  ]),
};

// Names with a regnal number, and nouns after a numeral, which are feminine: D. Maria II (segunda), a II Guerra
export const FEMININE_NAMES: ReadonlySet<string> = new Set([
  "maria", "isabel", "elizabeth", "beatriz", "leonor", "teresa",
]);


export const FEMININE_ROMAN_NOUNS: ReadonlySet<string> = new Set([
  "guerra", "edição", "bienal", "feira", "cimeira", "conferência", "jornada", "jornadas", "república", "olimpíada",
  "assembleia", "reunião", "semana", "mostra", "corrida", "marcha",
]);

// --- Notation: how numbers are written ---
// What an amount is counted by after a slash (por): 12 €/kg, doze euros por quilo
export const PER: Readonly<Record<string, string>> = {
  h: "por hora", hora: "por hora", min: "por minuto", minuto: "por minuto", s: "por segundo", dia: "por dia",
  semana: "por semana", "mês": "por mês", ano: "por ano", kg: "por quilo", g: "por grama", t: "por tonelada",
  ton: "por tonelada", l: "por litro", L: "por litro", ml: "por mililitro", mL: "por mililitro", km: "por quilômetro",
  m: "por metro", "m²": "por metro quadrado", m2: "por metro quadrado", "m³": "por metro cúbico",
  m3: "por metro cúbico", ha: "por hectare", kWh: "por quilowatt-hora", MWh: "por megawatt-hora",
  unidade: "por unidade",
};

export const NOTATION = notation({
  groupSeparators: ". \u00a0\u202f", decimalSeparators: ",.", timeSeparators: ":hH.", timeSuffixes: ["h", "hs"],
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true, per: PER,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "ponto", dot: "ponto", range: " a ", by: "por", score: " a ", minus: "menos", plus: "mais",
  fraction: " e ", rating: "de",
};

// Starts of words of sport, which make a falling or tied range a score: venceu 5-2, empate 1-1 (cinco a dois)
export const SPORT_WORDS: Keys = [
  "jogo", "gol", "placar", "venceu", "vencer", "venceram", "vitória", "derrot", "empat", "partida",
  "campeonato", "rodada", "jornada", "futebol", "triunf", "pontos",
];

// Starts of words of sets, which make a slashed pair a score: parciais de 3/6, 6/2 e 7/6 (três a seis)
export const SET_WORDS: Keys = ["parcia", "sets"];

// 24/03/2022, 07-11-2018
export const DATE_FORMAT = dateFormat({ separators: "/-", boundaries: ".,/-", yearRequired: false, months: MONTHS });

// 15.03.2024: with dots, only with a full year
export const DOT_DATE_FORMAT = dateFormat({ separators: ".", boundaries: ".,/-", shortYear: false, months: MONTHS });

// (11) 98765-4321, 0800-123456, 3456-7890, 213 456 789 (after a word of PHONE_WORDS): the longest first
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[2, 2], [4, 5], [4, 4]], parenthesized: true, closing: "-" }),
  phoneFormat({ groups: [[2, 2], [4, 5], [4, 4]], separators: " -", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[4, 5], [4, 7]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: " ", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[3, 3], [4, 4]], separators: "-", boundaries: ".,-", closing: "-" }),
];

// Words before a phone number written with spaces, which else reads as numbers: ligue para 213 456 789
export const PHONE_WORDS: ReadonlySet<string> = new Set([
  "para", "telefone", "tel", "ligue", "contato", "contacto", "número", "whatsapp", "fax",
]);

// 1º, 2 ª, 3°, 24.º: a sign, possibly after a space; 3° is an ordinal only by its words
export const ORDINAL_SIGNS: Keys = ["º", "ª", "°", ".º", ".ª", ".°"];

// Articles before 2° which make it an ordinal (o 2° lugar), not degrees (a 30°), and the feminine ones (na 5°)
export const ORDINAL_ARTICLES: ReadonlySet<string> = new Set([
  "o", "do", "no", "ao", "pelo", "à", "da", "na",
  "pela", "seu", "sua", "este", "esta", "nosso", "nossa",
]);
export const FEMININE_ORDINAL_ARTICLES: ReadonlySet<string> = new Set([
  "à", "da", "na", "pela", "sua", "esta", "nossa",
]);
// Nouns after 2° which make it an ordinal: 2° lugar, 1° andar
export const ORDINAL_NOUNS: ReadonlySet<string> = new Set([
  "lugar", "posto", "andar", "aniversário", "edição", "festival", "congresso", "concurso",
  "encontro", "jornada", "ano", "semestre", "trimestre", "bimestre", "nível", "torneio",
  "campeonato", "capítulo", "lote", "prêmio", "prémio", "etapa", "volta", "rodada",
  "secção", "seção", "distrito", "batalhão", "regimento", "grau", "dia", "semana",
  "período", "ciclo", "turno", "tempo", "round", "colocado", "colocada", "classificado",
  "classificada", "ofício", "cartório", "juizado", "vara", "mandato", "sessão", "salão",
  "simpósio", "seminário", "fórum", "certame", "convenção", "cúpula", "sargento", "tenente",
]);

// anos 90s, década de 80s, '80s, 1980s; but 30s alone is thirty seconds
export const DECADE_SUFFIXES: Keys = ["s"];
export const DECADE_FORMAT = suffixFormat({ apostrophes: true, boundaries: ".,-" });

export const ORDINAL_SIGN_FORMAT = suffixFormat({
  space: true,
  wordEnd: false,
});

export const SYMBOLS: Readings = {
  "&": ["e"],         "+": ["mais"],    "=": ["igual a"],     "@": ["arroba"],   "%": ["por cento"],
  "°": ["graus"],     "#": ["hashtag"], "²": ["ao quadrado"], "³": ["ao cubo"],  "π": ["pi"],
  "×": ["vezes"],     "€": ["euros"],   "$": ["dólares"],     "£": ["libras"],   "§": ["parágrafo"],
  "±": ["mais ou menos"], "+/-": ["mais ou menos"], "‰": ["por mil"],
  // Only before a number: p<0.05, ~$145, but <i>, ->, Año~
  "<": [{ words: "menor que", when: beforeValue }],
  ">": [{ words: "maior que", when: beforeValue }],
  "≤": [{ words: "menor ou igual a", when: beforeValue }],
  "≥": [{ words: "maior ou igual a", when: beforeValue }],
  "~": [{ words: "cerca de", when: beforeValue }],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: nº 5
    nº: [{ words: "número", when: beforeNumber }],
    Nº: [{ words: "número", when: beforeNumber }],
    "n.º": [{ words: "número", when: beforeNumber }],
    "N.º": [{ words: "número", when: beforeNumber }],
    "n°": [{ words: "número", when: beforeNumber }],
    "N°": [{ words: "número", when: beforeNumber }],
    "n.°": [{ words: "número", when: beforeNumber }],
    "N.°": [{ words: "número", when: beforeNumber }],
    "núm.": [{ words: "número", when: beforeNumber }],
    "art.": [{ words: "artigo", when: beforeNumber }],
    "Art.": [{ words: "artigo", when: beforeNumber }],
    "pág.": [{ words: "página", when: beforeNumber }],
    "§": [{ words: "parágrafo", when: beforeNumber }],
    "ext.": [{ words: "extensão", when: beforeNumber }],
    "Nº.": [{ words: "número", when: beforeNumber }],
    art: [{ words: "artigo", when: beforeNumber }],
    "cap.": [{ words: "capítulo", when: beforeNumber }],
    "Av.": [{ words: "Avenida", when: beforeName }, { words: "Avenida", when: beforeNumber }],
    "av.": [{ words: "avenida", when: beforeName }],
    // Anywhere
    "cfr.": ["conforme"],
    "Telef.": ["telefone"],
    "Tel.": ["telefone"],
    "p.": [{ words: "página", when: beforeNumber }],
    "pp.": [{ words: "páginas", when: beforeNumber }],
    "Dr.": [{ words: "Doutor", when: beforeName }],
    Tlm: ["telemóvel"],
    "Arq.": ["arquiteto"],
  },
  web: { dot: "ponto", at: "arroba", slash: "barra", hyphen: "hífen", underscore: "sublinhado",
    digits: ["zero", "um", "dois", "três", "quatro", "cinco", "seis", "sete", "oito", "nove"],
  },
};
