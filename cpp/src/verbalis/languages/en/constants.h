#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/context.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

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
using ::verbalis::numbers::RomanCues;
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
  {1000000000000000000ULL, U"quintillion"}, {1000000000000000ULL, U"quadrillion"}, {1000000000000ULL, U"trillion"},
  {1000000000ULL, U"billion"}, {1000000ULL, U"million"}, {1000ULL, U"thousand"}
};

// The scales of groups of three digits, from the thousands: 124,444,234,854,823,834,553
inline const std::vector<std::u32string> kGroupScales = {
  U"thousand", U"million", U"billion", U"trillion", U"quadrillion", U"quintillion", U"sextillion", U"septillion"
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
  {U"$", {U"dollar", U"dollars"}}, {U"€", {U"euro", U"euros"}}, {U"£", {U"pound", U"pounds"}}, {U"¥", {U"yen", U"yen"}},
  {U"₩", {U"won", U"won"}},        {U"US$", {U"US dollar", U"US dollars"}}, {U"$US", {U"US dollar", U"US dollars"}},
  {U"C$", {U"Canadian dollar", U"Canadian dollars"}}, {U"A$", {U"Australian dollar", U"Australian dollars"}},
  {U"AU$", {U"Australian dollar", U"Australian dollars"}}, {U"NZ$", {U"New Zealand dollar", U"New Zealand dollars"}},
  {U"S$", {U"Singapore dollar", U"Singapore dollars"}}, {U"HK$", {U"Hong Kong dollar", U"Hong Kong dollars"}}
};
inline const Keys kCurrencyKeys = keys_of(kCurrencies);

// What an amount is counted by after a slash: $20/mo, twenty dollars per month; $12/kg
inline const std::unordered_map<std::u32string, std::u32string> kPer = {
  {U"mo", U"month"}, {U"month", U"month"}, {U"yr", U"year"}, {U"year", U"year"}, {U"wk", U"week"},
  {U"week", U"week"}, {U"d", U"day"},     {U"day", U"day"},   {U"hr", U"hour"},  {U"h", U"hour"},
  {U"hour", U"hour"}, {U"min", U"minute"}, {U"kg", U"kilogram"}, {U"lb", U"pound"}, {U"g", U"gram"},
  {U"person", U"person"}, {U"night", U"night"}, {U"share", U"share"}, {U"barrel", U"barrel"}, {U"bbl", U"barrel"},
  {U"oz", U"ounce"}, {U"ton", U"ton"}, {U"tonne", U"tonne"}, {U"gallon", U"gallon"}, {U"kWh", U"kilowatt-hour"},
  {U"MWh", U"megawatt-hour"}, {U"s", U"second"}, {U"sec", U"second"}
};
inline const Keys kPerKeys = keys_of(kPer);

// The hundredth of a currency: $4.99, four dollars and ninety-nine cents
inline const std::unordered_map<std::u32string, Forms> kCents = {
  {U"$", {U"cent", U"cents"}}, {U"€", {U"cent", U"cents"}}, {U"£", {U"penny", U"pence"}}, {U"US$", {U"cent", U"cents"}},
  {U"$US", {U"cent", U"cents"}}, {U"C$", {U"cent", U"cents"}},  {U"A$", {U"cent", U"cents"}},
  {U"AU$", {U"cent", U"cents"}}, {U"NZ$", {U"cent", U"cents"}}, {U"S$", {U"cent", U"cents"}}, {U"HK$", {U"cent", U"cents"}}
};

