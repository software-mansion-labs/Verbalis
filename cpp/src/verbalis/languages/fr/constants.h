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
  {U"CHF", {U"franc suisse", U"francs suisses"}}, {U"M€", {U"million d'euros", U"millions d'euros"}},
  {U"€uros", {U"euro", U"euros"}},
  {U"M$", {U"million de dollars", U"millions de dollars"}}, {U"G$", {U"milliard de dollars", U"milliards de dollars"}},
  // A price per unit: 2 000 €/mois
  {U"€/mois", {U"euro par mois", U"euros par mois"}}, {U"€/an", {U"euro par an", U"euros par an"}},
  {U"€/jour", {U"euro par jour", U"euros par jour"}}, {U"€/semaine", {U"euro par semaine", U"euros par semaine"}},
  {U"€/h", {U"euro de l'heure", U"euros de l'heure"}}, {U"€/heure", {U"euro de l'heure", U"euros de l'heure"}},
  {U"€/nuit", {U"euro la nuit", U"euros la nuit"}}, {U"€/personne", {U"euro par personne", U"euros par personne"}},
  {U"€/kg", {U"euro le kilo", U"euros le kilo"}}, {U"€/m²", {U"euro le mètre carré", U"euros le mètre carré"}},
  {U"€/l", {U"euro le litre", U"euros le litre"}}
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
  {U"s", {U"seconde", U"secondes"}},
  {U"ch", {U"cheval", U"chevaux"}},
  {U"ko", {U"kilooctet", U"kilooctets"}},
  {U"ms", {U"milliseconde", U"millisecondes"}},
  {U"pts", {U"point", U"points"}},
  {U"kms", {U"kilomètre", U"kilomètres"}},
  {U"Ha", {U"hectare", U"hectares"}},
  {U"Kg", {U"kilogramme", U"kilogrammes"}},
  {U"mn", {U"minute", U"minutes"}},
  {U"sec", {U"seconde", U"secondes"}},
  {U"dB", {U"décibel", U"décibels"}},
  {U"MVA", {U"mégavoltampère", U"mégavoltampères"}},
  {U"kV", {U"kilovolt", U"kilovolts"}},
  {U"Mhz", {U"mégahertz", U"mégahertz"}},
  {U"Mbits/s", {U"mégabit par seconde", U"mégabits par seconde"}},
  {U"Gbit/s", {U"gigabit par seconde", U"gigabits par seconde"}},
  {U"° C", {U"degré Celsius", U"degrés Celsius"}},
  {U"p.", {U"page", U"pages"}},
  {U"pp.", {U"page", U"pages"}},
  {U"Gbps", {U"gigabit par seconde", U"gigabits par seconde"}},
  {U"gHz", {U"gigahertz", U"gigahertz"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"un demi"}, {{1, 3}, U"un tiers"}, {{2, 3}, U"deux tiers"}, {{1, 4}, U"un quart"},
  {{3, 4}, U"trois quarts"}
};

// Words after a fraction which mark it as one: 1/3 d'eau
inline const std::unordered_set<std::u32string> kFractionWords = {U"de", U"du", U"des", U"d'"};

