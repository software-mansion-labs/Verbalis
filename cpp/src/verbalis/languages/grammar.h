#pragma once

#include <string>

namespace verbalis::languages {

// Words languages build their readings out of: the number engine never reads these, only the languages do

// Words read in place of the symbols between numbers.
struct Joiners {
  std::u32string point;    // 3.8.10: three point eight point ten
  std::u32string dot;      // 10.0.0.51 (four parts): ten dot zero dot zero dot fifty-one
  std::u32string range;    // 10-15, with its own spaces: " to ", " bis ", "-"
  std::u32string by;       // 1920x1080
  std::u32string score;    // 2:1, with its own spaces: " to "
  std::u32string minus;    // -5 and 5 - 3
  std::u32string plus;     // 2 + 2
  std::u32string fraction; // 3 3/4, with its own spaces: " and ", " y "; a space if empty
  std::u32string half;     // 3 1/2 in place of the fraction, with its own spaces: " and a half"; unused if empty
  std::u32string rating;   // 4.5/5: four point five out of five; the slash stays if empty
};

// The forms of a noun after a number: one dollar, two dollars.
struct Forms {
  std::u32string one;
  std::u32string many;
};

// Vocabulary of readings: languages build their own readings out of these
enum class Case { NOMINATIVE = 0, GENITIVE, DATIVE, ACCUSATIVE, INSTRUMENTAL, LOCATIVE };
enum class Gender { MASCULINE = 0, FEMININE, NEUTER };

} // namespace verbalis::languages