inline const std::unordered_map<std::u32string, std::u32string> kScales = {
  {U"thousand", U"thousand"}, {U"million", U"million"}, {U"billion", U"billion"}, {U"trillion", U"trillion"},
  {U"k", U"thousand"},        {U"K", U"thousand"},      {U"M", U"million"},       {U"bn", U"billion"},
  {U"Million", U"million"},   {U"Billion", U"billion"}, {U"Trillion", U"trillion"},
  // Only after a currency: $30b, £7.45m
  {U"b", U"billion"},         {U"B", U"billion"},       {U"m", U"million"}
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
  {U"°",    {U"degree", U"degrees"}}, {U"″", {U"inch", U"inches"}},
  {U"m2",   {U"square meter", U"square meters"}},     {U"km2",  {U"square kilometer", U"square kilometers"}},
  {U"cm2",  {U"square centimeter", U"square centimeters"}}, {U"cm²", {U"square centimeter", U"square centimeters"}},
  {U"mm²",  {U"square millimeter", U"square millimeters"}}, {U"m3", {U"cubic meter", U"cubic meters"}},
  {U"cm3",  {U"cubic centimeter", U"cubic centimeters"}}, {U"cm³", {U"cubic centimeter", U"cubic centimeters"}},
  {U"km³",  {U"cubic kilometer", U"cubic kilometers"}}, {U"cc",   {U"cubic centimeter", U"cubic centimeters"}},
  {U"μm",   {U"micrometer", U"micrometers"}},         {U"µm",   {U"micrometer", U"micrometers"}},
  {U"Mbps", {U"megabit per second", U"megabits per second"}}, {U"mbps", {U"megabit per second", U"megabits per second"}},
  {U"Gbps", {U"gigabit per second", U"gigabits per second"}}, {U"kbps", {U"kilobit per second", U"kilobits per second"}},
  {U"KG",   {U"kilogram", U"kilograms"}},             {U"Kg",   {U"kilogram", U"kilograms"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"one half"}, {{1, 3}, U"one third"}, {{2, 3}, U"two thirds"}, {{1, 4}, U"one quarter"},
  {{3, 4}, U"three quarters"}
};

// Words after a fraction which mark it as one: 1/3 of
inline const std::unordered_set<std::u32string> kFractionWords = {U"of"};

inline const Keys kTimeSuffixes = {U"a.m.", U"p.m.", U"a.m", U"p.m", U"am.", U"pm.", U"am", U"pm", U"AM", U"PM",
                                   U"EST", U"EDT", U"CST", U"CDT", U"MST", U"MDT", U"PST", U"PDT", U"GMT", U"UTC"};

// Words before a number read digit by digit: PIN 1234, card ending in 8876
inline const std::unordered_set<std::u32string> kSpellingWords = {
  U"pin", U"otp", U"code", U"passcode", U"password", U"cvv", U"cvc", U"zip"
};

// Months, also abbreviated (lowercase)
inline const std::unordered_set<std::u32string> kMonthWords = {
  U"january", U"february", U"march", U"april", U"may", U"june", U"july", U"august", U"september", U"october",
  U"november", U"december", U"jan", U"feb", U"mar", U"apr", U"jun", U"jul", U"aug", U"sep", U"sept", U"oct",
  U"nov", U"dec"
};

// Words after a number which make it a year: 340 AD, 44 BC
inline const std::unordered_set<std::u32string> kEras = {
  U"ad", U"a.d", U"a.d.", U"bc", U"b.c", U"b.c.", U"bce", U"b.c.e", U"b.c.e.", U"ce", U"c.e", U"c.e."
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
  U"deep", U"old", U"later", U"earlier", U"overall", U"last", U"next", U"this"
};

// Henry VIII (the Eighth), Chapter IV (four), the XXI century (twenty-first)
inline const RomanCues kRomanCues = {
  .names = {
    U"henry", U"francis", U"edward", U"george", U"william", U"richard", U"charles", U"james", U"elizabeth", U"mary", U"victoria",
    U"anne", U"john", U"paul", U"benedict", U"leo", U"pius", U"gregory", U"clement", U"innocent", U"louis", U"philip",
    U"frederick", U"alexander", U"nicholas", U"peter", U"ivan", U"catherine", U"napoleon", U"ferdinand", U"alfonso",
    U"carlos", U"felipe", U"wilhelm", U"ludwig", U"constantine", U"ramesses"
  },
  .before = {
    U"chapter", U"part", U"volume", U"book", U"act", U"scene", U"section", U"article", U"appendix", U"phase",
    U"stage", U"level", U"war", U"season", U"episode", U"bowl", U"vatican", U"type", U"class", U"grade",
    U"category", U"tier", U"mark"
  },
  .after = {U"century", U"centuries", U"millennium"}
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U",", .decimal_separators = U".", .time_separators = U":.", .time_suffixes = kTimeSuffixes,
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"point", .dot = U"dot", .range = U" to ", .by = U"by", .score = U" to ", .minus = U"minus", .plus = U"plus",
  .fraction = U" and ", .half = U" and a half"
};

// 27.03.2026, 27-03-2026: the year has four digits (3.8.10 is a version)
inline const DateFormat kDateFormat = {.separators = U".-", .boundaries = U".,/-", .short_year = false};

// 1-555-123-4567, (555) 123-4567, 555.123.4567, 555 123 4567, 555-1234: the longest first, each its own pass
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 3}, {3, 3}, {4, 4}, {1, 4}}}, // With an extension: 123-123-5678-1111
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .prefix = U"1-"},
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .parenthesized = true},
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .separators = U"."},
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .separators = U" "},
  {.groups = {{3, 3}, {4, 4}}},
  {.groups = {{3, 3}, {2, 2}, {4, 4}}} // A Social Security number: 123-45-6789
};

