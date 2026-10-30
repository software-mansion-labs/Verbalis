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

  // vingtième, cinquième, neuvième, onzième; millionième, deux millionièmes: without "un" and the plural s
  std::u32string words = cardinal(n);
  if (words == U"un million" || words == U"un milliard") {
    words.erase(0, 3);
  }
  for (std::u32string_view plural : {U"vingts", U"cents", U"millions", U"milliards"}) {
    if (words.ends_with(plural)) {
      words.pop_back();
    }
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
    return !n || is_spoken_digits(number, 8) ? digits(number.integer) : cardinal(*n, feminine, cues.plural_s);
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

// A capitalized word after a number starts a title or a name, not a noun it counts: Rennes 1 Situation
bool feminine(std::u32string_view noun) {
  if (!noun.empty() && unicode::isupper(noun[0])) {
    return false;
  }
  std::u32string word = strings::to_lower(noun);
  word = word.substr(word.find_last_of(U'-') + 1);
  if (!kFeminineNouns.contains(word) && word.ends_with(U's')) {
    word.pop_back();
  }
  // eurodéputé is masculine as député is, but propriété is feminine although été is masculine
  auto ends_with = [&](const auto& ending) { return word.ends_with(ending); };
  auto masculine = [&](const auto& noun) { return word == noun || (noun.size() > 4 && word.ends_with(noun)); };
  bool feminine_ending = std::ranges::any_of(kFeminineEndings, ends_with);
  if (kFeminineNouns.contains(word) || (feminine_ending && !std::ranges::any_of(kMasculineInE, masculine))) {
    return true;
  }
  bool masculine_ending = std::ranges::any_of(kMasculineEndings, ends_with);
  bool date = std::ranges::find(kMonths, word) != kMonths.end() || kWeekdays.contains(word);
  return word.ends_with(U'e') && !kMasculineInE.contains(word) && !date && !masculine_ending;
}

// A lowercase noun follows: 10k abonnés
bool counted(const Context& context) {
  std::u32string word = context.after();
  bool lowercase_word = word.size() > 2 && std::ranges::all_of(word, [](char32_t c) { return unicode::islower(c); });
  return lowercase_word && !kStopWords.contains(word) && !kNumberWords.contains(word);
}

// An ungrouped number of five digits or more is an amount (166870 euros, 12500 habitants, 415297) unless something
// marks it as a code: a hyphen or a slash (0-89100-396-7), a word naming a code (numéro 1035836), or a name next to
// it (75005 Paris, Naca 23012) without a noun it counts after it (Environ 12500 personnes)
bool is_code(const Number& number, const Match& match) {
  if (number.grouped || !number.fraction.empty() || number.integer.size() < 5) {
    return false;
  }
  std::u32string_view head = match.context.head(), tail = match.context.tail();
  bool joined = (!head.empty() && strings::contains(std::u32string_view(U"-/"), head.back())) ||
                (!tail.empty() && strings::contains(std::u32string_view(U"-/"), tail.front()));
  auto words = words_of(head);
  std::u32string_view previous = words.empty() ? U"" : stripped(words.back());
  std::u32string after = match.context.after();
  bool name = (!previous.empty() && unicode::isupper(previous[0])) || (!after.empty() && unicode::isupper(after[0]));
  return joined || kCodeWords.contains(match.context.before()) || (name && !counted(match.context));
}

// A noun after a number is singular below 2: un virgule cinq kilomètre, deux kilomètres
const std::u32string& number_form(const Number& number, const Forms& forms) {
  return value(number) < 2 ? forms.one : forms.many;
}

// En 1900, vers 1980: a year, whose quatre-vingt and cent take no plural s
bool is_year(const Match& match) {
  if (match.kind != Kind::NUMBER || match.numbers.size() != 1) {
    return false;
  }
  const Number& number = match.numbers[0];
  uint64_t n = value(number);
  std::u32string previous = match.context.before();
  bool dimension = previous == U"par" || previous == U"fois"; // 1920x1080, read before
  return !number.grouped && number.fraction.empty() && n >= 1000 && n < 2100 && !counted(match.context) && !dimension;
}

Cues cues_of(const Match& match) {
  std::u32string previous = match.context.before();
  return {.feminine = counted(match.context) && feminine(match.context.after()),
          .plural_s = !kOrdinalWords.contains(previous) && !is_year(match),
          .equation = match.kind == Kind::DIMENSION && match.context.tail().find(U'=') != std::u32string_view::npos};
}

// A noun after "de": de dollars, d'euros
std::u32string of(const std::u32string& noun) {
  bool vowel = strings::contains(std::u32string_view(U"aeiouyéh"), noun[0]);
  return (vowel ? U"d'" : U"de ") + noun;
}

// Whether a whole amount ends in millions or billions, which take "de" before a noun: un million de dollars
bool round_millions(const Number& number) {
  uint64_t n = value(number);
  return number.fraction.empty() && n >= 1000000 && n % 1000000 == 0;
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

// 3 1/2 km: a whole number and a half before a masculine unit, read "trois kilomètres et demi" (the whole number by
// read_number, and "et demi" by the joiners)
class HalfUnitPattern : public Pattern {
public:
  HalfUnitPattern() : Pattern(Kind::FRACTION) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t whole_end = digits_end(text, position);
    if (!is_clear_before(text, position, U"./-:,") || whole_end == position || whole_end > position + 3 ||
        text.substr(whole_end, 5) != U" 1/2 ") {
      return std::nullopt;
    }
    size_t unit = whole_end + 5;
    for (auto key : keys_at(text, unit, kNotation.units)) {
      const std::u32string& one = kUnits.at(std::u32string(key)).one;
      if (is_clear_after(text, unit + key.size()) && !feminine(one.substr(0, one.find(U' ')))) {
        return Match{
            .kind = Kind::FRACTION,
            .span = {position, unit + key.size()},
            .numbers = {digits_at(text, position), digits_at(text, whole_end + 1), digits_at(text, whole_end + 3)},
            .unit = key};
      }
    }
    return std::nullopt;
  }
};

// 3 € 50, 12 € 99: euros and cents
class EuroCentsPattern : public Pattern {
public:
  EuroCentsPattern() : Pattern(Kind::MONEY) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t euro = skip_space(text, digits_end(text, position));
    size_t cents = skip_space(text, euro + 1);
    if (!is_clear_before(text, position, U".,-") || euro == position || euro >= text.size() || text[euro] != U'€' ||
        digits_end(text, cents) != cents + 2 || !is_clear_after(text, cents + 2) ||
        is_digit_after(text, cents + 2, U".,")) {
      return std::nullopt;
    }
    Number amount = digits_at(text, position);
    amount.span.end = cents + 2;
    amount.fraction = std::u32string(text.substr(cents, 2));
    return Match{.kind = Kind::MONEY, .span = {position, cents + 2}, .numbers = {amount}, .unit = U"€"};
  }
};

