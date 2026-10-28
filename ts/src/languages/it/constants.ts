import { dateFormat, keysOf, notation, phoneFormat, suffixFormat, type Fractions, type RomanCues } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Keys } from "../../text/scan.ts";
import { beforeName, beforeNumber, type Readings } from "../../text/readings.ts";
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
  "€": { one: "euro", many: "euro" },          "EUR": { one: "euro", many: "euro" },       "eur": { one: "euro", many: "euro" },
  "euro": { one: "euro", many: "euro" },       "Euro": { one: "euro", many: "euro" },
  "$": { one: "dollaro", many: "dollari" },    "US$": { one: "dollaro", many: "dollari" }, "USD": { one: "dollaro", many: "dollari" },
  "£": { one: "sterlina", many: "sterline" },  "GBP": { one: "sterlina", many: "sterline" },
  "¥": { one: "yen", many: "yen" },            "₽": { one: "rublo", many: "rubli" },       "RUB": { one: "rublo", many: "rubli" },
  "CHF": { one: "franco svizzero", many: "franchi svizzeri" }, "₹": { one: "rupia", many: "rupie" }
};

// Most scales are written as the word they are read with: 3 milioni
export const SCALES: Readonly<Record<string, Forms>> = {
  "mila":     { one: "mila", many: "mila" },         "k": { one: "mila", many: "mila" }, "K": { one: "mila", many: "mila" },
  "milione":  { one: "milione", many: "milione" },   "milioni":  { one: "milioni", many: "milioni" },
  "miliardo": { one: "miliardo", many: "miliardo" }, "miliardi": { one: "miliardi", many: "miliardi" },
  "mln":      { one: "milione", many: "milioni" },   "mld":      { one: "miliardo", many: "miliardi" },
};

// A singular ending in -a is feminine: una tonnellata, un'ora
export const UNITS: Readonly<Record<string, Forms>> = {
  "km": { one: "chilometro", many: "chilometri" },          "m": { one: "metro", many: "metri" },
  "cm": { one: "centimetro", many: "centimetri" },          "mm": { one: "millimetro", many: "millimetri" },
  "m²": { one: "metro quadro", many: "metri quadri" },      "m2": { one: "metro quadro", many: "metri quadri" },
  "mq": { one: "metro quadro", many: "metri quadri" },      "cm²": { one: "centimetro quadrato", many: "centimetri quadrati" },
  "km²": { one: "chilometro quadrato", many: "chilometri quadrati" }, "m³": { one: "metro cubo", many: "metri cubi" },
  "mc": { one: "metro cubo", many: "metri cubi" },          "kg": { one: "chilo", many: "chili" },
  "g": { one: "grammo", many: "grammi" },                   "gr": { one: "grammo", many: "grammi" },
  "mg": { one: "milligrammo", many: "milligrammi" },        "t": { one: "tonnellata", many: "tonnellate" },
  "l": { one: "litro", many: "litri" },                     "lt": { one: "litro", many: "litri" },
  "ml": { one: "millilitro", many: "millilitri" },          "cl": { one: "centilitro", many: "centilitri" },
  "dl": { one: "decilitro", many: "decilitri" },            "TB": { one: "terabyte", many: "terabyte" },
  "GB": { one: "giga", many: "giga" },                      "MB": { one: "mega", many: "mega" },
  "kB": { one: "kilobyte", many: "kilobyte" },              "Mbps": { one: "megabit al secondo", many: "megabit al secondo" },
  "Hz": { one: "hertz", many: "hertz" },                    "kHz": { one: "kilohertz", many: "kilohertz" },
  "MHz": { one: "megahertz", many: "megahertz" },           "GHz": { one: "gigahertz", many: "gigahertz" },
  "W": { one: "watt", many: "watt" },                       "kW": { one: "kilowatt", many: "kilowatt" },
  "MW": { one: "megawatt", many: "megawatt" },              "kWh": { one: "kilowattora", many: "kilowattora" },
  "V": { one: "volt", many: "volt" },                       "kV": { one: "kilovolt", many: "kilovolt" },
  "A": { one: "ampere", many: "ampere" },                   "mA": { one: "milliampere", many: "milliampere" },
  "mAh": { one: "milliampereora", many: "milliampereora" }, "CV": { one: "cavallo", many: "cavalli" },
  "cv": { one: "cavallo", many: "cavalli" },                "Nm": { one: "newton metro", many: "newton metro" },
  "kcal": { one: "chilocaloria", many: "chilocalorie" },    "dB": { one: "decibel", many: "decibel" },
  "km/h": { one: "chilometro orario", many: "chilometri orari" },
  "km/s": { one: "chilometro al secondo", many: "chilometri al secondo" },
  "m/s": { one: "metro al secondo", many: "metri al secondo" },
  "km/l": { one: "chilometro al litro", many: "chilometri al litro" },
  "°C": { one: "grado", many: "gradi" },                    "°F": { one: "grado Fahrenheit", many: "gradi Fahrenheit" },
  "°": { one: "grado", many: "gradi" },                     "h": { one: "ora", many: "ore" },
  "min": { one: "minuto", many: "minuti" },                 "sec": { one: "secondo", many: "secondi" },
  "ms": { one: "millisecondo", many: "millisecondi" },      "gg": { one: "giorno", many: "giorni" },
  "d": { one: "giorno", many: "giorni" }
};

