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

namespace verbalis::languages::fr {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::RomanCues;

inline constexpr std::array<std::u32string_view, 20> kOnes = {
  U"zéro", U"un",   U"deux",  U"trois",  U"quatre",   U"cinq",   U"six",   U"sept",     U"huit",     U"neuf",
  U"dix",  U"onze", U"douze", U"treize", U"quatorze", U"quinze", U"seize", U"dix-sept", U"dix-huit", U"dix-neuf"
};

// Tens, where 70 and 90 are built on 60 and 80: soixante-dix, quatre-vingt-dix
inline constexpr std::array<std::u32string_view, 9> kTens = {
  U"", U"dix", U"vingt", U"trente", U"quarante", U"cinquante", U"soixante", U"", U"quatre-vingt"
};

inline const std::vector<std::u32string> kMonths = {
  U"janvier", U"février", U"mars",      U"avril",   U"mai",      U"juin",
  U"juillet", U"août",    U"septembre", U"octobre", U"novembre", U"décembre"
};

inline const std::unordered_set<std::u32string> kWeekdays = {
  U"lundi", U"mardi", U"mercredi", U"jeudi", U"vendredi", U"samedi", U"dimanche"
};

inline const std::unordered_map<std::u32string, Forms> kCurrencies = {
  {U"€", {U"euro", U"euros"}},    {U"$", {U"dollar", U"dollars"}}, {U"£", {U"livre", U"livres"}},
  {U"CHF", {U"franc suisse", U"francs suisses"}}, {U"M€", {U"million d'euros", U"millions d'euros"}}
};

inline const std::unordered_map<std::u32string, std::u32string> kScales = {
  {U"millions", U"millions"}, {U"million", U"million"}, {U"milliards", U"milliards"}, {U"milliard", U"milliard"},
  {U"k", U"mille"},           {U"K", U"mille"}
};

inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"km", {U"kilomètre", U"kilomètres"}},           {U"m", {U"mètre", U"mètres"}},
  {U"cm", {U"centimètre", U"centimètres"}},         {U"mm", {U"millimètre", U"millimètres"}},
  {U"nm", {U"nanomètre", U"nanomètres"}},           {U"m²", {U"mètre carré", U"mètres carrés"}},
  {U"km²", {U"kilomètre carré", U"kilomètres carrés"}}, {U"m³", {U"mètre cube", U"mètres cubes"}},
  {U"ha", {U"hectare", U"hectares"}},               {U"kg", {U"kilogramme", U"kilogrammes"}},
  {U"g", {U"gramme", U"grammes"}},                  {U"mg", {U"milligramme", U"milligrammes"}},
  {U"t", {U"tonne", U"tonnes"}},                    {U"l", {U"litre", U"litres"}},
  {U"L", {U"litre", U"litres"}},                    {U"cl", {U"centilitre", U"centilitres"}},
  {U"ml", {U"millilitre", U"millilitres"}},         {U"mL", {U"millilitre", U"millilitres"}},
  {U"km/h", {U"kilomètre par heure", U"kilomètres par heure"}}, {U"°C", {U"degré Celsius", U"degrés Celsius"}},
  {U"°", {U"degré", U"degrés"}},                    {U"To", {U"téraoctet", U"téraoctets"}},
  {U"Go", {U"gigaoctet", U"gigaoctets"}},           {U"Mo", {U"mégaoctet", U"mégaoctets"}},
  {U"Ko", {U"kilooctet", U"kilooctets"}},           {U"GHz", {U"gigahertz", U"gigahertz"}},
  {U"MHz", {U"mégahertz", U"mégahertz"}},           {U"Hz", {U"hertz", U"hertz"}},
  {U"kW", {U"kilowatt", U"kilowatts"}},             {U"kWh", {U"kilowattheure", U"kilowattheures"}},
  {U"W", {U"watt", U"watts"}},                      {U"V", {U"volt", U"volts"}},
  {U"Mbit/s", {U"mégabit par seconde", U"mégabits par seconde"}}, {U"min", {U"minute", U"minutes"}},
  {U"s", {U"seconde", U"secondes"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"un demi"}, {{1, 3}, U"un tiers"}, {{2, 3}, U"deux tiers"}, {{1, 4}, U"un quart"},
  {{3, 4}, U"trois quarts"}
};

// Nouns in -e are feminine unless listed below or ending like a masculine noun; other feminine nouns are listed
inline const std::unordered_set<std::u32string> kFeminineNouns = {
  U"fois", U"voix", U"nuit", U"part", U"dent", U"main", U"fleur", U"mort", U"photo", U"vidéo", U"radio", U"moto",
  U"eau", U"loi", U"foi", U"fin", U"faim", U"peur", U"mer", U"cour", U"tour", U"souris", U"noix", U"paix", U"croix",
  U"forêt", U"image", U"page", U"plage", U"cage", U"nage", U"rage"
};
inline const std::unordered_set<std::u32string> kMasculineInE = {
  U"monde", U"livre", U"groupe", U"modèle", U"problème", U"système", U"programme", U"exemple", U"nombre", U"membre",
  U"homme", U"père", U"frère", U"rôle", U"style", U"site", U"espace", U"service", U"commerce", U"couple",
  U"magazine", U"musée", U"lycée", U"verre", U"chiffre", U"véhicule", U"stade", U"poste", U"signe", U"rythme",
  U"volume", U"domaine", U"vote", U"texte", U"téléphone", U"jeune", U"collège", U"kilowattheure", U"gramme",
  U"kilogramme", U"milligramme", U"pourcentage", U"ordre", U"type", U"genre", U"acte", U"arbre", U"autre",
  U"génome", U"crime", U"trimestre", U"semestre", U"siècle", U"masque", U"risque", U"casque", U"disque", U"manque",
  U"parc", U"vice", U"film"
};
inline const std::vector<std::u32string> kMasculineEndings = {U"age", U"ège", U"isme", U"ème", U"aire", U"tre", U"cle", U"ste", U"ome"};

