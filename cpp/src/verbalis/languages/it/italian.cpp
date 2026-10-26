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

namespace verbalis::languages::it {

namespace {
using namespace text;
using namespace numbers;

// ------------------------
// ----- Number words -----
// ------------------------

// ventuno, ventotto: a ten drops its vowel before uno and otto
std::u32string below_hundred(uint64_t n) {
  if (n < 20) {
    return std::u32string(kOnes[n]);
  }
  std::u32string tens(kTens[n / 10]);
  uint64_t unit = n % 10;
  if (unit == 1 || unit == 8) {
    tens.pop_back();
  }
  return tens + (unit ? std::u32string(kOnes[unit]) : U"");
}

// centotto, novecentottanta: a hundred drops its vowel before otto and ottanta
std::u32string below_thousand(uint64_t n) {
  uint64_t h = n / 100, r = n % 100;
  std::u32string head = h == 0 ? U"" : (h == 1 ? U"cento" : std::u32string(kOnes[h]) + U"cento");
  if (!head.empty() && (r == 8 || (r >= 80 && r <= 89))) {
    head.pop_back();
  }
  return head + (r || head.empty() ? below_hundred(r) : U"");
}

// ventitré, centotré, milletré: a compound ending in tre takes the accent
std::u32string accented(std::u32string_view words) {
  std::vector<std::u32string> out;
  for (auto word : strings::split(words, U' ')) {
    std::u32string w(word);
    if (w.size() > 3 && w.ends_with(U"tre")) {
      w.replace(w.size() - 3, 3, U"tré");
    }
    out.push_back(w);
  }
  return join(out);
}

// One word below a million (duemilaventidue), then un milione duecentomila
std::u32string cardinal(uint64_t n) {
  std::vector<std::u32string> words;
  for (auto [scale, one, many] :
       {std::tuple{1000000000ULL, U"un miliardo", U"miliardi"}, std::tuple{1000000ULL, U"un milione", U"milioni"}}) {
    if (uint64_t q = n / scale) {
      words.push_back(q == 1 ? std::u32string(one) : cardinal(q) + U" " + many);
    }
    n %= scale;
  }

  uint64_t thousands = n / 1000;
  n %= 1000;
  std::u32string head = thousands == 0 ? U"" : (thousands == 1 ? U"mille" : below_thousand(thousands) + U"mila");
  if (!head.empty() || n || words.empty()) {
    words.push_back(head + (n || head.empty() ? below_thousand(n) : U""));
  }
  return accented(join(words));
}

// primo ... decimo, then the cardinal with -esimo: undicesimo, ventitreesimo, millesimo
std::u32string ordinal(uint64_t n, char32_t ending) {
  if (n <= 10) {
    std::u32string word(kOrdinals[n]);
    word.back() = ending;
    return word;
  }

  std::u32string words = cardinal(n);
  if (words.ends_with(U"tré")) {
    words.back() = U'e';
  }
  if (words.ends_with(U"tre") || words.ends_with(U"sei")) {
    return words + U"esim" + ending;
  }
  if (words.ends_with(U"mila")) {
    return words.substr(0, words.size() - 4) + U"millesim" + ending;
  }
  return words.substr(0, words.size() - 1) + U"esim" + ending;
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.emplace_back(kOnes[c - U'0']);
  }
  return join(words);
}

// --------------------
// ----- Patterns -----
// --------------------

// '80: a decade written as an apostrophe and its two digits.
class DecadePattern : public Pattern {
public:
  DecadePattern() : Pattern(Kind::DECADE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position) || text[position] != U'\'') {
      return std::nullopt;
    }

    // Two digits: a ten other than 00
    Number decade = digits_at(text, position + 1);
    if (decade.integer.size() != 2 || decade.integer[0] == U'0' || decade.integer[1] != U'0' ||
        !is_clear_after(text, decade.span.end)) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DECADE, .span = {position, decade.span.end}, .numbers = {decade}};
  }
};

// --------------------
// ----- Language -----
// --------------------

