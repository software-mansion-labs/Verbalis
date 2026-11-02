#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/normalize.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/unicode.h>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace verbalis::languages::hi {

using ::verbalis::steps::Characters;

// Echo's classes, but the zero-width non-joiner and joiner stay: they decide how Devanagari letters join
inline const Characters kCharacters = {
  {U"…",                U"..."}, // Ellipsis
  {U"«»„“”‟˝",          U"\""},  // Quotes
  {U"‘’‚‛´`ʼ",          U"'"},   // Apostrophes
  {U"\u200b\ufeff",     U""},    // Invisible: zero-width space, byte order mark
  // Devanagari digits, read as numbers: ५४ -> 54
  {U"०", U"0"}, {U"१", U"1"}, {U"२", U"2"}, {U"३", U"3"}, {U"४", U"4"},
  {U"५", U"5"}, {U"६", U"6"}, {U"७", U"7"}, {U"८", U"8"}, {U"९", U"9"},
  // Subscript digits, as in formulas: H₂O -> H2O
  {U"₀", U"0"}, {U"₁", U"1"}, {U"₂", U"2"}, {U"₃", U"3"}, {U"₄", U"4"},
  {U"₅", U"5"}, {U"₆", U"6"}, {U"₇", U"7"}, {U"₈", U"8"}, {U"₉", U"9"}
};

using ::verbalis::numbers::DateFormat;
using ::verbalis::numbers::keys_of;
using ::verbalis::text::Keys;
using ::verbalis::numbers::Notation;
// Hindi has unique cardinal words for every number 0-99 — they are not
// compositionally derivable from tens + units like in most European
// languages. Above 99 the construction becomes regular: hundreds use
// "<digit> सौ", and Indian-numbering scales (हज़ार/लाख/करोड़) compose
// multiplicatively.
inline const std::unordered_map<int32_t, std::u32string> kCardinals = {
  {0,  U"शून्य"},      {1,  U"एक"},          {2,  U"दो"},          {3,  U"तीन"},         {4,  U"चार"},
  {5,  U"पाँच"},       {6,  U"छह"},          {7,  U"सात"},         {8,  U"आठ"},          {9,  U"नौ"},
  {10, U"दस"},         {11, U"ग्यारह"},       {12, U"बारह"},        {13, U"तेरह"},         {14, U"चौदह"},
  {15, U"पंद्रह"},      {16, U"सोलह"},        {17, U"सत्रह"},        {18, U"अठारह"},       {19, U"उन्नीस"},
  {20, U"बीस"},        {21, U"इक्कीस"},      {22, U"बाईस"},        {23, U"तेईस"},         {24, U"चौबीस"},
  {25, U"पच्चीस"},     {26, U"छब्बीस"},      {27, U"सत्ताईस"},      {28, U"अट्ठाईस"},     {29, U"उनतीस"},
  {30, U"तीस"},        {31, U"इकतीस"},       {32, U"बत्तीस"},       {33, U"तैंतीस"},        {34, U"चौंतीस"},
  {35, U"पैंतीस"},      {36, U"छत्तीस"},       {37, U"सैंतीस"},        {38, U"अड़तीस"},      {39, U"उनतालीस"},
  {40, U"चालीस"},      {41, U"इकतालीस"},    {42, U"बयालीस"},      {43, U"तैंतालीस"},      {44, U"चवालीस"},
  {45, U"पैंतालीस"},   {46, U"छियालीस"},     {47, U"सैंतालीस"},     {48, U"अड़तालीस"},     {49, U"उनचास"},
  {50, U"पचास"},       {51, U"इक्यावन"},     {52, U"बावन"},        {53, U"तिरेपन"},       {54, U"चौवन"},
  {55, U"पचपन"},       {56, U"छप्पन"},       {57, U"सत्तावन"},      {58, U"अट्ठावन"},      {59, U"उनसठ"},
  {60, U"साठ"},        {61, U"इकसठ"},       {62, U"बासठ"},        {63, U"तिरेसठ"},       {64, U"चौंसठ"},
  {65, U"पैंसठ"},       {66, U"छियासठ"},     {67, U"सड़सठ"},        {68, U"अड़सठ"},       {69, U"उनहत्तर"},
  {70, U"सत्तर"},       {71, U"इकहत्तर"},     {72, U"बहत्तर"},       {73, U"तिहत्तर"},       {74, U"चौहत्तर"},
  {75, U"पचहत्तर"},    {76, U"छिहत्तर"},      {77, U"सतहत्तर"},     {78, U"अठहत्तर"},      {79, U"उनासी"},
  {80, U"अस्सी"},      {81, U"इक्यासी"},     {82, U"बयासी"},       {83, U"तिरासी"},       {84, U"चौरासी"},
  {85, U"पचासी"},      {86, U"छियासी"},     {87, U"सत्तासी"},      {88, U"अट्ठासी"},      {89, U"नवासी"},
  {90, U"नब्बे"},       {91, U"इक्यानवे"},    {92, U"बानवे"},        {93, U"तिरानवे"},      {94, U"चौरानवे"},
  {95, U"पंचानवे"},    {96, U"छियानवे"},    {97, U"सत्तानवे"},     {98, U"अट्ठानवे"},     {99, U"निन्यानवे"}
};

