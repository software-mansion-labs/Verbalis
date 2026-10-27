#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/readings.h>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::it {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::RomanCues;
using ::verbalis::numbers::SuffixFormat;

inline constexpr std::array<std::u32string_view, 20> kOnes = {
  U"zero",   U"uno",    U"due",         U"tre",      U"quattro",    U"cinque",      U"sei",
  U"sette",  U"otto",   U"nove",        U"dieci",    U"undici",     U"dodici",      U"tredici",
  U"quattordici", U"quindici", U"sedici", U"diciassette", U"diciotto", U"diciannove"
};

inline constexpr std::array<std::u32string_view, 10> kTens = {
  U"", U"", U"venti", U"trenta", U"quaranta", U"cinquanta", U"sessanta", U"settanta", U"ottanta", U"novanta"
};

// Ordinals up to ten; later ones are the cardinal with -esimo
inline constexpr std::array<std::u32string_view, 11> kOrdinals = {
  U"", U"primo", U"secondo", U"terzo", U"quarto", U"quinto", U"sesto", U"settimo", U"ottavo", U"nono", U"decimo"
};

inline const std::vector<std::u32string> kMonths = {
  U"gennaio", U"febbraio", U"marzo",     U"aprile",  U"maggio",   U"giugno",
  U"luglio",  U"agosto",   U"settembre", U"ottobre", U"novembre", U"dicembre"
};

inline const std::unordered_map<std::u32string, Forms> kCurrencies = {
  {U"€", {U"euro", U"euro"}}, {U"$", {U"dollaro", U"dollari"}}, {U"£", {U"sterlina", U"sterline"}}
};

// Most scales are written as the word they are read with: 3 milioni
inline const std::unordered_map<std::u32string, Forms> kScales = {
  {U"mila", {U"mila", U"mila"}},             {U"k", {U"mila", U"mila"}},           {U"K", {U"mila", U"mila"}},
  {U"milione", {U"milione", U"milione"}},    {U"milioni", {U"milioni", U"milioni"}},
  {U"miliardo", {U"miliardo", U"miliardo"}}, {U"miliardi", {U"miliardi", U"miliardi"}},
  {U"mln", {U"milione", U"milioni"}},        {U"mld", {U"miliardo", U"miliardi"}}
};

inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"km", {U"chilometro", U"chilometri"}},          {U"m", {U"metro", U"metri"}},
  {U"cm", {U"centimetro", U"centimetri"}},          {U"mm", {U"millimetro", U"millimetri"}},
  {U"m²", {U"metro quadro", U"metri quadri"}},      {U"m2", {U"metro quadro", U"metri quadri"}},
  {U"km²", {U"chilometro quadrato", U"chilometri quadrati"}}, {U"m³", {U"metro cubo", U"metri cubi"}},
  {U"kg", {U"chilo", U"chili"}},                    {U"g", {U"grammo", U"grammi"}},
  {U"mg", {U"milligrammo", U"milligrammi"}},        {U"l", {U"litro", U"litri"}},
  {U"ml", {U"millilitro", U"millilitri"}},          {U"TB", {U"terabyte", U"terabyte"}},
  {U"GB", {U"giga", U"giga"}},                      {U"MB", {U"mega", U"mega"}},
  {U"Hz", {U"hertz", U"hertz"}},                    {U"kHz", {U"kilohertz", U"kilohertz"}},
  {U"MHz", {U"megahertz", U"megahertz"}},           {U"GHz", {U"gigahertz", U"gigahertz"}},
  {U"W", {U"watt", U"watt"}},                       {U"kW", {U"kilowatt", U"kilowatt"}},
  {U"kWh", {U"kilowattora", U"kilowattora"}},       {U"V", {U"volt", U"volt"}},
  {U"km/h", {U"chilometro orario", U"chilometri orari"}}, {U"°C", {U"grado", U"gradi"}},
  {U"°", {U"grado", U"gradi"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"un mezzo"}, {{1, 3}, U"un terzo"}, {{2, 3}, U"due terzi"}, {{1, 4}, U"un quarto"},
  {{3, 4}, U"tre quarti"}
};

// Words before a time with a dot, which make it a time: alle 21.30
inline const std::unordered_set<std::u32string> kTimeWords = {
  U"alle", U"ore", U"dalle", U"le", U"verso", U"entro", U"per"
};