class Italian final : public PatternLanguage {
public:
  Italian() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSigns, kOrdinalSignFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<DecadePattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (number.fraction.empty()) {
      auto n = parse_value(number.integer);
      if (!n || is_spoken_digits(number, 7)) {
        return digits(number.integer);
      }
      // 1 maggio: "1" before a month is "primo"
      bool month = std::ranges::find(kMonths, strings::to_lower(match.context.after())) != kMonths.end();
      return *n == 1 && month ? U"primo" : cardinal(*n);
    }

    // The separator is read as written: due virgola cinque, due punto cinque
    std::vector<std::u32string> fraction;
    size_t zeros = number.fraction.find_first_not_of(U'0');
    fraction.insert(fraction.end(), std::min(zeros, number.fraction.size()), U"zero");
    if (zeros != std::u32string::npos) {
      fraction.push_back(cardinal(parse_value(number.fraction).value_or(0)));
    }
    std::u32string head = number.integer.empty() ? U"" : cardinal(value(number)) + U" ";
    return head + (number.decimal_mark == U',' ? U"virgola " : U"punto ") + join(fraction);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    uint64_t day = value(match.numbers[0]);
    return (day == 1 ? U"primo" : cardinal(day)) + U" " + kMonths[value(match.numbers[1]) - 1] + U" " +
           cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // Digit by digit: tre tre tre, uno due tre ...
    std::vector<std::u32string> groups;
    for (const auto& group : match.numbers) {
      groups.push_back(digits(group.integer));
    }
    return join(groups);
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    return named_fraction(kFractions, match);
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    // With a dot, a time comes after "alle" or from 13 on: alle 9.30, 21.30, but versione 2.79
    uint64_t hours = value(match.numbers[0]), minutes = value(match.numbers[1]);
    bool decimal =
        match.separator == U'.' && !kTimeWords.contains(match.context.before()) && (hours < 13 || hours > 23);
    if (!fits_clock(match) || decimal) {
      return std::nullopt;
    }

    // alle 21.30: ventuno e trenta
    std::u32string hour = hours == 1 ? U"una" : cardinal(hours);
    return minutes ? hour + U" e " + cardinal(minutes) : hour;
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    if (value(match.numbers[0]) == 0) {
      return std::nullopt;
    }
    char32_t ending = match.suffix == U"ª" ? U'a' : (match.suffix == U"º" ? U'o' : match.suffix.back());
    return ordinal(value(match.numbers[0]), ending);
  }

  std::optional<std::u32string> read_decade(const Match& match) const override {
    return cardinal(value(match.numbers[0])); // gli anni '80: gli anni ottanta
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));

    // 3 mln €: tre milioni di euro, but 10 mila €: diecimila euro
    if (!match.scale.empty()) {
      bool thousands = kScales.at(std::u32string(match.scale)).one == U"mila";
      return scaled(number, match) + (thousands ? U" " : U" di ") + currency.many;
    }

    // 10,50 €: dieci euro e cinquanta
    if (number.fraction.size() == 2 && number.decimal_mark == U',') {
      Number whole{.integer = number.integer.empty() ? U"0" : number.integer, .grouped = number.grouped};
      uint64_t cents = parse_value(number.fraction).value_or(0);
      return read_number(whole, match) + U" " + (number.integer == U"1" ? currency.one : currency.many) +
             (cents ? U" e " + cardinal(cents) : U"");
    }
    return read_number(number, match) + U" " + (is_one(number) ? currency.one : currency.many);
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K: a resolution
    if ((match.scale == U"k" || match.scale == U"K") && value(match.numbers.back()) < 10) {
      return std::nullopt;
    }
    return scaled(number, match);
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return read_number(number, match) + U" per cento" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    return read_number(number, match) + U" " + (is_one(number) ? unit.one : unit.many);
  }

private:
  // An amount with its scale: settantaquattromila, tre milioni
  std::u32string scaled(const Number& number, const Match& match) const {
    const Forms& scale = kScales.at(std::u32string(match.scale));

    // 74 mila: settantaquattromila
    if (scale.one == U"mila" && number.fraction.empty()) {
      return cardinal(value(number) * 1000);
    }
    bool singular = value(number) < 2 && number.fraction.empty();
    return read_number(number, match) + U" " + (singular ? scale.one : scale.many);
  }
};
} // namespace

const Language& language() {
  static const Italian language;
  return language;
}

} // namespace verbalis::languages::it