// Indian-style large scales: thousand (हज़ार), lakh (लाख = 100k),
// crore (करोड़ = 10M). Hindi pluralization is invariant for these nouns
// when used as scale words after a numeral.
inline const std::map<std::int64_t, std::u32string> kLargeCardinals = {
  {1000LL,         U"हज़ार"},
  {100000LL,       U"लाख"},
  {10000000LL,     U"करोड़"},
  {1000000000LL,   U"अरब"},
  {100000000000LL, U"खरब"},
  {10000000000000LL,     U"नील"},
  {1000000000000000LL,   U"पद्म"},
  {100000000000000000LL, U"शंख"}
};

// Currencies. Hindi loanword currencies are invariable in form, so only the
// singular noun is stored.
inline const std::unordered_map<std::u32string, std::u32string> kCurrencies = {
  {U"$", U"डॉलर"}, {U"€", U"यूरो"}, {U"£", U"पाउंड"}, {U"₹", U"रुपये"}, {U"¥", U"येन"}, {U"₺", U"लीरा"},
  {U"₩", U"वॉन"},  {U"৳", U"टका"},  {U"₦", U"नाइरा"},
  {U"EUR", U"यूरो"}, {U"USD", U"डॉलर"}, {U"GBP", U"पाउंड"}, {U"PLN", U"ज़्लॉटी"}, {U"CHF", U"स्विस फ़्रैंक"},
  {U"CZK", U"चेक कोरुना"}
};

// The smaller unit of a currency, read after the whole amount: ₹9.50 -> नौ रुपये पचास पैसे
inline const std::unordered_map<std::u32string, std::u32string> kCents = {
  {U"$", U"सेंट"}, {U"€", U"सेंट"}, {U"£", U"पेंस"}, {U"₹", U"पैसे"}, {U"¥", U"सेन"}, {U"₺", U"कुरस"},
  {U"₩", U"जिओन"}, {U"৳", U"पैसे"}, {U"₦", U"कोबो"}
};

// Currencies written as words after an amount: 888.60 रुपये
inline const std::unordered_set<std::u32string> kCurrencyWords = {
  U"रुपये", U"रुपए", U"रुपया", U"रूपये", U"रूपए", U"डॉलर", U"यूरो", U"पाउंड"
};

// Scales written as words after an amount: ₹ 5 लाख
inline const Keys kScales = {U"हज़ार", U"लाख", U"करोड़", U"अरब", U"खरब", U"मिलियन", U"बिलियन", U"ट्रिलियन"};