// Nouns in -e, -té, -tion, -sion, and -aison are feminine unless listed below or ending like a masculine noun; other
// feminine nouns are listed
inline const std::unordered_set<std::u32string> kFeminineNouns = {
  U"fois", U"voix", U"nuit", U"part", U"dent", U"main", U"fleur", U"mort", U"photo", U"vidéo", U"radio", U"moto",
  U"eau", U"loi", U"foi", U"fin", U"faim", U"peur", U"mer", U"cour", U"tour", U"souris", U"noix", U"paix", U"croix",
  U"forêt", U"image", U"page", U"plage", U"cage", U"nage", U"rage", U"chanson", U"leçon", U"façon", U"boisson",
  U"prison", U"cuisson", U"sœur", U"fourmi", U"clé", U"paroi", U"tribu", U"vertu", U"brebis", U"perdrix"
};
inline const std::vector<std::u32string> kFeminineEndings = {U"té", U"tion", U"sion", U"aison"};
inline const std::unordered_set<std::u32string> kMasculineInE = {
  U"monde", U"livre", U"groupe", U"modèle", U"problème", U"système", U"programme", U"exemple", U"nombre", U"membre",
  U"homme", U"père", U"frère", U"rôle", U"style", U"site", U"espace", U"service", U"commerce", U"couple",
  U"magazine", U"musée", U"lycée", U"verre", U"chiffre", U"véhicule", U"stade", U"poste", U"signe", U"rythme",
  U"volume", U"domaine", U"vote", U"texte", U"téléphone", U"jeune", U"collège", U"kilowattheure", U"gramme",
  U"kilogramme", U"milligramme", U"pourcentage", U"ordre", U"type", U"genre", U"acte", U"arbre", U"autre",
  U"génome", U"crime", U"trimestre", U"semestre", U"siècle", U"masque", U"risque", U"casque", U"disque", U"manque",
  U"parc", U"vice", U"film", U"été", U"côté", U"comité", U"traité", U"député", U"pâté", U"arrêté", U"comté",
  U"karaté", U"velouté"
};
inline const std::vector<std::u32string> kMasculineEndings = {U"age", U"ège", U"isme", U"ème", U"aire", U"tre", U"cle", U"ste", U"ome"};

// Words after a number which are not the noun it counts
inline const std::unordered_set<std::u32string> kStopWords = {
  U"avec", U"dans", U"pour", U"mais", U"donc", U"puis", U"sur", U"sous", U"par", U"chez", U"vers", U"entre", U"sans",
  U"contre", U"comme", U"selon", U"alors", U"aussi", U"déjà", U"encore", U"très", U"plus", U"moins", U"tout",
  U"tous", U"toute", U"toutes", U"des"
};

inline const std::unordered_set<std::u32string> kNumberWords = {
  U"zéro", U"un", U"une", U"deux", U"trois", U"quatre", U"cinq", U"six", U"sept", U"huit", U"neuf", U"dix", U"onze",
  U"douze", U"treize", U"quatorze", U"quinze", U"seize", U"vingt", U"vingts", U"trente", U"quarante", U"cinquante",
  U"soixante", U"cent", U"cents", U"mille"
};

// Words before a number which make it an ordinal, without the plural s of quatre-vingts and cents: les années
// quatre-vingt, page deux cent, l'an mille neuf cent
inline const std::unordered_set<std::u32string> kOrdinalWords = {
  U"années", U"année", U"an", U"l'an", U"page", U"numéro", U"chapitre", U"article", U"chambre"
};

// Words before a version, which is not a date: version 3.8.10
inline const std::unordered_set<std::u32string> kVersionWords = {U"version", U"v", U"ios", U"android"};

// Words before a decade written with an s: les 80s
inline const std::unordered_set<std::u32string> kDecadeWords = {U"les", U"des", U"aux", U"dans", U"années"};

// Words before a number which make it a code: numéro 1035836, code 75001
inline const std::unordered_set<std::u32string> kCodeWords = {U"numéro", U"n°", U"nº", U"code", U"réf", U"référence"};

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
    U"épisode", U"annexe", U"phase", U"vatican", U"niveau", U"type", U"classe", U"catégorie", U"l'acte",
    U"l'article", U"l'annexe", U"l'épisode"
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
  .group_separators = U" \u00a0\u202f.'’", .decimal_separators = U",.", .time_separators = U":.h", .time_suffixes = {U"h"},
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"point", .dot = U"point", .range = U" à ", .by = U"par", .score = U" à ", .minus = U"moins", .plus = U"plus",
  .fraction = U" et ", .half = U" et demi", .rating = U"sur"
};

