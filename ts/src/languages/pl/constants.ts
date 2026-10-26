import { dateFormat, keysOf, notation, phoneFormat, suffixFormat } from "../../numbers/formats.ts";
import type { Terms } from "../../steps/terms.ts";
import { beforeNumber, type Readings } from "../../text/readings.ts";
import type { Keys } from "../../text/scan.ts";
import { Case, type Joiners } from "../grammar.ts";

// A noun's forms after a number: singular, then plural, each in the order of Case.
export interface Noun {
  readonly singular: readonly string[];
  readonly plural: readonly string[];
}

// A noun from its twelve forms, separated by spaces, or by "|" when they are made of several words.
export function noun(forms: string): Noun {
  const separator = forms.includes("|") ? "|" : " ";
  const singular: string[] = [];
  const plural: string[] = [];
  for (let i = 0, start = 0; i < 12; ++i) {
    const found = forms.indexOf(separator, start);
    const end = found === -1 ? forms.length : found;
    (i < 6 ? singular : plural).push(forms.slice(start, end));
    start = end + 1;
  }
  return { singular, plural };
}

// A regular masculine noun from its stem and its locative: metr, metrze.
export function masculine(stem: string, locative: string): Noun {
  const s = stem;
  return {
    singular: [s, `${s}a`, `${s}owi`, s, `${s}em`, locative],
    plural: [`${s}y`, `${s}ów`, `${s}om`, `${s}y`, `${s}ami`, `${s}ach`],
  };
}

// Cardinals: nominative/accusative, genitive/locative, dative, instrumental.
export const CARDINALS: ReadonlyMap<bigint, readonly [string, string, string, string]> = new Map([
  [0n,   ["zero", "zera", "zeru", "zerem"]],
  [2n,   ["dwa", "dwóch", "dwóm", "dwoma"]],
  [3n,   ["trzy", "trzech", "trzem", "trzema"]],
  [4n,   ["cztery", "czterech", "czterem", "czterema"]],
  [5n,   ["pięć", "pięciu", "pięciu", "pięcioma"]],
  [6n,   ["sześć", "sześciu", "sześciu", "sześcioma"]],
  [7n,   ["siedem", "siedmiu", "siedmiu", "siedmioma"]],
  [8n,   ["osiem", "ośmiu", "ośmiu", "ośmioma"]],
  [9n,   ["dziewięć", "dziewięciu", "dziewięciu", "dziewięcioma"]],
  [10n,  ["dziesięć", "dziesięciu", "dziesięciu", "dziesięcioma"]],
  [11n,  ["jedenaście", "jedenastu", "jedenastu", "jedenastoma"]],
  [12n,  ["dwanaście", "dwunastu", "dwunastu", "dwunastoma"]],
  [13n,  ["trzynaście", "trzynastu", "trzynastu", "trzynastoma"]],
  [14n,  ["czternaście", "czternastu", "czternastu", "czternastoma"]],
  [15n,  ["piętnaście", "piętnastu", "piętnastu", "piętnastoma"]],
  [16n,  ["szesnaście", "szesnastu", "szesnastu", "szesnastoma"]],
  [17n,  ["siedemnaście", "siedemnastu", "siedemnastu", "siedemnastoma"]],
  [18n,  ["osiemnaście", "osiemnastu", "osiemnastu", "osiemnastoma"]],
  [19n,  ["dziewiętnaście", "dziewiętnastu", "dziewiętnastu", "dziewiętnastoma"]],
  [20n,  ["dwadzieścia", "dwudziestu", "dwudziestu", "dwudziestoma"]],
  [30n,  ["trzydzieści", "trzydziestu", "trzydziestu", "trzydziestoma"]],
  [40n,  ["czterdzieści", "czterdziestu", "czterdziestu", "czterdziestoma"]],
  [50n,  ["pięćdziesiąt", "pięćdziesięciu", "pięćdziesięciu", "pięćdziesięcioma"]],
  [60n,  ["sześćdziesiąt", "sześćdziesięciu", "sześćdziesięciu", "sześćdziesięcioma"]],
  [70n,  ["siedemdziesiąt", "siedemdziesięciu", "siedemdziesięciu", "siedemdziesięcioma"]],
  [80n,  ["osiemdziesiąt", "osiemdziesięciu", "osiemdziesięciu", "osiemdziesięcioma"]],
  [90n,  ["dziewięćdziesiąt", "dziewięćdziesięciu", "dziewięćdziesięciu", "dziewięćdziesięcioma"]],
  [100n, ["sto", "stu", "stu", "stoma"]],
  [200n, ["dwieście", "dwustu", "dwustu", "dwustoma"]],
  [300n, ["trzysta", "trzystu", "trzystu", "trzystoma"]],
  [400n, ["czterysta", "czterystu", "czterystu", "czterystoma"]],
  [500n, ["pięćset", "pięciuset", "pięciuset", "pięciuset"]],
  [600n, ["sześćset", "sześciuset", "sześciuset", "sześciuset"]],
  [700n, ["siedemset", "siedmiuset", "siedmiuset", "siedmiuset"]],
  [800n, ["osiemset", "ośmiuset", "ośmiuset", "ośmiuset"]],
  [900n, ["dziewięćset", "dziewięciuset", "dziewięciuset", "dziewięciuset"]],
]);

