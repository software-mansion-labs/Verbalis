#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/unicode.h>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::de {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::RomanCues;
using ::verbalis::numbers::SuffixFormat;

inline constexpr std::array<std::u32string_view, 20> kOnes = {
  U"null",   U"eins",    U"zwei",    U"drei",      U"vier",      U"fünf",      U"sechs",    U"sieben",    U"acht",     U"neun",
  U"zehn",   U"elf",     U"zwölf",   U"dreizehn",  U"vierzehn",  U"fünfzehn",  U"sechzehn", U"siebzehn",  U"achtzehn", U"neunzehn"
};

inline constexpr std::array<std::u32string_view, 10> kTens = {
  U"", U"", U"zwanzig", U"dreißig", U"vierzig", U"fünfzig", U"sechzig", U"siebzig", U"achtzig", U"neunzig"
};

inline const std::vector<std::u32string> kMonths = {
  U"Januar", U"Februar", U"März",      U"April",   U"Mai",      U"Juni",
  U"Juli",   U"August",  U"September", U"Oktober", U"November", U"Dezember"
};

// Ordinal stems which differ from their cardinals: erst, dritt, siebt, acht
inline const std::unordered_map<std::u32string, std::u32string> kOrdinalStems = {
  {U"eins", U"ers"}, {U"drei", U"drit"}, {U"sieben", U"sieb"}, {U"acht", U"ach"}
};

// Currencies, and prices per unit (12 €/m²): their reading after "pro" follows the whole price
inline const std::unordered_map<std::u32string, std::u32string> kCurrencies = {
  {U"€", U"Euro"}, {U"Euro", U"Euro"}, {U"EUR", U"Euro"}, {U"$", U"Dollar"}, {U"Dollar", U"Dollar"}, {U"£", U"Pfund"},
  {U"CHF", U"Franken"},
  {U"€/m²", U"Euro pro Quadratmeter"},
  {U"€/qm", U"Euro pro Quadratmeter"},
  {U"€/kWh", U"Euro pro Kilowattstunde"},
  {U"€/l", U"Euro pro Liter"},
  {U"€/kg", U"Euro pro Kilogramm"},
  {U"€/Monat", U"Euro pro Monat"},
  {U"€/Jahr", U"Euro pro Jahr"},
  {U"€/Tag", U"Euro pro Tag"},
  {U"€/Std.", U"Euro pro Stunde"},
  {U"€/h", U"Euro pro Stunde"},
  {U"€/Stunde", U"Euro pro Stunde"},
  {U"€/Nacht", U"Euro pro Nacht"},
  {U"€/Person", U"Euro pro Person"},
  {U"$/oz", U"Dollar pro Unze"},
  {U"Dollar/oz", U"Dollar pro Unze"},
  {U"€/Std", U"Euro pro Stunde"},
  {U"Euro/Std.", U"Euro pro Stunde"}
};

inline const std::unordered_map<std::u32string, Forms> kScales = {
  {U"Mio.", {U"Million", U"Millionen"}},       {U"Million", {U"Million", U"Millionen"}},
  {U"Millionen", {U"Million", U"Millionen"}},  {U"Mrd.", {U"Milliarde", U"Milliarden"}},
  {U"Milliarde", {U"Milliarde", U"Milliarden"}}, {U"Milliarden", {U"Milliarde", U"Milliarden"}},
  {U"Tsd.", {U"tausend", U"tausend"}},         {U"k", {U"tausend", U"tausend"}},
  // Lowercase, as also written: 2,2 millionen €
  {U"million", {U"Million", U"Millionen"}},     {U"millionen", {U"Million", U"Millionen"}},
  {U"milliarde", {U"Milliarde", U"Milliarden"}}, {U"milliarden", {U"Milliarde", U"Milliarden"}}
};

inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"km", {U"Kilometer", U"Kilometer"}},       {U"m", {U"Meter", U"Meter"}},
  {U"cm", {U"Zentimeter", U"Zentimeter"}},     {U"mm", {U"Millimeter", U"Millimeter"}},
  {U"nm", {U"Nanometer", U"Nanometer"}},       {U"m²", {U"Quadratmeter", U"Quadratmeter"}},
  {U"km²", {U"Quadratkilometer", U"Quadratkilometer"}}, {U"m³", {U"Kubikmeter", U"Kubikmeter"}},
  {U"ha", {U"Hektar", U"Hektar"}},             {U"kg", {U"Kilogramm", U"Kilogramm"}},
  {U"g", {U"Gramm", U"Gramm"}},                {U"mg", {U"Milligramm", U"Milligramm"}},
  {U"t", {U"Tonne", U"Tonnen"}},               {U"l", {U"Liter", U"Liter"}},
  {U"ml", {U"Milliliter", U"Milliliter"}},     {U"TB", {U"Terabyte", U"Terabyte"}},
  {U"GB", {U"Gigabyte", U"Gigabyte"}},         {U"MB", {U"Megabyte", U"Megabyte"}},
  {U"Hz", {U"Hertz", U"Hertz"}},               {U"kHz", {U"Kilohertz", U"Kilohertz"}},
  {U"MHz", {U"Megahertz", U"Megahertz"}},      {U"GHz", {U"Gigahertz", U"Gigahertz"}},
  {U"W", {U"Watt", U"Watt"}},                  {U"kW", {U"Kilowatt", U"Kilowatt"}},
  {U"kWh", {U"Kilowattstunde", U"Kilowattstunden"}}, {U"V", {U"Volt", U"Volt"}},
  {U"km/h", {U"Kilometer pro Stunde", U"Kilometer pro Stunde"}}, {U"°C", {U"Grad Celsius", U"Grad Celsius"}},
  {U"°", {U"Grad", U"Grad"}},                  {U"h", {U"Stunde", U"Stunden"}},
  {U"Std.", {U"Stunde", U"Stunden"}},          {U"min", {U"Minute", U"Minuten"}},
  {U"Min.", {U"Minute", U"Minuten"}},          {U"s", {U"Sekunde", U"Sekunden"}},
  {U"Sek.", {U"Sekunde", U"Sekunden"}},        {U"ct", {U"Cent", U"Cent"}},
  {U"PS", {U"PS", U"PS"}},
  {U"kwh", {U"Kilowattstunde", U"Kilowattstunden"}}, {U"mv", {U"Millivolt", U"Millivolt"}},
  {U"MW", {U"Megawatt", U"Megawatt"}},         {U"GW", {U"Gigawatt", U"Gigawatt"}},
  {U"MWh", {U"Megawattstunde", U"Megawattstunden"}}, {U"GWh", {U"Gigawattstunde", U"Gigawattstunden"}},
  {U"TWh", {U"Terawattstunde", U"Terawattstunden"}},
  // Per unit: 42259/m², 56,3 /km²
  {U"/m²", {U"pro Quadratmeter", U"pro Quadratmeter"}}, {U"/km²", {U"pro Quadratkilometer", U"pro Quadratkilometer"}},
  {U"/m³", {U"pro Kubikmeter", U"pro Kubikmeter"}},
  {U"µm", {U"Mikrometer", U"Mikrometer"}},
  {U"μm", {U"Mikrometer", U"Mikrometer"}},
  {U"µg", {U"Mikrogramm", U"Mikrogramm"}},
  {U"μg", {U"Mikrogramm", U"Mikrogramm"}},
  {U"cm²", {U"Quadratzentimeter", U"Quadratzentimeter"}},
  {U"mm²", {U"Quadratmillimeter", U"Quadratmillimeter"}},
  {U"cm³", {U"Kubikzentimeter", U"Kubikzentimeter"}},
  {U"ft", {U"Fuß", U"Fuß"}},
  {U"mV", {U"Millivolt", U"Millivolt"}},
  {U"kV", {U"Kilovolt", U"Kilovolt"}},
  {U"mA", {U"Milliampere", U"Milliampere"}},
  {U"°F", {U"Grad Fahrenheit", U"Grad Fahrenheit"}},
  {U"″", {U"Zoll", U"Zoll"}},
  {U"MEUR", {U"Millionen Euro", U"Millionen Euro"}},
  {U"mEUR", {U"Millionen Euro", U"Millionen Euro"}},
  {U"Km", {U"Kilometer", U"Kilometer"}},
  {U"KM", {U"Kilometer", U"Kilometer"}},
  {U"KG", {U"Kilogramm", U"Kilogramm"}},
  {U"dB", {U"Dezibel", U"Dezibel"}},
  {U"db", {U"Dezibel", U"Dezibel"}},
  {U"qm", {U"Quadratmeter", U"Quadratmeter"}},
  {U"TL", {U"Teelöffel", U"Teelöffel"}},
  {U"EL", {U"Esslöffel", U"Esslöffel"}},
  {U"St.", {U"Stück", U"Stück"}},
  {U"Stk.", {U"Stück", U"Stück"}},
  {U"Stk", {U"Stück", U"Stück"}},
  {U"gr.", {U"Gramm", U"Gramm"}},
  {U"Vol.%", {U"Volumenprozent", U"Volumenprozent"}},
  {U"Vol.-%", {U"Volumenprozent", U"Volumenprozent"}},
  {U"mmHg", {U"Millimeter Quecksilbersäule", U"Millimeter Quecksilbersäule"}},
  {U"mah", {U"Milliamperestunde", U"Milliamperestunden"}},
  {U"mAh", {U"Milliamperestunde", U"Milliamperestunden"}},
  {U"Wh", {U"Wattstunde", U"Wattstunden"}},
  {U"L", {U"Liter", U"Liter"}},
  {U"Mm", {U"Millimeter", U"Millimeter"}},
  {U"° C", {U"Grad Celsius", U"Grad Celsius"}},
  {U"° F", {U"Grad Fahrenheit", U"Grad Fahrenheit"}},
  {U"l/m²", {U"Liter pro Quadratmeter", U"Liter pro Quadratmeter"}},
  {U"m³/h", {U"Kubikmeter pro Stunde", U"Kubikmeter pro Stunde"}},
  {U"m3/h", {U"Kubikmeter pro Stunde", U"Kubikmeter pro Stunde"}},
  {U"Gbit/s", {U"Gigabit pro Sekunde", U"Gigabit pro Sekunde"}},
  {U"GBit/s", {U"Gigabit pro Sekunde", U"Gigabit pro Sekunde"}},
  {U"Gb/s", {U"Gigabit pro Sekunde", U"Gigabit pro Sekunde"}},
  {U"Mbit/s", {U"Megabit pro Sekunde", U"Megabit pro Sekunde"}},
  {U"kWh/Jahr", {U"Kilowattstunden pro Jahr", U"Kilowattstunden pro Jahr"}},
  {U"Bq/kg", {U"Becquerel pro Kilogramm", U"Becquerel pro Kilogramm"}},
  {U"cd/m²", {U"Candela pro Quadratmeter", U"Candela pro Quadratmeter"}},
  {U"µg/kg", {U"Mikrogramm pro Kilogramm", U"Mikrogramm pro Kilogramm"}},
  {U"μg/kg", {U"Mikrogramm pro Kilogramm", U"Mikrogramm pro Kilogramm"}},
  {U"/min", {U"pro Minute", U"pro Minute"}},
  {U"St", {U"Stück", U"Stück"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"ein halb"}, {{1, 3}, U"ein Drittel"}, {{2, 3}, U"zwei Drittel"}, {{1, 4}, U"ein Viertel"},
  {{3, 4}, U"drei Viertel"}
};

