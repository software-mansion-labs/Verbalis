#pragma once

#include <verbalis/text/scan.h>

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::numbers {

using text::Keys;

// How a numeric date is written in a language: 12.05.2024, 12/05/24, 1. 5.
struct DateFormat {
  std::u32string separators = U"/";   // Between its parts: the same one twice in a date
  std::u32string boundaries = U".,/"; // Besides letters, what may not precede it (or a digit after it)
  bool year_required = true;
  bool short_year = true;         // Two-digit years: 12/05/24
  bool closing_separator = false; // A separator also closes the month: 1.5. and 1.5.2024
  bool spaced = false;            // An optional space after the day: 12. 5.
  Keys suffixes;                  // Abbreviations after the year: 2024 r.
  // A two-digit year below the pivot is in the 2000s (24: 2024); one from it on is in the 1900s (98: 1998) with
  // full_years, and stays as written (98) without
  int year_pivot = 30;
  bool full_years = false;
  // The names of the months, lowercase: a month may be written as its name or its first three letters or more, in
  // any case (23/Ago/2013, 06-jun-2012)
  std::vector<std::u32string> months;
};

// How a phone number is written: its groups of digits and what separates them.
struct PhoneFormat {
  std::vector<std::pair<size_t, size_t>> groups; // Length range of each group of digits
  std::u32string separators = U"-";
  bool optional_separators = false; // 0612345678 as well as 06 12 34 56 78
  std::u32string prefix;            // An optional literal start: "1-"
  bool parenthesized = false;       // The first group in parentheses, then an optional space: (555) 123-4567
  bool leading_zero = false;
  std::u32string boundaries = U"-"; // Besides letters, what may not stand right before it
  std::u32string closing = U"-";    // Besides letters, what may not stand right after it
};

// How a suffix marks a number: 21st, 1 º, 90's, 2020 r.
struct SuffixFormat {
  bool space = false;                // An optional space before the suffix
  bool word_end = true;              // No word character may follow the suffix
  bool apostrophes = false;          // '90s, 90's: optional apostrophes before the number and the suffix
  std::u32string boundaries = U".,"; // Besides letters, what may not stand right before the number
  bool sentence_dot = false;         // A dot closing the suffix is left to a sentence it also closes
};

// How numbers are written in a language, read by the generic patterns.
// Fractions with their own words, by numerator and denominator: 1/2 -> half
using Fractions = std::map<std::pair<uint64_t, uint64_t>, std::u32string>;

// The words which mark a Roman numeral as a number in a language (lowercase, as Context reads them).
struct RomanCues {
  std::unordered_set<std::u32string> names;  // A name before it takes a regnal number: Henry VIII, Jan Paweł II
  std::unordered_set<std::u32string> before; // A noun before it, which it numbers: chapter IV, siglo XXI
  std::unordered_set<std::u32string> after;  // A noun after it, which it numbers: XXI wieku, II Guerra Mundial
};

struct Notation {
  std::u32string group_separators;   // 1.000, 1 000, 1,000
  std::u32string decimal_separators; // 1,5, 1.5
  std::u32string time_separators;    // 18:30, 18.30, 18h30
  Keys time_suffixes;                // pm, Uhr, h
  Keys currencies;                   // $, zł, EUR
  Keys scales;                       // million, mln, k
  Keys units;                        // km, kg, °C
  bool fractions = false;            // Whether 3/4 reads as a fraction
  // What an amount is counted by after a slash, and its words: the h of 5 km/h, "per hour"
  std::unordered_map<std::u32string, std::u32string> per;
};

// The keys of a table, for a Notation built out of a language's constants.
template <typename Table>
Keys keys_of(const Table& table) {
  Keys keys;
  keys.reserve(table.size());
  for (const auto& [key, value] : table) {
    keys.emplace_back(key);
  }
  return keys;
}

} // namespace verbalis::numbers
