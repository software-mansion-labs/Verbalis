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

namespace verbalis::languages::pt {

namespace {
using namespace text;
using namespace numbers;

// What the words around a match say about reading its numbers
struct Cues {
  Gender gender = Gender::MASCULINE; // Of the noun after: duas pessoas; an hour is feminine (às duas)
  bool day = false;                  // 1 de maio: "1" before "de" and a month is "primeiro"
  bool heading = false;              // artigo 80º: an ordinal after a legal heading reads as a cardinal
  bool lettered = false;             // A320-200, 3-4x: a range touching letters is a plain hyphen
  bool named = false;                // R$ 50 reais: the currency is already named after the amount
};

bool is_ascii_letter(char32_t c) {
  return unicode::is_ascii(c) && unicode::isalpha(c);
}

// ------------------------
// ----- Number words -----
// ------------------------

std::u32string unit(uint64_t n, Gender gender) {
  if (gender == Gender::FEMININE && (n == 1 || n == 2)) {
    return n == 1 ? U"uma" : U"duas";
  }
  return std::u32string(kOnes[n]);
}

std::u32string below_hundred(uint64_t n, Gender gender) {
  if (n < 20) {
    return unit(n, gender);
  }
  return std::u32string(kTens[n / 10]) + (n % 10 ? U" e " + unit(n % 10, gender) : U"");
}

std::u32string below_thousand(uint64_t n, Gender gender) {
  uint64_t h = n / 100, r = n % 100;
  if (h == 0) {
    return below_hundred(r, gender);
  }
  if (n == 100) {
    return U"cem";
  }

  // duzentas pessoas
  std::u32string head(kHundreds[h]);
  if (gender == Gender::FEMININE && h > 1) {
    head.replace(head.size() - 2, 2, U"as");
  }
  return head + (r ? U" e " + below_hundred(r, gender) : U"");
}

// "e" joins the last group below 100 or of round hundreds: dois mil e vinte, mil quinhentos e vinte, mil duzentos e um
std::u32string cardinal(uint64_t n, Gender gender = Gender::MASCULINE) {
  std::vector<std::pair<uint64_t, std::u32string>> groups;
  for (auto [scale, one, many] :
       {std::tuple{1000000000000ULL, U"trilhão", U"trilhões"}, std::tuple{1000000000ULL, U"bilhão", U"bilhões"},
        std::tuple{1000000ULL, U"milhão", U"milhões"}}) {
    if (uint64_t q = n / scale) {
      groups.emplace_back(q, cardinal(q) + U" " + (q == 1 ? one : many));
    }
    n %= scale;
  }

  // Thousands agree with the noun: duas mil pessoas
  if (uint64_t q = n / 1000) {
    groups.emplace_back(q, q == 1 ? U"mil" : below_thousand(q, gender) + U" mil");
  }
  n %= 1000;
  if (n || groups.empty()) {
    groups.emplace_back(n, below_thousand(n, gender));
  }

  std::u32string words = groups[0].second;
  for (size_t i = 1; i < groups.size(); ++i) {
    uint64_t last = groups[i].first;
    bool joined = i + 1 == groups.size() && (last < 100 || last % 100 == 0);
    words += (joined ? U" e " : U" ") + groups[i].second;
  }
  return words;
}

// décimo terceiro, vigésima primeira, milésimo
std::u32string ordinal(uint64_t n, bool feminine) {
  char32_t ending = feminine ? U'a' : U'o';
  std::vector<std::u32string> words;
  if (uint64_t thousands = n / 1000) {
    words.push_back((thousands == 1 ? U"" : cardinal(thousands) + U" ") + U"milésim" + ending);
  }
  for (auto [scale, stems] :
       {std::pair{100, &kOrdinalHundreds}, std::pair{10, &kOrdinalTens}, std::pair{1, &kOrdinalOnes}}) {
    if (uint64_t digit = n / scale % 10) {
      words.push_back(std::u32string((*stems)[digit]) + ending);
    }
  }
  return join(words);
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.push_back(unit(c - U'0', Gender::MASCULINE));
  }
  return join(words);
}

// um milhão de reais, but dez mil reais
std::u32string with_de(const std::u32string& words) {
  bool scale = std::ranges::any_of(kScaleWords, [&](const auto& w) { return words.ends_with(w); });
  return scale ? words + U" de" : words;
}

// A number of a given gender, as before a currency or a unit
std::u32string number_with(const Number& number, Gender gender) {
  if (number.fraction.empty()) {
    auto n = parse_value(number.integer);
    return !n || is_spoken_digits(number, 7) ? digits(number.integer) : cardinal(*n, gender);
  }

  // The separator is read as written: dois vírgula cinco, dois ponto cinco
  std::u32string fraction;
  if (number.fraction.size() > 2) {
    fraction = digits(number.fraction);
  } else {
    std::vector<std::u32string> words;
    size_t zeros = number.fraction.find_first_not_of(U'0');
    words.insert(words.end(), std::min(zeros, number.fraction.size()), U"zero");
    if (zeros != std::u32string::npos) {
      words.push_back(cardinal(parse_value(number.fraction).value_or(0)));
    }
    fraction = join(words);
  }
  std::u32string head = number.integer.empty() ? U"" : cardinal(value(number), gender) + U" ";
  return head + (number.decimal_mark == U',' ? U"vírgula " : U"ponto ") + fraction;
}

// An amount with its scale: vinte e duas mil pessoas, três milhões
std::u32string scaled(const Number& number, std::u32string_view scale, Gender gender) {
  const Forms& forms = kScales.at(std::u32string(scale));
  if (forms.one == U"mil") {
    return number_with(number, gender) + U" mil";
  }
  return number_with(number, Gender::MASCULINE) + U" " + (value(number) < 2 ? forms.one : forms.many);
}

// -------------------
// ----- Context -----
// -------------------

bool feminine(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun);
  std::u32string singular = word;
  if (word.ends_with(U"ões") || word.ends_with(U"ães")) {
    singular = word.substr(0, word.size() - 3) + U"ão";
  } else if (word.ends_with(U"res") || word.ends_with(U"zes") || word.ends_with(U"les")) {
    singular.resize(word.size() - 2);
  } else if (word.ends_with(U's')) {
    singular.pop_back();
  }