// Words after a fraction which mark it as one: 2/3 der
inline const std::unordered_set<std::u32string> kFractionWords = {U"der", U"des", U"eines", U"einer", U"aller"};

// Determiners after which an ordinal takes a weak ending: am dritten, der dritte (nominative)
inline const std::unordered_set<std::u32string> kWeak = {
  U"den", U"dem", U"des", U"am", U"im", U"vom", U"zum", U"zur", U"beim", U"ins", U"diesem", U"diesen", U"dieses",
  U"jedem", U"jeden", U"einen", U"einem", U"einer", U"eines", U"keinen", U"keinem", U"keiner", U"keines",
  U"meinen", U"meinem", U"meiner", U"seinen", U"seinem", U"seiner", U"ihren", U"ihrem", U"ihrer", U"unseren",
  U"unserem", U"unserer"
};
inline const std::unordered_set<std::u32string> kWeakNominative = {
  U"der", U"die", U"das", U"diese", U"dieser", U"jede", U"jeder", U"jedes", U"eine", U"keine", U"meine", U"seine",
  U"ihre", U"unsere"
};

// ein-words, after which an ordinal takes a mixed ending: ein zweiter, ein drittes
inline const std::unordered_set<std::u32string> kMixed = {
  U"ein", U"kein", U"mein", U"dein", U"sein", U"ihr", U"unser", U"euer"
};