export const FRACTIONS: Fractions = {
  "1/2": "un mezzo", "1/3": "un terzo", "2/3": "due terzi", "1/4": "un quarto", "3/4": "tre quarti",
};

// Words after a fraction which mark it as one: 1/3 del
export const FRACTION_WORDS: ReadonlySet<string> = new Set(["di", "del", "della", "dello", "dei", "degli", "delle", "dell'"]);

// Words before a time with a dot, which make it a time: alle 21.30
export const TIME_WORDS: ReadonlySet<string> = new Set(["alle", "ore", "dalle", "le", "verso", "entro", "per"]);

// Umberto I°: an ordinal marker on a Roman numeral
export const ROMAN_MARKERS: Keys = ["°", "º", "ª"];

// Giovanni XXIII (ventitreesimo), nel XX secolo (ventesimo), capitolo IV (quarto), la II guerra (seconda)
export const ROMAN_CUES: RomanCues = {
  names: new Set([
    "giovanni", "paolo", "benedetto", "pio", "leone", "gregorio", "clemente", "innocenzo", "bonifacio", "emanuele",
    "umberto", "carlo", "luigi", "enrico", "filippo", "federico", "francesco", "ferdinando", "napoleone",
    "elisabetta", "giorgio", "guglielmo", "alessandro", "edoardo", "riccardo", "giacomo", "pietro", "amedeo",
    "ramses", "maria", "vittoria", "caterina", "giovanna",
  ]),
  before: new Set([
    "capitolo", "libro", "tomo", "atto", "scena", "parte", "titolo", "articolo", "secolo", "volume", "stagione",
    "fase", "legislatura", "canto",
  ]),
  after: new Set([
    "secolo", "secoli", "millennio", "guerra", "congresso", "edizione", "festival", "legislatura", "giornata",
    "convegno", "conferenza", "assemblea", "concorso", "forum", "simposio", "seminario", "campionato", "olimpiade",
    "repubblica", "anniversario", "centenario", "premio", "torneo", "salone", "settimana", "municipio",
    "circoscrizione", "rapporto", "piano", "stagione",
  ]),
};

// Names with a regnal number, and nouns around a numeral, which are feminine: Elisabetta II (seconda), parte II
export const FEMININE_NAMES: ReadonlySet<string> = new Set([
  "elisabetta", "maria", "vittoria", "caterina", "giovanna",
]);


export const FEMININE_ROMAN_NOUNS: ReadonlySet<string> = new Set([
  "scena", "parte", "stagione", "fase", "legislatura", "classe", "guerra", "edizione", "giornata", "conferenza",
  "assemblea", "olimpiade", "repubblica", "settimana", "circoscrizione",
]);

// The hundredth of a currency: 10,50 € dieci euro e cinquanta centesimi; 0,01 € un centesimo
export const CENTS: Readonly<Record<string, Forms>> = {
  "€": { one: "centesimo", many: "centesimi" }, "EUR": { one: "centesimo", many: "centesimi" }, "eur": { one: "centesimo", many: "centesimi" },
  "euro": { one: "centesimo", many: "centesimi" }, "Euro": { one: "centesimo", many: "centesimi" },
  "$": { one: "centesimo", many: "centesimi" }, "US$": { one: "centesimo", many: "centesimi" }, "USD": { one: "centesimo", many: "centesimi" },
  "£": { one: "penny", many: "penny" },         "GBP": { one: "penny", many: "penny" },
  "₽": { one: "copeco", many: "copechi" },      "RUB": { one: "copeco", many: "copechi" },
  "CHF": { one: "centesimo", many: "centesimi" }
};

