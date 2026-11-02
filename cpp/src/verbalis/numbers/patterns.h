#pragma once

#include "formats.h"
#include "match.h"

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::numbers {

using text::Keys;

// Recognizes one kind of numeric expression at a given position of a text.
class Pattern {
public:
  explicit Pattern(Kind kind) : kind_(kind) {}
  virtual ~Pattern() = default;

  Kind kind() const { return kind_; }

  /**
   * Matches the expression starting at given position, if there is one.
   * @param text the text, as rewritten by the patterns before.
   * @param position where the expression has to start.
   */
  virtual std::optional<Match> match(std::u32string_view text, size_t position) const = 0;

private:
  Kind kind_;
};

/**
 * Patterns of numeric expressions
 *
 * Generic patterns are added for every language and read its Notation.
 * Configurable patterns (dates, phones, suffixes) are added by each language with its own format.
 */

// ----------------------------
// ----- Generic patterns -----
// ----------------------------

// The generic patterns of a language.
std::vector<std::unique_ptr<Pattern>> generic_patterns(const Notation& notation);

// 2024-05-12: an ISO date. Its numbers are ordered as day, month, year.
class IsoDatePattern : public Pattern {
public:
  IsoDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// 3.8.10, 10.0.0.51; a chain of thousands groups (1.000.000) is left to numbers.
class VersionPattern : public Pattern {
public:
  explicit VersionPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  std::u32string group_;
};

// The " - " of 5 - 3.
class DifferencePattern : public Pattern {
public:
  DifferencePattern() : Pattern(Kind::DIFFERENCE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// The "+" of 2 + 2 or 90+6, with the spaces around it.
class SumPattern : public Pattern {
public:
  SumPattern() : Pattern(Kind::SUM) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// The "-" of 10-15 and 2020-2022. Its numbers are the digits on both sides.
class RangePattern : public Pattern {
public:
  RangePattern() : Pattern(Kind::RANGE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// The "x" of 1920x1080, or of 3 m x 4 m (the units are left to UnitPattern).
class DimensionPattern : public Pattern {
public:
  explicit DimensionPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Keys units_;
};

// 3/4, and 3 1/2: its numbers are then the whole number, the numerator, and the denominator.
class FractionPattern : public Pattern {
public:
  FractionPattern() : Pattern(Kind::FRACTION) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  // A fraction without a whole number: 3/4
  static std::optional<Match> simple(std::u32string_view text, size_t position);
};

// 14:30:05: hours, minutes, and seconds
class SecondsPattern : public Pattern {
public:
  /**
   * @param suffixes words which may follow it, after an optional space, as part of it: 02:02:23 Uhr.
   * @param loose takes seconds above 59, but no time right after a colon.
   */
  explicit SecondsPattern(Keys suffixes = {}, bool loose = false)
      : Pattern(Kind::TIME), suffixes_(std::move(suffixes)), loose_(loose) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Keys suffixes_;
  bool loose_;
};

/**
 * 1'27"23, 2h11'13", 23"42, 5'9", 18'': a time or a length written with prime marks.
 *
 * A time is a TIME match with the separator ′. Its unit is the unit of its first number, "h", "min", or "s", and each
 * number after it counts the next one; the digits after the seconds mark (23"42) are the fraction of the seconds.
 * Feet and inches (5'9") are a TIME match of the unit "ft". A lone number of inches (18'') is a UNIT match of the
 * unit ″, and of feet (400'), of "ft".
 */
class PrimePattern : public Pattern {
public:
  /**
   * @param cues lowercase words of a text which make its minutes and seconds (3'16") and a lone 23″ a time; without
   *        one, they are left as written and a lone 23″ is inches.
   * @param feet reads 5'9" as feet and inches, and a lone 400' as feet.
   * @param minutes lowercase words among the three before or after a lone 10' which make it minutes: durante más de
   *        10', aos 6′ do segundo tempo; without one, it is left as written.
   */
  PrimePattern(const Notation& notation, std::unordered_set<std::u32string> cues, bool feet = false,
               std::unordered_set<std::u32string> minutes = {})
      : Pattern(Kind::TIME),
        decimals_(notation.decimal_separators),
        cues_(std::move(cues)),
        feet_(feet),
        minutes_(std::move(minutes)) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  // Whether a word of a text is one of the cues
  static bool cued(std::u32string_view text, const std::unordered_set<std::u32string>& cues);

  std::u32string decimals_;
  std::unordered_set<std::u32string> cues_;
  bool feet_;
  std::unordered_set<std::u32string> minutes_;
};

// XIXe, XIIवीं: an ordinal written in Roman numerals, with a suffix; one letter only if it is I, V or X.
class RomanOrdinalPattern : public Pattern {
public:
  explicit RomanOrdinalPattern(Keys suffixes) : Pattern(Kind::ORDINAL), suffixes_(std::move(suffixes)) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Keys suffixes_;
};

// 1,000th, 1.000.º, 1 001.ᵉʳ: an ordinal of a grouped number
class GroupedOrdinalPattern : public Pattern {
public:
  GroupedOrdinalPattern(Keys suffixes, std::u32string group_separators)
      : Pattern(Kind::ORDINAL), suffixes_(std::move(suffixes)), group_separators_(std::move(group_separators)) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Keys suffixes_;
  std::u32string group_separators_;
};

// The "/" of 4,2/5 and 4.5/10: a rating, a decimal out of 5, 10, 20, or 100.
class RatingPattern : public Pattern {
public:
  explicit RatingPattern(const Notation& notation) : Pattern(Kind::RATING), decimals_(notation.decimal_separators) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  std::u32string decimals_;
};

// The minus of -5.
class SignPattern : public Pattern {
public:
  SignPattern() : Pattern(Kind::SIGN) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  /**
   * Whether a hyphen after a word and a space joins the number to the word rather than signs it: after an acronym
   * (COVID -19, but GMT -4), before a year (Enhogar -2016), or opening a dash pair (por décimo -1.000 euros la serie-).
   */
  static bool joins(std::u32string_view text, size_t position);
};

// 18:30, 10.30 pm, 21h30: hours and minutes, with an optional suffix.
class TimePattern : public Pattern {
public:
  explicit TimePattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  // Whether a comma before a position follows a suffix: 11:20AM,2:40PM
  bool after_suffix(std::u32string_view text, size_t position) const;
  // Whether a time may end at a position: not a longer number, nor a measure (1.30 min), unless a suffix closes it
  bool ends_at(std::u32string_view text, size_t position, bool suffixed = false) const;

  std::u32string separators_;
  Keys suffixes_;
  Keys measures_;
};

// 7pm, 18 Uhr, 21h: an hour marked by its suffix.
class HourPattern : public Pattern {
public:
  explicit HourPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Keys suffixes_;
};

// The ":" of 2:1.
class ScorePattern : public Pattern {
public:
  ScorePattern() : Pattern(Kind::SCORE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// $5, € 1,5 million: a currency symbol before an amount.
class PrefixedMoneyPattern : public Pattern {
public:
  // @param words currencies written as words which also precede an amount: USD 5
  explicit PrefixedMoneyPattern(const Notation& notation, const Keys& words = {});

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
  Keys currencies_; // Only the ones which are not words
  Keys per_;
};

// 50$, 12,99 zł, 10-15 €, 3 mln zł: an amount, possibly a range, followed by its currency.
class MoneyPattern : public Pattern {
public:
  explicit MoneyPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
  Keys per_;
};

// 3 mln, 500k, 1.5 million
class ScalePattern : public Pattern {
public:
  explicit ScalePattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
};

// 20%, 10-15 %, 8%ige
class PercentPattern : public Pattern {
public:
  explicit PercentPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
};

// 5 kg, 10-15 km, 230 V; a unit of a single capital letter only after a space.
class UnitPattern : public Pattern {
public:
  explicit UnitPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
  Keys letters_; // Units of a single capital letter (W, V)
  Keys per_;
};

// 21-year-old, 45-metrowe: a number joined to a word by a hyphen.
class CompoundPattern : public Pattern {
public:
  explicit CompoundPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
};

// MP3, A380, COVID-19, H.264: a token mixing letters and digits.
class CodePattern : public Pattern {
public:
  CodePattern() : Pattern(Kind::CODE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// 42, 1 000,5, .5: any other number.
class NumberPattern : public Pattern {
public:
  explicit NumberPattern(const Notation& notation);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Notation notation_;
};

// ---------------------------------
// ----- Configurable patterns -----
// ---------------------------------

// A numeric date. Its numbers are ordered as day, month and year (if written).
class DatePattern : public Pattern {
public:
  explicit DatePattern(DateFormat format);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  // The date without (zero) or with its year, if it ends at a valid position: with glue, also before a letter
  std::optional<Match> complete(std::u32string_view text, Match date, size_t position, bool glue) const;

  DateFormat format_;
};

// +48 600 123 456, +1 (555) 123-4567: a phone number with a country code. Its numbers are the code, then the groups.
class InternationalPhonePattern : public Pattern {
public:
  InternationalPhonePattern() : Pattern(Kind::INTERNATIONAL_PHONE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override;
};

// A phone number. Its numbers are its groups of digits.
class PhonePattern : public Pattern {
public:
  explicit PhonePattern(PhoneFormat format);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  // The phone number starting with its first group at a position
  std::optional<Match> groups_at(std::u32string_view text, size_t start, size_t position) const;

  PhoneFormat format_;
};

// Digits followed by a suffix from a table.
class SuffixPattern : public Pattern {
public:
  SuffixPattern(Kind kind, Keys suffixes, SuffixFormat format = {});

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  Keys suffixes_;
  SuffixFormat format_;
};

// A Roman numeral as a word of its own: VIII, XXI, IV.; one letter only if it is I, V or X. Its number holds its value.
// Whether it is a number, and how it reads, is left to the reading (see roman_role): CD and MX are mostly not.
class RomanPattern : public Pattern {
public:
  /**
   * @param dot also takes a dot closing it, as German ordinals do (Ludwig XIV. regierte), unless the text ends there.
   * @param suffixes words which may follow it, after an optional space, as part of it: XIX w.
   */
  explicit RomanPattern(bool dot = false, Keys suffixes = {})
      : Pattern(Kind::ROMAN), dot_(dot), suffixes_(std::move(suffixes)) {}
  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  bool dot_;
  Keys suffixes_;
};

// 3. Platz, 55. sesję: an ordinal written as digits with a dot, followed by a word.
class DotOrdinalPattern : public Pattern {
public:
  /**
   * @param before_punctuation also before a comma or at the end of the text: lat 70., Punkt 3.
   * @param sentence_dot leaves the dot to a sentence it also closes.
   */
  explicit DotOrdinalPattern(bool before_punctuation = false, bool sentence_dot = false);

  std::optional<Match> match(std::u32string_view text, size_t position) const override;

private:
  bool before_punctuation_;
  bool sentence_dot_;
};

} // namespace verbalis::numbers
