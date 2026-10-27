#pragma once

#include <verbalis/text/document.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/scan.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace verbalis::steps {

using text::Document;
using text::Keys;
using text::Readings;
using text::Span;

// The words between the parts of a web address.
struct WebWords {
  std::u32string dot;        // example dot com
  std::u32string at;         // jan at example dot com
  std::u32string slash;      // example dot com slash news
  std::u32string hyphen;     // my hyphen site dot com; the hyphen stays as written if empty
  std::u32string underscore; // jan underscore nowak at example dot com; stays as written if empty
  // C colon backslash Users: paths (/home/desktop, C:\Users) and file names (report.pdf) stay as written if empty
  std::u32string backslash;
  std::u32string colon;
  std::vector<std::u32string> digits; // The names of the digits 0-9, read one by one: covid one nine dot gov
};

// The terms of a language: written forms which are not said as written.
struct Terms {
  // nr -> numer before a number; St. -> Saint before a name, Street otherwise
  Readings abbreviations;
  WebWords web;
};

/**
 * Handles non-numeric and non-symbolic terms.
 * 
 * Example: web addresses (www.example.com/news, jan@example.com), abbreviations (nr 5, e.g.).
 */
class TermReader {
public:
  explicit TermReader(Terms terms);

  void apply(Document& document) const;

private:
  // A term found in a text, with its words
  struct Term {
    Span span;
    std::u32string words;
  };

  // https://www.example.com/news, jan.kowalski@example.pl: its parts joined with the words of the separators
  std::optional<Term> read_address(std::u32string_view text, size_t position) const;

  // /home/desktop, C:\Users\HP, home/desktop, report.pdf: a path or a file name, with its separators read
  std::optional<Term> read_path(std::u32string_view text, size_t position) const;

  // The longest abbreviation at a position with a reading in its context
  std::optional<Term> read_abbreviation(std::u32string_view text, size_t position) const;

  Terms terms_;
  Keys keys_; // Of the abbreviations
};

} // namespace verbalis::steps