// "1" by gender (masculine, feminine, neuter), each in the order of Case.
export const ONE = [
  ["jeden", "jednego", "jednemu", "jeden", "jednym", "jednym"],
  ["jedna", "jednej", "jednej", "jedną", "jedną", "jednej"],
  ["jedno", "jednego", "jednemu", "jedno", "jednym", "jednym"],
] as const;

// Stems of ordinals, which take adjective endings: pierwsz-y, drug-iego.
export const ORDINAL_STEMS: ReadonlyMap<bigint, string> = new Map([
  [1n, "pierwsz"],  [2n, "drug"],    [3n, "trzec"],    [4n, "czwart"],   [5n, "piąt"],
  [6n, "szóst"],    [7n, "siódm"],   [8n, "ósm"],      [9n, "dziewiąt"], [10n, "dziesiąt"],
  [11n, "jedenast"], [12n, "dwunast"], [13n, "trzynast"], [14n, "czternast"], [15n, "piętnast"],
  [16n, "szesnast"], [17n, "siedemnast"], [18n, "osiemnast"], [19n, "dziewiętnast"], [20n, "dwudziest"],
  [30n, "trzydziest"], [40n, "czterdziest"], [50n, "pięćdziesiąt"], [60n, "sześćdziesiąt"],
  [70n, "siedemdziesiąt"], [80n, "osiemdziesiąt"], [90n, "dziewięćdziesiąt"],
  [100n, "setn"], [200n, "dwusetn"], [300n, "trzechsetn"], [400n, "czterechsetn"], [500n, "pięćsetn"],
  [600n, "sześćsetn"], [700n, "siedemsetn"], [800n, "osiemsetn"], [900n, "dziewięćsetn"],
]);

// Adjective endings by gender (masculine, feminine, neuter, plural), each in the order of Case.
export const ADJECTIVE_ENDINGS = [
  ["y", "ego", "emu", "y", "ym", "ym"],
  ["a", "ej", "ej", "ą", "ą", "ej"],
  ["e", "ego", "emu", "e", "ym", "ym"],
  ["e", "ych", "ym", "e", "ymi", "ych"],
] as const;