// Units, invariable like the currencies: 5 km, 5 किमी -> पाँच किलोमीटर
inline const std::unordered_map<std::u32string, std::u32string> kUnits = {
  {U"km", U"किलोमीटर"},       {U"किमी", U"किलोमीटर"},     {U"m", U"मीटर"},             {U"cm", U"सेंटीमीटर"},
  {U"सेमी", U"सेंटीमीटर"},     {U"mm", U"मिलीमीटर"},       {U"मिमी", U"मिलीमीटर"},       {U"km²", U"वर्ग किलोमीटर"},
  {U"m²", U"वर्ग मीटर"},       {U"kg", U"किलोग्राम"},       {U"किग्रा", U"किलोग्राम"},     {U"g", U"ग्राम"},
  {U"mg", U"मिलीग्राम"},       {U"l", U"लीटर"},            {U"ml", U"मिलीलीटर"},        {U"km/h", U"किलोमीटर प्रति घंटा"},
  {U"km/hr", U"किलोमीटर प्रति घंटा"}, {U"°C", U"डिग्री सेल्सियस"}, {U"°F", U"डिग्री फारेनहाइट"}, {U"ft", U"फीट"},
  {U"MW", U"मेगावाट"},         {U"kW", U"किलोवाट"},        {U"GB", U"जीबी"},            {U"MB", U"एमबी"},
  {U"ha", U"हेक्टेयर"},         {U"ac", U"एकड़"},           {U"mi", U"मील"},             {U"in", U"इंच"},
  {U"yd", U"यार्ड"},            {U"µm", U"माइक्रोमीटर"},      {U"mi/hr", U"मील प्रति घंटा"}, {U"mph", U"मील प्रति घंटा"},
  {U"qt", U"क्वार्ट"},          {U"gal", U"गैलन"},          {U"pt", U"पिंट"},            {U"cg", U"सेंटीग्राम"},
  {U"dg", U"डेसीग्राम"},        {U"t", U"टन"},             {U"st", U"स्टोन"},           {U"lb", U"पाउंड"},
  {U"oz", U"आउन्स"},           {U"q", U"क्विंटल"},          {U"yr", U"साल"},             {U"″", U"इंच"},
  {U"W", U"वाट"},              {U"Km/h", U"किलोमीटर प्रति घंटा"}, {U"x", U"गुना"}, {U"Hz", U"हर्ट्ज़"},
  // Abbreviations with ० (read as 0) or ॰ as their dot: 34 किमी०, 20 कि॰मी॰, 6 मी॰, 3,000 कि॰
  {U"किमी0", U"किलोमीटर"},     {U"कि॰मी॰", U"किलोमीटर"},     {U"मी॰", U"मीटर"},           {U"कि॰", U"किलोग्राम"},
  {U"रु.", U"रुपये"}
};

// Words of a text about time, which make its 3'16" minutes and seconds, and a lone 25″ seconds
inline const std::unordered_set<std::u32string> kPrimeCues = {
  U"समय", U"दौड़", U"रेस", U"रिकॉर्ड", U"मैराथन", U"मिनट", U"सेकंड"
};

// Words around a 5'9" which make it feet and inches: उसकी लंबाई 5'9" है
inline const std::unordered_set<std::u32string> kHeightCues = {U"लंबाई", U"कद", U"ऊंचाई", U"ऊँचाई", U"लंबा", U"लंबी"};

// A time or a height written with prime marks: 1'23"45 -> एक मिनट तेईस दशमलव चार पाँच सेकंड
inline const std::unordered_map<Prime, Forms> kPrimeUnits = {
  {Prime::HOUR, {U"घंटा", U"घंटे"}}, {Prime::MINUTE, {U"मिनट", U"मिनट"}}, {Prime::SECOND, {U"सेकंड", U"सेकंड"}},
  {Prime::FOOT, {kUnits.at(U"ft"), kUnits.at(U"ft")}}, {Prime::INCH, {kUnits.at(U"″"), kUnits.at(U"″")}},
};

// Months: standard Gregorian month names in Hindi (used as-is in dates).
inline const std::vector<std::u32string> kMonths = {
  U"",          U"जनवरी",     U"फ़रवरी",    U"मार्च",      U"अप्रैल",     U"मई",         U"जून",
  U"जुलाई",      U"अगस्त",      U"सितंबर",    U"अक्टूबर",    U"नवंबर",      U"दिसंबर"
};

