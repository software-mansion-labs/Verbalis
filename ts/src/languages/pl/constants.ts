import { dateFormat, keysOf, notation, phoneFormat, suffixFormat } from "../../numbers/formats.ts";
import type { Characters } from "../../steps/normalize.ts";
import type { Terms } from "../../steps/terms.ts";
import type { Context } from "../../text/context.ts";
import { beforeName, beforeNumber, type Condition, type Reading, type Readings } from "../../text/readings.ts";
import type { Keys } from "../../text/scan.ts";
import { toLower } from "../../text/strings.ts";
import { isDigit } from "../../text/unicode.ts";
import { Case, Gender, type Joiners } from "../grammar.ts";

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
  // The endings after the stem of 1, 2, 3, 7, 8, and from 20: 1-szym, 2-gim, 3-cim, 3-ciej, 7-mym, 20-stym
  ["cim", "ym"],   ["gim", "ym"],  ["szym", "ym"], ["mym", "ym"],   ["stym", "ym"],
  ["ciej", "ej"],  ["giej", "ej"], ["szej", "ej"], ["mej", "ej"],   ["stej", "ej"],
  ["ciego", "ego"], ["giego", "ego"], ["szego", "ego"], ["mego", "ego"], ["stego", "ego"],
  ["cia", "a"],    ["ga", "a"],    ["sza", "a"],   ["ma", "a"],     ["sta", "a"],
  ["cie", "e"],    ["gie", "e"],   ["sze", "e"],   ["me", "e"],     ["ste", "e"],
  ["my", "y"],     ["sty", "y"],
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

// A noun followed by words which don't inflect: złotych za metr kwadratowy
export function followed(base: Noun, words: string): Noun {
  return {
    singular: base.singular.map((form) => `${form} ${words}`),
    plural: base.plural.map((form) => `${form} ${words}`),
  };
}

export const CURRENCIES: Readonly<Record<string, Noun>> = {
  "zł": ZLOTY, "PLN": ZLOTY, "€": EURO, "EUR": EURO, "$": DOLLAR, "USD": DOLLAR,
  "£": masculine("funt", "funcie"), "gr": GROSZ,
  // Prices per unit: 100 zł/m² sto złotych za metr kwadratowy
  "zł/m²": followed(ZLOTY, "za metr kwadratowy"), "zł/m2": followed(ZLOTY, "za metr kwadratowy"),
  "zł/h": followed(ZLOTY, "za godzinę"),           "zł/godz.": followed(ZLOTY, "za godzinę"),
  "zł/kg": followed(ZLOTY, "za kilogram"),         "zł/szt.": followed(ZLOTY, "za sztukę"),
  "zł/l": followed(ZLOTY, "za litr"),              "zł/os.": followed(ZLOTY, "za osobę"),
  "zł/km": followed(ZLOTY, "za kilometr"),         "zł/mies.": followed(ZLOTY, "miesięcznie"),
  "€/m²": followed(EURO, "za metr kwadratowy"),
};

export const SCALES: Readonly<Record<string, Noun>> = {
  "tys.": THOUSAND, "mln": MILLION, "mld": BILLION,
};

// A noun with an adjective after it, from the adjective's twelve forms: metr kwadratowy, metrów kwadratowych.
export function qualified(base: Noun, adjective: string): Noun {
  const forms = noun(adjective);
  return {
    singular: base.singular.map((form, c) => `${form} ${forms.singular[c]}`),
    plural: base.plural.map((form, c) => `${form} ${forms.plural[c]}`),
  };
}

export const SQUARE = "kwadratowy kwadratowego kwadratowemu kwadratowy kwadratowym kwadratowym kwadratowe kwadratowych kwadratowym kwadratowe kwadratowymi kwadratowych";
export const CUBIC = "sześcienny sześciennego sześciennemu sześcienny sześciennym sześciennym sześcienne sześciennych sześciennym sześcienne sześciennymi sześciennych";
export const METRE = masculine("metr", "metrze");
export const KILOMETRE = masculine("kilometr", "kilometrze");
export const CENTIMETRE = masculine("centymetr", "centymetrze");
export const POINT = noun("punkt punktu punktowi punkt punktem punkcie punkty punktów punktom punkty punktami punktach");

export const UNITS: Readonly<Record<string, Noun>> = {
  "km": KILOMETRE,   "m": METRE,
  "m²": qualified(METRE, SQUARE),           "m2": qualified(METRE, SQUARE),
  "km²": qualified(KILOMETRE, SQUARE),      "km2": qualified(KILOMETRE, SQUARE),
  "cm²": qualified(CENTIMETRE, SQUARE),     "m³": qualified(METRE, CUBIC),
  "m3": qualified(METRE, CUBIC),            "cm³": qualified(CENTIMETRE, CUBIC),
  "km/h": qualified(KILOMETRE, "na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę"),
  "pkt": POINT,   "pkt.": POINT,
  "pkt. proc.": qualified(POINT, "procentowy procentowego procentowemu procentowy procentowym procentowym procentowe procentowych procentowym procentowe procentowymi procentowych"),
  "pkt proc.": qualified(POINT, "procentowy procentowego procentowemu procentowy procentowym procentowym procentowe procentowych procentowym procentowe procentowymi procentowych"),
  "proc.": noun("procent procent procentowi procent procentem procencie procent procent procentom procent procentami procentach"),
  "‰": noun("promil promila promilowi promil promilem promilu promile promili promilom promile promilami promilach"),
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
  "lipiec", "sierpień", "wrzesień", "październik", "listopad", "grudzień", "styczniu", "lutym", "marcu",
  "kwietniu", "maju", "czerwcu", "lipcu", "sierpniu", "wrześniu", "październiku", "listopadzie", "grudniu",
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
  ["z", Case.GENITIVE], ["ze", Case.GENITIVE], ["nad", Case.INSTRUMENTAL],
  ["pod", Case.INSTRUMENTAL], ["przed", Case.INSTRUMENTAL], ["między", Case.INSTRUMENTAL],
  ["pomiędzy", Case.INSTRUMENTAL],
  ["przy", Case.LOCATIVE],
]);

