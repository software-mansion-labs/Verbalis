#pragma once

#include "grammar.h"

#include <verbalis/numbers/patterns.h>
#include <verbalis/steps/normalize.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/document.h>
#include <verbalis/text/readings.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace verbalis::languages {

using text::Document;
using text::Readings;
using steps::Characters;
using numbers::Kind;
using numbers::Match;
using numbers::Notation;
using numbers::Number;
using numbers::Pattern;
using steps::Terms;

// What the pipeline needs from a language: the tables of its steps, and how it reads numbers.
class Language {
public:
  struct Tables {
    const Characters* characters = nullptr; // Its character classes, if not the default ones
    const Terms* terms = nullptr;           // Its abbreviations and web words, if any
    const Readings* symbols = nullptr;      // Its symbol words, if any
  };

  virtual ~Language() = default;

  const Tables& tables() const { return tables_; }

  // Queues the edits that read the numbers of a document as words.
  virtual void read_numbers(Document& document) const = 0;

protected:
  explicit Language(Tables tables) : tables_(tables) {}

private:
  Tables tables_;
};

/**
 * A language whose numbers are found by patterns and said by its readings.
 *
 * The patterns run in priority order (see Kind), each one over the whole text as the patterns before it left it.
 * Every match is said by the reading of its kind. A reading returns nullopt to leave its match as written, so it
 * also decides whether a match really is what its pattern found: a time can turn out to be a decimal (1.13).
 * The words around a match are in match.context.
 *
 * A language must say its numbers (read_number) and the digits of codes and phone numbers (read_code_digits); every
 * other reading has a default built on those two.
 */
class PatternLanguage : public Language {
public:
  void read_numbers(Document& document) const final;

protected:
  PatternLanguage(Tables tables, const Notation& notation, Joiners joiners);

  // Adds one of the language's own patterns; it goes before the generic patterns of its kind.
  void add_pattern(std::unique_ptr<Pattern> pattern);

  const Joiners& joiners() const { return joiners_; }

  // A number, in the context of its match: 42, 1 000,5, .5
  virtual std::u32string read_number(const Number& number, const Match& match) const = 0;

  // A run of digits in a code or a phone number: the 380 of A380
  virtual std::u32string read_code_digits(std::u32string_view digits) const = 0;

  // Readings of whole matches; by default their numbers one by one
  virtual std::optional<std::u32string> read_date(const Match& match) const;

  // A phone number's groups. It also reads the groups of an international number, as a match of that kind.
  virtual std::optional<std::u32string> read_phone(const Match& match) const;
  virtual std::optional<std::u32string> read_fraction(const Match& match) const;
  virtual std::optional<std::u32string> read_time(const Match& match) const;
  virtual std::optional<std::u32string> read_ordinal(const Match& match) const;
  virtual std::optional<std::u32string> read_decade(const Match& match) const;
  virtual std::optional<std::u32string> read_compound(const Match& match) const;
  virtual std::optional<std::u32string> read_plain(const Match& match) const; // Any other number

  // Amounts: each says the last number of its match; a range (10-15 km) gets its start and the range joiner before it
  virtual std::optional<std::u32string> read_money(const Number& amount, const Match& match) const;
  virtual std::optional<std::u32string> read_scale(const Number& amount, const Match& match) const;
  virtual std::optional<std::u32string> read_percent(const Number& amount, const Match& match) const;
  virtual std::optional<std::u32string> read_unit(const Number& amount, const Match& match) const;

  // Symbols between numbers (DIFFERENCE, SUM, RANGE, DIMENSION, SIGN, SCORE), from the joiners by default
  virtual std::optional<std::u32string> read_joiner(const Match& match) const;
  virtual std::u32string read_range(const Number& from, const Number& to, const Match& match) const;

  // The plural of a reading, for codes like F-16s; none by default.
  virtual std::optional<std::u32string> plural(std::u32string_view /*words*/) const { return std::nullopt; }

private:
  using Amount = std::optional<std::u32string> (PatternLanguage::*)(const Number&, const Match&) const;

  std::optional<std::u32string> read_match(const Match& match) const;

  // Shared readings
  std::u32string read_each(const Match& match, std::u32string_view separator = U" ") const; // Its numbers one by one
  std::u32string read_version(const Match& match) const;
  // An amount read as a number followed by a word as written: 5 km, 10 %
  std::u32string with_word(const Number& amount, const Match& match, std::u32string_view word) const;
  std::optional<std::u32string> read_international_phone(const Match& match) const;
  std::u32string read_code(const Match& match) const;
  std::optional<std::u32string> read_amount(const Match& match, Amount reading) const;

  struct Entry {
    std::unique_ptr<Pattern> pattern;
    bool generic;
  };
  std::vector<Entry> patterns_; // In priority order: by kind, then the language's own before the generic ones
  Joiners joiners_;
};

} // namespace verbalis::languages
