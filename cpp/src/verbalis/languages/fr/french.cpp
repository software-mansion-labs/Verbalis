#include "constants.h"

#include <verbalis/languages/language.h>
#include <verbalis/numbers/scanning.h>
#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>
#include <verbalis/text/utf8.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::fr {

namespace {
using namespace text;
using namespace numbers;

// What the words around a match say about reading its numbers
struct Cues {
  bool feminine = false; // vingt et une heures: before a feminine noun
  bool plural_s = true;  // quatre-vingts, deux cents; but les années quatre-vingt
  bool equation = false; // 12 x 4 = 48: "x" is "fois" rather than "par"
};

// ------------------------
// ----- Number words -----
// ------------------------

std::u32string below_hundred(uint64_t n, bool feminine, bool plural_s) {
  if (n < 20) {
    return feminine && n == 1 ? U"une" : std::u32string(kOnes[n]);
  }

  // soixante-dix, quatre-vingt-dix
  uint64_t t = n / 10, u = n % 10;
  if (t == 7 || t == 9) {
    t--;
    u += 10;
  }
  if (u == 0) {
    return std::u32string(kTens[t]) + (t == 8 && plural_s ? U"s" : U"");
  }
  std::u32string unit = feminine && u == 1 ? U"une" : std::u32string(kOnes[u]);
  return std::u32string(kTens[t]) + ((u == 1 || u == 11) && t < 8 ? U" et " : U"-") + unit;
}

std::u32string below_thousand(uint64_t n, bool feminine, bool plural_s) {
  uint64_t h = n / 100, r = n % 100;
  if (h == 0) {
    return below_hundred(r, feminine, plural_s);
  }
  std::u32string head = h == 1 ? U"cent" : std::u32string(kOnes[h]) + U" cent" + (r == 0 && plural_s ? U"s" : U"");
  return head + (r ? U" " + below_hundred(r, feminine, plural_s) : U"");
}

// A cardinal; `plural_s` is false when "mille" follows (deux cent mille, quatre-vingt mille).
std::u32string cardinal(uint64_t n, bool feminine = false, bool plural_s = true) {
  if (n < 1000) {
    return below_thousand(n, feminine, plural_s);
  }
  if (n < 1000000) {
    uint64_t t = n / 1000, r = n % 1000;
    std::u32string head = t == 1 ? U"mille" : below_thousand(t, false, false) + U" mille";
    return head + (r ? U" " + below_thousand(r, feminine, plural_s) : U"");
  }
  for (auto [scale, word] : {std::pair{1000000000ULL, U"milliard"}, std::pair{1000000ULL, U"million"}}) {
    if (n >= scale) {
      uint64_t q = n / scale, r = n % scale;
      std::u32string head = cardinal(q) + U" " + std::u32string(word) + (q > 1 ? U"s" : U"");
      return head + (r ? U" " + cardinal(r, feminine, plural_s) : U"");
    }
  }
  return U"";
}

std::u32string ordinal(uint64_t n, bool feminine = false) {
  if (n == 1) {
    return feminine ? U"première" : U"premier";
  }

  // vingtième, cinquième, neuvième, onzième
  std::u32string words = cardinal(n);
  if (words.ends_with(U"vingts") || words.ends_with(U"cents")) {
    words.pop_back();
  }
  if (words.ends_with(U"neuf")) {
    words.back() = U'v';
  } else if (words.ends_with(U"cinq")) {
    words += U"u";
  }
  if (words.ends_with(U'e')) {
    words.pop_back();
  }
  return words + U"ième";
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.emplace_back(kOnes[c - U'0']);
  }
  return join(words);
}

// A number of a given gender, as before a currency or a unit
std::u32string gendered(const Number& number, const Cues& cues, bool feminine) {
  if (number.fraction.empty()) {
    auto n = parse_value(number.integer);
    return !n || is_spoken_digits(number, 5) ? digits(number.integer) : cardinal(*n, feminine, cues.plural_s);
  }

  // Up to three decimals read as a number: zéro virgule zéro cinq, deux virgule soixante-quinze
  std::u32string fraction;
  if (number.fraction.size() > 3) {
    fraction = digits(number.fraction);
  } else {
    std::vector<std::u32string> words;
    size_t zeros = number.fraction.find_first_not_of(U'0');
    words.insert(words.end(), std::min(zeros, number.fraction.size()), U"zéro");
    if (zeros != std::u32string::npos) {
      words.push_back(cardinal(parse_value(number.fraction).value_or(0)));
    }
    fraction = join(words);
  }
  std::u32string head = number.integer.empty() ? U"" : cardinal(value(number), feminine) + U" ";
  return head + U"virgule " + fraction;
}