export const LOCATIVE_PREPOSITIONS: ReadonlySet<string> = new Set(["w", "we", "o", "po", "przy", "na"]);

// Prepositional phrases which govern the genitive, with the preposition they need: w ciągu dwóch tygodni
export const COMPLEX_PREPOSITIONS: ReadonlyMap<string, string> = new Map([
  ["ciągu", "w"], ["trakcie", "w"], ["czasie", "w"], ["wyniku", "w"], ["przypadku", "w"],
  ["zakresie", "w"], ["ramach", "w"], ["celu", "w"], ["postaci", "w"], ["powodu", "z"],
  ["przeciągu", "w"], ["okresie", "w"],
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

// Nouns of men in the genitive plural, after which a number takes its genitive form: piętnastu pracowników
export const VIRILE_NOUNS: ReadonlySet<string> = new Set([
  "uczniów", "studentów", "pracowników", "mężczyzn", "chłopców", "żołnierzy", "policjantów", "strażaków", "lekarzy",
  "nauczycieli", "zawodników", "piłkarzy", "kibiców", "posłów", "senatorów", "radnych", "polityków", "kandydatów",
  "uczestników", "pasażerów", "turystów", "mieszkańców", "obywateli", "pacjentów", "klientów", "użytkowników",
  "gości", "sędziów", "prawników", "dziennikarzy", "autorów", "artystów", "muzyków", "aktorów", "naukowców",
  "badaczy", "ekspertów", "specjalistów", "inżynierów", "kierowców", "rolników", "górników", "ratowników",
  "ministrów", "członków", "kolegów", "przyjaciół", "braci", "synów", "Polaków", "polaków", "zwycięzców",
  "finalistów", "laureatów", "absolwentów", "ochotników", "wolontariuszy", "funkcjonariuszy", "oficerów",
  "generałów", "księży", "biskupów", "więźniów", "zakładników", "pracowników", "robotników", "sportowców",
  "trenerów", "graczy", "chłopaków", "panów", "emerytów", "rodziców", "ojców", "dziadków", "wnuków", "sąsiadów",
  "świadków", "podejrzanych", "zatrzymanych", "rannych", "zabitych",
]);

// Counted nouns whose nominative plural is also their genitive: trzy razy, cztery dni
export const SAME_IN_BOTH_CASES: ReadonlySet<string> = new Set(["razy", "dni"]);

export const CENT = noun("cent centa centowi cent centem cencie centy centów centom centy centami centach");

// Currencies whose hundredth is a cent: 3,50 € trzy euro pięćdziesiąt centów
export const CENTS: ReadonlySet<string> = new Set(["€", "EUR", "$", "USD"]);

export const CURRENCY_WORDS: ReadonlySet<string> = new Set(["zł", "złoty", "złote", "złotych"]);

// Stems of nouns whose numbers are feminine: dwie osoby, dwie godziny
export const FEMININE_PLURALS = [
  "osob", "osób", "godzin", "minut", "sekund", "złotówk", "kobiet", "stron", "sztuk", "rzecz", "częś",
  "noc", "dob", "klas", "wersj", "sesj", "edycj", "zapał", "tablic", "butelk", "szklank", "łyżk",
  "dziewczyn", "dziewcząt", "córk", "córek", "siostr", "sióstr", "matk", "matek", "rodzin", "uczennic",
  "zawodniczk", "zawodniczek",
] as const;

// Words before a time with a dot, which make it a time: o 18.30
export const TIME_WORDS: ReadonlySet<string> = new Set([
  "o", "godzinie", "godz", "od", "do", "około", "koło", "po", "przed", "na", "między", "za",
]);

// Nouns which often follow an ordinal, with their twelve forms, so that the ordinal agrees: w 3. rundzie (trzeciej),
// na 3. piętrze (trzecim), z 1. miejsca (pierwszego), 18. urodziny (osiemnaste)
export const ORDINAL_NOUNS: readonly (readonly [Gender, string])[] = [
  [Gender.FEMININE, "runda rundy rundzie rundę rundą rundzie rundy rund rundom rundy rundami rundach"],
  [Gender.FEMININE, "minuta minuty minucie minutę minutą minucie minuty minut minutom minuty minutami minutach"],
  [Gender.FEMININE, "połowa połowy połowie połowę połową połowie połowy połów połowom połowy połowami połowach"],
  [Gender.FEMININE, "tura tury turze turę turą turze tury tur turom tury turami turach"],
  [Gender.FEMININE, "strona strony stronie stronę stroną stronie strony stron stronom strony stronami stronach"],
  [Gender.FEMININE, "kolejka kolejki kolejce kolejkę kolejką kolejce kolejki kolejek kolejkom kolejki kolejkami kolejkach"],
  [Gender.FEMININE, "edycja edycji edycji edycję edycją edycji edycje edycji edycjom edycje edycjami edycjach"],
  [Gender.FEMININE, "klasa klasy klasie klasę klasą klasie klasy klas klasom klasy klasami klasach"],
  [Gender.FEMININE, "liga ligi lidze ligę ligą lidze ligi lig ligom ligi ligami ligach"],
  [Gender.FEMININE, "pozycja pozycji pozycji pozycję pozycją pozycji pozycje pozycji pozycjom pozycje pozycjami pozycjach"],
  [Gender.FEMININE, "sesja sesji sesji sesję sesją sesji sesje sesji sesjom sesje sesjami sesjach"],
  [Gender.FEMININE, "rocznica rocznicy rocznicy rocznicę rocznicą rocznicy rocznice rocznic rocznicom rocznice rocznicami rocznicach"],
  [Gender.FEMININE, "kadencja kadencji kadencji kadencję kadencją kadencji kadencje kadencji kadencjom kadencje kadencjami kadencjach"],
  [Gender.FEMININE, "godzina godziny godzinie godzinę godziną godzinie godziny godzin godzinom godziny godzinami godzinach"],
  [Gender.FEMININE, "wojna wojny wojnie wojnę wojną wojnie wojny wojen wojnom wojny wojnami wojnach"],
  [Gender.FEMININE, "nagroda nagrody nagrodzie nagrodę nagrodą nagrodzie nagrody nagród nagrodom nagrody nagrodami nagrodach"],
  [Gender.FEMININE, "faza fazy fazie fazę fazą fazie fazy faz fazom fazy fazami fazach"],
  [Gender.FEMININE, "seria serii serii serię serią serii serie serii seriom serie seriami seriach"],
  [Gender.FEMININE, "sekunda sekundy sekundzie sekundę sekundą sekundzie sekundy sekund sekundom sekundy sekundami sekundach"],
  [Gender.FEMININE, "doba doby dobie dobę dobą dobie doby dób dobom doby dobami dobach"],
  [Gender.FEMININE, "kategoria kategorii kategorii kategorię kategorią kategorii kategorie kategorii kategoriom kategorie kategoriami kategoriach"],
  [Gender.FEMININE, "linia linii linii linię linią linii linie linii liniom linie liniami liniach"],
  [Gender.FEMININE, "część części części część częścią części części części częściom części częściami częściach"],
  [Gender.FEMININE, "odsłona odsłony odsłonie odsłonę odsłoną odsłonie odsłony odsłon odsłonom odsłony odsłonami odsłonach"],
  [Gender.FEMININE, "kwarta kwarty kwarcie kwartę kwartą kwarcie kwarty kwart kwartom kwarty kwartami kwartach"],
  [Gender.NEUTER, "piętro piętra piętru piętro piętrem piętrze piętra pięter piętrom piętra piętrami piętrach"],
  [Gender.NEUTER, "miejsce miejsca miejscu miejsce miejscem miejscu miejsca miejsc miejscom miejsca miejscami miejscach"],
  [Gender.NEUTER, "wydanie wydania wydaniu wydanie wydaniem wydaniu wydania wydań wydaniom wydania wydaniami wydaniach"],
  [Gender.NEUTER, "okrążenie okrążenia okrążeniu okrążenie okrążeniem okrążeniu okrążenia okrążeń okrążeniom okrążenia okrążeniami okrążeniach"],
  [Gender.NEUTER, "stulecie stulecia stuleciu stulecie stuleciem stuleciu stulecia stuleci stuleciom stulecia stuleciami stuleciach"],
  [Gender.NEUTER, "spotkanie spotkania spotkaniu spotkanie spotkaniem spotkaniu spotkania spotkań spotkaniom spotkania spotkaniami spotkaniach"],
  [Gender.NEUTER, "półrocze półrocza półroczu półrocze półroczem półroczu półrocza półroczy półroczom półrocza półroczami półroczach"],
  [Gender.NEUTER, "urodziny urodzin urodzinom urodziny urodzinami urodzinach urodziny urodzin urodzinom urodziny urodzinami urodzinach"],
  [Gender.MASCULINE, "kwartał kwartału kwartałowi kwartał kwartałem kwartale kwartały kwartałów kwartałom kwartały kwartałami kwartałach"],
  [Gender.MASCULINE, "etap etapu etapowi etap etapem etapie etapy etapów etapom etapy etapami etapach"],
  [Gender.MASCULINE, "mecz meczu meczowi mecz meczem meczu mecze meczów meczom mecze meczami meczach"],
  [Gender.MASCULINE, "sezon sezonu sezonowi sezon sezonem sezonie sezony sezonów sezonom sezony sezonami sezonach"],
  [Gender.MASCULINE, "dzień dnia dniowi dzień dniem dniu dni dni dniom dni dniami dniach"],
  [Gender.MASCULINE, "tydzień tygodnia tygodniowi tydzień tygodniem tygodniu tygodnie tygodni tygodniom tygodnie tygodniami tygodniach"],
  [Gender.MASCULINE, "miesiąc miesiąca miesiącowi miesiąc miesiącem miesiącu miesiące miesięcy miesiącom miesiące miesiącami miesiącach"],
  [Gender.MASCULINE, "wiek wieku wiekowi wiek wiekiem wieku wieki wieków wiekom wieki wiekami wiekach"],
  [Gender.MASCULINE, "raz razu razowi raz razem razie razy razy razom razy razami razach"],
  [Gender.MASCULINE, "punkt punktu punktowi punkt punktem punkcie punkty punktów punktom punkty punktami punktach"],
  [Gender.MASCULINE, "set seta setowi seta setem secie sety setów setom sety setami setach"],
  [Gender.MASCULINE, "stopień stopnia stopniowi stopień stopniem stopniu stopnie stopni stopniom stopnie stopniami stopniach"],
  [Gender.MASCULINE, "poziom poziomu poziomowi poziom poziomem poziomie poziomy poziomów poziomom poziomy poziomami poziomach"],
  [Gender.MASCULINE, "tom tomu tomowi tom tomem tomie tomy tomów tomom tomy tomami tomach"],
  [Gender.MASCULINE, "rozdział rozdziału rozdziałowi rozdział rozdziałem rozdziale rozdziały rozdziałów rozdziałom rozdziały rozdziałami rozdziałach"],
  [Gender.MASCULINE, "odcinek odcinka odcinkowi odcinek odcinkiem odcinku odcinki odcinków odcinkom odcinki odcinkami odcinkach"],
  [Gender.MASCULINE, "turniej turnieju turniejowi turniej turniejem turnieju turnieje turniejów turniejom turnieje turniejami turniejach"],
  [Gender.MASCULINE, "festiwal festiwalu festiwalowi festiwal festiwalem festiwalu festiwale festiwali festiwalom festiwale festiwalami festiwalach"],
  [Gender.MASCULINE, "kongres kongresu kongresowi kongres kongresem kongresie kongresy kongresów kongresom kongresy kongresami kongresach"],
  [Gender.MASCULINE, "krok kroku krokowi krok krokiem kroku kroki kroków krokom kroki krokami krokach"],
  [Gender.MASCULINE, "rząd rzędu rzędowi rząd rzędem rzędzie rzędy rzędów rzędom rzędy rzędami rzędach"],
];

// Units of time, whose amount after "w" is a span (w 10 min: within ten minutes), not a place
export const TIME_UNITS: ReadonlySet<string> = new Set(["h", "min", "s", "ms", "godz.", "sek."]);

// The forms of "godzina", with the case of the hour after them: o godzinie 15 (piętnastej), do godziny 16 (szesnastej)
export const HOUR_WORDS: ReadonlyMap<string, Case> = new Map<string, Case>([
  ["godzina", Case.NOMINATIVE], ["godziny", Case.GENITIVE], ["godzinie", Case.LOCATIVE],
  ["godzinę", Case.ACCUSATIVE], ["godziną", Case.INSTRUMENTAL],
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

// XIX w.: the abbreviation of wiek after a Roman numeral
export const CENTURY_ABBREVIATIONS: Keys = ["w."];

// Names which take a regnal number, by case (nominative, genitive, dative, accusative, instrumental, locative)
// and gender: Jan Paweł II (drugi), Jana Pawła II (drugiego), Elżbiety II (drugiej)
export const REGNAL_NAMES: ReadonlyArray<readonly [string, Gender]> = [
  ["jan jana janowi jana janem janie", Gender.MASCULINE],
  ["paweł pawła pawłowi pawła pawłem pawle", Gender.MASCULINE],
  ["benedykt benedykta benedyktowi benedykta benedyktem benedykcie", Gender.MASCULINE],
  ["leon leona leonowi leona leonem leonie", Gender.MASCULINE],
  ["pius piusa piusowi piusa piusem piusie", Gender.MASCULINE],
  ["grzegorz grzegorza grzegorzowi grzegorza grzegorzem grzegorzu", Gender.MASCULINE],
  ["karol karola karolowi karola karolem karolu", Gender.MASCULINE],
  ["henryk henryka henrykowi henryka henrykiem henryku", Gender.MASCULINE],
  ["ludwik ludwika ludwikowi ludwika ludwikiem ludwiku", Gender.MASCULINE],
  ["kazimierz kazimierza kazimierzowi kazimierza kazimierzem kazimierzu", Gender.MASCULINE],
  ["zygmunt zygmunta zygmuntowi zygmunta zygmuntem zygmuncie", Gender.MASCULINE],
  ["władysław władysława władysławowi władysława władysławem władysławie", Gender.MASCULINE],
  ["bolesław bolesława bolesławowi bolesława bolesławem bolesławie", Gender.MASCULINE],
  ["mieszko mieszka mieszkowi mieszka mieszkiem mieszku", Gender.MASCULINE],
  ["jerzy jerzego jerzemu jerzego jerzym jerzym", Gender.MASCULINE],
  ["edward edwarda edwardowi edwarda edwardem edwardzie", Gender.MASCULINE],
  ["ryszard ryszarda ryszardowi ryszarda ryszardem ryszardzie", Gender.MASCULINE],
  ["wilhelm wilhelma wilhelmowi wilhelma wilhelmem wilhelmie", Gender.MASCULINE],
  ["fryderyk fryderyka fryderykowi fryderyka fryderykiem fryderyku", Gender.MASCULINE],
  ["aleksander aleksandra aleksandrowi aleksandra aleksandrem aleksandrze", Gender.MASCULINE],
  ["mikołaj mikołaja mikołajowi mikołaja mikołajem mikołaju", Gender.MASCULINE],
  ["piotr piotra piotrowi piotra piotrem piotrze", Gender.MASCULINE],
  ["napoleon napoleona napoleonowi napoleona napoleonem napoleonie", Gender.MASCULINE],
  ["filip filipa filipowi filipa filipem filipie", Gender.MASCULINE],
  ["ferdynand ferdynanda ferdynandowi ferdynanda ferdynandem ferdynandzie", Gender.MASCULINE],
  ["józef józefa józefowi józefa józefem józefie", Gender.MASCULINE],
  ["franciszek franciszka franciszkowi franciszka franciszkiem franciszku", Gender.MASCULINE],
  ["leopold leopolda leopoldowi leopolda leopoldem leopoldzie", Gender.MASCULINE],
  ["klemens klemensa klemensowi klemensa klemensem klemensie", Gender.MASCULINE],
  ["iwan iwana iwanowi iwana iwanem iwanie", Gender.MASCULINE],
  ["elżbieta elżbiety elżbiecie elżbietę elżbietą elżbiecie", Gender.FEMININE],
  ["katarzyna katarzyny katarzynie katarzynę katarzyną katarzynie", Gender.FEMININE],
];

// Nouns numbered by a Roman numeral before or after them, with their gender: rozdział IV (czwarty), XXI wieku
// (dwudziestego pierwszego), II wojna (druga). A form ending with "-" stands for every word it starts.
export const ROMAN_NOUNS: ReadonlyArray<readonly [string, Gender]> = [
  ["wiek-", Gender.MASCULINE], ["stuleci-", Gender.NEUTER], ["tysiącleci-", Gender.NEUTER],
  ["wojn-", Gender.FEMININE], ["rzeczpospolit-", Gender.FEMININE], ["rzeczypospolit-", Gender.FEMININE],
  ["kadencj-", Gender.FEMININE], ["edycj-", Gender.FEMININE], ["połow-", Gender.FEMININE],
  ["kwartał-", Gender.MASCULINE], ["kwartal-", Gender.MASCULINE], ["nagrod-", Gender.FEMININE],
  ["półrocz-", Gender.NEUTER], ["miejsc-", Gender.NEUTER], ["zjazd-", Gender.MASCULINE], ["kongres-", Gender.MASCULINE],
  ["sesj-", Gender.FEMININE], ["festiwal-", Gender.MASCULINE], ["konkurs-", Gender.MASCULINE],
  ["piętr-", Gender.NEUTER], ["kategori-", Gender.FEMININE], ["rozdzia-", Gender.MASCULINE],
  ["liga", Gender.FEMININE], ["ligi", Gender.FEMININE], ["lidze", Gender.FEMININE], ["ligę", Gender.FEMININE],
  ["klasa", Gender.FEMININE], ["klasy", Gender.FEMININE], ["klasie", Gender.FEMININE], ["klasę", Gender.FEMININE],
  ["rok", Gender.MASCULINE], ["roku", Gender.MASCULINE], ["stopnia", Gender.MASCULINE],
  ["stopień", Gender.MASCULINE], ["stopniu", Gender.MASCULINE], ["tura", Gender.FEMININE], ["tury", Gender.FEMININE],
  ["turze", Gender.FEMININE], ["turę", Gender.FEMININE], ["etap", Gender.MASCULINE], ["etapu", Gender.MASCULINE],
  ["etapie", Gender.MASCULINE], ["faza", Gender.FEMININE], ["fazy", Gender.FEMININE], ["fazie", Gender.FEMININE],
  ["fazę", Gender.FEMININE], ["grupa", Gender.FEMININE], ["grupy", Gender.FEMININE], ["grupie", Gender.FEMININE],
  ["miejsce", Gender.NEUTER], ["miejsca", Gender.NEUTER], ["miejscu", Gender.NEUTER], ["część", Gender.FEMININE],
  ["części", Gender.FEMININE], ["tom", Gender.MASCULINE], ["tomu", Gender.MASCULINE], ["tomie", Gender.MASCULINE],
  ["sezon", Gender.MASCULINE], ["sezonu", Gender.MASCULINE], ["sezonie", Gender.MASCULINE],
];

// --- Notation: how numbers are written ---
export const NOTATION = notation({
  groupSeparators: "   .", decimalSeparators: ",.", timeSeparators: ":.",
  currencies: keysOf(CURRENCIES), scales: keysOf(SCALES), units: keysOf(UNITS), fractions: true,
});

// Words before the name of a street, after which a/b is a house number: ulica Marszałkowska 10/12
export const STREET_WORDS: ReadonlySet<string> = new Set([
  "ulica", "ulicy", "ulicę", "ulicą", "aleja", "alei", "aleję", "aleje", "alejach", "plac", "placu", "placem", "osiedle",
  "osiedlu", "rondo", "rondzie",
]);

// Imperatives which take an amount as their object: Dodaj 1/4 łyżeczki (jedną czwartą)
export const IMPERATIVES: ReadonlySet<string> = new Set([
  "dodaj", "dodać", "kup", "weź", "wsyp", "wlej", "dosyp", "odlej", "odsyp", "zjedz", "wypij", "użyj",
]);

// Words after a fraction which are not the noun it divides: Ułamek 1/2 jest prosty, Wynik to 3/8
export const FRACTION_STOPS: ReadonlySet<string> = new Set([
  "to", "jest", "są", "był", "była", "było", "i", "a", "oraz", "lub", "albo", "czy", "w", "we",
  "na", "z", "ze", "do", "od", "po", "przez", "dla", "za", "o", "się", "nie", "że", "jak",
  // Plural spans of time: 2/3 dni is two or three days, but 3/4 godziny three quarters of an hour
  "dni", "tygodni", "miesięcy", "lat", "godzin", "minut", "sekund", "razy",
]);

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
export const JOINERS: Joiners = {
  point: "kropka", dot: "kropka", range: "-", by: "na", score: " do ", minus: "minus", plus: "plus",
  fraction: " i ", half: " i pół",
};

// 15.08.2020 r.: the year has four digits, "r." may follow it
export const DATE_FORMAT = dateFormat({ separators: ".", boundaries: ".,", shortYear: false, suffixes: ["r."] });

// 600 123 456, 22 123 45 67 (after a word of PHONE_WORDS), 600-123-456, 68-324-22-55
export const PHONE_FORMATS = [
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: " ", boundaries: "", closing: "" }),
  phoneFormat({ groups: [[2, 2], [3, 3], [2, 2], [2, 2]], separators: " ", boundaries: "", closing: "" }),
  phoneFormat({ groups: [[3, 3], [3, 3], [3, 3]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[2, 2], [3, 3], [2, 2], [2, 2]], separators: "-", boundaries: ".,-", closing: "-" }),
  phoneFormat({ groups: [[1, 1], [3, 3], [3, 3], [3, 3]], separators: " ", leadingZero: true, boundaries: "", closing: "" }),
  phoneFormat({ groups: [[3, 3], [3, 3]], separators: " ", boundaries: "", closing: "" }),
  // 31-123 Kraków: a postal code, before the name of its town
  phoneFormat({ groups: [[2, 2], [3, 3]], separators: "-", boundaries: ".,-", closing: "-" }),
];

// Words before a phone number written with spaces, which else reads as a count: numer 600 123 456
export const PHONE_WORDS: ReadonlySet<string> = new Set([
  "numer", "numerem", "numeru", "tel", "telefon", "telefonu", "telefonem", "infolinia", "faks", "fax",
  "komórka", "komórki", "komórkowy", "komórkowego", "kontaktowy", "kontaktowego", "zaufania", "alarmowy",
  "alarmowego", "sms", "whatsapp",
]);

// Verbs of calling, after which "pod" or "na" takes a phone number: zadzwoń pod 112, dzwoń na 503 222 111
export const CALL_VERBS: ReadonlySet<string> = new Set([
  "dzwoń", "zadzwoń", "dzwońcie", "zadzwońcie", "dzwonić", "zadzwonić", "dzwoni", "zadzwonił", "dzwonił",
  "pisz", "napisz", "wyślij",
]);

// 2020 r.: a year with "roku" after it
export const YEAR_SUFFIXES: Keys = ["r."];
export const YEAR_FORMAT = suffixFormat({ space: true, sentenceDot: true });

// Echo's classes, and Polish letters read as Latin-1 from Windows-1250 or ISO 8859-2: zupe³nie -> zupełnie
export const CHARACTERS: Characters = [
  { characters: "…", canonical: "..." }, // Ellipsis
  { characters: "«»„“”‟˝", canonical: '"' }, // Quotes
  { characters: "‘’‚‛´`ʼ", canonical: "'" }, // Apostrophes
  { characters: "​‌‍﻿", canonical: "" }, // Invisible: zero-width space, non-joiner, joiner, BOM
  { characters: "³", canonical: "ł", letter: true },   { characters: "£", canonical: "Ł", letter: true },
  { characters: "¹±", canonical: "ą", letter: true },  { characters: "¥¡", canonical: "Ą", letter: true },
  { characters: "œ¶\u009c", canonical: "ś", letter: true }, { characters: "Œ¦\u008c", canonical: "Ś", letter: true },
  { characters: "¿", canonical: "ż", letter: true },   { characters: "¯", canonical: "Ż", letter: true },
  { characters: "Ÿ¼\u009f", canonical: "ź", letter: true }, { characters: "\u008f¬", canonical: "Ź", letter: true },
  { characters: "ê", canonical: "ę", letter: true },   { characters: "Ê", canonical: "Ę", letter: true },
  { characters: "æ", canonical: "ć", letter: true },   { characters: "Æ", canonical: "Ć", letter: true },
  { characters: "ñ", canonical: "ń", letter: true },   { characters: "Ñ", canonical: "Ń", letter: true },
];

export const SYMBOLS: Readings = {
  "&": ["end"],        "+": ["plus"],           "=": ["równa się"], "%": ["procent"],  "°": ["stopni"],
  "#": [""],           "/": [""],               "²": ["do kwadratu"], "³": ["do sześcianu"],
  "π": ["pi"],         "×": ["razy"],           "€": ["euro"],     "$": ["dolarów"],   "@": ["małpa"],
  "±": ["plus minus"], "‰": ["promili"],        "§": ["paragraf"],
};

// Prepositions before an inflected abbreviation, by the case they give it: przy ul. Długiej, od ul. Długiej
export const LOCATIVE_WORDS: ReadonlySet<string> = new Set(["w", "we", "na", "o", "po", "przy"]);
export const GENITIVE_WORDS: ReadonlySet<string> = new Set([
  "od", "do", "z", "ze", "u", "obok", "koło", "około", "ok", "wzdłuż", "dla", "spod", "sprzed", "zza",
  "naprzeciwko", "blisko", "niedaleko", "wokół", "bez", "róg", "rogu", "okolicach", "okolice",
  // A title after the name of a street or a patron: ul. ks. Popiełuszki, im. prof. Nowaka
  "ul", "al", "pl", "os", "ulica", "ulicy", "aleja", "alei", "plac", "placu", "im", "imienia", "pw",
]);
export const INSTRUMENTAL_WORDS: ReadonlySet<string> = new Set(["pod", "nad", "przed", "między", "pomiędzy"]);

/**
 * The case of an abbreviated noun: from the preposition before it, or the ending of the word after it.
 * skręć w ul. Długą (accusative), idź ul. Długą (instrumental), przy ul. Długiej (locative), z prof. Nowakiem
 * (instrumental), do dyr. Kowalskiego (genitive). A comma or a bracket right before it ends the phrase of the word
 * before: Camping Plus sp. z o. o., ul. Kuropatwia
 */
export function abbreviationCase(context: Context): Case {
  const head = context.head();
  let last = head.length - 1;
  while (last >= 0 && (head[last] === " " || head[last] === " ")) {
    last--;
  }
  const clause = last >= 0 && ',;:()[]"'.includes(head[last]);
  const before = clause ? "" : context.before();
  const next = toLower(context.after());
  if (next.endsWith("ą")) {
    return before === "w" || before === "we" || before === "na" ? Case.ACCUSATIVE : Case.INSTRUMENTAL;
  }
  if (LOCATIVE_WORDS.has(before)) {
    return Case.LOCATIVE;
  }
  const along = before === "z" && context.before(2) === "zgodnie"; // zgodnie z art. 5
  if (along || INSTRUMENTAL_WORDS.has(before) || next.endsWith("em") || next.endsWith("im") || next.endsWith("ym")) {
    return Case.INSTRUMENTAL;
  }
  if (next.endsWith("owi") || next.endsWith("emu")) {
    return Case.DATIVE;
  }
  if (GENITIVE_WORDS.has(before) || next.endsWith("ego") || next.endsWith("ej")) {
    return Case.GENITIVE;
  }
  return Case.NOMINATIVE;
}

// The readings of an abbreviated noun from its six forms (in the order of Case), separated by "|"; each one only when
// a condition holds, if any
export function declined(forms: string, when?: Condition): Reading[] {
  const parts = forms.split("|");
  const readings: Reading[] = [];
  for (let c = 1; c < 6; ++c) {
    readings.push({ words: parts[c], when: (context) => (!when || when(context)) && abbreviationCase(context) === c });
  }
  readings.push(when ? { words: parts[0], when } : parts[0]);
  return readings;
}

// The readings of an abbreviation which mostly ends a sentence, with its dot when a capitalized word follows: itd. Potem
export function closing(words: string): Reading[] {
  return [{ words: `${words}.`, when: beforeName }, words];
}

// Between numbers: the "m." of mieszkanie in ul. Mickiewicza 3 m. 4
export function afterNumberBeforeNumber(context: Context): boolean {
  const before = context.before();
  return beforeNumber(context) && before !== "" && isDigit(before[before.length - 1]);
}

// The word after it ends with one of given endings (lowercase): św. Anny
export function beforeEnding(endings: readonly string[]): Condition {
  return (context) => {
    const next = toLower(context.after());
    return endings.some((ending) => next.endsWith(ending));
  };
}

// The word before it is one of given words (lowercase): Msza św.
export function afterWord(words: readonly string[]): Condition {
  const set = new Set(words);
  return (context) => set.has(context.before());
}

// Saints whose names end with -a in the nominative (św. Anna), unlike the genitive of the others (św. Jana)
export const FEMALE_SAINTS: ReadonlySet<string> = new Set([
  "anna", "maria", "barbara", "teresa", "katarzyna", "jadwiga", "faustyna", "małgorzata", "elżbieta",
  "zofia", "agnieszka", "kinga", "łucja", "monika", "cecylia", "helena", "weronika", "rozalia", "klara",
  "joanna", "magdalena", "urszula", "marta", "brygida", "apolonia", "genowefa",
]);

// św. after a noun it describes (Msza św.), then before the name of a saint, by its ending
export const SAINT: readonly Reading[] = [
  { words: "święta", when: afterWord(["msza"]) },
  { words: "świętej", when: afterWord(["mszy"]) },
  { words: "świętą", when: afterWord(["mszę", "mszą"]) },
  { words: "święte", when: afterWord(["pismo"]) },
  { words: "świętego", when: afterWord(["pisma"]) },
  { words: "świętej", when: beforeEnding(["y", "i"]) },
  { words: "święta", when: (context) => FEMALE_SAINTS.has(toLower(context.after())) },
  { words: "świętego", when: beforeEnding(["a", "ego"]) },
  { words: "świętą", when: beforeEnding(["ą", "ę"]) },
  { words: "świętym", when: beforeEnding(["em", "im", "ym"]) },
  { words: "świętemu", when: beforeEnding(["owi", "emu"]) },
  "święty",
];

export const TERMS: Terms = {
  abbreviations: {
    // Before a number only: nr 5
    "nr": [{ words: "numer", when: beforeNumber }],
    "Nr": [{ words: "numer", when: beforeNumber }],
    "nr.": [{ words: "numer", when: beforeNumber }],
    "ust.": [{ words: "ustęp", when: beforeNumber }],
    "pkt": [{ words: "punkt", when: beforeNumber }],
    "pkt.": [{ words: "punkt", when: beforeNumber }],
    "art.": declined("artykuł|artykułu|artykułowi|artykuł|artykułem|artykule", beforeNumber),
    "str.": declined("strona|strony|stronie|stronę|stroną|stronie", beforeNumber),
    "poz.": [{ words: "pozycja", when: beforeNumber }],
    "ok.": [{ words: "około", when: beforeNumber }],
    "Ok.": [{ words: "Około", when: beforeNumber }],
    "lok.": [{ words: "lokal", when: beforeNumber }],
    "m.": [{ words: "mieszkanie", when: afterNumberBeforeNumber }], // ul. Mickiewicza 3 m. 4
    "#": [{ words: "numer", when: beforeNumber }],
    "§": declined("paragraf|paragrafu|paragrafowi|paragraf|paragrafem|paragrafie", beforeNumber),
    // Inflected nouns: przy ul. Długiej, z prof. Nowakiem
    "ul.": declined("ulica|ulicy|ulicy|ulicę|ulicą|ulicy"),
    "Ul.": declined("Ulica|Ulicy|Ulicy|Ulicę|Ulicą|Ulicy"),
    "al.": declined("aleja|alei|alei|aleję|aleją|alei", beforeName),
    "Al.": declined("Aleje|Alej|Alejom|Aleje|Alejami|Alejach", beforeName),
    "pl.": declined("plac|placu|placowi|plac|placem|placu", beforeName),
    "os.": declined("osiedle|osiedla|osiedlu|osiedle|osiedlem|osiedlu", beforeName),
    "godz.": declined("godzina|godziny|godzinie|godzinę|godziną|godzinie"),
    "Godz.": declined("Godzina|Godziny|Godzinie|Godzinę|Godziną|Godzinie"),
    "w godz.": ["w godzinach"], // w godz. 8-16
    "W godz.": ["W godzinach"],
    "tel.": declined("telefon|telefonu|telefonowi|telefon|telefonem|telefonie"),
    "Tel.": declined("Telefon|Telefonu|Telefonowi|Telefon|Telefonem|Telefonie"),
    "dr": declined("doktor|doktora|doktorowi|doktora|doktorem|doktorze"),
    "dr.": declined("doktor|doktora|doktorowi|doktora|doktorem|doktorze"),
    "prof.": declined("profesor|profesora|profesorowi|profesora|profesorem|profesorze"),
    "Prof.": declined("Profesor|Profesora|Profesorowi|Profesora|Profesorem|Profesorze"),
    "Dr": declined("Doktor|Doktora|Doktorowi|Doktora|Doktorem|Doktorze"),
    "Ks.": declined("Ksiądz|Księdza|Księdzu|Księdza|Księdzem|Księdzu"),
    "Gen.": declined("Generał|Generała|Generałowi|Generała|Generałem|Generale"),
    "mgr": declined("magister|magistra|magistrowi|magistra|magistrem|magistrze"),
    "inż.": declined("inżynier|inżyniera|inżynierowi|inżyniera|inżynierem|inżynierze"),
    "dyr.": declined("dyrektor|dyrektora|dyrektorowi|dyrektora|dyrektorem|dyrektorze"),
    "gen.": declined("generał|generała|generałowi|generała|generałem|generale"),
    "płk": declined("pułkownik|pułkownika|pułkownikowi|pułkownika|pułkownikiem|pułkowniku"),
    "ks.": declined("ksiądz|księdza|księdzu|księdza|księdzem|księdzu"),
    "św.": SAINT,
    "hab.": ["habilitowany"],
    // tzw. by the ending of the noun after it; -ie is mostly the dative or locative of a feminine noun (strefie)
    "tzw.": [
      { words: "tak zwana", when: beforeEnding(["a"]) }, { words: "tak zwaną", when: beforeEnding(["ą"]) },
      { words: "tak zwanego", when: beforeEnding(["ego", "u"]) }, { words: "tak zwanej", when: beforeEnding(["ej", "ie"]) },
      { words: "tak zwanych", when: beforeEnding(["ych", "ów"]) }, { words: "tak zwanym", when: beforeEnding(["ym"]) },
      { words: "tak zwane", when: beforeEnding(["e", "o"]) }, "tak zwany",
    ],
    // Anywhere
    "np.": ["na przykład"],
    "Np.": ["Na przykład"],
    "itd.": closing("i tak dalej"),
    "itp.": closing("i tym podobne"),
    "tzn.": ["to znaczy"],
    "tj.": ["to jest"],
    "m.in.": ["między innymi"],
    "M.in.": ["Między innymi"],
    "wg": ["według"],
    "Wg": ["Według"],
    "wg.": ["według"],
    "Wg.": ["Według"],
    "ds.": ["do spraw"],
    "im.": [{ words: "imienia", when: beforeName }],
    "pt.": [{ words: "pod tytułem", when: beforeName }],
    "nt.": ["na temat"],
    "ws.": ["w sprawie"],
    "pw.": ["pod wezwaniem"],
    "zob.": ["zobacz"],
    "Zob.": ["Zobacz"],
    "dn.": ["dnia"],
    "br.": closing("bieżącego roku"),
    "ub.r.": closing("ubiegłego roku"),
    "ub. r.": closing("ubiegłego roku"),
    "p.n.e.": closing("przed naszą erą"),
    "n.e.": closing("naszej ery"),
    "nr tel.": ["numer telefonu"],
    "Nr tel.": ["Numer telefonu"],
    "tel. kom.": ["telefon komórkowy"],
    "Tel. kom.": ["Telefon komórkowy"],
    "sp. z o.o.": closing("spółka z ograniczoną odpowiedzialnością"),
    "Sp. z o.o.": closing("Spółka z ograniczoną odpowiedzialnością"),
    "sp. z o. o.": closing("spółka z ograniczoną odpowiedzialnością"),
    "Sp. z o. o.": closing("Spółka z ograniczoną odpowiedzialnością"),
    "z o.o.": closing("z ograniczoną odpowiedzialnością"),
    "S.A.": closing("Spółka Akcyjna"),
  },
  web: { dot: "kropka", at: "małpa", slash: "ukośnik", hyphen: "myślnik", underscore: "podkreślnik",
    digits: ["zero", "jeden", "dwa", "trzy", "cztery", "pięć", "sześć", "siedem", "osiem", "dziewięć"],
  },
};
