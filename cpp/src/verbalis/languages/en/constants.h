#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/readings.h>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::en {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::SuffixFormat;

inline constexpr std::array<std::u32string_view, 20> kOnes = {
  U"zero",    U"one",     U"two",       U"three",    U"four",     U"five",    U"six",       U"seven",    U"eight",    U"nine",
  U"ten",     U"eleven",  U"twelve",    U"thirteen", U"fourteen", U"fifteen", U"sixteen",   U"seventeen",U"eighteen", U"nineteen"
};

inline constexpr std::array<std::u32string_view, 10> kTens = {
  U"", U"", U"twenty", U"thirty", U"forty", U"fifty", U"sixty", U"seventy", U"eighty", U"ninety"
};

// Scale words, largest first
inline const std::vector<std::pair<uint64_t, std::u32string>> kScaleWords = {
  {1000000000000ULL, U"trillion"}, {1000000000ULL, U"billion"}, {1000000ULL, U"million"}, {1000ULL, U"thousand"}
};

// Ordinals which are not formed with "th": cardinal word -> ordinal word
inline const std::unordered_map<std::u32string, std::u32string> kIrregularOrdinals = {
  {U"one",   U"first"},  {U"two",   U"second"}, {U"three", U"third"},  {U"five",  U"fifth"},
  {U"eight", U"eighth"}, {U"nine",  U"ninth"},  {U"twelve", U"twelfth"}
};

inline const std::vector<std::u32string> kMonths = {
  U"January", U"February", U"March",     U"April",   U"May",      U"June",
  U"July",    U"August",   U"September", U"October", U"November", U"December"
};

inline const std::unordered_map<std::u32string, Forms> kCurrencies = {
  {U"$", {U"dollar", U"dollars"}}, {U"€", {U"euro", U"euros"}}, {U"£", {U"pound", U"pounds"}}, {U"¥", {U"yen", U"yen"}}
};

inline const std::unordered_map<std::u32string, std::u32string> kScales = {
  {U"thousand", U"thousand"}, {U"million", U"million"}, {U"billion", U"billion"}, {U"trillion", U"trillion"},
  {U"k", U"thousand"},        {U"K", U"thousand"},      {U"M", U"million"},       {U"bn", U"billion"}
};

inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"km",   {U"kilometer", U"kilometers"}},           {U"m",    {U"meter", U"meters"}},
  {U"cm",   {U"centimeter", U"centimeters"}},         {U"mm",   {U"millimeter", U"millimeters"}},
  {U"nm",   {U"nanometer", U"nanometers"}},           {U"m²",   {U"square meter", U"square meters"}},
  {U"km²",  {U"square kilometer", U"square kilometers"}}, {U"m³", {U"cubic meter", U"cubic meters"}},
  {U"kg",   {U"kilogram", U"kilograms"}},             {U"g",    {U"gram", U"grams"}},
  {U"mg",   {U"milligram", U"milligrams"}},           {U"lb",   {U"pound", U"pounds"}},
  {U"lbs",  {U"pound", U"pounds"}},                   {U"oz",   {U"ounce", U"ounces"}},
  {U"ft",   {U"foot", U"feet"}},                      {U"l",    {U"liter", U"liters"}},
  {U"L",    {U"liter", U"liters"}},                   {U"ml",   {U"milliliter", U"milliliters"}},
  {U"mL",   {U"milliliter", U"milliliters"}},         {U"TB",   {U"terabyte", U"terabytes"}},
  {U"GB",   {U"gigabyte", U"gigabytes"}},             {U"MB",   {U"megabyte", U"megabytes"}},
  {U"KB",   {U"kilobyte", U"kilobytes"}},             {U"kB",   {U"kilobyte", U"kilobytes"}},
  {U"Hz",   {U"hertz", U"hertz"}},                    {U"kHz",  {U"kilohertz", U"kilohertz"}},
  {U"MHz",  {U"megahertz", U"megahertz"}},            {U"GHz",  {U"gigahertz", U"gigahertz"}},
  {U"W",    {U"watt", U"watts"}},                     {U"kW",   {U"kilowatt", U"kilowatts"}},
  {U"kWh",  {U"kilowatt-hour", U"kilowatt-hours"}},   {U"V",    {U"volt", U"volts"}},
  {U"mph",  {U"mile per hour", U"miles per hour"}},   {U"km/h", {U"kilometer per hour", U"kilometers per hour"}},
  {U"°C",   {U"degree Celsius", U"degrees Celsius"}}, {U"°F",   {U"degree Fahrenheit", U"degrees Fahrenheit"}},
  {U"°",    {U"degree", U"degrees"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"one half"}, {{1, 3}, U"one third"}, {{2, 3}, U"two thirds"}, {{1, 4}, U"one quarter"},
  {{3, 4}, U"three quarters"}
};

inline const Keys kTimeSuffixes = {U"a.m.", U"p.m.", U"am.", U"pm.", U"am", U"pm", U"AM", U"PM"};

// Months, also abbreviated (lowercase)
inline const std::unordered_set<std::u32string> kMonthWords = {
  U"january", U"february", U"march", U"april", U"may", U"june", U"july", U"august", U"september", U"october",
  U"november", U"december", U"jan", U"feb", U"mar", U"apr", U"jun", U"jul", U"aug", U"sep", U"sept", U"oct",
  U"nov", U"dec"
};

