#include "constants.h"

#include <verbalis/languages/language.h>
#include <verbalis/numbers/scanning.h>
#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::en {

namespace {
using namespace text;
using namespace numbers;

// ------------------------
// ----- Number words -----
// ------------------------

std::u32string cardinal(uint64_t n) {
  if (n < 20) {
    return std::u32string(kOnes[n]);
  }
  if (n < 100) {
    return std::u32string(kTens[n / 10]) + (n % 10 ? U"-" + std::u32string(kOnes[n % 10]) : U"");
  }
  if (n < 1000) {
    return std::u32string(kOnes[n / 100]) + U" hundred" + (n % 100 ? U" " + cardinal(n % 100) : U"");
  }
  for (const auto& [value, word] : kScaleWords) {
    if (n >= value) {
      return cardinal(n / value) + U" " + word + (n % value ? U" " + cardinal(n % value) : U"");
    }
  }
  return U"";
}

std::u32string ordinal(uint64_t n);

// A day before its month: the twenty-fifth of
std::u32string day_of(uint64_t day, bool after_the) {
  return (after_the ? U"" : U"the ") + ordinal(day) + U" of";
}

std::u32string ordinal(uint64_t n) {
  // Only the last word changes: twenty-first, one hundred and twelfth
  std::u32string words = cardinal(n);
  size_t start = words.find_last_of(U" -") + 1;
  std::u32string last = words.substr(start);
  words.erase(start);

  if (auto it = kIrregularOrdinals.find(last); it != kIrregularOrdinals.end()) {
    return words + it->second;
  }
  return words + (last.ends_with(U'y') ? last.substr(0, last.size() - 1) + U"ieth" : last + U"th");
}

// Year-style reading, also of 3-4 digit codes: nineteen oh five, two thousand seven, nine fifty-six
std::u32string pairs(uint64_t n) {
  uint64_t high = n / 100, low = n % 100;
  if (n >= 1000 && n % 1000 < 10) {
    return cardinal(n);
  }
  if (low == 0) {
    return cardinal(high) + U" hundred";
  }
  return cardinal(high) + U" " + (low < 10 ? U"oh " + std::u32string(kOnes[low]) : cardinal(low));
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.emplace_back(kOnes[c - U'0']);
  }
  return join(words);
}

// An amount of money: a cardinal, unless written with a leading zero
std::u32string amount(const Number& number) {
  auto n = parse_value(number.integer);
  bool zero = number.integer.size() > 1 && number.integer[0] == U'0';
  return n && !zero ? cardinal(*n) : digits(number.integer);
}

// a.m. and p.m. as their lowercase abbreviations are said; AM and PM stay as written
std::u32string ampm(std::u32string_view suffix) {
  std::u32string base = strings::to_lower(suffix.substr(0, suffix.find_last_not_of(U'.') + 1));
  bool lowercase = std::ranges::none_of(suffix, [](char32_t c) { return unicode::isupper(c); });
  if (lowercase && (base == U"am" || base == U"pm")) {
    return base == U"am" ? U"a.m." : U"p.m.";
  }
  return std::u32string(suffix);
}

// What the words around a match say about reading its numbers
struct Cues {
  bool after_month = false; // March 3: a day of the month, read as an ordinal
  bool counted = false;     // 1999 people: a count rather than a year
  bool attributive =
      false; // a $5 bill: before a noun (not a verb in -ed or -ing, nor an adverb), the unit stays singular
  bool before_month = false; // 25 July 2012: a day before the name of its month
  bool after_the = false;    // the 25th, 24/25 May: no article before it
  bool year = false;         // in 1999: a year is expected
  bool era = false;          // 340 AD: an era follows
  bool amount = false;       // $2019, 1980 kg: an amount, never read as a year
  bool spelled = false;      // ending in 8876, PIN 1234: digits to read one by one
  bool alone = false;        // the whole text: 10111 or 31/32 alone is a number, not a code or a date
};

// Whether a word is an adverb by its ending: $60 million annually, -7.3°C respectively; not rally, family
bool is_adverb(std::u32string_view word) {
  return word.size() > 6 && (word.ends_with(U"ally") || word.ends_with(U"ively") || word.ends_with(U"fully") ||
                             word.ends_with(U"ously") || word.ends_with(U"ately"));
}

// A plural noun, lowercased: people, months
bool is_plural_noun(const std::u32string& word) {
  return kIrregularPlurals.contains(word) ||
         (word.size() > 3 && word.ends_with(U's') && !word.ends_with(U"ss") && !kStopWords.contains(word));
}