// Giovanni XXIII (ventitreesimo), nel XX secolo (ventesimo), capitolo IV (quarto), la II guerra (seconda)
inline const RomanCues kRomanCues = {
  .names = {
    U"giovanni", U"paolo", U"benedetto", U"pio", U"leone", U"gregorio", U"clemente", U"innocenzo", U"bonifacio",
    U"emanuele", U"umberto", U"carlo", U"luigi", U"enrico", U"filippo", U"federico", U"francesco", U"ferdinando",
    U"napoleone", U"elisabetta", U"giorgio", U"guglielmo", U"alessandro", U"edoardo", U"riccardo", U"giacomo",
    U"pietro", U"amedeo", U"ramses", U"maria", U"vittoria", U"caterina", U"giovanna"
  },
  .before = {
    U"capitolo", U"libro", U"tomo", U"atto", U"scena", U"parte", U"titolo", U"articolo", U"secolo", U"volume",
    U"stagione", U"fase", U"legislatura", U"canto"
  },
  .after = {
    U"secolo", U"secoli", U"millennio", U"guerra", U"congresso", U"edizione", U"festival", U"legislatura",
    U"giornata", U"convegno", U"conferenza", U"assemblea", U"concorso", U"forum", U"simposio", U"seminario",
    U"campionato", U"olimpiade", U"repubblica", U"anniversario", U"centenario", U"premio", U"torneo", U"salone",
    U"settimana", U"municipio", U"circoscrizione", U"rapporto", U"piano", U"stagione"
  }
};

// Umberto I°: an ordinal marker on a Roman numeral
inline const Keys kRomanMarkers = {U"°", U"º", U"ª"};

// Names with a regnal number, and nouns around a numeral, which are feminine: Elisabetta II (seconda), parte II
inline const std::unordered_set<std::u32string> kFeminineNames = {
  U"elisabetta", U"maria", U"vittoria", U"caterina", U"giovanna"
};
inline const std::unordered_set<std::u32string> kFeminineRomanNouns = {
  U"scena", U"parte", U"stagione", U"fase", U"legislatura", U"classe", U"guerra", U"edizione", U"giornata",
  U"conferenza", U"assemblea", U"olimpiade", U"repubblica", U"settimana", U"circoscrizione"
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U".", .decimal_separators = U",.", .time_separators = U":.",
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"punto", .dot = U"punto", .range = U"-", .by = U"per", .score = U" a ", .minus = U"meno", .plus = U"più"
};

// 24/03/2022
inline const DateFormat kDateFormat = {.boundaries = U".,/-"};

// 333 1234567, 06 1234 5678, 06 12345678
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 3}, {6, 7}}, .separators = U" "},
  {.groups = {{2, 4}, {3, 4}, {4, 4}}, .separators = U" ", .leading_zero = true},
  {.groups = {{2, 4}, {6, 8}}, .separators = U" ", .leading_zero = true}
};

// 1º, 2 ª: a sign, possibly after a space
inline const Keys kOrdinalSigns = {U"º", U"ª"};
inline const SuffixFormat kOrdinalSignFormat = {.space = true};

// 21esimo, 33esima
inline const Keys kOrdinalSuffixes = {U"esimo", U"esima", U"esimi", U"esime"};

using ::verbalis::text::Readings;

// "&" only joins English names: AT&T, R&B
inline const Readings kSymbols = {
  {U"&", {U"and"}},         {U"+", {U"più"}},       {U"=", {U"uguale a"}}, {U"@", {U"chiocciola"}}, {U"%", {U"per cento"}},
  {U"°", {U"gradi"}},       {U"#", {U"hashtag"}},   {U"²", {U"al quadrato"}}, {U"³", {U"al cubo"}}, {U"π", {U"pi greco"}},
  {U"×", {U"per"}},         {U"€", {U"euro"}},      {U"$", {U"dollari"}},  {U"£", {U"sterline"}},   {U"§", {U"paragrafo"}}
};

using ::verbalis::text::before_number;
using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: n. 5
    {U"n.", {{U"numero", before_number}}},
    {U"N.", {{U"numero", before_number}}},
    {U"nr.", {{U"numero", before_number}}},
    {U"n°", {{U"numero", before_number}}},
    {U"N°", {{U"numero", before_number}}},
    {U"art.", {{U"articolo", before_number}}},
    {U"Art.", {{U"articolo", before_number}}},
    {U"pag.", {{U"pagina", before_number}}},
    {U"cap.", {{U"capitolo", before_number}}},
    {U"§", {{U"paragrafo", before_number}}},
  },
  .web = {.dot = U"punto", .at = U"chiocciola", .slash = U"slash"}
};

} // namespace verbalis::languages::it
