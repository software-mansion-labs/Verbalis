#include "constants.h"

#include <verbalis/languages/language.h>
#include <verbalis/numbers/scanning.h>
#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::hi {

namespace {
using namespace text;
using namespace numbers;

// ------------------------
// ----- Number words -----
// ------------------------

// Every number up to 99 is its own word; then "<digit> सौ", and the Indian scales (हज़ार, लाख, करोड़)
std::u32string cardinal(uint64_t n) {
  if (n < 100) {
    return kCardinals.at(static_cast<int32_t>(n));
  }
  if (n < 1000) {
    uint64_t rest = n % 100;
    return kCardinals.at(static_cast<int32_t>(n / 100)) + U" सौ" + (rest ? U" " + cardinal(rest) : U"");
  }

  // Hindi keeps the "एक" of every scale: एक हज़ार, एक लाख
  for (auto it = kLargeCardinals.rbegin(); it != kLargeCardinals.rend(); ++it) {
    auto base = static_cast<uint64_t>(it->first);
    if (n >= base) {
      uint64_t low = n % base;
      return cardinal(n / base) + U" " + it->second + (low ? U" " + cardinal(low) : U"");
    }
  }
  return U"";
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.push_back(cardinal(c - U'0'));
  }
  return join(words);
}

// --------------------
// ----- Language -----
// --------------------

// Hindi reads its numbers without context so far.
// TODO: outdated - Hindi readings predate the context-aware design and have no reference verbalizer.
// Missing: Indian digit grouping (1,00,000), Devanagari digits, units and context (gender) of the counted noun.
class Hindi final : public PatternLanguage {
public:
  Hindi() : PatternLanguage({.characters = &kCharacters, .terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (number.fraction.empty() && is_spoken_digits(number, 7)) {
      return digits(number.integer);
    }

    // The fraction is read digit by digit, without its trailing zeros: 0.50 -> शून्य दशमलव पाँच
    std::u32string_view fraction = number.fraction;
    fraction = fraction.substr(0, fraction.find_last_not_of(U'0') + 1);
    std::u32string integer = cardinal(value(number));
    return fraction.empty() ? integer : integer + U" दशमलव " + digits(fraction);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // सत्ताईस मार्च दो हज़ार छब्बीस
    return cardinal(value(match.numbers[0])) + U" " + kMonths[value(match.numbers[1])] + U" " +
           cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    // Common fractions have their own nouns, others are "<a> बटा <b>"
    uint64_t a = value(match.numbers[0]), b = value(match.numbers[1]);
    if (a == 1 && b == 2) {
      return U"आधा";
    }
    if (b == 3 || b == 4) {
      return cardinal(a) + (b == 3 ? U" तिहाई" : U" चौथाई");
    }
    return cardinal(a) + U" बटा " + cardinal(b);
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    // The suffix gives the form: masculine (वाँ), feminine (वीं) or plural (वें)
    size_t form = std::ranges::find(kOrdinalSuffixes, match.suffix) - kOrdinalSuffixes.begin();
    uint64_t n = value(match.numbers[0]);

    // पहला, दूसरा, तीसरा, चौथा, छठा: irregular
    if (auto it = kOrdinalsSpecial.find(static_cast<int32_t>(n)); n < 10 && it != kOrdinalsSpecial.end()) {
      return it->second[form];
    }
    return cardinal(n) + kOrdinalEndings[form];
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    return read_number(number, match) + U" " + kCurrencies.at(std::u32string(match.unit));
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return read_number(number, match) + U" प्रतिशत";
  }
};
} // namespace

const Language& language() {
  static const Hindi language;
  return language;
}

} // namespace verbalis::languages::hi