// Articles and articulated prepositions before 3° which make it an ordinal (il 3° posto), not degrees (a 30°)
export const ORDINAL_ARTICLES: ReadonlySet<string> = new Set([
  "il", "lo", "la", "al", "allo", "alla", "del", "dello", "della", "nel", "nello", "nella", "dal", "dallo", "dalla",
  "sul", "sullo", "sulla", "un", "una", "questo", "questa", "suo", "sua", "mio", "mia", "tuo", "tua", "nostro",
  "nostra", "vostro", "vostra", "l", "all", "dell", "nell", "dall", "sull",
]);

// The articles above which are feminine; an elided one (l'11° edizione) takes the gender of the noun after it
export const FEMININE_ARTICLES: ReadonlySet<string> = new Set([
  "la", "alla", "della", "nella", "dalla", "sulla", "una", "questa", "sua", "mia", "tua", "nostra", "vostra",
]);

// Words after 1 which are not nouns it counts: 1 su 3 (uno su tre), 1 di loro, 1 a 0
export const STOP_WORDS: ReadonlySet<string> = new Set([
  "e", "ed", "o", "od", "a", "ad", "di", "da", "in", "su", "per", "con", "tra", "fra", "del", "della", "dei", "degli",
  "delle", "dello", "dell", "al", "alla", "allo", "ai", "agli", "alle", "all", "nel", "nella", "nello", "nei",
  "negli", "nelle", "nell", "dal", "dalla", "dallo", "dai", "dagli", "dalle", "dall", "sul", "sulla", "sullo", "sui",
  "sugli", "sulle", "sull", "col", "coi", "è", "era", "sono", "ha", "hanno", "che", "contro", "più", "meno", "x",
  "come", "ma", "se", "non", "il", "lo", "la", "le", "gli", "i", "un", "una", "uno", "rispetto", "circa", "dopo",
  "prima", "fa", "oppure", "anche", "poi", "così", "invece",
]);

// Words before 1 which make it a count of the noun after it (con 1 figlio: un figlio), unlike a label (articolo 1)
export const COUNT_WORDS: ReadonlySet<string> = new Set([
  "con", "per", "in", "di", "da", "a", "ad", "su", "e", "ed", "o", "solo", "soltanto", "almeno", "circa", "ogni",
  "tra", "fra", "entro", "dopo", "oltre", "ha", "ho", "hai", "hanno", "abbiamo", "avete", "aveva", "avevano", "c'è",
  "è", "era", "costa", "pesa", "dura", "misura", "già", "ancora", "appena", "quasi", "manca", "mancano", "resta",
  "restano", "rimane", "più", "meno", "che", "cioè", "sotto", "sopra", "verso", "contro", "senza", "servono", "serve",
  "basta", "bastano",
]);

// Nouns in -a which are masculine (un problema), and nouns in -e which are feminine (una notte)
export const MASCULINE_IN_A: ReadonlySet<string> = new Set([
  "problema", "sistema", "programma", "tema", "schema", "clima", "dramma", "diploma", "poeta", "pilota", "atleta",
  "pianeta", "papa", "panorama", "cinema", "dilemma", "enigma", "telegramma", "diagramma", "teorema", "emblema",
  "stemma", "sofà", "cruciverba", "pigiama", "collega", "giornalista", "artista", "turista", "ciclista", "dentista",
  "regista", "barista", "autista", "tassista", "sindacalista",
]);

export const FEMININE_IN_E: ReadonlySet<string> = new Set([
  "notte", "chiave", "parte", "classe", "fase", "legge", "gente", "mente", "morte", "sede", "nave", "neve", "fame",
  "sete", "voce", "croce", "pace", "luce", "rete", "carne", "madre", "moglie", "torre", "valle", "fonte", "sorte",
  "frase", "base", "lente", "arte", "febbre", "specie", "serie", "superficie", "volpe", "ape", "fede", "tosse",
  "stirpe", "siepe", "lode", "cassaforte", "automobile", "canzone", "nube", "pelle", "vite", "dote", "radice",
  "cornice", "attrice", "lavatrice", "calcolatrice", "stampante", "corrente", "costante",
]);