// Forms of numbers inside compounds: 45-metrowe -> czterdziestopięciometrowe.
export const COMBINING: ReadonlyMap<bigint, string> = new Map([
  [1n, "jedno"],     [2n, "dwu"],        [3n, "trzy"],        [4n, "cztero"],      [5n, "pięcio"],
  [6n, "sześcio"],   [7n, "siedmio"],    [8n, "ośmio"],       [9n, "dziewięcio"],  [10n, "dziesięcio"],
  [11n, "jedenasto"], [12n, "dwunasto"], [13n, "trzynasto"],  [14n, "czternasto"], [15n, "piętnasto"],
  [16n, "szesnasto"], [17n, "siedemnasto"], [18n, "osiemnasto"], [19n, "dziewiętnasto"], [20n, "dwudziesto"],
  [30n, "trzydziesto"], [40n, "czterdziesto"], [50n, "pięćdziesięcio"], [60n, "sześćdziesięcio"],
  [70n, "siedemdziesięcio"], [80n, "osiemdziesięcio"], [90n, "dziewięćdziesięcio"],
  [100n, "stu"], [200n, "dwustu"], [300n, "trzystu"], [400n, "czterystu"], [500n, "pięćset"],
  [600n, "sześćset"], [700n, "siedemset"], [800n, "osiemset"], [900n, "dziewięćset"],
]);

// Hyphenated endings which make a number an ordinal (lat 70-tych), with the adjective ending they stand for.
export const ORDINAL_SUFFIXES: ReadonlyMap<string, string> = new Map([
  ["tych", "ych"], ["ych", "ych"], ["ty", "y"],    ["y", "y"],     ["szy", "y"],    ["gi", "y"],
  ["ci", "y"],     ["go", "ego"],  ["ego", "ego"], ["tego", "ego"], ["ej", "ej"],    ["tej", "ej"],
  ["ym", "ym"],    ["tym", "ym"],  ["ta", "a"],    ["a", "a"],     ["te", "e"],     ["e", "e"],
  ["mu", "emu"],   ["temu", "emu"],
]);

// Nouns counted by numbers
export const THOUSAND = noun("tysiąc tysiąca tysiącowi tysiąc tysiącem tysiącu tysiące tysięcy tysiącom tysiące tysiącami tysiącach");
export const MILLION = masculine("milion", "milionie");
export const BILLION = masculine("miliard", "miliardzie");
export const ZLOTY = noun("złoty złotego złotemu złoty złotym złotym złote złotych złotym złote złotymi złotych");
export const GROSZ = noun("grosz grosza groszowi grosz groszem groszu grosze groszy groszom grosze groszami groszach");
export const PERCENT = noun("procent procent procentowi procent procentem procencie procent procent procentom procent procentami procentach");
export const EURO = noun("euro euro euro euro euro euro euro euro euro euro euro euro");
export const DOLLAR = masculine("dolar", "dolarze");
export const DEGREE = "stopień stopnia stopniowi stopień stopniem stopniu stopnie stopni stopniom stopnie stopniami stopniach";

export const CURRENCIES: Readonly<Record<string, Noun>> = {
  "zł": ZLOTY, "PLN": ZLOTY, "€": EURO, "EUR": EURO, "$": DOLLAR, "USD": DOLLAR,
  "£": masculine("funt", "funcie"), "gr": GROSZ,
};

export const SCALES: Readonly<Record<string, Noun>> = {
  "tys.": THOUSAND, "mln": MILLION, "mld": BILLION,
};