Cues cues_of(const Match& match) {
  std::u32string previous = match.context.before();
  std::u32string next = match.context.after();
  std::u32string next_lower = strings::to_lower(next);

  // 1999 people: a plural noun after a 4-digit number makes it a count, unless a year is expected (in 1999)
  bool plural_noun = is_plural_noun(next_lower);
  // March 3, august 23, but "it may 3 times", "in May, 24 people": a month right before the number, capitalized
  // if it is also a common word
  auto words = words_of(match.context.head());
  std::u32string_view written = words.empty() ? U"" : words.back();
  std::u32string_view name = stripped(written);
  bool common = previous == U"may" || previous == U"march" || previous == U"mar";
  // July 5 to 12: the end of a range of days from a one-digit day is an ordinal too; the corpus reads March 14-23 as
  // "fourteenth to twenty-three"
  std::u32string day = match.context.before(2);
  bool range_end =
      previous == U"to" && day.size() == 1 && unicode::isdigit(day[0]) && kMonthWords.contains(match.context.before(3));
  bool after_month = range_end || (kMonthWords.contains(previous) && name.end() == written.end() &&
                                   (!common || unicode::isupper(name[0])));
  bool year_cue = kYearCues.contains(previous) || after_month;

  // 25 July, 26th May 2012, but Top 10 March Madness picks, Quarter 1 (Jan
  std::u32string_view tail = match.context.tail();
  auto after = words_of(tail);
  bool closed = !after.empty() && stripped(after[0]).end() != after[0].end(); // May, Jan: no word follows
  std::u32string following = match.context.after(2);
  // A lowercase month counts too, unless it is a common word (may, march) without a year after it: 17 may 2010
  bool common_month = next_lower == U"may" || next_lower == U"march" || next_lower == U"mar";
  bool lowercase_month = !common_month || (!following.empty() && unicode::isdigit(following[0]));
  bool month = tail.size() > 1 && tail[0] == U' ' && unicode::isalpha(tail[1]) &&
               (unicode::isupper(tail[1]) || lowercase_month) && kMonthWords.contains(next_lower) &&
               (closed || following.empty() || !unicode::isupper(following[0]));
  // 24/25 May, 23-24 June: the article goes before the first day
  std::u32string_view head = match.context.head();
  bool in_range = !head.empty() && (head.back() == U'/' || head.back() == U'-');

  return {.after_month = after_month,
          .counted = plural_noun && !year_cue,
          .attributive = !next.empty() && unicode::isalpha(next[0]) && !kStopWords.contains(next_lower) &&
                         !tail.starts_with(U" (") && !next_lower.ends_with(U"ed") && !next_lower.ends_with(U"ing") &&
                         !is_adverb(next_lower),
          .before_month = month,
          .after_the = previous == U"the" || in_range,
          .year = year_cue,
          .era = kEras.contains(next_lower),
          .amount = match.kind == Kind::MONEY || match.kind == Kind::SCALE || match.kind == Kind::PERCENT ||
                    match.kind == Kind::UNIT,
          // PIN 1234, your otp is 9453, ending in 8876
          .spelled = kSpellingWords.contains(previous) ||
                     ((previous == U"is" || previous == U"was") && kSpellingWords.contains(match.context.before(2))) ||
                     (previous == U"in" && match.context.before(2) == U"ending"),
          .alone = match.context.head().empty() && tail.empty()};
}

// --------------------
// ----- Patterns -----
// --------------------

// 10/06/2005, 11/17/05: a date with slashes and a year, month first unless the first number can't be a month
// (25/12/2020). Its numbers are as written.
class SlashDatePattern : public Pattern {
public:
  SlashDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,/-")) {
      return std::nullopt;
    }
    std::vector<Number> parts;
    size_t end = position;
    for (size_t i = 0; i < 3; i++) {
      if (i > 0 && (end >= text.size() || text[end] != U'/')) {
        return std::nullopt;
      }
      size_t start = i == 0 ? end : end + 1;
      end = digits_end(text, start);
      size_t length = end - start;
      if (length == 0 || (i < 2 ? length > 2 : length != 2 && length != 4)) {
        return std::nullopt;
      }
      parts.push_back(digits_at(text, start));
    }
    if (!is_clear_after(text, end, U"/") || is_digit_after(text, end, U".,/")) {
      return std::nullopt;
    }
    uint64_t a = value(parts[0]), b = value(parts[1]);
    uint64_t month = a <= 12 ? a : b, day = a <= 12 ? b : a;
    if (month < 1 || month > 12 || day < 1 || day > 31) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DATE, .span = {position, end}, .numbers = parts, .separator = U'/'};
  }
};

// 2016/07/03, 1998/2/30, 2012.01.05: a date with its year first. Its numbers are the day, the month, and the year,
// as an ISO date's.
class YearFirstDatePattern : public Pattern {
public:
  YearFirstDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t year_end = digits_end(text, position);
    if (!is_clear_before(text, position, U".,/-") || year_end != position + 4 || year_end >= text.size() ||
        (text[year_end] != U'/' && text[year_end] != U'.')) {
      return std::nullopt;
    }
    char32_t separator = text[year_end];
    size_t month = year_end + 1, month_end = digits_end(text, month);
    if (month_end == month || month_end > month + 2 || month_end >= text.size() || text[month_end] != separator) {
      return std::nullopt;
    }
    size_t day = month_end + 1, day_end = digits_end(text, day);
    // With dots, two digits each (2012.01.05), which a version number seldom has
    size_t shortest = separator == U'.' ? 2 : 1;
    if (day_end < day + shortest || day_end > day + 2 || month_end < month + shortest ||
        !is_clear_after(text, day_end, U"/") || is_digit_after(text, day_end, U".,/")) {
      return std::nullopt;
    }
    Number y = digits_at(text, position), m = digits_at(text, month), d = digits_at(text, day);
    if (value(m) < 1 || value(m) > 12 || value(d) < 1 || value(d) > 31) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DATE, .span = {position, day_end}, .numbers = {d, m, y}, .separator = U'-'};
  }
};

// 30-May-2018, 5-Jan-20: a day, the name of its month, and a year, joined by hyphens. Its numbers are the day,
// the month, and the year.
class NamedMonthDatePattern : public Pattern {
public:
  NamedMonthDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t day_end = digits_end(text, position);
    if (!is_clear_before(text, position, U".,/-") || day_end == position || day_end > position + 2 ||
        day_end >= text.size() || text[day_end] != U'-') {
      return std::nullopt;
    }
    size_t name_end = day_end + 1;
    while (name_end < text.size() && unicode::isalpha(text[name_end])) {
      name_end++;
    }
    std::u32string name = strings::to_lower(text.substr(day_end + 1, name_end - day_end - 1));
    auto month = std::ranges::find_if(kMonths, [&](const auto& m) { return strings::to_lower(m).starts_with(name); });
    // The terms step may have written out an abbreviation already: 5-Jan-2020 is 5-January 2020 here
    if (name.size() < 3 || !kMonthWords.contains(name) || month == kMonths.end() || name_end >= text.size() ||
        (text[name_end] != U'-' && text[name_end] != U' ')) {
      return std::nullopt;
    }
    // After a space, a four-digit year only: July 28-August 31 is a range
    size_t end = digits_end(text, name_end + 1), year = end - name_end - 1;
    if ((year != 2 && year != 4) || (text[name_end] == U' ' && year != 4) || !is_clear_after(text, end) ||
        is_digit_after(text, end, U".,-/")) {
      return std::nullopt;
    }
    Number day = digits_at(text, position);
    if (value(day) < 1 || value(day) > 31) {
      return std::nullopt;
    }
    Number number{.integer = to_digits(static_cast<uint64_t>(month - kMonths.begin() + 1))};
    return Match{.kind = Kind::DATE,
                 .span = {position, end},
                 .numbers = {day, number, digits_at(text, name_end + 1)},
                 .separator = U'-'};
  }
};

