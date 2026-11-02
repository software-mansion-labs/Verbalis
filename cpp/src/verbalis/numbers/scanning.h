#pragma once

#include "formats.h"
#include "match.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * Scanning numbers
 *
 * Written numbers at a position of a text, on top of the shared scanning in text/scan.h, and the shared steps of the
 * languages' readings: values of digits, digit-by-digit decisions, and the parts of codes.
 */
namespace verbalis::numbers {

using text::Span;

// The digits starting at a position, as a number.
Number digits_at(std::u32string_view text, size_t position);

/**
 * A written number at a position: 1,000.5 or 1 000,5 or 15.
 * @returns its possible readings, in the order a regex would try them: grouped first,
 *          each with and then without its fraction. Empty if no number starts there.
 */
std::vector<Number> numbers_at(std::u32string_view text, size_t position, std::u32string_view group,
                               std::u32string_view decimal);

// The value of a run of digits; nullopt if it is empty or too long to hold.
std::optional<uint64_t> parse_value(std::u32string_view digits);

// The value of a number's integer part; 0 if it has none or it is too long to hold.
uint64_t value(const Number& number);

// Whether a number is "1" as written, which counts a singular noun.
bool is_one(const Number& number);

// Whether an integer is read digit by digit: a leading zero, or a long code that is not a round amount.
bool is_spoken_digits(const Number& number, size_t longest);

/**
 * A day of a month written with a leading zero, without it: 05 de mayo, 04 juin; nullopt for any other number.
 * @param months lowercase.
 * @param of a word which may stand between the day and the month: "de".
 */
std::optional<Number> unpadded_day(const Number& number, const Match& match, std::span<const std::u32string> months,
                                   std::u32string_view of = U"");

// The value of the digits of a code or phone number, or nullopt if they read digit by digit: a leading zero, more than
// longest digits, or too long to hold.
std::optional<uint64_t> code_value(std::u32string_view digits, size_t longest);

/**
 * Whether a degree sign, as a unit or an ordinal suffix, marks an ordinal (el 2° lugar, il 17° Meeting), not degrees (a 30°): one of a language's
 * articles before it, one of its nouns after it, or a name after it, which is not a compass point (33° Sur).
 */
bool is_ordinal_degree(const Match& unit, const std::unordered_set<std::u32string>& articles,
                       const std::unordered_set<std::u32string>& nouns);

// Whether a time's hours and minutes fit a clock: at most 24 and 59. An hour alone always fits.
bool fits_clock(const Match& time);

// The numerator and denominator of a fraction: its last two numbers (3 1/2 has its whole number first).
std::pair<uint64_t, uint64_t> fraction_of(const Match& fraction);

// The words of a fraction with its own words (1/2: half), or nullopt.
std::optional<std::u32string> named_fraction(const Fractions& fractions, const Match& fraction);

/**
 * Whether a fraction is written as one: after a whole number (3 1/2), with a binary denominator before an inch mark
 * (3/8"), or with a common denominator (to 10, 12, 16, 100, 1000) before one of a language's words (1/3 of). A bare
 * 3/4 may be a date, a score, or a rating.
 * @param words lowercase; a word with an apostrophe matches up to it (d', dell')
 */
bool is_marked_fraction(const Match& fraction, const std::unordered_set<std::u32string>& words);

// The suffix of a match with a space before it, to follow its reading; empty if the match has none.
std::u32string spaced_suffix(const Match& match);

// A value written as digits: 14 -> "14".
std::u32string to_digits(uint64_t value);

// The value of a Roman numeral written in its canonical form (XIX, not XVIIII), up to 3999; nullopt otherwise.
std::optional<uint64_t> roman_value(std::u32string_view numeral);

// What the words around a Roman numeral make it: the number of a name, or of a noun before or after it.
enum class RomanRole { NAME = 0, BEFORE, AFTER };

// The role of a Roman numeral match by a language's cues, or nullopt if nothing marks it as a number.
std::optional<RomanRole> roman_role(const Match& match, const RomanCues& cues);

// The parts of a code (MP3, H.264, F-16s): digits with their decimals, ".digits", runs of other characters,
// and single apostrophes, dots and hyphens.
std::vector<std::u32string_view> code_parts(std::u32string_view code);

} // namespace verbalis::numbers
