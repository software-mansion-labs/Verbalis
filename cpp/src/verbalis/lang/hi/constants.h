#pragma once

#include <verbalis/processor/common/conditions.h>
#include <verbalis/processor/layers/normalize/types.h>
#include <verbalis/processor/layers/num2word/notation.h>
#include <verbalis/processor/layers/term2word/types.h>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace verbalis::hi::constants {

// --- Character Normalization ---
namespace normalize {
  using ::verbalis::processor::normalize::Characters;

  // Echo's classes, but the zero-width non-joiner and joiner stay: they decide how Devanagari letters join
  inline const Characters kCharacters = {
    {U"…",                U"..."}, // Ellipsis
    {U"«»„“”‟˝",          U"\""},  // Quotes
    {U"‘’‚‛´`ʼ",          U"'"},   // Apostrophes
    {U"\u200b\ufeff",     U""}     // Invisible: zero-width space, byte order mark
  };
} // namespace normalize

// --- Number to Word Normalization ---
namespace num2word {
  using ::verbalis::processor::num2word::DateFormat;
  using ::verbalis::processor::num2word::Joiners;
  using ::verbalis::processor::num2word::keys_of;
  using ::verbalis::processor::num2word::Notation;
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
    {10000000LL,     U"करोड़"}
  };

  // Currencies. Hindi loanword currencies are invariable in form, so only the
  // singular noun is stored.
  inline const std::unordered_map<std::u32string, std::u32string> kCurrencies = {
    {U"$", U"डॉलर"},
    {U"€", U"यूरो"},
    {U"£", U"पाउंड"}
  };

  // Months: standard Gregorian month names in Hindi (used as-is in dates).
  inline const std::vector<std::u32string> kMonths = {
    U"",          U"जनवरी",     U"फ़रवरी",    U"मार्च",      U"अप्रैल",     U"मई",         U"जून",
    U"जुलाई",      U"अगस्त",      U"सितंबर",    U"अक्तूबर",    U"नवंबर",      U"दिसंबर"
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

  // Ordinal suffixes after digits, in the order of OrdinalForms: 5वाँ, 5वीं, 5वें
  inline const std::vector<std::u32string> kOrdinalSuffixes = {
    U"वाँ", U"वीं", U"वें"
  };

  // --- Notation: how numbers are written ---
  inline const Notation kNotation = {
    .group_separators = U",", .decimal_separators = U".", .time_separators = U":", .currencies = keys_of(kCurrencies),
    .fractions = true
  };

  // Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
  inline const Joiners kJoiners = {
    .point = U"दशमलव", .dot = U"दशमलव", .range = U" से ", .by = U"गुणा", .score = U" ", .minus = U"माइनस", .plus = U"प्लस"
  };

  // 27.03.2026, 27-03-2026: the year has four digits
  inline const DateFormat kDateFormat = {.separators = U".-/", .short_year = false};
} // namespace num2word

// --- Symbols to Word Normalization ---
namespace sym2word {
  using ::verbalis::processor::Readings;

  // TODO: not verified against a reference verbalizer yet
  inline const Readings kSymbols = {
    {U"&", {U"और"}},       {U"+", {U"प्लस"}},     {U"=", {U"बराबर"}},    {U"%", {U"प्रतिशत"}},  {U"@", {U"एट"}},
    {U"°", {U"डिग्री"}},    {U"$", {U"डॉलर"}},     {U"€", {U"यूरो"}},      {U"£", {U"पाउंड"}},    {U"₹", {U"रुपये"}}
  };
} // namespace sym2word

// --- Terms to Word Normalization ---
namespace term2word {
  using ::verbalis::processor::term2word::Terms;

  inline const Terms kTerms = {
    .web = {.dot = U"डॉट", .at = U"एट", .slash = U"स्लैश"}
  };
} // namespace term2word

} // namespace verbalis::hi::constants