export const UNITS: Readonly<Record<string, Noun>> = {
  "km": masculine("kilometr", "kilometrze"),   "m": masculine("metr", "metrze"),
  "cm": masculine("centymetr", "centymetrze"), "mm": masculine("milimetr", "milimetrze"),
  "nm": masculine("nanometr", "nanometrze"),   "kg": masculine("kilogram", "kilogramie"),
  "g": masculine("gram", "gramie"),            "mg": masculine("miligram", "miligramie"),
  "l": masculine("litr", "litrze"),            "ml": masculine("mililitr", "mililitrze"),
  "ha": masculine("hektar", "hektarze"),       "W": masculine("wat", "wacie"),
  "kW": masculine("kilowat", "kilowacie"),     "MW": masculine("megawat", "megawacie"),
  "V": masculine("wolt", "wolcie"),            "GB": masculine("gigabajt", "gigabajcie"),
  "MB": masculine("megabajt", "megabajcie"),   "TB": masculine("terabajt", "terabajcie"),
  "kWh": noun("kilowatogodzina kilowatogodziny kilowatogodzinie kilowatogodzinę kilowatogodziną kilowatogodzinie kilowatogodziny kilowatogodzin kilowatogodzinom kilowatogodziny kilowatogodzinami kilowatogodzinach"),
  "Hz": noun("herc herca hercowi herc hercem hercu herce herców hercom herce hercami hercach"),
  "°C": noun("stopień Celsjusza|stopnia Celsjusza|stopniowi Celsjusza|stopień Celsjusza|stopniem Celsjusza|stopniu Celsjusza|stopnie Celsjusza|stopni Celsjusza|stopniom Celsjusza|stopnie Celsjusza|stopniami Celsjusza|stopniach Celsjusza"),
  "°": noun(DEGREE),
  "m²": noun("metr kwadratowy|metra kwadratowego|metrowi kwadratowemu|metr kwadratowy|metrem kwadratowym|metrze kwadratowym|metry kwadratowe|metrów kwadratowych|metrom kwadratowym|metry kwadratowe|metrami kwadratowymi|metrach kwadratowych"),
  "t": noun("tona tony tonie tonę toną tonie tony ton tonom tony tonami tonach"),
  "h": noun("godzina godziny godzinie godzinę godziną godzinie godziny godzin godzinom godziny godzinami godzinach"),
  "min": noun("minuta minuty minucie minutę minutą minucie minuty minut minutom minuty minutami minutach"),
  "s": noun("sekunda sekundy sekundzie sekundę sekundą sekundzie sekundy sekund sekundom sekundy sekundami sekundach"),
};

// Months in the genitive, as dates read them: 27 marca
export const MONTHS_GENITIVE = [
  "stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca",
  "lipca", "sierpnia", "września", "października", "listopada", "grudnia",
] as const;

export const MONTH_WORDS: ReadonlySet<string> = new Set([
  "stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca", "lipca", "sierpnia", "września",
  "października", "listopada", "grudnia", "styczeń", "luty", "marzec", "kwiecień", "maj", "czerwiec",
  "lipiec", "sierpień", "wrzesień", "październik", "listopad", "grudzień",
]);

// The case a preposition gives the number after it: od pięciu, z trzema
export const PREPOSITION_CASES: ReadonlyMap<string, Case> = new Map<string, Case>([
  ["od", Case.GENITIVE], ["do", Case.GENITIVE], ["bez", Case.GENITIVE], ["dla", Case.GENITIVE],
  ["u", Case.GENITIVE], ["około", Case.GENITIVE], ["koło", Case.GENITIVE], ["obok", Case.GENITIVE],
  ["podczas", Case.GENITIVE], ["według", Case.GENITIVE], ["wśród", Case.GENITIVE],
  ["spośród", Case.GENITIVE], ["sprzed", Case.GENITIVE], ["spod", Case.GENITIVE], ["zza", Case.GENITIVE],
  ["znad", Case.GENITIVE], ["oprócz", Case.GENITIVE], ["prócz", Case.GENITIVE], ["zamiast", Case.GENITIVE],
  ["wokół", Case.GENITIVE], ["dookoła", Case.GENITIVE], ["naprzeciwko", Case.GENITIVE],
  ["blisko", Case.GENITIVE], ["niedaleko", Case.GENITIVE], ["powyżej", Case.GENITIVE],
  ["poniżej", Case.GENITIVE],
  ["dzięki", Case.DATIVE], ["przeciw", Case.DATIVE], ["przeciwko", Case.DATIVE], ["ku", Case.DATIVE],
  ["wbrew", Case.DATIVE],
  ["przez", Case.ACCUSATIVE], ["ponad", Case.ACCUSATIVE], ["na", Case.ACCUSATIVE], ["o", Case.ACCUSATIVE],
  ["za", Case.ACCUSATIVE], ["po", Case.ACCUSATIVE], ["w", Case.ACCUSATIVE], ["we", Case.ACCUSATIVE],
  ["z", Case.INSTRUMENTAL], ["ze", Case.INSTRUMENTAL], ["nad", Case.INSTRUMENTAL],
  ["pod", Case.INSTRUMENTAL], ["przed", Case.INSTRUMENTAL], ["między", Case.INSTRUMENTAL],
  ["pomiędzy", Case.INSTRUMENTAL],
  ["przy", Case.LOCATIVE],
]);