// Words after which an amount stays in the nominative: ist 1 Euro, als 1 Euro, aber 1 Euro ist
inline const std::unordered_set<std::u32string> kNominativeWords = {
  U"ist", U"sind", U"war", U"waren", U"wird", U"werden", U"wurde", U"wurden", U"bleibt", U"blieb", U"sei", U"seien",
  U"wäre", U"als", U"wie", U"und", U"oder", U"pro", U"je", U"mal", U"aber", U"denn", U"sondern", U"doch", U"dass",
  U"weil", U"wenn", U"ob", U"obwohl", U"da", U"damit", U"sodass"
};

// Prepositions by the case they govern
inline const std::unordered_set<std::u32string> kDative = {
  U"mit", U"nach", U"bei", U"seit", U"von", U"zu", U"aus", U"gegenüber", U"in", U"an", U"auf", U"unter", U"über",
  U"vor", U"hinter", U"neben", U"zwischen", U"ab"
};
inline const std::unordered_set<std::u32string> kAccusative = {U"für", U"durch", U"gegen", U"ohne", U"um", U"bis"};
inline const std::unordered_set<std::u32string> kGenitive = {
  U"wegen", U"während", U"trotz", U"statt", U"innerhalb", U"außerhalb"
};

// Words before a date (am 12. 3.), and before a year (im Jahr 1945, and months)
inline const std::unordered_set<std::u32string> kDateCues = {U"am", U"vom", U"zum", U"bis", U"ab", U"seit", U"den", U"der"};
inline const std::unordered_set<std::u32string> kYearCues = {
  U"jahr", U"jahre", U"jahren", U"im", U"seit", U"bis", U"von", U"ab", U"anfang", U"ende", U"mitte", U"sommer",
  U"winter", U"frühjahr", U"herbst", U"frühling", U"anno", U"januar", U"februar", U"märz", U"april", U"mai",
  U"juni", U"juli", U"august", U"september", U"oktober", U"november", U"dezember"
};

// Nouns which stay singular after a number: 1500 Stück. Like plural nouns, they make a 4-digit number a count.
inline const std::unordered_set<std::u32string> kMeasureWords = {
  U"Stück", U"Mal", U"Prozent", U"Grad", U"Euro", U"Dollar", U"Pfund", U"Kilo", U"Gramm", U"Paar", U"Cent",
  U"Franken"
};