// Hindi ordinals have three gender/number forms. Values 1-4 and 6 take
// fully irregular forms; the rest of the ordinals are derived by appending
// a regular ending to the cardinal word at verbalization time.
// Index order matches OrdinalForm: { masculine, feminine, plural }.
using OrdinalForms = std::array<std::u32string, 3>;
inline const std::unordered_map<int32_t, OrdinalForms> kOrdinalsSpecial = {
  {1, {U"पहला",   U"पहली",   U"पहले"}},
  {2, {U"दूसरा",  U"दूसरी",  U"दूसरे"}},
  {3, {U"तीसरा",  U"तीसरी",  U"तीसरे"}},
  {4, {U"चौथा",   U"चौथी",   U"चौथे"}},
  {6, {U"छठा",    U"छठी",    U"छठे"}}
};

// Regular ordinal endings appended directly to the last cardinal token.
// Order matches OrdinalForm: masculine / feminine / plural.
inline const std::array<std::u32string, 3> kOrdinalEndings = {
  U"वाँ", U"वीं", U"वें"
};

// Ordinal suffixes after digits, by their form in OrdinalForms: 5वाँ, 5वां, 5वीं, 5वें, and the endings of the
// irregular ordinals: 1ला, 2री, 4थे, 6ठा
inline const std::unordered_map<std::u32string, size_t> kOrdinalSuffixes = {
  {U"वाँ", 0}, {U"वां", 0}, {U"वा", 0}, {U"वॉ", 0}, {U"वीं", 1}, {U"वी", 1}, {U"वें", 2}, {U"ला", 0}, {U"ली", 1},
  {U"ले", 2},  {U"रा", 0},  {U"री", 1}, {U"रे", 2},  {U"था", 0}, {U"थी", 1}, {U"थे", 2},  {U"ठा", 0}, {U"ठी", 1},
  {U"ठे", 2}
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U",", .decimal_separators = U".", .time_separators = U":.", .currencies = keys_of(kCurrencies),
  .scales = kScales, .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"दशमलव", .dot = U"डॉट", .range = U" से ", .by = U"बाई", .score = U" ", .minus = U"माइनस", .plus = U"प्लस",
  .fraction = U" और "
};

// 27.03.2026, 27-03-2026: the year has four digits. Without it (06-05), the reading decides.
inline const DateFormat kDateFormat = {.separators = U".-/", .year_required = false, .short_year = false};

using ::verbalis::text::Readings;

// TODO: not verified against a reference verbalizer yet
inline const Readings kSymbols = {
  {U"&", {U"और"}},       {U"+", {U"प्लस"}},     {U"=", {U"बराबर"}},    {U"%", {U"प्रतिशत"}},  {U"@", {U"एट"}},
  {U"°", {U"डिग्री"}},    {U"$", {U"डॉलर"}},     {U"€", {U"यूरो"}},      {U"£", {U"पाउंड"}},    {U"₹", {U"रुपये"}}
};

// Words which mark the number after them as a year: सन 1999, साल 1971, 31 मई 1990
inline const std::unordered_set<std::u32string> kYearsAfter = {
  U"सन", U"सन्", U"साल", U"वर्ष", U"जनवरी", U"फ़रवरी", U"फरवरी", U"मार्च", U"अप्रैल", U"मई", U"जून", U"जुलाई", U"अगस्त",
  U"सितंबर", U"सितम्बर", U"अक्टूबर", U"अक्तूबर", U"नवंबर", U"नवम्बर", U"दिसंबर", U"दिसम्बर"
};

// Words which mark the number before them as a year: 1999 में, 1990 ईसवी, 120 ईसा पूर्व
inline const std::unordered_set<std::u32string> kYearsBefore = {
  U"में", U"का", U"की", U"के", U"से", U"तक", U"ईसवी", U"ईस्वी", U"ईसा"
};

// Words before a time written with a dot: सुबह 8.30
inline const std::unordered_set<std::u32string> kDayTimes = {U"सुबह", U"सवेरे", U"दोपहर", U"शाम", U"रात", U"तड़के"};

// Words before a number which make it a code, read digit by digit from four digits on: नंबर 1124, पिन कोड 110023
inline const std::unordered_set<std::u32string> kCodeCues = {
  U"नंबर", U"नम्बर", U"नं", U"कोड", U"पिन", U"पिनकोड", U"oclc", U"isbn", U"issn"
};

using ::verbalis::numbers::PhoneFormat;