// Nouns in -ione which are masculine: un milione, un campione
export const MASCULINE_IN_IONE: ReadonlySet<string> = new Set([
  "milione", "campione", "padiglione", "battaglione", "scorpione", "rione", "bastione",
]);

// Nouns after 3° which make it an ordinal: 3° lotto, 17° Meeting
export const ORDINAL_NOUNS: ReadonlySet<string> = new Set([
  "posto", "lotto", "anno", "semestre", "trimestre", "tasto", "giorno", "anniversario",
  "edizione", "piano", "classificato", "classificata", "livello", "torneo", "campionato", "capitolo",
  "premio", "tappa", "giro", "sezione", "distretto", "battaglione", "reggimento", "stormo",
  "grado", "meeting", "festival", "congresso", "concorso", "incontro", "giornata", "turno",
  "tempo", "round", "circolo", "municipio", "reparto", "corso", "convegno", "raduno",
  "trofeo", "memorial", "salone", "forum", "secolo", "movimento",
]);
export const ELIDED_ARTICLES: ReadonlySet<string> = new Set([
  "l", "all", "dell", "nell", "dall", "sull",
]);

// Words before a number with a dot which make it a label read with punto: versione 2.1, paragrafo 4.2
export const VERSION_WORDS: ReadonlySet<string> = new Set([
  "versione", "release", "v", "ver", "paragrafo", "par", "art", "articolo", "comma", "punto", "sezione", "capitolo",
  "cap", "regola", "nota", "tabella", "figura", "fig", "allegato", "modello", "aggiornamento", "standard",
  "protocollo",
]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: ".", decimalSeparators: ",.", timeSeparators: ":.",
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "punto", dot: "punto", range: "-", by: "per", score: " a ", minus: "meno", plus: "più",
  fraction: " e ", half: " e mezzo", rating: "su",
};

// 24/03/2022, 24.03.2022, 24-03-2022
export const DATE_FORMAT = dateFormat({ separators: "/.-", boundaries: ".,/-" });

// 333 1234567, 06 1234 5678, 06 12345678
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [6, 7]], separators: " " }),
  phoneFormat({ groups: [[2, 4], [3, 4], [4, 4]], separators: " ", leadingZero: true }),
  phoneFormat({ groups: [[2, 4], [6, 8]], separators: " ", leadingZero: true }),
  phoneFormat({ groups: [[2, 4], [5, 8]], separators: "-/.", leadingZero: true }),
];

// 1º, 2 ª, 3°, 2.°: a sign, possibly after a space
export const ORDINAL_SIGNS: Keys = ["º", "ª", "°", ".°", ".º", ".ª"];
export const ORDINAL_SIGN_FORMAT = suffixFormat({ space: true });

// 21esimo, 33esima
export const ORDINAL_SUFFIXES: Keys = ["esimo", "esima", "esimi", "esime"];