  if (kMasculineInA.contains(singular) || kMasculineInA.contains(word)) {
    return false;
  }
  bool feminine_ending = std::ranges::any_of(kFeminineEndings, [&](const auto& e) { return singular.ends_with(e); });
  return kFeminineNouns.contains(singular) || kFeminineNouns.contains(word) || feminine_ending;
}

// The gender of the noun after a number; an hour is feminine: às duas, duas da tarde
Gender gender_of(const Context& context) {
  std::u32string word = context.after();
  if ((word == U"da" || word == U"de") && kDayParts.contains(strings::to_lower(context.after(2)))) {
    return Gender::FEMININE;
  }
  if (word.empty() || !unicode::isalpha(word[0]) || kStopWords.contains(strings::to_lower(word))) {
    return kHourWords.contains(context.before()) ? Gender::FEMININE : Gender::MASCULINE;
  }
  return feminine(word) ? Gender::FEMININE : Gender::MASCULINE;
}

// A lowercase noun follows: 500k seguidores
bool counted(const Context& context) {
  std::u32string word = context.after();
  bool lowercase_word = word.size() > 2 && std::ranges::all_of(word, [](char32_t c) { return unicode::islower(c); });
  return lowercase_word && !kStopWords.contains(word);
}

// A range touching letters: A320-200, 3-4x
bool lettered(const Match& range, const Context& context) {
  std::u32string_view head = context.head(), tail = context.tail();
  size_t before = range.numbers[0].span.begin, after = range.numbers[1].span.end - range.span.end;
  return (before > 0 && is_ascii_letter(head[before - 1])) || (after < tail.size() && is_ascii_letter(tail[after]));
}

Cues cues_of(const Match& match) {
  const Context& context = match.context;
  Cues cues{.gender = gender_of(context),
            .day = strings::to_lower(context.after()) == U"de" &&
                   std::ranges::find(kMonths, strings::to_lower(context.after(2))) != kMonths.end(),
            .heading = kHeadings.contains(context.before())};
  if (match.kind == Kind::RANGE) {
    cues.lettered = lettered(match, context);
  }

  // R$ 50 reais
  if (match.kind == Kind::MONEY) {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    std::u32string next = strings::to_lower(context.after());
    cues.named = next == currency.one || next == currency.many;
  }
  return cues;
}

// --------------------
// ----- Language -----
// --------------------