// 2Q22, 3Q2022, 1H23: a quarter or a half of a year. Its numbers are the quarter or half and the year.
class QuarterPattern : public Pattern {
public:
  QuarterPattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position) || position + 3 >= text.size()) {
      return std::nullopt;
    }
    char32_t part = text[position], letter = text[position + 1];
    bool quarter = letter == U'Q' && part >= U'1' && part <= U'4',
         half = letter == U'H' && (part == U'1' || part == U'2');
    size_t end = digits_end(text, position + 2);
    if (!(quarter || half) || (end != position + 4 && end != position + 6) || !is_clear_after(text, end)) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DATE,
                 .span = {position, end},
                 .numbers = {digits_at(text, position), digits_at(text, position + 2)},
                 .separator = letter};
  }
};

// 1/4th, 1/3RD, 23 4/5th: a fraction marked by the suffix of its denominator. Its numbers are as a FractionPattern's.
class SuffixedFractionPattern : public Pattern {
public:
  SuffixedFractionPattern() : Pattern(Kind::FRACTION) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U"./")) {
      return std::nullopt;
    }
    std::vector<Number> numbers;
    size_t start = position, whole_end = digits_end(text, position);
    if (whole_end > position && whole_end + 1 < text.size() && text[whole_end] == U' ') {
      numbers.push_back(digits_at(text, position));
      start = whole_end + 1;
    }
    size_t slash = digits_end(text, start);
    if (slash == start || slash >= text.size() || text[slash] != U'/') {
      return std::nullopt;
    }
    size_t end = digits_end(text, slash + 1);
    std::u32string suffix = strings::to_lower(text.substr(end, std::min<size_t>(2, text.size() - end)));
    if (end == slash + 1 || (suffix != U"st" && suffix != U"nd" && suffix != U"rd" && suffix != U"th") ||
        !is_clear_after(text, end + 2)) {
      return std::nullopt;
    }
    numbers.push_back(digits_at(text, start));
    numbers.push_back(digits_at(text, slash + 1));
    return Match{.kind = Kind::FRACTION,
                 .span = {position, end + 2},
                 .numbers = numbers,
                 .suffix = text.substr(end, 2),
                 .separator = U'/'};
  }
};

// 1960s-80s, 1960s-1980s: a range of decades. Its numbers are the first and the last decade.
class DecadeRangePattern : public Pattern {
public:
  DecadeRangePattern() : Pattern(Kind::DECADE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t first = digits_end(text, position);
    if (!is_clear_before(text, position, U".,-") || first != position + 4 || first + 2 >= text.size() ||
        text.substr(first, 2) != U"s-") {
      return std::nullopt;
    }
    size_t last = first + 2, last_end = digits_end(text, last);
    if ((last_end != last + 2 && last_end != last + 4) || last_end >= text.size() || text[last_end] != U's' ||
        !is_clear_after(text, last_end + 1)) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DECADE,
                 .span = {position, last_end + 1},
                 .numbers = {digits_at(text, position), digits_at(text, last)},
                 .suffix = U"s"};
  }
};

// 21:51:31, 2:08:29: hours, minutes, and seconds. Its numbers are the three parts; a race time (1:03.91) has two,
// minutes and seconds with their fraction.
class SecondsPattern : public Pattern {
public:
  SecondsPattern() : Pattern(Kind::TIME) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,:")) {
      return std::nullopt;
    }
    std::vector<Number> parts;
    size_t end = position;
    for (size_t i = 0; i < 3; i++) {
      if (i == 2 && end + 1 < text.size() && text[end] == U'.' && unicode::isdigit(text[end + 1])) {
        size_t fraction_end = digits_end(text, end + 1);
        parts[1].fraction = text.substr(end + 1, fraction_end - end - 1);
        parts[1].decimal_mark = U'.';
        parts[1].span.end = end = fraction_end;
        break;
      }
      if (i > 0 && (end >= text.size() || text[end] != U':')) {
        return std::nullopt;
      }
      size_t start = i == 0 ? end : end + 1;
      end = digits_end(text, start);
      size_t length = end - start;
      if (length == 0 || length > 2 || (i > 0 && (length != 2 || text[start] > U'5'))) {
        return std::nullopt;
      }
      parts.push_back(digits_at(text, start));
    }
    if (!is_clear_after(text, end) || is_digit_after(text, end, U".,:")) {
      return std::nullopt;
    }
    return Match{.kind = Kind::TIME, .span = {position, end}, .numbers = parts, .separator = U':'};
  }
};