// "&" reads as "e", in names too: Gratta&Vinci, H&M
export const SYMBOLS: Readings = {
  "&": ["e"],       "+": ["più"],     "=": ["uguale a"],    "@": ["chiocciola"], "%": ["per cento"],
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
    "n.°": [{ words: "numero", when: beforeNumber }],
    "N.°": [{ words: "numero", when: beforeNumber }],
    "nº": [{ words: "numero", when: beforeNumber }],
    "Nº": [{ words: "numero", when: beforeNumber }],
    "n.º": [{ words: "numero", when: beforeNumber }],
    "N.º": [{ words: "numero", when: beforeNumber }],
    "art.": [{ words: "articolo", when: beforeNumber }],
    "Art.": [{ words: "articolo", when: beforeNumber }],
    "pag.": [{ words: "pagina", when: beforeNumber }],
    "cap.": [{ words: "capitolo", when: beforeNumber }],
    "§": [{ words: "paragrafo", when: beforeNumber }],
    "pp.": [{ words: "pagine", when: beforeNumber }],
    "fig.": [{ words: "figura", when: beforeNumber }],
    "Fig.": [{ words: "figura", when: beforeNumber }],
    "tab.": [{ words: "tabella", when: beforeNumber }],
    "vol.": [{ words: "volume", when: beforeNumber }],
    // Titles, which drop their final -e before a name: Sig. Rossi, signor Rossi
    "sig.": [{ words: "signor", when: beforeName }, "signore"],
    "Sig.": [{ words: "signor", when: beforeName }, "signore"],
    "sig.ra": ["signora"],
    "Sig.ra": ["signora"],
    "sig.na": ["signorina"],
    "Sig.na": ["signorina"],
    "sigg.": ["signori"],
    "Sigg.": ["signori"],
    "dott.": [{ words: "dottor", when: beforeName }, "dottore"],
    "Dott.": [{ words: "dottor", when: beforeName }, "dottore"],
    "dr.": [{ words: "dottor", when: beforeName }],
    "Dr.": [{ words: "dottor", when: beforeName }],
    "dott.ssa": ["dottoressa"],
    "Dott.ssa": ["dottoressa"],
    "prof.": [{ words: "professor", when: beforeName }, "professore"],
    "Prof.": [{ words: "professor", when: beforeName }, "professore"],
    "prof.ssa": ["professoressa"],
    "Prof.ssa": ["professoressa"],
    "ing.": [{ words: "ingegner", when: beforeName }, "ingegnere"],
    "Ing.": [{ words: "ingegner", when: beforeName }, "ingegnere"],
    "avv.": ["avvocato"],
    "Avv.": ["avvocato"],
    "arch.": ["architetto"],
    "Arch.": ["architetto"],
    "geom.": ["geometra"],
    "Geom.": ["geometra"],
    "rag.": [{ words: "ragionier", when: beforeName }],
    "Rag.": [{ words: "ragionier", when: beforeName }],
    "on.": [{ words: "onorevole", when: beforeName }],
    "On.": [{ words: "onorevole", when: beforeName }],
    "sen.": [{ words: "senatore", when: beforeName }],
    "Sen.": [{ words: "senatore", when: beforeName }],
    "mons.": [{ words: "monsignor", when: beforeName }],
    "Mons.": [{ words: "monsignor", when: beforeName }],
    "gent.mo": ["gentilissimo"],
    "Gent.mo": ["gentilissimo"],
    "gent.ma": ["gentilissima"],
    "Gent.ma": ["gentilissima"],
    "egr.": ["egregio"],
    "Egr.": ["egregio"],
    "spett.le": ["spettabile"],
    "Spett.le": ["spettabile"],
    // Places in addresses: p.za Grande, v.le Monza, c.so Italia
    "p.za": ["piazza"],
    "P.za": ["piazza"],
    "p.zza": ["piazza"],
    "P.zza": ["piazza"],
    "v.le": ["viale"],
    "V.le": ["viale"],
    "c.so": ["corso"],
    "C.so": ["corso"],
    "l.go": ["largo"],
    "L.go": ["largo"],
    // Anywhere
    "dip.": ["dipartimento"],
    "Dip.": ["dipartimento"],
    "ecc.": ["eccetera"],
    "etc.": ["eccetera"],
    "ad es.": ["ad esempio"],
    "Ad es.": ["ad esempio"],
    "per es.": ["per esempio"],
    "Per es.": ["per esempio"],
    "p.es.": ["per esempio"],
    "p. es.": ["per esempio"],
    "es.": ["esempio"],
    "cfr.": ["confronta"],
    "Cfr.": ["confronta"],
    "tel.": ["telefono"],
    "Tel.": ["telefono"],
    "ca.": ["circa"],
    "sec.": [{ words: "secolo", when: beforeName }],
    "a.C.": ["avanti Cristo"],
    "d.C.": ["dopo Cristo"],
    "d.c.": ["dopo Cristo"],
    "N.B.": ["nota bene"],
    "n.b.": ["nota bene"],
    "P.S.": ["post scriptum"],
    "q.tà": ["quantità"],
    "tot.": ["totale"],
  },
  web: { dot: "punto", at: "chiocciola", slash: "slash", hyphen: "trattino", underscore: "trattino basso",
    digits: ["zero", "uno", "due", "tre", "quattro", "cinque", "sei", "sette", "otto", "nove"],
  },
  closing: new Set(["ecc.", "etc."]),
};