// Noun gender by ending: the exceptions that matter after numbers (matched as compound endings), then the rules
inline const std::vector<std::pair<std::u32string, Gender>> kGenders = {
  {U"jahrhundert", Gender::NEUTER}, {U"jahrtausend", Gender::NEUTER}, {U"semester", Gender::NEUTER},
  {U"mitglied", Gender::NEUTER},    {U"kapitel", Gender::NEUTER},     {U"quartal", Gender::NEUTER},
  {U"prozent", Gender::NEUTER},     {U"zimmer", Gender::NEUTER},      {U"rennen", Gender::NEUTER},
  {U"person", Gender::FEMININE},    {U"gramm", Gender::NEUTER},       {U"stück", Gender::NEUTER},
  {U"spiel", Gender::NEUTER},       {U"leben", Gender::NEUTER},       {U"liter", Gender::MASCULINE},
  {U"meter", Gender::MASCULINE},    {U"monat", Gender::MASCULINE},    {U"kunde", Gender::MASCULINE},
  {U"junge", Gender::MASCULINE},    {U"kilo", Gender::NEUTER},        {U"jahr", Gender::NEUTER},
  {U"kind", Gender::NEUTER},        {U"glas", Gender::NEUTER},        {U"haus", Gender::NEUTER},
  {U"land", Gender::NEUTER},        {U"buch", Gender::NEUTER},        {U"bild", Gender::NEUTER},
  {U"ende", Gender::NEUTER},        {U"pfund", Gender::NEUTER},       {U"euro", Gender::MASCULINE},
  {U"name", Gender::MASCULINE},     {U"käse", Gender::MASCULINE},     {U"grad", Gender::MASCULINE},
  {U"mal", Gender::NEUTER},         {U"tor", Gender::NEUTER},         {U"ei", Gender::NEUTER}
};
inline const std::vector<std::u32string> kFeminineEndings = {
  U"e", U"ung", U"heit", U"keit", U"schaft", U"ion", U"tät", U"ik", U"ur", U"enz", U"anz", U"ei", U"in", U"ie"
};
inline const std::vector<std::u32string> kNeuterEndings = {U"chen", U"lein", U"ment", U"um", U"tum", U"ma", U"o"};

// Ludwig XIV. (der Vierzehnte), Kapitel III (drei), im XX. Jahrhundert (zwanzigsten)
inline const RomanCues kRomanCues = {
  .names = {
    U"ludwig", U"karl", U"friedrich", U"wilhelm", U"heinrich", U"otto", U"rudolf", U"albrecht", U"maximilian",
    U"ferdinand", U"leopold", U"joseph", U"franz", U"georg", U"eduard", U"richard", U"johannes", U"paul", U"benedikt",
    U"leo", U"pius", U"gregor", U"klemens", U"innozenz", U"elisabeth", U"katharina", U"maria", U"viktoria", U"philipp",
    U"napoleon", U"alexander", U"nikolaus", U"peter", U"iwan", U"konstantin", U"ramses"
  },
  .before = {
    U"kapitel", U"teil", U"band", U"buch", U"akt", U"szene", U"abschnitt", U"artikel", U"anhang", U"phase", U"stufe",
    U"klasse", U"staffel", U"folge", U"runde", U"typ", U"liga"
  },
  .after = {
    U"jahrhundert", U"jahrhunderts", U"jahrhunderte", U"jahrhunderten", U"jahrtausend", U"jahrtausends",
    U"weltkrieg", U"weltkriegs", U"weltkrieges", U"vatikanische", U"vatikanischen"
  }
};

// Names with a regnal number which are feminine: Elisabeth die Zweite
inline const std::unordered_set<std::u32string> kFeminineNames = {
  U"elisabeth", U"katharina", U"maria", U"viktoria"
};

// Words after hours, minutes, and seconds, which the reading says: 02:02:23 Uhr, 13:46:36 Stunden
inline const Keys kSecondsSuffixes = {U"Uhr", U"Stunden", U"Std.", U"h"};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U". \u00a0\u202f", .decimal_separators = U",.", .time_separators = U":.", .time_suffixes = {U"Uhr"},
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"Komma", .dot = U"Punkt", .range = U" bis ", .by = U"mal", .score = U" zu ", .minus = U"minus", .plus = U"plus",
  .fraction = U" ", .rating = U"von"
};