// 05/11, 05/11/2024, 1.5.2024: the year is optional
inline const DateFormat kDateFormat = {.separators = U"./", .boundaries = U".,/", .year_required = false};

// 12-05-2024, but not 12-05, which may be a range or a score
inline const DateFormat kDashDateFormat = {.separators = U"-", .boundaries = U".,/"};

// 06 12 34 56 78, 06.12.34.56.78, 0612345678
inline const PhoneFormat kPhoneFormat = {
  .groups = {{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}}, .separators = U" .", .optional_separators = true,
  .leading_zero = true, .boundaries = U".,", .closing = U""
};

// 1er, 1re, 2e, 2ème, 2nd, 3èmes
inline const Keys kOrdinalSuffixes = {
  U"ères", U"èmes", U"ère", U"ere", U"ème", U"eme", U"ers", U"er", U"res", U"re", U"es", U"ndes", U"nde", U"nds", U"nd",
  U"e",    U"è"
};

using ::verbalis::text::Readings;

inline const Readings kSymbols = {
  {U"&", {U"et"}},       {U"+", {U"plus"}},    {U"=", {U"égale"}}, {U"@", {U"arobase"}}, {U"%", {U"pour cent"}},
  {U"°", {U"degrés"}},   {U"§", {U"paragraphe"}}, {U"#", {U"hashtag"}}, {U"²", {U"carré"}}, {U"³", {U"cube"}},
  {U"π", {U"pi"}},       {U"×", {U"fois"}},    {U"½", {U"un demi"}}, {U"€", {U"euros"}},  {U"$", {U"dollars"}},
  {U"£", {U"livres"}},   {U"±", {U"plus ou moins"}}, {U"+/-", {U"plus ou moins"}}, {U"‰", {U"pour mille"}},
  {U"≤", {U"inférieur ou égal à"}},
  {U"°C", {U"degrés Celsius"}}, {U"M€", {U"millions d'euros"}}
};

using ::verbalis::text::after_any;
using ::verbalis::text::before_name;
using ::verbalis::text::before_number;

// Days of a month, the word before a month abbreviation in a date: 2 janv. 2003, 1er févr.
inline const std::unordered_set<std::u32string> kDays = [] {
  std::unordered_set<std::u32string> days = {U"1er"};
  for (char32_t day = 1; day <= 31; day++) {
    std::u32string tens(1, U'0' + day / 10), ones(1, U'0' + day % 10);
    days.insert(day < 10 ? ones : tens + ones);
    days.insert(tens + ones); // 02 janv.
  }
  return days;
}();
using ::verbalis::steps::Terms;

// XXe s., Ve s.: the century after a Roman ordinal (not after Le, De, Ce, Me)
inline bool after_roman_ordinal(const ::verbalis::text::Context& context) {
  auto words = ::verbalis::text::words_of(context.head());
  std::u32string_view word = words.empty() ? U"" : ::verbalis::text::stripped(words.back());
  bool numeral = word.size() > 2 || word == U"Ie" || word == U"Ve" || word == U"Xe";
  return numeral && word.back() == U'e' && word.find_first_not_of(U"IVXLCDM") == word.size() - 1;
}

// St-Étienne: a name after a hyphen
inline bool before_hyphenated_name(const ::verbalis::text::Context& context) {
  std::u32string_view tail = context.tail();
  return tail.size() > 1 && tail[0] == U'-' && ::verbalis::text::unicode::isupper(tail[1]);
}

// M. et Mme Durand
inline bool before_and_madame(const ::verbalis::text::Context& context) {
  return context.tail().starts_with(U" et Mme");
}

