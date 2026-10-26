#pragma once

#include <verbalis/text/context.h>
#include <verbalis/text/document.h>

#include <string>
#include <string_view>
#include <vector>

namespace verbalis::numbers {

using text::Context;
using text::Span;

// Types of numeric expressions, in priority order: at a given position of a text,
// patterns of earlier kinds are tried first. Languages add patterns, but never kinds.
enum class Kind {
  // Whole expressions whose parts are numbers
  DATE = 0,            // 12.05.2024, 2024-05-12, 1. 5.
  INTERNATIONAL_PHONE, // +48 600 123 456, +1 (555) 123-4567: before a language's own formats of its groups
  PHONE,               // 555-123-4567, 06 12 34 56 78
  VERSION,             // 3.8.10, 10.0.0.51

  // Symbols between numbers (only the symbol is matched)
  DIFFERENCE, // the " - " of 5 - 3
  SUM,        // the "+" of 2 + 2 and 90+6
  RANGE,      // the "-" of 10-15 and 2020-2022
  DIMENSION,  // the "x" of 1920x1080 and 3 m x 4 m
  FRACTION,   // 3/4
  SIGN,       // the minus of -5
  TIME,       // 18:30, 7pm, 18 Uhr, 21h30
  SCORE,      // the ":" of 2:1

  // Numbers with what they count
  MONEY,   // $5, 12,99 zł, 10-15 €, 3 mln zł
  SCALE,   // 3 mln, 500k, 1.5 million
  PERCENT, // 20%, 10-15%, 8%ige
  UNIT,    // 5 kg, 10-15 km

  // Numbers marked by a suffix or joined to letters
  ORDINAL,  // 21st, 3., 1er, 1º
  DECADE,   // 1990s, 90er, '80
  COMPOUND, // 21-year-old, 45-metrowe
  CODE,     // MP3, A380, COVID-19

  // Any other number
  NUMBER, // 42, 1 000,5, .5, 2020 r.
};

// A number as written: "1 000,5" -> integer "1000", fraction "5".
struct Number {
  Span span;                 // Where it is written
  std::u32string integer;    // Digits without group separators; empty in ".5"
  std::u32string fraction;   // Digits after the decimal mark; empty if none
  char32_t decimal_mark = 0; // The decimal mark as written, if any
  bool grouped = false;      // Written with group separators: 1 000, 1,000
};

// A numeric expression found in a text: its structure only, with no decision on how to read it.
struct Match {
  Kind kind = Kind::NUMBER;
  Span span;
  std::u32string_view text;    // As written
  std::vector<Number> numbers; // In written order: one for most kinds, two for ranges, three for dates
  std::u32string_view unit;    // A unit or currency key ("km", "zł"), or a law word of a DATE ("Ley")
  std::u32string_view scale;   // A scale key ("mln", "million")
  std::u32string_view suffix;  // Marks after the number: "st", "er", "." (ordinals), "ige" (8%ige), "pm", "r."
  char32_t separator = 0;      // The symbol between numbers: '-', ':', '.', '/', 'h', 'x'
  Context context;             // The words around it, set when it is read
};

} // namespace verbalis::numbers