// Words before a 4-digit number which make it a year: in 1999, since 1999 (and months)
inline const std::unordered_set<std::u32string> kYearCues = {
  U"in", U"of", U"since", U"from", U"by", U"until", U"till", U"circa", U"year", U"the", U"during", U"before",
  U"after", U"early", U"late", U"mid"
};

inline const std::unordered_set<std::u32string> kIrregularPlurals = {
  U"people", U"children", U"men", U"women", U"feet", U"persons"
};

// Words before a time with a dot, which make it a time: at 10.30
inline const std::unordered_set<std::u32string> kTimeWords = {
  U"at", U"from", U"until", U"till", U"to", U"by", U"around", U"about", U"before", U"after", U"since", U"between",
  U"and", U"is", U"it's", U"or"
};

// Words after an amount that do not make it an adjective ("$5 a month" vs "a $5 bill")
inline const std::unordered_set<std::u32string> kStopWords = {
  U"a", U"an", U"the", U"and", U"or", U"but", U"nor", U"in", U"on", U"at", U"to", U"for", U"of", U"per", U"with",
  U"from", U"by", U"into", U"than", U"more", U"less", U"each", U"every", U"is", U"are", U"was", U"were", U"be",
  U"been", U"will", U"would", U"can", U"could", U"should", U"may", U"might", U"must", U"plus", U"minus", U"times",
  U"anyway", U"now", U"then", U"so", U"that", U"this", U"these", U"those", U"it", U"if", U"when", U"which", U"who",
  U"what", U"where", U"why", U"how", U"you", U"i", U"we", U"they", U"he", U"she", U"as", U"up", U"down", U"off",
  U"out", U"over", U"under", U"about", U"around", U"after", U"before", U"back", U"away", U"ago", U"versus", U"vs",
  U"too", U"also", U"only", U"just", U"even", U"still", U"yet", U"already", U"not", U"no", U"there", U"here",
  U"all", U"both", U"while", U"because", U"since", U"until", U"though", U"although", U"unless", U"has", U"have",
  U"had", U"do", U"does", U"did", U"get", U"got", U"instead", U"worth", U"today", U"tomorrow", U"tonight",
  U"outside", U"inside", U"apart", U"ahead", U"behind", U"above", U"below", U"long", U"wide", U"high", U"tall",
  U"deep", U"old", U"later", U"earlier", U"overall"
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U",", .decimal_separators = U".", .time_separators = U":.", .time_suffixes = kTimeSuffixes,
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"point", .dot = U"dot", .range = U" to ", .by = U"by", .score = U" to ", .minus = U"minus", .plus = U"plus"
};

// 27.03.2026, 27-03-2026: the year has four digits (3.8.10 is a version)
inline const DateFormat kDateFormat = {.separators = U".-", .boundaries = U".,/-", .short_year = false};

// 1-555-123-4567, (555) 123-4567, 555.123.4567, 555 123 4567, 555-1234: the longest first, each its own pass
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .prefix = U"1-"},
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .parenthesized = true},
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .separators = U"."},
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .separators = U" "},
  {.groups = {{3, 3}, {4, 4}}}
};

// 21st, 2nd, 3rd, 4th
inline const Keys kOrdinalSuffixes = {U"st", U"nd", U"rd", U"th"};

// 1990s, '90s, 90's
inline const Keys kDecadeSuffixes = {U"s"};
inline const SuffixFormat kDecadeFormat = {.apostrophes = true, .boundaries = U".,-"};

using ::verbalis::text::Readings;

inline const Readings kSymbols = {
  {U"&", {U"and"}},      {U"+", {U"plus"}},    {U"=", {U"equals"}},  {U"@", {U"at"}},      {U"%", {U"percent"}},
  {U"°", {U"degrees"}},  {U"#", {U"number"}},  {U"²", {U"squared"}}, {U"³", {U"cubed"}},   {U"π", {U"pi"}},
  {U"×", {U"times"}},    {U"±", {U"plus or minus"}}, {U"½", {U"one half"}}
};

using ::verbalis::text::before_name;
using ::verbalis::text::before_number;
using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: No. 5
    {U"No.", {{U"number", before_number}}},
    {U"no.", {{U"number", before_number}}},
    {U"Nos.", {{U"numbers", before_number}}},
    {U"vol.", {{U"volume", before_number}}},
    {U"Vol.", {{U"volume", before_number}}},
    {U"p.", {{U"page", before_number}}},
    {U"pp.", {{U"pages", before_number}}},
    {U"ch.", {{U"chapter", before_number}}},
    {U"Ch.", {{U"chapter", before_number}}},
    {U"fig.", {{U"figure", before_number}}},
    {U"Fig.", {{U"figure", before_number}}},
    // Anywhere
    {U"e.g.", {U"for example"}},
    {U"i.e.", {U"that is"}},
    {U"etc.", {U"et cetera"}},
    {U"vs.", {U"versus"}},
    {U"Mr.", {U"Mister"}},
    {U"Mrs.", {U"Missus"}},
    {U"Dr.", {{U"Doctor", before_name}}},
    {U"St.", {{U"Saint", before_name}, U"Street"}},
  },
  .web = {.dot = U"dot", .at = U"at", .slash = U"slash"}
};

} // namespace verbalis::languages::en