// -------------------
// ----- Context -----
// -------------------

bool feminine(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun);
  word = word.substr(word.find_last_of(U'-') + 1);
  if (!kFeminineNouns.contains(word) && word.ends_with(U's')) {
    word.pop_back();
  }
  if (kFeminineNouns.contains(word)) {
    return true;
  }
  bool masculine_ending = std::ranges::any_of(kMasculineEndings, [&](const auto& e) { return word.ends_with(e); });
  bool date = std::ranges::find(kMonths, word) != kMonths.end() || kWeekdays.contains(word);
  return word.ends_with(U'e') && !kMasculineInE.contains(word) && !date && !masculine_ending;
}

// A lowercase noun follows: 10k abonnés
bool counted(const Context& context) {
  std::u32string word = context.after();
  bool lowercase_word = word.size() > 2 && std::ranges::all_of(word, [](char32_t c) { return unicode::islower(c); });
  return lowercase_word && !kStopWords.contains(word) && !kNumberWords.contains(word);
}

// A noun after a number is singular below 2: un virgule cinq kilomètre, deux kilomètres
const std::u32string& number_form(const Number& number, const Forms& forms) {
  return value(number) < 2 ? forms.one : forms.many;
}

Cues cues_of(const Match& match) {
  std::u32string previous = match.context.before();
  return {.feminine = feminine(match.context.after()),
          .plural_s = previous != U"années" && previous != U"année",
          .equation = match.kind == Kind::DIMENSION && match.context.tail().find(U'=') != std::u32string_view::npos};
}

// An amount with its scale: trois millions, un virgule cinq million, dix mille
std::u32string scaled(const Number& number, std::u32string_view scale, const Cues& cues) {
  const std::u32string& word = kScales.at(std::u32string(scale));
  if (word == U"mille") {
    return gendered(number, cues, false) + U" mille";
  }
  std::u32string one = word.ends_with(U's') ? word.substr(0, word.size() - 1) : word;
  return gendered(number, cues, false) + U" " + number_form(number, {one, one + U"s"});
}

// --------------------
// ----- Patterns -----
// --------------------

int roman_digit(char32_t c) {
  switch (c) {
    case U'I':
      return 1;
    case U'V':
      return 5;
    case U'X':
      return 10;
    case U'L':
      return 50;
    case U'C':
      return 100;
    default:
      return 0;
  }
}

// The value of a Roman numeral written in its canonical form (XIX, not XVIIII).
std::optional<int> roman_value(std::u32string_view numeral) {
  int value = 0;
  for (size_t i = 0; i < numeral.size(); ++i) {
    int digit = roman_digit(numeral[i]);
    value += i + 1 < numeral.size() && digit < roman_digit(numeral[i + 1]) ? -digit : digit;
  }

  static constexpr std::array<std::pair<int, std::u32string_view>, 9> kNumerals = {
      {{100, U"C"}, {90, U"XC"}, {50, U"L"}, {40, U"XL"}, {10, U"X"}, {9, U"IX"}, {5, U"V"}, {4, U"IV"}, {1, U"I"}}};
  std::u32string canonical;
  for (int rest = value; const auto& [numeral_value, symbol] : kNumerals) {
    for (; rest >= numeral_value; rest -= numeral_value) {
      canonical += symbol;
    }
  }
  return canonical == numeral ? std::optional(value) : std::nullopt;
}

// XIXe, Ier, IIIe: an ordinal written in Roman numerals.
class RomanOrdinalPattern : public Pattern {
public:
  RomanOrdinalPattern() : Pattern(Kind::ORDINAL) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position)) {
      return std::nullopt;
    }

    // Two Roman digits or more, or a single I (Ier)
    size_t end = position;
    while (end < text.size() && roman_digit(text[end])) {
      end++;
    }
    std::u32string_view numeral = text.substr(position, end - position);
    if (numeral.size() < 2 && numeral != U"I") {
      return std::nullopt;
    }
    auto value = roman_value(numeral);
    if (!value) {
      return std::nullopt;
    }

    for (auto suffix : keys_at(text, end, {U"er", U"re", U"ère", U"e", U"ème"})) {
      if (is_clear_after(text, end + suffix.size())) {
        std::u32string digits = utf8_to_u32(std::to_string(*value));
        return Match{.kind = Kind::ORDINAL,
                     .span = {position, end + suffix.size()},
                     .numbers = {{.span = {position, end}, .integer = digits}},
                     .suffix = suffix};
      }
    }
    return std::nullopt;
  }
};

// --------------------
// ----- Language -----
// --------------------