// $.01, $20/mo, £10/wk, $12/kg: an amount after its currency, with no units (.01) or followed by what it is counted by.
// The suffix is the key of kPer.
class PricePattern : public Pattern {
public:
  PricePattern() : Pattern(Kind::MONEY) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position)) {
      return std::nullopt;
    }
    for (auto currency : keys_at(text, position, kCurrencyKeys)) {
      size_t start = skip_space(text, position + currency.size());
      Number amount;
      if (start + 1 < text.size() && text[start] == U'.' && unicode::isdigit(text[start + 1])) {
        size_t end = digits_end(text, start + 1);
        amount = {.span = {start, end},
                  .fraction = std::u32string(text.substr(start + 1, end - start - 1)),
                  .decimal_mark = U'.'};
      } else if (auto numbers = numbers_at(text, start, U",", U"."); !numbers.empty()) {
        amount = numbers[0];
      } else {
        continue;
      }

      Match money{.kind = Kind::MONEY, .span = {position, amount.span.end}, .numbers = {amount}, .unit = currency};
      size_t slash = amount.span.end;
      if (slash < text.size() && text[slash] == U'/') {
        for (auto key : keys_at(text, slash + 1, kPerKeys)) {
          if (is_clear_after(text, slash + 1 + key.size())) {
            money.span.end = slash + 1 + key.size();
            money.suffix = key;
            return money;
          }
        }
      }
      if (amount.integer.empty() && is_clear_after(text, amount.span.end) && !is_digit_after(text, amount.span.end)) {
        return money;
      }
    }
    return std::nullopt;
  }
};

// 2pm-5pm, 5%-10%, $250-$300, 4:00 am - 8:12 pm: the hyphen of a range whose ends are marked as amounts or times.
// It has no numbers; the generic RangePattern takes the ranges of bare numbers.
class MarkedRangePattern : public Pattern {
public:
  MarkedRangePattern() : Pattern(Kind::RANGE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    Span span{position, position + 1};
    if (text.substr(position).starts_with(U" - ")) {
      span.end = position + 3;
    } else if (text[position] != U'-') {
      return std::nullopt;
    }
    std::u32string head = strings::to_lower(text.substr(0, span.begin));
    std::u32string_view tail = text.substr(span.end);

    // The end before it: a percentage or a time with am or pm; or any amount before a currency
    std::u32string_view mark = head;
    while (!mark.empty() && (mark.back() == U'.' || unicode::isalpha(mark.back()))) {
      mark.remove_suffix(1);
    }
    std::u32string_view suffix = std::u32string_view(head).substr(mark.size());
    while (!mark.empty() && mark.back() == U' ') {
      mark.remove_suffix(1);
    }
    bool time = (suffix == U"am" || suffix == U"pm" || suffix == U"a.m." || suffix == U"p.m.") && !mark.empty() &&
                unicode::isdigit(mark.back());
    bool percent = head.size() > 1 && head.back() == U'%' && unicode::isdigit(head[head.size() - 2]);
    bool amount = !head.empty() && unicode::isdigit(head.back());

    // The end after it: a number, or an amount after its currency
    bool currency = std::ranges::any_of(keys_at(tail, 0, kCurrencyKeys), [&](auto key) {
      return is_digit_after(tail, key.size()) || is_digit_after(tail, skip_space(tail, key.size()));
    });
    bool number = !tail.empty() && unicode::isdigit(tail[0]);
    if (((time || percent) && (number || currency)) || (amount && currency)) {
      return Match{.kind = Kind::RANGE, .span = span, .separator = U'-'};
    }
    return std::nullopt;
  }
};

// 2*8, 2 * 10: the times of a product, between numbers
class ProductPattern : public Pattern {
public:
  ProductPattern() : Pattern(Kind::DIMENSION) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t star = skip_space(text, position);
    if (position == 0 || !unicode::isdigit(text[position - 1]) || star >= text.size() || text[star] != U'*') {
      return std::nullopt;
    }
    size_t end = skip_space(text, star + 1);
    if (end >= text.size() || !unicode::isdigit(text[end])) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DIMENSION, .span = {position, end}, .separator = U'*'};
  }
};

// 1-2=5, 1- 2 = 5: the minus of an equation
class EquationMinusPattern : public Pattern {
public:
  EquationMinusPattern() : Pattern(Kind::DIFFERENCE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (position == 0 || text[position] != U'-' || !unicode::isdigit(text[position - 1])) {
      return std::nullopt;
    }
    size_t number = skip_space(text, position + 1);
    size_t end = digits_end(text, number);
    if (end == number || skip_space(text, end) >= text.size() || text[skip_space(text, end)] != U'=') {
      return std::nullopt;
    }
    return Match{.kind = Kind::DIFFERENCE, .span = {position, number}, .separator = U'-'};
  }
};

// A straight double quote as a unit: 27" monitor
constexpr std::u32string_view kInchMark = U"\"";
const Forms kInches = {U"inch", U"inches"};

const Forms& unit_forms(std::u32string_view unit) {
  return unit == kInchMark ? kInches : kUnits.at(std::u32string(unit));
}

// Whether a straight double quote right after a number is an inch mark (27" monitor), not the end of a quotation
// ("67" matters): the quotes before it, other than inch marks, are all closed
bool is_inch_mark(std::u32string_view text, size_t position) {
  if (position >= text.size() || text[position] != U'"') {
    return false;
  }
  int open = 0;
  for (size_t i = 0; i < position; i++) {
    bool inch = text[i] == U'"' && i > 0 && unicode::isdigit(text[i - 1]);
    open += text[i] == U'“' || (text[i] == U'"' && !inch);
    open -= text[i] == U'”';
  }
  return open % 2 == 0;
}

// 27" monitor, 21.5" iMac: a number of inches, of at most three digits
class InchPattern : public Pattern {
public:
  InchPattern() : Pattern(Kind::UNIT) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,'’′")) {
      return std::nullopt;
    }
    for (auto& number : numbers_at(text, position, kNotation.group_separators, kNotation.decimal_separators)) {
      if (number.integer.size() <= 3 && is_inch_mark(text, number.span.end)) {
        return Match{
            .kind = Kind::UNIT, .span = {position, number.span.end + 1}, .numbers = {number}, .unit = kInchMark};
      }
    }
    return std::nullopt;
  }
};