// 1.5.2024, 1.5., 12. 3.: the year is optional, a dot closes the month
inline const DateFormat kDateFormat = {
  .separators = U".", .boundaries = U".,", .year_required = false, .closing_separator = true, .spaced = true
};

// 90er, 90ern, 90ers
inline const Keys kDecadeSuffixes = {U"er", U"ern", U"ers"};

// Ordinal endings written after the digits: am 3ten, zum 20sten, dem 121tem
inline const Keys kOrdinalSuffixes = {
  U"te", U"ten", U"tem", U"ter", U"tes", U"ste", U"sten", U"stem", U"ster", U"stes"
};
inline const SuffixFormat kDecadeFormat = {.boundaries = U".,-"};

using ::verbalis::text::Readings;
using ::verbalis::text::after_any;

// Words before "§" which take its plural: im § 5 (in dem Paragrafen)
inline const std::unordered_set<std::u32string> kParagraphPlurals = {
  U"im", U"vom", U"zum", U"beim", U"dem", U"des", U"den"
};

inline const Readings kSymbols = {
  {U"&", {U"und"}},       {U"€", {U"Euro"}},      {U"$", {U"Dollar"}},    {U"£", {U"Pfund"}},    {U"+", {U"plus"}},
  {U"=", {U"gleich"}},    {U"@", {U"at"}},        {U"%", {U"Prozent"}},   {U"°", {U"Grad"}},     {U"#", {U"Hashtag"}},
  {U"²", {U"Quadrat"}},   {U"³", {U"hoch drei"}}, {U"¹", {U"hoch eins"}}, {U"π", {U"Pi"}},       {U"×", {U"mal"}},
  {U"½", {U"ein halb"}},  {U"§§", {U"Paragrafen"}}, {U"§", {{U"Paragrafen", after_any(kParagraphPlurals)}, U"Paragraf"}},
  {U"≥", {U"größer oder gleich"}},
  {U"≤", {U"kleiner oder gleich"}},
  {U"±", {U"plus minus"}},
  {U"+/-", {U"plus minus"}}
};

using ::verbalis::text::before_number;

// St. Pauli: Sankt before a name, but not after a number (5 St. Äpfel counts pieces)
inline bool saint(const ::verbalis::text::Context& context) {
  std::u32string before = context.before();
  return ::verbalis::text::before_name(context) &&
         (before.empty() || !::verbalis::text::unicode::isdigit(before.back()));
}

