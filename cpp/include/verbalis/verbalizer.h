#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace verbalis {

// Which steps run, and readings added to the language's own. Every step is on by default.
struct Options {
  bool normalize = true; // One form for equivalent characters: quotes, apostrophes, dashes, ellipses
  bool trim = true;      // Single spaces between words, none at the edges or before punctuation
  bool terms = true;     // Abbreviations and web addresses as words: "e.g." -> "for example"
  bool numbers = true;   // Numbers, dates, times, money, and units as words: "$5" -> "five dollars"
  bool symbols = true;   // Remaining symbols as words: "&" -> "and"

  std::map<std::string, std::string> abbreviations; // Added or replaced readings: {"approx.", "approximately"}
  std::map<std::string, std::string> symbol_words;  // Added or replaced readings: {"™", "trademark"}
};

// One edit of the input: the text in [start, end), counted in Unicode code points, became `replacement`.
struct Change {
  size_t start = 0;
  size_t end = 0;
  std::string replacement;
};

// A verbalized text, with the edits that turned the input into it.
struct Result {
  std::string text;
  std::vector<Change> changes;
};

/**
 * Rewrites text the way it is read aloud: numbers, dates, symbols, abbreviations, and web addresses as words.
 * Text is UTF-8. A Verbalizer is immutable after construction, so one instance can serve many threads. process and
 * process_with_changes throw only on a bug in Verbalis (a std::logic_error from conflicting edits), never on input.
 */
class Verbalizer {
public:
  /**
   * @param language a code from languages(), for example "en-us" or "pl".
   * @throws std::invalid_argument if the language isn't supported.
   */
  explicit Verbalizer(std::string_view language, const Options& options = {});
  ~Verbalizer();
  Verbalizer(Verbalizer&&) noexcept;
  Verbalizer& operator=(Verbalizer&&) noexcept;

  // The text as it is read aloud.
  std::string process(std::string_view text) const;

  // The text as it is read aloud, with the edits of the input that produced it, in order.
  Result process_with_changes(std::string_view text) const;

  // The supported language codes.
  static const std::vector<std::string>& languages();

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace verbalis