// 1m65: a height in meters and centimeters
class MetersPattern : public Pattern {
public:
  MetersPattern() : Pattern(Kind::UNIT) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t m = digits_end(text, position);
    if (!is_clear_before(text, position, U".,-") || m == position || m > position + 1 || m >= text.size() ||
        text[m] != U'm' || digits_end(text, m + 1) != m + 3 || !is_clear_after(text, m + 3)) {
      return std::nullopt;
    }
    Number length = digits_at(text, position);
    length.span.end = m + 3;
    length.fraction = std::u32string(text.substr(m + 1, 2));
    return Match{.kind = Kind::UNIT, .span = {position, m + 3}, .numbers = {length}, .unit = U"m"};
  }
};

// les 17/18 juin, les 17-18-19 juin: days of a month, listed
class DayListPattern : public Pattern {
public:
  DayListPattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,/-")) {
      return std::nullopt;
    }
    Match days{.kind = Kind::DATE, .span = {position, position}};
    for (size_t p = position;; p++) {
      size_t end = digits_end(text, p);
      uint64_t day = parse_value(text.substr(p, end - p)).value_or(0);
      if (end > p + 2 || day < 1 || day > 31) {
        return std::nullopt;
      }
      days.numbers.push_back(digits_at(text, p));
      days.span.end = end;
      bool next = end < text.size() && (text[end] == U'/' || text[end] == U'-');
      if (!next || (days.separator && text[end] != days.separator)) {
        break;
      }
      days.separator = text[end];
      p = end;
    }

    // Two days or more with a slash, or three with a hyphen (17-18 juin is a range), then a month
    size_t least = days.separator == U'/' ? 2 : 3;
    size_t month = days.span.end + 1;
    size_t month_end = month;
    while (month_end < text.size() && unicode::isalpha(text[month_end])) {
      month_end++;
    }
    bool before_month = days.span.end < text.size() && text[days.span.end] == U' ' &&
                        std::ranges::find(kMonths, text.substr(month, month_end - month)) != kMonths.end();
    if (days.numbers.size() < least || !before_month) {
      return std::nullopt;
    }
    return days;
  }
};