export const LOCATIVE_PREPOSITIONS: ReadonlySet<string> = new Set(["w", "we", "o", "po", "przy", "na"]);

// Prepositional phrases which govern the genitive, with the preposition they need: w ciągu dwóch tygodni
export const COMPLEX_PREPOSITIONS: ReadonlyMap<string, string> = new Map([
  ["ciągu", "w"], ["trakcie", "w"], ["czasie", "w"], ["wyniku", "w"], ["przypadku", "w"],
  ["zakresie", "w"], ["ramach", "w"], ["celu", "w"], ["postaci", "w"], ["powodu", "z"],
]);

// Stems of amount nouns: a number after them is their genitive attribute (od kwoty trzystu tysięcy)
export const AMOUNT_NOUNS = [
  "rzęd", "wysokoś", "gruboś", "średnic", "długoś", "szerokoś", "głębokoś", "wiek", "kwot", "kwoc",
  "poziom", "stan", "strat", "strac", "wartoś", "liczb", "cen", "koszt", "opłat", "kar", "moc",
  "wag", "wadz", "pojemnoś", "powierzchn", "prędkoś", "temperatur", "objętoś", "odległoś", "armi",
  "grup", "okres", "limit", "próg", "prog", "budżet", "deficyt", "nadwyżk", "dochod", "dochód",
  "przychod", "przychód", "wydatk", "wzrost", "spad", "zysk", "dotacj", "wynagrodzen", "pensj",
] as const;

export const NOUN_ENDINGS: ReadonlySet<string> = new Set([
  "", "a", "ą", "ę", "y", "i", "u", "e", "ie", "em", "iem", "om", "ów", "ami", "ach", "ć", "ci",
  "cią", "ze", "ia", "ię", "ii", "ek", "ku",
]);

// Stems of verbs whose object is in the genitive: potrzebowali stu czterdziestu gramów
export const GENITIVE_VERBS = [
  "potrzeb", "używ", "użyj", "użyć", "wymag", "szuk", "poszuk", "unik", "brak", "zabrak", "życz",
  "dotycz", "uczy", "nauczy", "słucha", "pilnu",
] as const;

// Words skipped when looking for the verb governing a number: brakuje nam trzech osób
export const CLITICS: ReadonlySet<string> = new Set([
  "mi", "ci", "mu", "jej", "nam", "wam", "im", "się", "sobie", "już", "jeszcze", "teraz", "tu",
  "tam", "aż", "nawet",
]);

export const COPULAS: ReadonlySet<string> = new Set([
  "jest", "są", "był", "była", "było", "były", "będzie", "będą", "to",
]);

// Prepositions before a bare year (od 2020 do 2022), with the case of the year ordinal after them
export const YEAR_PREPOSITIONS: ReadonlyMap<string, Case> = new Map<string, Case>([
  ["od", Case.GENITIVE], ["do", Case.GENITIVE], ["z", Case.GENITIVE], ["ze", Case.GENITIVE],
  ["sprzed", Case.GENITIVE], ["około", Case.GENITIVE], ["przed", Case.INSTRUMENTAL],
  ["po", Case.LOCATIVE], ["na", Case.ACCUSATIVE],
]);

export const COUNT_NOUNS: ReadonlySet<string> = new Set([
  "osób", "lat", "dni", "godzin", "minut", "sekund", "razy", "sztuk", "ludzi", "dzieci", "tygodni",
  "miesięcy",
]);

export const CURRENCY_WORDS: ReadonlySet<string> = new Set(["zł", "złoty", "złote", "złotych"]);

// Stems of nouns whose numbers are feminine: dwie osoby, dwie godziny
export const FEMININE_PLURALS = [
  "osob", "osób", "godzin", "minut", "sekund", "złotówk", "kobiet", "stron", "sztuk", "rzecz", "częś",
  "noc", "dob", "klas", "wersj", "sesj", "edycj", "zapał", "tablic", "butelk", "szklank", "łyżk",
] as const;