// 21st, 2nd, 3rd, 4th
inline const Keys kOrdinalSuffixes = {U"st", U"nd", U"rd", U"th"};

// 1990s, '90s, 90's
inline const Keys kDecadeSuffixes = {U"s"};
inline const SuffixFormat kDecadeFormat = {.apostrophes = true, .boundaries = U".,-"};

using ::verbalis::text::Readings;

inline const Readings kSymbols = {
  {U"&", {U"and"}},      {U"+", {U"plus"}},    {U"=", {U"equals"}},  {U"@", {U"at"}},      {U"%", {U"percent"}},
  {U"°", {U"degrees"}},  {U"#", {U"hash"}},    {U"²", {U"squared"}}, {U"³", {U"cubed"}},   {U"π", {U"pi"}},
  {U"×", {U"times"}},    {U"±", {U"plus or minus"}}, {U"½", {U"one half"}},
  {U"§", {U"section"}},    {U"$", {U"dollar"}}, {U"€", {U"euro"}},
  {U"£", {U"pound"}},      {U"¥", {U"yen"}},      {U"₩", {U"won"}}
};

using ::verbalis::text::before_name;
using ::verbalis::text::before_number;
using ::verbalis::steps::Terms;

// After the name of a street: 1428 Elm St., on 5th St.; on Main St. unless a name follows (Ascension St. Joseph),
// and not at the start of a sentence (Visit St. Louis)
inline bool after_street(const ::verbalis::text::Context& context) {
  namespace unicode = ::verbalis::text::unicode;
  auto words = ::verbalis::text::words_of(context.head());
  if (words.size() < 2) {
    return false;
  }
  std::u32string_view name = words.back(), before = words[words.size() - 2];
  bool ordinal = name.size() > 2 && unicode::isdigit(name[0]) &&
                 (name.ends_with(U"st") || name.ends_with(U"nd") || name.ends_with(U"rd") || name.ends_with(U"th"));
  bool numbered = unicode::isdigit(before.back()) && unicode::isupper(name[0]);
  bool named = unicode::isupper(name[0]) && !::verbalis::text::strings::contains(std::u32string_view(U".!?"), before.back()) &&
               !::verbalis::text::before_name(context);
  return ordinal || numbered || named;
}

// A digit right before, possibly before a space: 25 Sept., 25 jul 2012
inline bool after_number(const ::verbalis::text::Context& context) {
  std::u32string_view head = context.head();
  if (!head.empty() && head.back() == U' ') {
    head.remove_suffix(1);
  }
  return !head.empty() && ::verbalis::text::unicode::isdigit(head.back());
}