// --------------------
// ----- Language -----
// --------------------

class French final : public PatternLanguage {
public:
  French() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DayListPattern>());
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<DatePattern>(kDashDateFormat));
    add_pattern(std::make_unique<PhonePattern>(kPhoneFormat));
    add_pattern(std::make_unique<EuroCentsPattern>());
    add_pattern(std::make_unique<MetersPattern>());
    add_pattern(std::make_unique<HalfUnitPattern>());
    add_pattern(std::make_unique<PrimePattern>(kNotation, kPrimeCues));
    add_pattern(std::make_unique<GroupedOrdinalPattern>(kOrdinalSuffixes, U" \u00a0\u202f"));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<RomanOrdinalPattern>(kRomanOrdinalSuffixes));
    add_pattern(std::make_unique<RomanPattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (is_code(number, match)) {
      return digits(number.integer);
    }
    // 3 1/2 km: trois kilomètres (et demi)
    if (match.kind == Kind::FRACTION && !match.unit.empty()) {
      return cardinal(value(number)) + U" " + number_form(number, kUnits.at(std::u32string(match.unit)));
    }
    Cues cues = cues_of(match);
    return gendered(number, cues, number.fraction.empty() && cues.feminine);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 4);
    return n ? cardinal(*n, false, false) : digits(text); // A380: a trois cent quatre-vingt
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // les dix-sept, dix-huit juin
    if (match.numbers.size() > 1 && !match.numbers[0].integer.starts_with(U'0') &&
        std::ranges::find(kMonths, match.context.after()) != kMonths.end()) {
      std::vector<std::u32string> days;
      for (const auto& day : match.numbers) {
        days.push_back(day.integer == U"1" ? ordinal(1) : cardinal(value(day)));
      }
      return join(days, U", ");
    }
    // 3.8.10 after "version", or with a one-digit day and month and a short year, is a version
    const auto& numbers = match.numbers;
    bool short_version = match.separator == U'.' && numbers.size() == 3 && numbers[2].integer.size() == 2 &&
                         numbers[0].integer.size() == 1 && numbers[1].integer.size() == 1;
    if (short_version || kVersionWords.contains(match.context.before())) {
      return std::nullopt;
    }
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
    if (auto words = named_fraction(kFractions, match)) {
      return words;
    }
    // un cinquième, deux tiers, trois quarts; else trois sur sept
    auto [a, b] = fraction_of(match);
    if (b < 2 || a >= b || (!is_marked_fraction(match, kFractionWords) && !is_plain_fraction(match))) {
      return cardinal(a) + U" sur " + cardinal(b);
    }
    std::u32string denominator = b == 2 ? U"demi" : b == 3 ? U"tiers" : b == 4 ? U"quart" : ordinal(b);
    return cardinal(a) + U" " + denominator + (a > 1 && b != 3 ? U"s" : U"");
  }

  // A bare unit fraction of a small denominator: 1/5, 1/10; but 2/9 may be a score (5/9 au tir), 1/6 tirs a count, and
  // (1/6) a part of a series
  static bool is_plain_fraction(const Match& match) {
    auto [a, b] = fraction_of(match);
    std::u32string_view head = match.context.head();
    return a == 1 && b <= 10 && !counted(match.context) && !head.ends_with(U'(');
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    if (match.separator == U'′') {
      return primes(match);
    }
    if (match.numbers.size() == 2) {
      // à 18.30, but à 0.40%
      bool decimal = match.separator == U'.' &&
                     (!kTimeWords.contains(match.context.before()) || match.context.tail().starts_with(U'%'));
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

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    auto role = roman_role(match, kRomanCues);
    if (!role) {
      return std::nullopt;
    }
    switch (*role) {
      case RomanRole::NAME: // Louis quatorze, but François premier, Élisabeth première
        return n == 1 ? (kFeminineNames.contains(match.context.before()) ? U"première" : U"premier") : cardinal(n);
      case RomanRole::AFTER: // le XXI siècle: vingt et unième; la V République: cinquième
        return ordinal(n, kFeminineRomanNouns.contains(strings::to_lower(match.context.after())));
      default:
        return cardinal(n); // chapitre trois
    }
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    std::u32string_view suffix = match.suffix;
    std::u32string words;
    if (suffix.starts_with(U"nd")) {
      words = suffix.starts_with(U"nde") ? U"seconde" : U"second";
    } else {
      bool feminine = suffix == U"re" || suffix == U"ère" || suffix == U"ere" || suffix == U"res" || suffix == U"ères";
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
      return words + U" " + of(currency.many);
    }

    // A price: trois euros cinquante, quatre-vingt-dix-neuf centimes, trois euros cinquante par mois
    bool feminine = currency.one == U"livre";
    if (number.fraction.size() == 2) {
      uint64_t cents = parse_value(number.fraction).value_or(0);
      bool per = match.unit.find(U'/') != std::u32string_view::npos;
      std::u32string unit = per ? currency.one.substr(currency.one.find(U' ')) : U"";
      if (value(number) == 0) {
        return cardinal(cents) + U" centimes" + unit;
      }
      Number whole{.integer = number.integer, .grouped = number.grouped};
      const std::u32string& words = number.integer == U"1" ? currency.one : currency.many;
      return gendered(whole, cues, feminine) + U" " + words.substr(0, words.size() - unit.size()) + U" " +
             cardinal(cents) + unit;
    }
    const std::u32string& noun = number_form(number, currency);
    return gendered(number, cues, feminine) + U" " + (round_millions(number) ? of(noun) : noun);
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
    // les 80s, les 1980s: a decade (les années quatre-vingt), not seconds
    uint64_t n = value(number);
    bool decade = n % 10 == 0 && ((n >= 10 && n < 100) || (n >= 1000 && n < 10000)) && number.fraction.empty();
    if (match.unit == U"s" && decade && match.numbers.size() == 1 && !strings::contains(match.text, U' ') &&
        kDecadeWords.contains(match.context.before())) {
      return U"années " + cardinal(n, false, false);
    }
    // 1,80 m, 1m65: un mètre quatre-vingts, un mètre soixante-cinq
    bool meters = match.unit == U"m" && n >= 1 && n < 10 && !number.fraction.empty() && number.fraction.size() <= 2;
    if (meters && match.numbers.size() == 1) {
      uint64_t centimeters = parse_value(number.fraction).value_or(0) * (number.fraction.size() == 1 ? 10 : 1);
      Number whole{.integer = number.integer};
      return cardinal(n) + U" " + number_form(whole, kUnits.at(U"m")) +
             (centimeters ? U" " + cardinal(centimeters) : U"");
    }
    const Forms& unit = match.unit == U"″" ? kInch : kUnits.at(std::u32string(match.unit));
    std::u32string_view first_word = std::u32string_view(unit.one).substr(0, unit.one.find(U' '));
    return gendered(number, cues_of(match), feminine(first_word)) + U" " + number_form(number, unit);
  }

  std::optional<std::u32string> read_joiner(const Match& match) const override {
    if (match.kind == Kind::DIMENSION) {
      return cues_of(match).equation ? U" fois " : U" par ";
    }
    // 0-89100-396-7: a hyphen in a code, not a range
    if (match.kind == Kind::RANGE && std::ranges::any_of(match.numbers, [](const Number& number) {
          return !number.grouped && number.integer.size() >= 5 && !number.integer.ends_with(U"000");
        })) {
      return std::nullopt;
    }
    return PatternLanguage::read_joiner(match);
  }

private:
  // 1'27"23: une minute vingt-sept secondes et vingt-trois centièmes; 23"42: vingt-trois secondes quarante-deux
  static std::optional<std::u32string> primes(const Match& match) {
    size_t first = match.unit == U"h" ? 0 : match.unit == U"min" ? 1 : 2;
    std::vector<std::u32string> words;
    for (size_t i = 0; i < match.numbers.size(); ++i) {
      uint64_t n = value(match.numbers[i]);
      words.push_back(cardinal(n, true) + U" " + (n < 2 ? kPrimeUnits[first + i].one : kPrimeUnits[first + i].many));
    }
    const std::u32string& fraction = match.numbers.back().fraction;
    if (!fraction.empty()) {
      std::u32string hundredths = cardinal(parse_value(fraction).value_or(0));
      words.push_back(first == 1 && fraction.size() == 2 ? U"et " + hundredths + U" centièmes" : hundredths);
    }
    return join(words);
  }
};
} // namespace

const Language& language() {
  static const French language;
  return language;
}

} // namespace verbalis::languages::fr