// The unit right after a match, of at least two letters after a space, or an inch mark: 4 1/2 lbs, 100 million kg,
// 3/8"
std::optional<Match> with_unit(std::u32string_view text, Match match) {
  static const Keys units = [] {
    Keys keys;
    std::ranges::copy_if(keys_of(kUnits), std::back_inserter(keys), [](auto key) { return key.size() > 1; });
    return keys;
  }();
  size_t start = match.span.end;
  if (is_inch_mark(text, start)) {
    match.span.end = start + 1;
    match.unit = kInchMark;
    return match;
  }
  if (start >= text.size() || text[start] != U' ') {
    return std::nullopt;
  }
  for (auto unit : keys_at(text, start + 1, units)) {
    if (is_clear_after(text, start + 1 + unit.size())) {
      match.span.end = start + 1 + unit.size();
      match.unit = unit;
      return match;
    }
  }
  return std::nullopt;
}

// 4 1/2 lbs, 1/2 kg: a fraction of a unit
class FractionUnitPattern : public Pattern {
public:
  FractionUnitPattern() : Pattern(Kind::FRACTION) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    auto match = fractions_.match(text, position);
    return match ? with_unit(text, *match) : std::nullopt;
  }

private:
  FractionPattern fractions_;
};

// 100 million kg: an amount of a unit with a scale word
class ScaleUnitPattern : public Pattern {
public:
  ScaleUnitPattern() : Pattern(Kind::SCALE), scales_(kNotation) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    auto match = scales_.match(text, position);
    if (!match || !kScales.contains(std::u32string(match->scale)) || match->scale.size() < 4) {
      return std::nullopt;
    }
    return with_unit(text, *match);
  }

private:
  ScalePattern scales_;
};

// 12/kg, 12kg/kg, 3 cc/s: an amount per unit, whose numerator unit is optional. The unit after the slash is the
// match's suffix.
class PerUnitPattern : public Pattern {
public:
  PerUnitPattern() : Pattern(Kind::UNIT), units_(keys_of(kUnits)), per_(kPerKeys) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U"./")) {
      return std::nullopt;
    }
    for (auto& number : numbers_at(text, position, kNotation.group_separators, kNotation.decimal_separators)) {
      size_t slash = number.span.end;
      std::u32string_view unit;
      for (auto key : keys_at(text, skip_space(text, slash), units_)) {
        if (size_t end = skip_space(text, slash) + key.size(); end < text.size() && text[end] == U'/') {
          unit = key;
          slash = end;
          break;
        }
      }
      if (slash >= text.size() || text[slash] != U'/') {
        continue;
      }
      for (const Keys* table : {&per_, &units_}) {
        for (auto per : keys_at(text, slash + 1, *table)) {
          if (is_clear_after(text, slash + 1 + per.size())) {
            return Match{.kind = Kind::UNIT,
                         .span = {position, slash + 1 + per.size()},
                         .numbers = {number},
                         .unit = unit,
                         .suffix = per};
          }
        }
      }
    }
    return std::nullopt;
  }

private:
  Keys units_;
  const Keys& per_;
};

// --------------------
// ----- Language -----
// --------------------