// Between a house number and a capitalized name: 708 N 1st St, 1211 E Arques Ave
inline bool between_number_and_street(const ::verbalis::text::Context& context) {
  std::u32string_view tail = context.tail();
  size_t next = tail.find_first_not_of(U' ');
  bool street = next != std::u32string_view::npos && next > 0 &&
                (::verbalis::text::unicode::isupper(tail[next]) || ::verbalis::text::unicode::isdigit(tail[next]));
  return after_number(context) && street;
}

// Before a ZIP code, possibly after a comma: Santa Clara, CA 95051
inline bool before_zip(const ::verbalis::text::Context& context) {
  std::u32string_view tail = context.tail();
  size_t start = tail.starts_with(U",") ? 1 : 0;
  if (start >= tail.size() || tail[start] != U' ') {
    return false;
  }
  size_t digits = start + 1, end = digits;
  while (end < tail.size() && ::verbalis::text::unicode::isdigit(tail[end])) {
    end++;
  }
  return end - digits == 5 && (end == tail.size() || !::verbalis::text::unicode::isword(tail[end]));
}

// At the end of an address, after its town: 123 Main St, Springfield, IL.
inline bool after_town(const ::verbalis::text::Context& context) {
  std::u32string_view head = context.head(), tail = context.tail();
  if (!head.ends_with(U", ") || !(tail.empty() || ::verbalis::text::strings::contains(std::u32string_view(U".,;)!?"), tail[0]))) {
    return false;
  }
  head.remove_suffix(2);
  size_t comma = head.rfind(U", ");
  if (comma == std::u32string_view::npos || !::verbalis::text::unicode::isupper(head[comma + 2])) {
    return false;
  }
  return std::ranges::any_of(head.substr(0, comma), ::verbalis::text::unicode::isdigit);
}

// Two-letter codes of the US states, read before a ZIP code
inline const std::vector<std::pair<std::u32string, std::u32string>> kStates = {
  {U"AL", U"Alabama"}, {U"AK", U"Alaska"}, {U"AZ", U"Arizona"}, {U"AR", U"Arkansas"}, {U"CA", U"California"},
  {U"CO", U"Colorado"}, {U"CT", U"Connecticut"}, {U"DE", U"Delaware"}, {U"FL", U"Florida"}, {U"GA", U"Georgia"},
  {U"HI", U"Hawaii"}, {U"ID", U"Idaho"}, {U"IL", U"Illinois"}, {U"IN", U"Indiana"}, {U"IA", U"Iowa"},
  {U"KS", U"Kansas"}, {U"KY", U"Kentucky"}, {U"LA", U"Louisiana"}, {U"ME", U"Maine"}, {U"MD", U"Maryland"},
  {U"MA", U"Massachusetts"}, {U"MI", U"Michigan"}, {U"MN", U"Minnesota"}, {U"MS", U"Mississippi"},
  {U"MO", U"Missouri"}, {U"MT", U"Montana"}, {U"NE", U"Nebraska"}, {U"NV", U"Nevada"}, {U"NH", U"New Hampshire"},
  {U"NJ", U"New Jersey"}, {U"NM", U"New Mexico"}, {U"NY", U"New York"}, {U"NC", U"North Carolina"},
  {U"ND", U"North Dakota"}, {U"OH", U"Ohio"}, {U"OK", U"Oklahoma"}, {U"OR", U"Oregon"}, {U"PA", U"Pennsylvania"},
  {U"RI", U"Rhode Island"}, {U"SC", U"South Carolina"}, {U"SD", U"South Dakota"}, {U"TN", U"Tennessee"},
  {U"TX", U"Texas"}, {U"UT", U"Utah"}, {U"VT", U"Vermont"}, {U"VA", U"Virginia"}, {U"WA", U"Washington"},
  {U"WV", U"West Virginia"}, {U"WI", U"Wisconsin"}, {U"WY", U"Wyoming"}, {U"DC", U"D C"}
};