// Phone numbers, read digit by digit: 03862-351791, (02229) 411128, 022.29411128, 0 (80) 26411128; and without a trunk
// zero, only after a word of phones: 1374-309988, 98765 43210
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 7}, {4, 8}}, .separators = U"-. ", .leading_zero = true},
  {.groups = {{3, 5}, {6, 8}}, .separators = U" ", .parenthesized = true, .leading_zero = true},
  {.groups = {{2, 4}, {6, 8}}, .separators = U" ", .prefix = U"0 ", .parenthesized = true},
  {.groups = {{4, 4}, {6, 6}}, .separators = U"- "},
  {.groups = {{5, 5}, {5, 5}}, .separators = U"- "}
};
inline const std::unordered_set<std::u32string> kPhoneCues = {
  U"नंबर", U"नम्बर", U"फोन", U"फ़ोन", U"मोबाइल", U"कॉल", U"संपर्क", U"हेल्पलाइन", U"फैक्स", U"फ़ैक्स", U"टेलीफोन", U"टेलीफ़ोन"
};

// Words right before a number which make it a count, not the postal code ending an address: मंगलवार को 51340.
inline const std::unordered_set<std::u32string> kCountCues = {
  U"को", U"से", U"में", U"के", U"की", U"का", U"ने", U"पर", U"तक", U"संख्या", U"लगभग", U"करीब", U"कुल", U"और"
};

// Words before a hyphen and a number which make it a minus, with the count cues: तापमान -7; after another word, the
// hyphen joins (कोविड -19, सेक्टर -1).
inline const std::unordered_set<std::u32string> kSignCues = {U"तापमान", U"पारा"};

// English ordinals as said in Hindi: 7th क्रॉस -> सेवंथ क्रॉस. By number up to 19; then the tens and their ordinals.
inline const std::array<std::u32string, 20> kEnglishOrdinals = {
  U"",          U"फ़र्स्ट",     U"सेकंड",      U"थर्ड",       U"फ़ोर्थ",      U"फ़िफ्थ",     U"सिक्स्थ",
  U"सेवंथ",     U"एटथ",       U"नाइंथ",      U"टेंथ",       U"इलेवंथ",     U"ट्वेल्फ्थ",   U"थर्टींथ",
  U"फ़ोर्टींथ",   U"फ़िफ्टींथ",   U"सिक्सटींथ",   U"सेवनटींथ",   U"एटींथ",      U"नाइनटींथ"
};
inline const std::array<std::u32string, 10> kEnglishTens = {
  U"", U"", U"ट्वेंटी", U"थर्टी", U"फ़ोर्टी", U"फ़िफ्टी", U"सिक्सटी", U"सेवंटी", U"एटी", U"नाइंटी"
};
inline const std::array<std::u32string, 10> kEnglishTenths = {
  U"", U"", U"ट्वेंटिएथ", U"थर्टिएथ", U"फ़ोर्टिएथ", U"फ़िफ्टिएथ", U"सिक्सटिएथ", U"सेवंटिएथ", U"एटिएथ", U"नाइंटिएथ"
};
inline const Keys kEnglishOrdinalSuffixes = {U"st", U"nd", U"rd", U"th"};

// Latin capitals as said in Hindi, spelled in codes: ABC-123 -> ए बी सी एक सौ तेईस
inline const std::array<std::u32string, 26> kLetters = {
  U"ए",  U"बी",  U"सी",  U"डी",   U"ई",   U"एफ",  U"जी",  U"एच",  U"आई",  U"जे",  U"के",  U"एल",  U"एम",
  U"एन", U"ओ",  U"पी",  U"क्यू",  U"आर",  U"एस",  U"टी",  U"यू",  U"वी",  U"डब्ल्यू", U"एक्स", U"वाई", U"ज़ेड"
};

// Capitalized words of codes which are said as words, not spelled: COVID-19
inline const std::unordered_set<std::u32string> kCodeWords = {U"COVID", U"SARS", U"NASA", U"ISRO", U"UEFA", U"FIFA"};

using ::verbalis::numbers::RomanCues;