class French final : public PatternLanguage {
public:
  French() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<PhonePattern>(kPhoneFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<RomanOrdinalPattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    Cues cues = cues_of(match);
    return gendered(number, cues, number.fraction.empty() && cues.feminine);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 4);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // Without a year only a zero-padded day or month is a date: 05/11, but not 3/4
    if (match.numbers.size() != 3 && match.numbers[0].integer[0] != U'0' && match.numbers[1].integer[0] != U'0') {
      return std::nullopt;
    }
    uint64_t day = value(match.numbers[0]);
    std::u32string words = (day == 1 ? ordinal(1) : cardinal(day)) + U" " + kMonths[value(match.numbers[1]) - 1];
    return match.numbers.size() < 3 ? words : words + U" " + cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // In pairs: zéro six douze trente-quatre ...
    std::vector<std::u32string> words;
    for (const auto& pair : match.numbers) {
      bool zero = pair.integer.size() == 2 && pair.integer[0] == U'0';
      if (pair.integer.size() > 3) {
        words.push_back(digits(pair.integer)); // A longer group of a foreign number
      } else {
        words.push_back(zero ? U"zéro " + std::u32string(kOnes[pair.integer[1] - U'0']) : cardinal(value(pair)));
      }
    }
    return join(words);
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    return named_fraction(kFractions, match)
        .value_or(cardinal(value(match.numbers[0])) + U" sur " + cardinal(value(match.numbers[1])));
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    if (match.numbers.size() == 2) {
      bool decimal = match.separator == U'.' && !kTimeWords.contains(match.context.before());
      if (!fits_clock(match) || decimal) {
        return std::nullopt;
      }
    }
    uint64_t hours = value(match.numbers[0]);
    std::u32string words = cardinal(hours, true) + (hours < 2 ? U" heure" : U" heures");
    if (match.numbers.size() == 2 && value(match.numbers[1]) > 0) {
      words += U" " + cardinal(value(match.numbers[1]), true);
    }
    return words;
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    std::u32string_view suffix = match.suffix;
    std::u32string words;
    if (suffix.starts_with(U"nd")) {
      words = suffix.starts_with(U"nde") ? U"seconde" : U"second";
    } else {
      bool feminine = suffix == U"re" || suffix == U"ère" || suffix == U"res" || suffix == U"ères";
      words = ordinal(value(match.numbers[0]), feminine);
    }
    return words + (suffix.ends_with(U's') ? U"s" : U"");
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    Forms currency = kCurrencies.at(std::u32string(match.unit));
    std::u32string scale(match.scale);
    if (match.unit == U"M€") {
      scale = U"millions";
      currency = {U"euro", U"euros"};
    }
    Cues cues = cues_of(match);

    if (!scale.empty()) {
      // trois millions d'euros, but dix mille euros
      std::u32string words = scaled(number, scale, cues);
      if (kScales.at(scale) == U"mille") {
        return words + U" " + currency.many;
      }
      bool vowel = strings::contains(std::u32string_view(U"aeiouyéh"), currency.many[0]);
      return words + (vowel ? U" d'" : U" de ") + currency.many;
    }

    // A price: trois euros cinquante, quatre-vingt-dix-neuf centimes
    bool feminine = currency.one == U"livre";
    if (number.fraction.size() == 2) {
      uint64_t cents = parse_value(number.fraction).value_or(0);
      if (value(number) == 0) {
        return cardinal(cents) + U" centimes";
      }
      Number whole{.integer = number.integer, .grouped = number.grouped};
      return gendered(whole, cues, feminine) + U" " + (number.integer == U"1" ? currency.one : currency.many) + U" " +
             cardinal(cents);
    }
    return gendered(number, cues, feminine) + U" " + number_form(number, currency);
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K, 3k de Q: a letter, read as a code
    if (kScales.at(std::u32string(match.scale)) == U"mille" && !counted(match.context)) {
      return std::nullopt;
    }
    return scaled(number, match.scale, cues_of(match));
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return gendered(number, cues_of(match), false) + U" pour cent" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    std::u32string_view first_word = std::u32string_view(unit.one).substr(0, unit.one.find(U' '));
    return gendered(number, cues_of(match), feminine(first_word)) + U" " + number_form(number, unit);
  }

  std::optional<std::u32string> read_joiner(const Match& match) const override {
    if (match.kind == Kind::DIMENSION) {
      return cues_of(match).equation ? U" fois " : U" par ";
    }
    return PatternLanguage::read_joiner(match);
  }
};
} // namespace

const Language& language() {
  static const French language;
  return language;
}

} // namespace verbalis::languages::fr