// M. Dupont, but not the M. of J. M. Ayuso
inline bool before_name_not_initial(const ::verbalis::text::Context& context) {
  return before_name(context) && context.before().size() != 1;
}

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: n° 5
    {U"n°", {{U"numéro", before_number}}},
    {U"N°", {{U"numéro", before_number}}},
    {U"nº", {{U"numéro", before_number}}},
    {U"n.°", {{U"numéro", before_number}}},
    {U"N.°", {{U"numéro", before_number}}},
    {U"Nº", {{U"numéro", before_number}}},
    {U"n.º", {{U"numéro", before_number}}},
    {U"N.º", {{U"numéro", before_number}}},
    {U"p.", {{U"page", before_number}}},
    {U"art.", {{U"article", before_number}}},
    {U"Art.", {{U"article", before_number}}},
    {U"pp.", {{U"pages", before_number}}},
    {U"chap.", {{U"chapitre", before_number}}},
    {U"vol.", {{U"volume", before_number}}},
    {U"fig.", {{U"figure", before_number}}},
    {U"s.", {{U"siècle", after_roman_ordinal}}},
    // Before a name: M. Dupont, Dr Martin, St-Étienne
    {U"M.", {{U"monsieur", before_name_not_initial}, {U"monsieur", before_and_madame}}},
    {U"Mr", {{U"monsieur", before_name}}},
    {U"Dr", {{U"docteur", before_name}}},
    {U"Dr.", {{U"docteur", before_name}}},
    {U"Pr", {{U"professeur", before_name}}},
    {U"St", {{U"saint", before_name}, {U"saint", before_hyphenated_name}}},
    {U"Ste", {{U"sainte", before_name}, {U"sainte", before_hyphenated_name}}},
    {U"Mgr", {{U"monseigneur", before_name}}},
    // After a day: 2 janv. 2003
    {U"janv.", {{U"janvier", after_any(kDays)}}},
    {U"févr.", {{U"février", after_any(kDays)}}},
    {U"fév.", {{U"février", after_any(kDays)}}},
    {U"avr.", {{U"avril", after_any(kDays)}}},
    {U"juil.", {{U"juillet", after_any(kDays)}}},
    {U"sept.", {{U"septembre", after_any(kDays)}}},
    {U"oct.", {{U"octobre", after_any(kDays)}}},
    {U"nov.", {{U"novembre", after_any(kDays)}}},
    {U"déc.", {{U"décembre", after_any(kDays)}}},
    // Anywhere
    {U"MM.", {U"messieurs"}},
    {U"Mme", {U"madame"}},
    {U"Mmes", {U"mesdames"}},
    {U"Mlle", {U"mademoiselle"}},
    {U"Mlles", {U"mesdemoiselles"}},
    {U"Mᵐᵉ", {U"madame"}},
    {U"Mᵐᵉˢ", {U"mesdames"}},
    {U"Mˡˡᵉ", {U"mademoiselle"}},
    {U"Mˡˡᵉˢ", {U"mesdemoiselles"}},
    {U"Dʳ", {U"docteur"}},
    {U"Dʳˢ", {U"docteurs"}},
    {U"Dʳᵉ", {U"docteure"}},
    {U"Dʳᵉˢ", {U"docteures"}},
    {U"Pʳ", {U"professeur"}},
    {U"etc.", {U"et cetera"}},
    {U"Tél.", {U"téléphone"}},
    {U"tél.", {U"téléphone"}},
    {U"cf.", {U"confer"}},
    {U"env.", {U"environ"}},
    {U"c.-à-d.", {U"c'est-à-dire"}},
    {U"p. ex.", {U"par exemple"}},
    {U"J.-C.", {U"Jésus-Christ"}},
    {U"av. J.-C.", {U"avant Jésus-Christ"}},
    {U"apr. J.-C.", {U"après Jésus-Christ"}},
  },
  .web = {.dot = U"point", .at = U"arobase", .slash = U"slash", .hyphen = U"tiret", .underscore = U"tiret bas",
          .digits = {U"zéro", U"un", U"deux", U"trois", U"quatre", U"cinq", U"six", U"sept", U"huit", U"neuf"}},
  .closing = {U"etc."}
};

} // namespace verbalis::languages::fr