// Street types after the name of a street: Laguna Ct, San Tomas Expy
inline const std::vector<std::pair<std::u32string, std::u32string>> kStreetTypes = {
  {U"Ave", U"Avenue"}, {U"Blvd", U"Boulevard"}, {U"Rd", U"Road"}, {U"Ct", U"Court"}, {U"Ln", U"Lane"},
  {U"Pkwy", U"Parkway"}, {U"Hwy", U"Highway"}, {U"hwy", U"Highway"}, {U"Expy", U"Expressway"}, {U"Dr", U"Drive"},
  {U"St", U"Street"}, {U"st", U"Street"}, {U"Pl", U"Place"}, {U"Sq", U"Square"}, {U"Ter", U"Terrace"}
};

// Directions between a house number and its street
inline const std::vector<std::pair<std::u32string, std::u32string>> kDirections = {
  {U"N", U"North"}, {U"S", U"South"}, {U"E", U"East"}, {U"W", U"West"}, {U"NE", U"Northeast"},
  {U"NW", U"Northwest"}, {U"SE", U"Southeast"}, {U"SW", U"Southwest"}
};

// Month abbreviations and their names
inline const std::vector<std::pair<std::u32string, std::u32string>> kMonthAbbreviations = {
  {U"Jan", U"January"}, {U"Feb", U"February"}, {U"Mar", U"March"},    {U"Apr", U"April"},
  {U"Jun", U"June"},    {U"Jul", U"July"},     {U"Aug", U"August"},   {U"Sep", U"September"},
  {U"Sept", U"September"}, {U"Oct", U"October"}, {U"Nov", U"November"}, {U"Dec", U"December"}
};

inline const Terms kTermsBase = {
  .abbreviations = {
    // Before a number only: No. 5, #5
    {U"#", {{U"number", before_number}}},
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
    {U"401(k)", {U"four oh one k"}},
    {U"e.g.", {U"for example"}},
    {U"i.e.", {U"that is"}},
    {U"etc.", {U"et cetera"}},
    {U"vs.", {U"versus"}},
    {U"Mr.", {U"Mister"}},
    {U"Mrs.", {U"Missus"}},
    {U"Dr.", {{U"Doctor", before_name}}},
    {U"St.", {{U"Street", after_street}, {U"Saint", before_name}, U"Street"}},
  },
  .web = {.dot = U"dot", .at = U"at", .slash = U"slash", .hyphen = U"dash", .underscore = U"underscore",
          .backslash = U"backslash", .colon = U"colon",
          .digits = {U"zero", U"one", U"two", U"three", U"four", U"five", U"six", U"seven", U"eight", U"nine"}}
};

// With the month abbreviations around a number: Jan 15, jan. 15, JAN 15 2020, Jan-15; 25 Sept., 25 jul 2012 (but not
// "mar", a verb, in lowercase)
inline const Terms kTerms = [] {
  Terms terms = kTermsBase;
  for (const auto& [abbreviation, name] : kMonthAbbreviations) {
    std::u32string lower = ::verbalis::text::strings::to_lower(abbreviation);
    std::u32string upper;
    for (char32_t c : abbreviation) {
      upper += ::verbalis::text::unicode::toupper(c);
    }
    for (const auto& form : {abbreviation, lower, upper}) {
      if (form == U"mar") {
        continue;
      }
      for (const auto& key : {form, form + U"."}) {
        terms.abbreviations[key] = {{name, before_number}, {name, after_number}};
      }
      terms.abbreviations[form + U"-"] = {{name, before_number}};
    }
  }
  for (const auto& [code, state] : kStates) {
    terms.abbreviations[code] = {{state, before_zip}, {state, after_town}};
  }
  for (const auto& [abbreviation, name] : kStreetTypes) {
    for (const auto& key : {abbreviation, abbreviation + U"."}) {
      auto& readings = terms.abbreviations[key];
      readings.insert(readings.begin(), {name, after_street});
    }
  }
  terms.closing = {U"etc.", U"St."};
  for (const auto& [abbreviation, name] : kStreetTypes) {
    terms.closing.insert(abbreviation + U".");
  }
  for (const auto& [abbreviation, name] : kDirections) {
    for (const auto& key : {abbreviation, abbreviation + U"."}) {
      terms.abbreviations[key] = {{name, between_number_and_street}};
    }
  }
  return terms;
}();

} // namespace verbalis::languages::en