// Words after a number which are not the noun it counts
inline const std::unordered_set<std::u32string> kStopWords = {
  U"avec", U"dans", U"pour", U"mais", U"donc", U"puis", U"sur", U"sous", U"par", U"chez", U"vers", U"entre", U"sans",
  U"contre", U"comme", U"selon", U"alors", U"aussi", U"déjà", U"encore", U"très", U"plus", U"moins", U"tout",
  U"tous", U"toute", U"toutes"
};

inline const std::unordered_set<std::u32string> kNumberWords = {
  U"zéro", U"un", U"une", U"deux", U"trois", U"quatre", U"cinq", U"six", U"sept", U"huit", U"neuf", U"dix", U"onze",
  U"douze", U"treize", U"quatorze", U"quinze", U"seize", U"vingt", U"vingts", U"trente", U"quarante", U"cinquante",
  U"soixante", U"cent", U"cents", U"mille"
};

// Words before a time with a dot, which make it a time: à 18.30
inline const std::unordered_set<std::u32string> kTimeWords = {
  U"à", U"vers", U"dès", U"de", U"jusqu'à", U"avant", U"après", U"entre", U"et"
};

// XIXe, Ier, Ire: the suffixes of an ordinal in Roman numerals
inline const Keys kRomanOrdinalSuffixes = {U"er", U"re", U"ère", U"e", U"ème"};

// Louis XIV (quatorze), chapitre III (trois), le XXI siècle (vingt et unième); XIXe and Ier have their own pattern
inline const RomanCues kRomanCues = {
  .names = {
    U"louis", U"henri", U"charles", U"philippe", U"françois", U"jean", U"paul", U"jean-paul", U"benoît", U"léon",
    U"pie", U"grégoire", U"clément", U"innocent", U"napoléon", U"élisabeth", U"édouard", U"george", U"georges",
    U"guillaume", U"frédéric", U"nicolas", U"alexandre", U"pierre", U"ramsès", U"ferdinand", U"alphonse", U"carlos",
    U"felipe", U"victor-emmanuel", U"emmanuel", U"catherine", U"marie", U"victoria"
  },
  .before = {
    U"chapitre", U"tome", U"livre", U"acte", U"scène", U"titre", U"article", U"partie", U"volume", U"saison",
    U"épisode", U"annexe", U"phase", U"vatican", U"niveau", U"type", U"classe", U"catégorie"
  },
  .after = {
    U"siècle", U"siècles", U"millénaire", U"république", U"empire", U"arrondissement", U"congrès", U"édition",
    U"festival", U"législature", U"régiment", U"corps", U"armée"
  }
};

// Names with a regnal number which are feminine (Élisabeth Ire), and nouns after a numeral which make it feminine
inline const std::unordered_set<std::u32string> kFeminineNames = {U"élisabeth", U"catherine", U"marie", U"victoria"};
inline const std::unordered_set<std::u32string> kFeminineRomanNouns = {
  U"république", U"édition", U"législature", U"armée"
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U" \u00a0\u202f.", .decimal_separators = U",.", .time_separators = U":.h", .time_suffixes = {U"h"},
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"virgule", .dot = U"point", .range = U" à ", .by = U"par", .score = U" à ", .minus = U"moins", .plus = U"plus"
};

// 05/11, 05/11/2024, 1.5.2024: the year is optional
inline const DateFormat kDateFormat = {.separators = U"./", .boundaries = U".,/", .year_required = false};

// 06 12 34 56 78, 06.12.34.56.78, 0612345678
inline const PhoneFormat kPhoneFormat = {
  .groups = {{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}}, .separators = U" .", .optional_separators = true,
  .leading_zero = true, .boundaries = U".,", .closing = U""
};

// 1er, 1re, 2e, 2ème, 2nd, 3èmes
inline const Keys kOrdinalSuffixes = {
  U"ères", U"èmes", U"ère", U"ème", U"eme", U"ers", U"er", U"res", U"re", U"es", U"ndes", U"nde", U"nds", U"nd",
  U"e",    U"è"
};

using ::verbalis::text::Readings;

inline const Readings kSymbols = {
  {U"&", {U"et"}},       {U"+", {U"plus"}},    {U"=", {U"égale"}}, {U"@", {U"arobase"}}, {U"%", {U"pour cent"}},
  {U"°", {U"degrés"}},   {U"§", {U"paragraphe"}}, {U"#", {U"hashtag"}}, {U"²", {U"carré"}}, {U"³", {U"cube"}},
  {U"π", {U"pi"}},       {U"×", {U"fois"}},    {U"½", {U"un demi"}}, {U"€", {U"euros"}},  {U"$", {U"dollars"}},
  {U"£", {U"livres"}}
};

using ::verbalis::text::before_number;
using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: n° 5
    {U"n°", {{U"numéro", before_number}}},
    {U"N°", {{U"numéro", before_number}}},
    {U"nº", {{U"numéro", before_number}}},
    {U"p.", {{U"page", before_number}}},
    {U"art.", {{U"article", before_number}}},
    {U"Art.", {{U"article", before_number}}},
  },
  .web = {.dot = U"point", .at = U"arobase", .slash = U"slash"}
};

} // namespace verbalis::languages::fr