// Days of a month, the word before a month abbreviation in a date: 2. Jan. 2003
inline const std::unordered_set<std::u32string> kDays = [] {
  std::unordered_set<std::u32string> days;
  for (char32_t day = 1; day <= 31; day++) {
    std::u32string tens(1, U'0' + day / 10), ones(1, U'0' + day % 10);
    days.insert(day < 10 ? ones : tens + ones);
    days.insert(tens + ones); // 02. Jan.
  }
  return days;
}();
using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: Abs. 5
    {U"Abs.", {{U"Absatz", before_number}}},
    {U"Nr.", {{U"Nummer", before_number}}},
    {U"Tel.", {U"Telefon"}},
    {U"i.V.m.", {U"in Verbindung mit"}},
    {U"incl.", {U"inklusive"}},
    {U"St.", {{U"Sankt", saint}}},
    {U"S.", {{U"Seite", before_number}}},
    {U"Art.", {{U"Artikel", before_number}}},
    {U"Kap.", {{U"Kapitel", before_number}}},
    {U"Bd.", {{U"Band", before_number}}},
    // After a day: 2. Jan. 2003
    {U"Jan.", {{U"Januar", after_any(kDays)}}},
    {U"Feb.", {{U"Februar", after_any(kDays)}}},
    {U"Mär.", {{U"März", after_any(kDays)}}},
    {U"Apr.", {{U"April", after_any(kDays)}}},
    {U"Jun.", {{U"Juni", after_any(kDays)}}},
    {U"Jul.", {{U"Juli", after_any(kDays)}}},
    {U"Aug.", {{U"August", after_any(kDays)}}},
    {U"Sep.", {{U"September", after_any(kDays)}}},
    {U"Sept.", {{U"September", after_any(kDays)}}},
    {U"Okt.", {{U"Oktober", after_any(kDays)}}},
    {U"Nov.", {{U"November", after_any(kDays)}}},
    {U"Dez.", {{U"Dezember", after_any(kDays)}}},
    {U"jan.", {{U"Januar", after_any(kDays)}}},
    {U"feb.", {{U"Februar", after_any(kDays)}}},
    {U"mär.", {{U"März", after_any(kDays)}}},
    {U"apr.", {{U"April", after_any(kDays)}}},
    {U"jun.", {{U"Juni", after_any(kDays)}}},
    {U"jul.", {{U"Juli", after_any(kDays)}}},
    {U"aug.", {{U"August", after_any(kDays)}}},
    {U"sep.", {{U"September", after_any(kDays)}}},
    {U"sept.", {{U"September", after_any(kDays)}}},
    {U"okt.", {{U"Oktober", after_any(kDays)}}},
    {U"nov.", {{U"November", after_any(kDays)}}},
    {U"dez.", {{U"Dezember", after_any(kDays)}}},
    // Anywhere
    {U"ca.", {U"circa"}},
    {U"z.B.", {U"zum Beispiel"}},
    {U"z. B.", {U"zum Beispiel"}},
    {U"d.h.", {U"das heißt"}},
    {U"d. h.", {U"das heißt"}},
    {U"bzw.", {U"beziehungsweise"}},
    {U"usw.", {U"und so weiter"}},
    {U"u.a.", {U"unter anderem"}},
    {U"u. a.", {U"unter anderem"}},
    {U"z.T.", {U"zum Teil"}},
    {U"z. T.", {U"zum Teil"}},
    {U"ggf.", {U"gegebenenfalls"}},
    {U"evtl.", {U"eventuell"}},
    {U"inkl.", {U"inklusive"}},
    {U"vgl.", {U"vergleiche"}},
    {U"etc.", {U"et cetera"}},
    {U"Dr.", {U"Doktor"}},
    {U"Prof.", {U"Professor"}},
    {U"Mr.", {U"Mister"}},
    {U"Mrs.", {U"Misses"}},
    {U"Ms.", {U"Miss"}},
    {U"MwSt.", {U"Mehrwertsteuer"}},
    {U"Mwst.", {U"Mehrwertsteuer"}},
    {U"Abb.", {U"Abbildung"}},
    {U"gem.", {U"gemäß"}},
    {U"a. M.", {U"am Main"}},
    {U"a.M.", {U"am Main"}},
    {U"mind.", {U"mindestens"}},
    {U"Az.", {U"Aktenzeichen"}},
    {U"v. Chr.", {U"vor Christus"}},
    {U"n. Chr.", {U"nach Christus"}},
    {U"Vj.", {U"Vorjahr"}},
    {U"Jahrb.", {U"Jahrbuch"}},
    {U"erm.", {U"ermäßigt"}},
    {U"rel.", {U"relative"}},
    {U"Jg.", {U"Jahrgang"}},
    {U"Tbl.", {U"Tablette"}},
    {U"tgl.", {U"täglich"}},
  },
  .web = {.dot = U"Punkt", .at = U"at", .slash = U"Schrägstrich", .hyphen = U"Bindestrich", .underscore = U"Unterstrich",
          .digits = {U"null", U"eins", U"zwei", U"drei", U"vier", U"fünf", U"sechs", U"sieben", U"acht", U"neun"}},
  .closing = {U"usw.", U"etc."}
};

} // namespace verbalis::languages::de