class English final : public PatternLanguage {
public:
  English() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<SlashDatePattern>());
    add_pattern(std::make_unique<YearFirstDatePattern>());
    add_pattern(std::make_unique<NamedMonthDatePattern>());
    add_pattern(std::make_unique<QuarterPattern>());
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<EquationMinusPattern>());
    add_pattern(std::make_unique<MarkedRangePattern>());
    add_pattern(std::make_unique<ProductPattern>());
    add_pattern(std::make_unique<SecondsPattern>());
    add_pattern(std::make_unique<PrimePattern>(kNotation, std::unordered_set<std::u32string>{}, true));
    add_pattern(std::make_unique<PricePattern>());
    add_pattern(std::make_unique<SuffixedFractionPattern>());
    add_pattern(std::make_unique<FractionUnitPattern>());
    add_pattern(std::make_unique<ScaleUnitPattern>());
    add_pattern(std::make_unique<PerUnitPattern>());
    add_pattern(std::make_unique<InchPattern>());
    add_pattern(std::make_unique<GroupedOrdinalPattern>(kOrdinalSuffixes, U","));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<DecadeRangePattern>());
    add_pattern(std::make_unique<SuffixPattern>(Kind::DECADE, kDecadeSuffixes, kDecadeFormat));
    add_pattern(std::make_unique<RomanPattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (number.fraction.empty()) {
      return integer(number, cues_of(match));
    }

    // .45 caliber: forty-five; .5 has no zero before its point
    std::u32string next = strings::to_lower(match.context.after());
    if (number.integer.empty() && (next == U"caliber" || next == U"calibre" || next == U"cal")) {
      return cardinal(value(Number{.integer = number.fraction}));
    }
    std::u32string head = number.integer.empty() ? U"" : cardinal(value(number)) + U" ";
    return head + (number.fraction == U"0" ? U"point oh" : U"point " + digits(number.fraction));
  }

  std::u32string read_code(const Match& match) const override {
    if (!cues_of(match).spelled) {
      return PatternLanguage::read_code(match);
    }
    // otp ab9453: after a spelling cue, digits one by one
    std::vector<std::u32string> words;
    for (auto part : code_parts(match.text)) {
      if (std::ranges::all_of(part, unicode::isdigit)) {
        words.push_back(digits(part));
      } else if (part != U"-" && part != U".") {
        words.push_back(read_code_letters(part));
      }
    }
    return join(words);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = parse_value(text);
    if (text.size() > 1 && text[0] == U'0') {
      std::vector<std::u32string> words;
      for (char32_t c : text) {
        words.emplace_back(c == U'0' ? U"oh" : kOnes[c - U'0']);
      }
      return join(words);
    }
    if (!n || text.size() > 4) {
      return digits(text);
    }
    return text.size() <= 2 ? cardinal(*n) : pairs(*n);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // 2Q22: the second quarter of twenty-two; 1H23: the first half of twenty-three
    if (match.separator == U'Q' || match.separator == U'H') {
      const Number& year = match.numbers[1];
      return U"the " + ordinal(value(match.numbers[0])) + (match.separator == U'Q' ? U" quarter of " : U" half of ") +
             (year.integer.size() == 4 ? pairs(value(year)) : cardinal(value(year)));
    }
    // 10/06/2005: October sixth (25/12/2020 has its day first); 15.01.2020: the fifteenth of January
    uint64_t day = value(match.numbers[0]), month = value(match.numbers[1]);
    bool month_first = match.separator == U'/' && day <= 12;
    if (month_first) {
      std::swap(day, month);
    }
    std::u32string name = kMonths[month - 1];
    std::u32string words =
        month_first ? name + U" " + ordinal(day) : day_of(day, match.context.before() == U"the") + U" " + name;
    if (match.numbers.size() < 3) {
      return words;
    }
    // 1998: nineteen ninety-eight; 05: oh five
    const Number& year = match.numbers[2];
    uint64_t y = value(year);
    std::u32string year_words = year.integer.size() == 4 ? pairs(y) : (y < 10 ? U"oh " + cardinal(y) : cardinal(y));
    return words + U" " + year_words;
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // 1-555-...: the leading one joins the area code
    std::vector<std::u32string> groups;
    for (const auto& number : match.numbers) {
      groups.push_back(number.integer);
    }
    if (match.text.starts_with(U"1-")) {
      groups[0] = U"1" + groups[0];
    }

    std::vector<std::u32string> words;
    for (const auto& group : groups) {
      words.push_back(digits(group));
    }
    return join(words, U", ");
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    if (auto words = named_fraction(kFractions, match)) {
      return words;
    }
    // one fifth, two thirds, three quarters, one hundredth
    auto [a, b] = fraction_of(match);
    bool suffixed = !match.suffix.empty(); // 1/4th
    bool alone = cues_of(match).alone;     // 22/3 alone: also an improper fraction
    if (b < 1 || (a >= b && !alone) ||
        !(suffixed || alone || !match.unit.empty() || is_marked_fraction(match, kFractionWords))) {
      return std::nullopt;
    }
    if (b == 1) {
      return cardinal(a) + U" over one";
    }
    std::u32string denominator = b == 2 ? U"half" : b == 4 ? U"quarter" : ordinal(b);
    if (a != 1) {
      denominator = b == 2 ? U"halves" : denominator + U"s";
    }
    return cardinal(a) + U" " + denominator;
  }

  std::optional<std::u32string> read_mixed_fraction(const Match& match) const override {
    auto words = PatternLanguage::read_mixed_fraction(match);
    if (!words || match.unit.empty()) {
      return words;
    }
    const Forms& unit = unit_forms(match.unit);
    // a 3/8" bolt: before a noun, the unit stays singular
    if (cues_of(match).attributive) {
      return *words + U" " + unit.one;
    }
    if (match.numbers.size() == 3) {
      return *words + U" " + unit.many;
    }
    // 1/2 kg: half a kilogram; 3/4 in: three quarters of an inch
    std::u32string article =
        std::u32string_view(U"aeiou").find(unit.one[0]) == std::u32string_view::npos ? U"a" : U"an";
    return (*words == U"one half" ? U"half" : *words + U" of") + U" " + article + U" " + unit.one;
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    if (match.separator == U'′') {
      return primes(match);
    }
    uint64_t hours = value(match.numbers[0]);
    std::u32string suffix = ampm(match.suffix);

    // 1:03.91: one minute three point nine one seconds
    if (match.numbers.size() == 2 && !match.numbers[1].fraction.empty()) {
      std::u32string minutes = hours ? cardinal(hours) + (hours == 1 ? U" minute " : U" minutes ") : U"";
      return minutes + read_number(match.numbers[1], match) + U" seconds";
    }

    // 10:00:00 p.m.: a clock time, ten p.m. (its a.m. or p.m. stays after it)
    std::u32string next = strings::to_lower(match.context.after());
    bool clock =
        next == U"a.m." || next == U"p.m." || next == U"am" || next == U"pm" || next == U"a.m" || next == U"p.m";
    if (match.numbers.size() == 3 && clock && value(match.numbers[1]) == 0 && value(match.numbers[2]) == 0 &&
        hours >= 1 && hours <= 12) {
      return cardinal(hours);
    }
    // 1:38:33 p.m., at 01:38:33: a clock time, one thirty-eight thirty-three; but "coming in at 8:51:00" is a duration
    bool stamp = match.context.before() == U"at" && match.numbers[0].integer.size() == 2;
    if (match.numbers.size() == 3 && (clock || stamp) && value(match.numbers[1]) != 0) {
      Match minutes = match;
      minutes.numbers.pop_back();
      auto words = read_time(minutes);
      uint64_t seconds = value(match.numbers[2]);
      if (words && seconds) {
        *words += U" " + (seconds < 10 ? U"oh " + cardinal(seconds) : cardinal(seconds));
      }
      return words;
    }
    // 2:08:29: two hours eight minutes and twenty-nine seconds, without the parts that are zero
    if (match.numbers.size() == 3) {
      std::vector<std::u32string> parts;
      for (auto [number, one, many] :
           {std::tuple{hours, U"hour", U"hours"}, std::tuple{value(match.numbers[1]), U"minute", U"minutes"},
            std::tuple{value(match.numbers[2]), U"second", U"seconds"}}) {
        if (number) {
          parts.push_back(cardinal(number) + U" " + (number == 1 ? one : many));
        }
      }
      if (parts.size() > 1) {
        parts.back() = U"and " + parts.back();
      }
      return parts.empty() ? U"zero seconds" : join(parts);
    }

    // 7pm: an hour alone needs am or pm
    if (match.numbers.size() == 1) {
      if (hours > 12 || !strings::contains(std::u32string_view(U"apAP"), match.suffix[0])) {
        return std::nullopt;
      }
      return cardinal(hours) + U" " + suffix;
    }

    // 1.13 is a decimal, unless a time is expected: at 10.30; but 2.53% and 4.00%, at 1.50 times, and 2.30 kg
    std::u32string_view tail = match.context.tail();
    std::u32string word = match.context.after();
    bool amount = tail.starts_with(U'%') || tail.starts_with(U" %") || word == U"times" || word == U"percent" ||
                  kUnits.contains(word);
    bool decimal =
        match.separator == U'.' && match.suffix.empty() && (amount || !kTimeWords.contains(match.context.before()));
    if (!fits_clock(match) || decimal) {
      return std::nullopt;
    }

    // ten thirty, ten oh five, ten o'clock
    const auto& minutes = match.numbers[1].integer;
    std::u32string words = cardinal(hours);
    if (minutes != U"00") {
      uint64_t m = value(match.numbers[1]);
      words += minutes[0] == U'0' ? U" oh " + std::u32string(kOnes[m]) : U" " + cardinal(m);
    }
    if (!suffix.empty()) {
      return words + U" " + suffix;
    }
    return words + (minutes == U"00" ? U" o'clock" : U"");
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    // the 26th May: the twenty-sixth of May
    uint64_t n = value(match.numbers[0]);
    Cues cues = cues_of(match);
    return cues.before_month && n >= 1 && n <= 31 ? day_of(n, cues.after_the) : ordinal(n);
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    // Henry the Eighth, the twenty-first century, Chapter four
    uint64_t n = value(match.numbers[0]);
    // IV as the whole text: a number (but I alone is the pronoun)
    if (match.context.head().empty() && match.context.tail().empty() && match.text.size() > 1) {
      return cardinal(n);
    }
    switch (auto role = roman_role(match, kRomanCues); role.value_or(RomanRole::BEFORE)) {
      case RomanRole::NAME:
        return U"the " + ordinal(n);
      case RomanRole::AFTER:
        return ordinal(n);
      default:
        return role ? std::optional(cardinal(n)) : std::nullopt;
    }
  }

  std::optional<std::u32string> read_decade(const Match& match) const override {
    // 1960s-80s: nineteen sixties to eighties
    std::u32string words = plural(read_number(match.numbers[0], match)).value_or(U"");
    if (match.numbers.size() == 2) {
      const Number& to = match.numbers[1];
      words += U" to " + *plural(to.integer.size() == 4 ? pairs(value(to)) : cardinal(value(to)));
    }
    return words;
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    Cues cues = cues_of(match);

    // $20/mo: twenty dollars per month
    std::u32string per = match.suffix.empty() ? U"" : U" per " + kPer.at(std::u32string(match.suffix));

    // A price: four dollars and ninety-nine cents, two dollars, fifty cents; $20.5 twenty dollars and fifty cents
    auto cents = kCents.find(std::u32string(match.unit));
    // ($2.5-3 billion is a range of a larger amount)
    size_t places = number.fraction.size();
    // (a range of a larger amount, $2.5-$3 billion, or a glued abbreviation, £10.8bn2)
    std::u32string_view after = match.context.tail();
    bool range = after.starts_with(U'-') || after.starts_with(U" -") || after.starts_with(U" to ") ||
                 (!after.empty() && unicode::isalpha(after[0]));
    if ((places == 2 || (places == 1 && !range)) && match.scale.empty() && cents != kCents.end()) {
      uint64_t whole = value(number), n = parse_value(number.fraction).value_or(0) * (places == 1 ? 10 : 1);
      std::u32string cent_words = cardinal(n) + U" " + (n == 1 ? cents->second.one : cents->second.many);
      if (whole == 0 && n) {
        return cent_words + per;
      }
      bool one =
          whole == 1 || (n == 0 && cues.attributive && per.empty()); // a $69.00 target: a sixty-nine dollar target
      std::u32string words = amount(number) + U" " + (one ? currency.one : currency.many);
      return (n ? words + U" and " + cent_words : words) + per;
    }

    // $18854, $2019: an amount, never digits by default nor a year
    std::u32string words = number.fraction.empty() ? amount(number) : read_number(number, match);
    if (!match.scale.empty()) {
      words += U" " + kScales.at(std::u32string(match.scale));
    }
    bool one = number.integer == U"1" && number.fraction.empty() && match.scale.empty();
    return words + U" " + (one || (cues.attributive && per.empty()) ? currency.one : currency.many) + per;
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K: a resolution; 5b, 3 m: a letter or a unit, a scale only after a currency
    if (match.scale == U"K" || match.scale == U"b" || match.scale == U"B" || match.scale == U"m") {
      return std::nullopt;
    }
    std::u32string words = read_number(number, match) + U" " + kScales.at(std::u32string(match.scale));
    return match.unit.empty() ? words : words + U" " + unit_forms(match.unit).many;
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return read_number(number, match) + U" percent" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    std::u32string words = read_number(number, match);
    if (!match.unit.empty()) {
      const Forms& unit = unit_forms(match.unit);
      bool one = number.fraction.empty() && value(number) == 1;
      words += U" " + (one || cues_of(match).attributive ? unit.one : unit.many);
    }
    // 12 kg/m²: per square meter
    if (!match.suffix.empty()) {
      auto per = kPer.find(std::u32string(match.suffix));
      words += U" per " + (per != kPer.end() ? per->second : kUnits.at(std::u32string(match.suffix)).one);
    }
    return words;
  }

  std::optional<std::u32string> read_joiner(const Match& match) const override {
    // A marked range has no numbers: 2pm-5pm; a product: 2*8
    if (match.kind == Kind::RANGE && match.numbers.empty()) {
      return joiners().range;
    }
    if (match.kind == Kind::DIMENSION && match.separator == U'*') {
      return U" times ";
    }
    // 10 - 30 seconds, 2025 - 2027: a range; 5 - 3 = 2: a minus; 2017 - 1:00pm: a pause before a time
    if (match.kind == Kind::DIFFERENCE && match.text == U" - ") {
      std::u32string_view tail = match.context.tail();
      size_t number_end = digits_end(tail, 0), end = skip_space(tail, number_end);
      if (number_end < tail.size() && tail[number_end] == U':') {
        return U" - ";
      }
      bool equation = tail.substr(end).starts_with(U'=') || tail.substr(end).starts_with(U"equals");
      return equation ? U" " + joiners().minus + U" " : joiners().range;
    }
    return PatternLanguage::read_joiner(match);
  }

  std::u32string read_range(const Number& from, const Number& to, const Match& match) const override {
    if (!from.fraction.empty() || !to.fraction.empty()) {
      return joiners().range;
    }

    // A range of amounts: 1980-1970 kg, 10-15%
    if (match.kind == Kind::MONEY || match.kind == Kind::PERCENT || match.kind == Kind::UNIT) {
      return joiners().range;
    }

    // 10-15 is "ten to fifteen", 1960-80 "nineteen sixty to eighty", 2024-25 "twenty twenty-four to twenty-five"; but
    // scores and two-digit seasons keep "-": 51-44, 21-22
    uint64_t a = value(from), b = value(to);
    bool season = from.integer.size() == 2 && b == a + 1;
    bool decades = from.integer.size() == 4 && to.integer.size() == 2 && b > a % 100;
    // 1-59 months, sizes 0-14, July 5-12: rising across a digit, before a plural noun, after a range word, or as days
    // of a month; but bowling figures keep "-": took 3-53
    std::u32string_view tail = match.context.tail();
    bool glued = to.integer.size() < tail.size() &&
                 (unicode::isalpha(tail[to.integer.size()]) || tail[to.integer.size()] == U'-');
    bool longer =
        from.integer.size() < to.integer.size() && b > a && !glued &&
        (is_plural_noun(strings::to_lower(match.context.after(2))) || kRangeWords.contains(match.context.before(2)) ||
         (kMonthWords.contains(match.context.before(2)) && b <= 31));
    return ((from.integer.size() == to.integer.size() && b > a) || decades || longer) && !season ? joiners().range :
                                                                                                   U"-";
  }

  std::optional<std::u32string> plural(std::u32string_view words) const override {
    if (words.ends_with(U'y')) {
      return std::u32string(words.substr(0, words.size() - 1)) + U"ies";
    }
    return std::u32string(words) + (words.ends_with(U'x') ? U"es" : U"s");
  }