// Nouns numbered by a Roman numeral after them (कक्षा XII, अध्याय IV, विश्व युद्ध II), or before them (XC विद्यार्थी)
inline const RomanCues kRomanCues = {
  .before = {
    U"कक्षा", U"अध्याय", U"भाग", U"खंड", U"खण्ड", U"युद्ध", U"योजना", U"राजमार्ग", U"श्रेणी", U"मिशन", U"चरण", U"पाठ",
    U"अनुच्छेद", U"अनुसूची", U"सत्र", U"स्तर", U"वर्ग", U"संस्करण", U"सीज़न", U"सीजन", U"टाइप", U"फेज़", U"फेज"
  },
  .after = {U"विद्यार्थी", U"छात्र"}
};

// Ordinal suffixes which may follow a Roman numeral after a space: LIII वा गणतंत्र दिवस
inline const Keys kRomanOrdinalSuffixes = {U"वाँ", U"वां", U"वा", U"वीं", U"वें"};

// A number right before: the year of an era (1990 ई., 297-272 ई. पू.)
inline bool after_number(const text::Context& context) {
  std::u32string word = context.before();
  return !word.empty() && text::unicode::isdigit(word.back());
}

// A title before a name: at the start, or after punctuation or a postposition. Not the verb of "रोक ले."
inline bool before_name(const text::Context& context) {
  static const std::unordered_set<std::u32string> words = {U"", U"के", U"की", U"का", U"ने", U"को", U"से", U"और", U"व"};
  std::u32string_view head = context.head();
  size_t end = head.find_last_not_of(U' ');
  bool punctuation = end != std::u32string_view::npos && !text::unicode::isalpha(head[end]);
  return !context.after().empty() && (punctuation || words.contains(context.before()));
}

using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Titles before a name: डॉ. अमित -> डॉक्टर अमित
    {U"डॉ.", {U"डॉक्टर"}},
    {U"डॉ0", {U"डॉक्टर"}},
    {U"डॉ॰", {U"डॉक्टर"}},
    {U"डॉ", {U"डॉक्टर"}},
    {U"प्रो.", {U"प्रोफेसर"}},
    {U"प्रो॰", {U"प्रोफ़ेसर"}},
    {U"इं.", {{U"इंजीनियर", before_name}}},
    {U"ले.", {{U"लेफ्टिनेंट", before_name}}},
    {U"वै.", {{U"वैज्ञानिक", before_name}}},
    {U"कु.", {{U"कुमारी", before_name}}},
    {U"मा.", {{U"मास्टर", before_name}}},
    // Eras after a year: 1990 ई., 120 ई. पू., ७०० ईसा पू.
    {U"ई.", {{U"ईसवी", after_number}}},
    {U"ई. पू.", {{U"ईसा पूर्व", after_number}}},
    {U"ई.पू.", {{U"ईसा पूर्व", after_number}}},
    {U"पू.", {{U"पूर्व", text::after_any({U"ईसा"})}}},
    // With ० (read as 0) or ॰ as the dot: 1825 ई०, १९१३ ई॰, ४००० ई॰पू॰
    {U"ई0", {{U"ईसवी", after_number}}},
    {U"ई॰", {{U"ईसवी", after_number}}},
    {U"ई॰पू॰", {{U"ईसा पूर्व", after_number}}},
    {U"ईपू", {{U"ईसा पूर्व", after_number}}},
    {U"वि.", {{U"विक्रम संवत्", after_number}}},
    // Anywhere: प्रति है०, नियम क्र. ७२
    {U"है0", {U"हेक्टेयर"}},
    {U"कु0", {U"कुन्तल"}},
    {U"क्र.", {U"क्रमांक"}},
    {U"प्र.", {U"प्रतिशत"}}
  },
  .web = {.dot = U"डॉट", .at = U"एट", .slash = U"फॉरवर्ड स्लैश", .hyphen = U"हाइफ़न", .underscore = U"अंडरस्कोर", .backslash = U"बैकवर्ड स्लैश",
          .colon = U"कोलन",
          .digits = {U"शून्य", U"एक", U"दो", U"तीन", U"चार", U"पाँच", U"छह", U"सात", U"आठ", U"नौ"}}
};

} // namespace verbalis::languages::hi