class Portuguese final : public PatternLanguage {
public:
  Portuguese() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSigns, kOrdinalSignFormat));
    add_pattern(std::make_unique<RomanPattern>(false, kRomanMarkers));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    Cues cues = cues_of(match);
    if (number.fraction.empty() && cues.day && number.integer == U"1") {
      return U"primeiro";
    }
    return number_with(number, number.fraction.empty() ? cues.gender : Gender::MASCULINE);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    uint64_t day = value(match.numbers[0]);
    return (day == 1 ? U"primeiro" : cardinal(day)) + U" de " + kMonths[value(match.numbers[1]) - 1] + U" de " +
           cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // 2021-2022: a range of years; 213 456 789 is a count, unless a word says it is a phone number
    if (match.kind == Kind::PHONE) {
      bool years = std::ranges::all_of(match.numbers, [](const Number& group) {
        return group.integer.size() == 4 && value(group) >= 1900 && value(group) <= 2100;
      });
      if (years || (match.separator == U' ' && !kPhoneWords.contains(match.context.before()))) {
        return std::nullopt;
      }
    }

    std::vector<std::u32string> groups;
    for (const auto& group : match.numbers) {
      groups.push_back(group.integer == U"0800" ? U"zero oitocentos" : digits(group.integer));
    }
    return join(groups, U", ");
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    return named_fraction(kFractions, match);
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    if (!fits_clock(match)) {
      return std::nullopt;
    }

    // 14h30: catorze e trinta; 21h: vinte e uma horas
    uint64_t hours = value(match.numbers[0]);
    uint64_t minutes = match.numbers.size() > 1 ? value(match.numbers[1]) : 0;
    std::u32string hour = cardinal(hours, Gender::FEMININE);
    return minutes ? hour + U" e " + cardinal(minutes) : hour + (hours == 1 ? U" hora" : U" horas");
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    auto role = roman_role(match, kRomanCues);
    if (!role) {
      return std::nullopt;
    }
    std::u32string previous = match.context.before();
    switch (*role) {
      case RomanRole::NAME: { // Dom Pedro segundo, Dona Maria segunda, but Bento dezesseis
        bool feminine = kFeminineNames.contains(previous);
        return n <= 10 ? ordinal(n, feminine) : cardinal(n, feminine ? Gender::FEMININE : Gender::MASCULINE);
      }
      case RomanRole::AFTER: // a Segunda Guerra, o terceiro congresso
        return ordinal(n, kFeminineRomanNouns.contains(strings::to_lower(match.context.after())));
      default: // século quinto, Vaticano segundo, but século vinte e um; capítulo quatro
        return (previous.starts_with(U"século") || previous == U"vaticano") && n <= 10 ? ordinal(n, false) : cardinal(n);
    }
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    if (n == 0) {
      return std::nullopt;
    }
    return n >= 10 && cues_of(match).heading ? cardinal(n) : ordinal(n, match.suffix == U"ª");
  }

  std::optional<std::u32string> read_compound(const Match& match) const override {
    return number_with(match.numbers[0], Gender::MASCULINE) + U"-" + std::u32string(match.suffix);
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    Gender gender = currency.one == U"libra" ? Gender::FEMININE : Gender::MASCULINE;

    // R$ 37,517: prices have two decimals, so three digits are a thousands group
    Number amount = number;
    if (number.decimal_mark == U',' && !number.integer.empty() && number.integer.size() <= 3 &&
        number.fraction.size() == 3) {
      amount = {.integer = number.integer + number.fraction};
    }

    if (cues_of(match).named) {
      return match.scale.empty() ? number_with(amount, gender) : scaled(amount, match.scale, gender);
    }
    if (!match.scale.empty()) {
      return with_de(scaled(amount, match.scale, gender)) + U" " + currency.many;
    }

    // A price: vinte e nove reais e noventa centavos
    if (amount.fraction.size() == 2 && amount.decimal_mark == U',') {
      uint64_t cents = parse_value(amount.fraction).value_or(0);
      std::u32string centavos = cardinal(cents) + (cents == 1 ? U" centavo" : U" centavos");
      if (value(amount) == 0) {
        return centavos;
      }
      Number whole{.integer = amount.integer, .grouped = amount.grouped};
      std::u32string words =
          with_de(number_with(whole, gender)) + U" " + (amount.integer == U"1" ? currency.one : currency.many);
      return cents ? words + U" e " + centavos : words;
    }
    return with_de(number_with(amount, gender)) + U" " + (is_one(amount) ? currency.one : currency.many);
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K, 2K: a resolution; but 500k is quinhentos mil
    if ((match.scale == U"k" || match.scale == U"K") && value(match.numbers.back()) < 10 && !counted(match.context)) {
      return std::nullopt;
    }
    return scaled(number, match.scale, cues_of(match).gender);
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return number_with(number, Gender::MASCULINE) + U" por cento" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    std::u32string_view first_word = std::u32string_view(unit.one).substr(0, unit.one.find(U' '));
    Gender gender = feminine(first_word) ? Gender::FEMININE : Gender::MASCULINE;
    return number_with(number, gender) + U" " + (value(number) < 2 ? unit.one : unit.many);
  }

  std::u32string read_range(const Number& from, const Number& to, const Match& match) const override {
    // 2016-2022 and 10-15 read "a"; scores and formations (4-3-3, 60-40) keep "-"
    bool plain = from.fraction.empty() && to.fraction.empty() && !from.grouped && !to.grouped;
    bool rising = from.integer.size() == to.integer.size() && value(to) > value(from);
    return plain && rising && !cues_of(match).lettered ? joiners().range : U"-";
  }
};
} // namespace

const Language& language() {
  static const Portuguese language;
  return language;
}

} // namespace verbalis::languages::pt