private:
  // 5'9": five feet nine inches; 1'27"23: one minute twenty-seven point two three seconds
  std::u32string primes(const Match& match) const {
    static const std::array<Forms, 3> times = {{{U"hour", U"hours"}, {U"minute", U"minutes"}, {U"second", U"seconds"}}};
    static const std::array<Forms, 2> lengths = {{{U"foot", U"feet"}, {U"inch", U"inches"}}};
    size_t first = match.unit == U"h" ? 0 : match.unit == U"min" ? 1 : 2;
    std::vector<std::u32string> words;
    for (size_t i = 0; i < match.numbers.size(); ++i) {
      const Number& number = match.numbers[i];
      const Forms& unit = match.unit == U"ft" ? lengths[i] : times[first + i];
      bool one = number.fraction.empty() && value(number) == 1;
      std::u32string count = number.fraction.empty() ? cardinal(value(number)) : read_number(number, match);
      words.push_back(count + U" " + (one ? unit.one : unit.many));
    }
    return join(words);
  }

  // An integer as a year, a day of a month or a count, digit by digit if it is a code
  std::u32string integer(const Number& number, const Cues& cues) const {
    auto n = parse_value(number.integer);
    // 124,444,234,854,823,834,553: too long to hold, read by its groups
    if (!n && number.grouped && number.integer[0] != U'0' && number.integer.size() <= 3 * (kGroupScales.size() + 1)) {
      return groups(number.integer);
    }
    // 12345: digit by digit, as a code; but 10111 residents, 12345 km
    bool spoken = is_spoken_digits(number, 5) && !(number.integer[0] != U'0' && (cues.counted || cues.amount)) &&
                  !(cues.alone && number.integer[0] != U'0' && number.integer.size() <= 9);
    if (!n || spoken || (cues.spelled && number.integer.size() > 2 && !number.grouped)) {
      return digits(number.integer);
    }
    if (cues.after_month && *n >= 1 && *n <= 31) {
      return ordinal(*n);
    }
    if (cues.before_month && *n >= 1 && *n <= 31 && !number.grouped) {
      return day_of(*n, cues.after_the);
    }
    // 340 AD: a year of three digits
    if (cues.era && number.integer.size() == 3 && !number.grouped && !cues.amount) {
      return pairs(*n);
    }
    // Four digits in pairs, as a year, an address, or a model (1999, 2788 San Tomas, GTX 1080), unless they count
    if (number.integer.size() == 4 && !number.grouped && !cues.amount) {
      return cues.counted ? cardinal(*n) : pairs(*n);
    }
    return cardinal(*n);
  }

  // A long grouped number by its groups of three digits: one hundred twenty-four quintillion ...
  static std::u32string groups(std::u32string_view digits) {
    std::vector<std::u32string> words;
    size_t first = digits.size() % 3 ? digits.size() % 3 : 3, count = (digits.size() - first) / 3;
    for (size_t start = 0, index = count; start < digits.size(); index--) {
      size_t length = start == 0 ? first : 3;
      uint64_t group = parse_value(digits.substr(start, length)).value_or(0);
      if (group) {
        words.push_back(cardinal(group) + (index ? U" " + kGroupScales[index - 1] : U""));
      }
      start += length;
    }
    return join(words);
  }
};
} // namespace

const Language& language() {
  static const English language;
  return language;
}

} // namespace verbalis::languages::en