// Words before a time with a dot, which make it a time: o 18.30
export const TIME_WORDS: ReadonlySet<string> = new Set([
  "o", "godzinie", "godz", "od", "do", "około", "koło", "po", "przed", "na", "między", "za",
]);

// Stems of headings: Punkt 3., w punkcie 5.; the ones of ORDINAL_HEADINGS read as ordinals even without a dot
export const HEADINGS = ["podpunkt", "punkt", "punkc", "rozdział", "paragraf", "artykuł", "pkt"] as const;
export const ORDINAL_HEADINGS = ["podpunkt", "punkt", "punkc", "paragraf", "pkt"] as const;

// Words before the ordinal of a decade, with its case: z lat 70., w latach 80.
export const DECADE_WORDS: ReadonlyMap<string, Case> = new Map<string, Case>([
  ["lata", Case.NOMINATIVE],     ["lat", Case.GENITIVE],       ["latach", Case.LOCATIVE],
  ["latami", Case.INSTRUMENTAL], ["roczników", Case.GENITIVE], ["rocznikach", Case.LOCATIVE],
]);

export const CONJUNCTIONS: ReadonlySet<string> = new Set(["a", "i", "lub", "albo", "czy", "oraz"]);

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: "   .", decimalSeparators: ",.", timeSeparators: ":.",
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS),
});

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "kropka", dot: "kropka", range: "-", by: "na", score: " do ", minus: "minus", plus: "plus",
};

// 15.08.2020 r.: the year has four digits, "r." may follow it
export const DATE_FORMAT = dateFormat({ separators: ".", boundaries: ".,", shortYear: false, suffixes: ["r."] });

// 600 123 456, 22 123 45 67 (after a word of PHONE_WORDS), 600-123-456, 68-324-22-55
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: " ", boundaries: "", closing: "" }),
  phoneFormat({ groups: [[2, 2], [3, 3], [2, 2], [2, 2]], separators: " ", boundaries: "", closing: "" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[2, 2], [3, 3], [2, 2], [2, 2]], separators: "-", boundaries: ".,-", closing: "-" }),
];

// Words before a phone number written with spaces, which else reads as a count: numer 600 123 456
export const PHONE_WORDS: ReadonlySet<string> = new Set([
  "numer", "numerem", "numeru", "tel", "telefon", "telefonu", "telefonem", "infolinia", "faks", "fax",
]);

// 2020 r.: a year with "roku" after it
export const YEAR_SUFFIXES: Keys = ["r."];
export const YEAR_FORMAT = suffixFormat({ space: true, sentenceDot: true });

export const SYMBOLS: Readings = {
  "&": ["and"],        "+": ["plus"],           "=": ["równa się"], "%": ["procent"],  "°": ["stopni"],
  "#": [""],           "/": [""],               "²": ["do kwadratu"], "³": ["do sześcianu"],
  "π": ["pi"],         "×": ["razy"],           "€": ["euro"],     "$": ["dolarów"],   "@": ["małpa"],
};

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: nr 5
    "nr": [{ words: "numer", when: beforeNumber }],
    "Nr": [{ words: "numer", when: beforeNumber }],
    "nr.": [{ words: "numer", when: beforeNumber }],
    "ust.": [{ words: "ustęp", when: beforeNumber }],
    "pkt": [{ words: "punkt", when: beforeNumber }],
    "art.": [{ words: "artykuł", when: beforeNumber }],
    "str.": [{ words: "strona", when: beforeNumber }],
    "poz.": [{ words: "pozycja", when: beforeNumber }],
    "ok.": [{ words: "około", when: beforeNumber }],
    // Anywhere
    "np.": ["na przykład"],
    "itd.": ["i tak dalej"],
    "itp.": ["i tym podobne"],
    "tzn.": ["to znaczy"],
    "tj.": ["to jest"],
    "m.in.": ["między innymi"],
    "wg": ["według"],
  },
  web: { dot: "kropka", at: "małpa", slash: "ukośnik" },
};
