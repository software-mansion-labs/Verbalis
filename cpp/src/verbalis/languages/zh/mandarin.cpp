#include "constants.h"

#include <verbalis/languages/language.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace verbalis::languages::zh {

namespace {
using namespace text;

bool is_digit(char32_t c) {
  return (c >= U'0' && c <= U'9') || (c >= U'０' && c <= U'９');
}

bool is_letter(char32_t c) {
  return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
}

int digit_value(char32_t c) {
  return c <= U'9' ? static_cast<int>(c - U'0') : static_cast<int>(c - U'０');
}

// The character at i, or 0 past the end.
char32_t at(std::u32string_view s, size_t i) {
  return i < s.size() ? s[i] : 0;
}

char32_t before(std::u32string_view s, size_t i) {
  return i > 0 ? s[i - 1] : 0;
}

bool starts_with(std::u32string_view s, size_t i, std::u32string_view word) {
  return i <= s.size() && s.substr(i).starts_with(word);
}

bool is_one_of(char32_t c, std::u32string_view characters) {
  return c != 0 && characters.find(c) != std::u32string_view::npos;
}

// A number can start at i: no letter, digit, point, or comma after a digit is written right before it.
bool free_before(std::u32string_view s, size_t i) {
  char32_t c = before(s, i);
  return !is_digit(c) && !is_letter(c) && c != U'.' && !(c == U',' && is_digit(before(s, i - 1)));
}

// Length of the digit run starting at i.
size_t digits_at(std::u32string_view s, size_t i) {
  size_t j = i;
  while (j < s.size() && is_digit(s[j])) {
    j++;
  }
  return j - i;
}

// Length of a number starting at i, with groups of thousands and decimals ("1,234.5"), or 0.
size_t number_at(std::u32string_view s, size_t i) {
  size_t n = digits_at(s, i);
  if (n == 0) {
    return 0;
  }
  size_t j = i + n;
  if (n <= 3) {
    while (at(s, j) == U',' && digits_at(s, j + 1) == 3) {
      j += 4;
    }
  }
  if (at(s, j) == U'.') {
    if (size_t fraction = digits_at(s, j + 1); fraction > 0) {
      j += 1 + fraction;
    }
  }
  return j - i;
}

// The value of a short run of digits.
uint64_t value_of(std::u32string_view digits) {
  uint64_t value = 0;
  for (char32_t c : digits) {
    if (is_digit(c)) {
      value = value * 10 + static_cast<uint64_t>(digit_value(c));
    }
  }
  return value;
}

// Length of the longest word of a table starting at i, or 0.
template <typename Table, typename Word>
size_t longest_at(std::u32string_view s, size_t i, const Table& table, Word word) {
  size_t length = 0;
  for (const auto& entry : table) {
    std::u32string_view w = word(entry);
    if (w.size() > length && starts_with(s, i, w)) {
      length = w.size();
    }
  }
  return length;
}

// Reads the digits of a number without its sign and decimal part, e.g. "1024" -> "一千零二十四", by groups of four
// digits: a zero between digits is read once, and a leading 2 of thousands or of the first group is 两 (两千, 两万).
std::optional<std::u32string> integer_to_words(std::u32string_view digits) {
  size_t first = 0;
  while (first + 1 < digits.size() && digit_value(digits[first]) == 0) {
    first++;
  }
  digits = digits.substr(first);
  if (digits.size() > 4 * kGroupUnits.size()) {
    return std::nullopt;
  }

  std::u32string words;
  size_t groups = (digits.size() + 3) / 4;
  bool zero = false; // A zero is pending: an empty group, or the zeros at the start of a group
  for (size_t g = 0; g < groups; g++) {
    size_t end = digits.size() - 4 * (groups - 1 - g);
    std::u32string_view group = digits.substr(g == 0 ? 0 : end - 4, g == 0 ? end : 4);
    if (value_of(group) == 0) {
      zero = !words.empty();
      continue;
    }
    for (size_t k = 0; k < group.size(); k++) {
      int digit = digit_value(group[k]);
      size_t power = group.size() - 1 - k;
      if (digit == 0) {
        zero = !words.empty();
        continue;
      }
      if (zero) {
        words += kDigits[0];
        zero = false;
      }
      bool two = digit == 2 && (power == 3 || (g == 0 && group.size() == 1 && groups > 1));
      words += two ? kTwo : std::u32string(1, kDigits[static_cast<size_t>(digit)]);
      words += kDigitUnits[power];
    }
    words += kGroupUnits[groups - 1 - g];
    zero = false;
  }

  if (words.starts_with(U"一十")) {
    words.erase(0, 1);
  }
  return words.empty() ? std::u32string(1, kDigits[0]) : words;
}

// Reads a number such as "-1,012.5" ("负一千零一十二点五"); nullopt if its integer part has more than 16 digits.
std::optional<std::u32string> to_cardinal(std::u32string_view number) {
  std::u32string sign;
  if (number.starts_with(U'-')) {
    sign = kNegative;
    number.remove_prefix(1);
  }

  size_t point = number.find(U'.');
  std::u32string integer;
  for (char32_t c : number.substr(0, point)) {
    if (c != U',') {
      integer += c;
    }
  }
  auto words = integer_to_words(integer);
  if (!words) {
    return std::nullopt;
  }

  *words = sign + *words;
  if (point != std::u32string_view::npos && point + 1 < number.size()) {
    *words += kPoint;
    for (char32_t c : number.substr(point + 1, kMaxDecimals)) {
      *words += kDigits[static_cast<size_t>(digit_value(c))];
    }
  }
  return words;
}

// A count before a measure: a lone 2 is 两 ("两美元").
std::optional<std::u32string> to_count(std::u32string_view number) {
  return number.size() == 1 && digit_value(number[0]) == 2 ? std::optional(kTwo) : to_cardinal(number);
}

// Reads a number digit by digit, such as a year ("2024" -> "二零二四").
std::u32string to_digits(std::u32string_view number) {
  std::u32string words;
  for (char32_t c : number) {
    if (c == U'-') {
      words += kNegative;
    } else if (c == U'.') {
      words += kPoint;
    } else if (is_digit(c)) {
      words += kDigits[static_cast<size_t>(digit_value(c))];
    }
  }
  return words;
}

// A match at a position: its length, and the words in its place (nullopt to keep it as written).
struct Reading {
  size_t length;
  std::optional<std::u32string> words;
};

// One pass: replaces every reading left to right, like re.sub(). `read` returns the reading at a position, if any.
template <typename Read>
void substitute(Document& draft, Read read) {
  std::u32string_view text = draft.text();
  for (size_t i = 0; i < text.size();) {
    auto reading = read(text, i);
    if (!reading) {
      i++;
      continue;
    }
    if (reading->words) {
      draft.replace({i, i + reading->length}, std::move(*reading->words));
    }
    i += reading->length;
  }
  draft.commit();
}

// ----- Dates -----

bool is_year(std::u32string_view digits, uint64_t from = 1000) {
  uint64_t value = value_of(digits);
  return digits.size() == 4 && value >= from && value <= 2099;
}

bool is_month_day(uint64_t month, uint64_t day) {
  return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

std::u32string date_words(std::u32string_view year, std::u32string_view month, std::u32string_view day) {
  std::u32string words;
  if (!year.empty()) {
    words += to_digits(year) + kYear;
  }
  if (!month.empty()) {
    words += *to_cardinal(month) + kMonth;
  }
  if (!day.empty()) {
    words += *to_cardinal(day) + kDay;
  }
  return words;
}

// "2008-08-08", "08/2008", "08.08": digit groups with one separator.
std::optional<Reading> read_numeric_date(std::u32string_view s, size_t i) {
  std::vector<std::u32string_view> groups = {s.substr(i, digits_at(s, i))};
  size_t j = i + groups[0].size();
  char32_t separator = at(s, j);
  if (!is_one_of(separator, U"-/.·")) {
    return std::nullopt;
  }
  while (groups.size() < 4 && at(s, j) == separator && digits_at(s, j + 1) > 0) {
    groups.push_back(s.substr(j + 1, digits_at(s, j + 1)));
    j += 1 + groups.back().size();
  }
  if (groups.size() < 2 || groups.size() > 3 || (at(s, j) == U'.' && is_digit(at(s, j + 1)))) {
    return std::nullopt;
  }

  bool dotted = separator == U'.';
  auto words = [&](std::u32string_view year, std::u32string_view month, std::u32string_view day) {
    return Reading{j - i, date_words(year, month, day)};
  };
  if (groups.size() == 3) {
    const auto& [a, b, c] = std::tie(groups[0], groups[1], groups[2]);
    if ((is_year(a) || (a.size() == 2 && (dotted || separator == U'/'))) && b.size() <= 2 && c.size() <= 2 &&
        is_month_day(value_of(b), value_of(c))) {
      return words(a, b, c);
    }
    if (is_year(c) && a.size() <= 2 && b.size() <= 2) {
      bool day_first = value_of(a) > 12;
      if (is_month_day(value_of(day_first ? b : a), value_of(day_first ? a : b))) {
        return words(c, day_first ? b : a, day_first ? a : b);
      }
    }
    return std::nullopt;
  }

  const auto& [a, b] = std::tie(groups[0], groups[1]);
  if (is_year(a, 1900) && (b.size() == 2 || (b.size() == 1 && !dotted)) && is_month_day(value_of(b), 1) &&
      (dotted || value_of(b) != (value_of(a) + 1) % 100) && !is_one_of(at(s, j), U"万亿萬億元%％倍")) {
    return words(a, b, U"");
  }
  if (is_year(b, 1900) && (a.size() == 2 || (a.size() == 1 && !dotted)) && is_month_day(value_of(a), 1)) {
    return words(b, a, U"");
  }
  if (a.size() == 2 && b.size() == 2 && (a[0] == U'0' || b[0] == U'0') && is_month_day(value_of(a), value_of(b))) {
    return words(U"", a, b);
  }
  // (24/10): a day which can't be a month, then the month, in parentheses
  bool parenthesized = i > 0 && is_one_of(s[i - 1], U"(（") && is_one_of(at(s, j), U")）");
  if (parenthesized && separator == U'/' && value_of(a) > 12 && is_month_day(value_of(b), value_of(a))) {
    return words(U"", b, a);
  }
  return std::nullopt;
}

// An era after a year: "ad", "a.d.", "bc", "b.c.", in any case. Returns its length and whether it is before Christ.
std::optional<std::pair<size_t, bool>> era_at(std::u32string_view s, size_t i) {
  for (auto [era, bc] : {std::pair{U"a.d.", false}, {U"b.c.", true}, {U"ad", false}, {U"bc", true}}) {
    std::u32string_view word = era;
    bool found = i + word.size() <= s.size();
    for (size_t k = 0; found && k < word.size(); k++) {
      found = (s[i + k] | 0x20) == word[k];
    }
    if (found && !is_letter(at(s, i + word.size()))) {
      return std::pair{word.size(), bc};
    }
  }
  return std::nullopt;
}

// "2024年5月1日": years digit by digit (a short year of years as a count), months and days as numbers.
std::optional<Reading> read_written_date(std::u32string_view s, size_t i) {
  constexpr size_t kNone = std::u32string_view::npos;
  // The position of a marker after a number ending at k, maybe after a space: "2020 年"
  auto marker = [&](size_t k, std::u32string_view markers) {
    size_t m = at(s, k) == U' ' ? k + 1 : k;
    return is_one_of(at(s, m), markers) ? m : kNone;
  };
  auto table = [](const auto& w) -> std::u32string_view { return w; };

  std::u32string words;
  size_t j = i;
  size_t n = digits_at(s, i);
  size_t to = digits_at(s, i + n + 1);
  if (size_t year = marker(i + 5 + to, U"年");
      n == 4 && is_one_of(at(s, i + 4), U"-~～－—到至") && (to == 2 || to == 4) && year != kNone) {
    // "2020-2021年", "2009到2012年"
    words = to_digits(s.substr(i, 4)) + kRangeTo + to_digits(s.substr(i + 5, to)) + kYear;
    j = year + 1;
  } else if (year = marker(i + n, U"年"); n >= 2 && n <= 4 && year != kNone) {
    // A year of two digits is a count of years ("20年后") unless a month follows or it starts with 0; one of three
    // digits is a year unless it is round and a word of time follows ("700年前"); one of four digits is a year unless
    // it is past 2099 outside an era ("5000年"), or a round thousand before a word of time ("1000年前")
    j = year + 1;
    size_t k = at(s, j) == U' ' ? j + 1 : j;
    size_t month = digits_at(s, k);
    uint64_t value = value_of(s.substr(i, n));
    bool count = longest_at(s, j, kYearCounts, table) > 0 && !starts_with(s, j, U"前后") && !starts_with(s, j, U"前後");
    bool era = s.substr(0, i).ends_with(kAnnoDomini) || s.substr(0, i).ends_with(kBeforeChrist);
    bool digits = (n == 4 && (value <= 2099 || era) && !(count && value % 1000 == 0)) ||
                  (n == 2 && (s[i] == U'0' || (month >= 1 && month <= 2 && marker(k + month, U"月") != kNone))) ||
                  (n == 3 && !(count && value % 10 == 0));
    words = (digits ? to_digits(s.substr(i, n)) : *to_cardinal(s.substr(i, n))) + kYear;
  } else if (n == 4 && is_year(s.substr(i, 4), 1900)) {
    // "2020款", and a year cited after a name or in brackets: "Price 2008,", "(2003)"
    char32_t next = at(s, i + 4);
    bool cited = (before(s, i) == U'(' || (before(s, i) == U' ' && is_letter(before(s, i - 1)))) && next <= 0x7F &&
                 !is_one_of(next, U"-~/");
    if (cited || longest_at(s, i + 4, kYearWords, table) > 0) {
      return Reading{4, to_digits(s.substr(i, 4))};
    }
  }
  for (std::u32string_view markers : {U"月", U"日号號"}) {
    size_t k = j > i && at(s, j) == U' ' ? j + 1 : j;
    size_t m = digits_at(s, k);
    if (size_t p = m >= 1 && m <= 2 ? marker(k + m, markers) : kNone; p != kNone) {
      words += *to_cardinal(s.substr(k, m)) + s[p];
      j = p + 1;
    }
  }

  // "2934bc" -> "公元前二九三四年"
  if (j == i && n <= 4) {
    if (auto era = era_at(s, i + n)) {
      return Reading{n + era->first, (era->second ? kBeforeChrist : kAnnoDomini) + to_digits(s.substr(i, n)) + kYear};
    }
    return std::nullopt;
  }
  if (j == i) {
    return std::nullopt;
  }
  if (auto era = era_at(s, j)) {
    return Reading{j + era->first - i, (era->second ? kBeforeChrist : kAnnoDomini) + words};
  }
  return Reading{j - i, words};
}

std::optional<Reading> read_date(std::u32string_view s, size_t i) {
  bool after_date = before(s, i) == U'-' && is_one_of(before(s, i - 1), kDateMarkers);
  if (!is_digit(at(s, i)) || !free_before(s, i) || (is_one_of(before(s, i), U"-/:") && !after_date)) {
    return std::nullopt;
  }
  if (auto reading = read_numeric_date(s, i)) {
    return reading;
  }
  return read_written_date(s, i);
}

// ----- Times and ratios -----

struct Clock {
  size_t length;
  std::u32string_view hours, minutes, seconds;
};

// "13:10", "13: 10", "13:10:36"
std::optional<Clock> clock_at(std::u32string_view s, size_t i) {
  size_t h = digits_at(s, i);
  if (h == 0 || at(s, i + h) != U':') {
    return std::nullopt;
  }
  size_t j = i + h + 1;
  if (at(s, j) == U' ') {
    j++;
  }
  size_t m = digits_at(s, j);
  if (m == 0) {
    return std::nullopt;
  }
  Clock clock{0, s.substr(i, h), s.substr(j, m), U""};
  j += m;
  if (at(s, j) == U':' && digits_at(s, j + 1) == 2) {
    clock.seconds = s.substr(j + 1, 2);
    j += 3;
  }
  if (at(s, j) == U':' && is_digit(at(s, j + 1))) {
    return std::nullopt;
  }
  clock.length = j - i;
  return clock;
}

// A time, not a ratio such as "1:50,000"
bool is_time(std::u32string_view s, const Clock& clock) {
  return number_at(s, static_cast<size_t>(clock.minutes.data() - s.data())) == clock.minutes.size() &&
         clock.hours.size() <= 2 && value_of(clock.hours) <= 24 && clock.minutes.size() == 2 &&
         value_of(clock.minutes) <= 59 && (clock.seconds.empty() || value_of(clock.seconds) <= 59);
}

std::u32string time_words(const Clock& clock) {
  uint64_t hours = value_of(clock.hours);
  uint64_t minutes = value_of(clock.minutes);
  std::u32string words = (hours == 2 ? kTwo : *to_cardinal(clock.hours)) + kHour;
  if (minutes > 0 || !clock.seconds.empty()) {
    words +=
        (minutes < 10 && minutes > 0 ? std::u32string(1, kDigits[0]) : U"") + *to_cardinal(clock.minutes) + kMinute;
  }
  if (!clock.seconds.empty()) {
    words += *to_cardinal(clock.seconds) + kSecond;
  }
  return words;
}

// "am", "a.m.", "pm", "p.m.", in any case. Returns its length and whether it is after noon.
std::optional<std::pair<size_t, bool>> meridiem_at(std::u32string_view s, size_t i) {
  for (auto [meridiem, pm] : {std::pair{U"a.m.", false}, {U"p.m.", true}, {U"am", false}, {U"pm", true}}) {
    std::u32string_view word = meridiem;
    bool found = i + word.size() <= s.size();
    for (size_t k = 0; found && k < word.size(); k++) {
      found = (s[i + k] | 0x20) == word[k];
    }
    if (found && !is_letter(at(s, i + word.size()))) {
      return std::pair{word.size(), pm};
    }
  }
  return std::nullopt;
}

// "13:10:36" -> "十三点十分三十六秒", "1:02pm" -> "下午一点零二分", "8:45-9:00" -> "八点四十五分到九点"; a pair of
// numbers that is no time is a ratio or a score: "2:0" -> "二比零".
std::optional<Reading> read_time(std::u32string_view s, size_t i) {
  if (!is_digit(at(s, i)) || !free_before(s, i) || before(s, i) == U':') {
    return std::nullopt;
  }
  auto clock = clock_at(s, i);
  if (!clock) {
    return std::nullopt;
  }
  size_t j = i + clock->length;
  if (!is_time(s, *clock)) {
    if (!clock->seconds.empty() || clock->hours.size() > 3 || clock->minutes.size() > 3) {
      return std::nullopt;
    }
    auto minutes = static_cast<size_t>(clock->minutes.data() - s.data());
    j = minutes + number_at(s, minutes); // "1:300,000"
    auto second = to_cardinal(s.substr(minutes, j - minutes));
    return second ? std::optional(Reading{j - i, *to_cardinal(clock->hours) + kRatio + *second}) : std::nullopt;
  }

  std::u32string words = time_words(*clock);
  size_t space = at(s, j) == U' ' ? 1 : 0;
  if (auto meridiem = meridiem_at(s, j + space)) {
    uint64_t hours = value_of(clock->hours);
    words = (!meridiem->second ? kMorning : hours < 12 ? kAfternoon : hours == 12 ? kNoon : U"") + words;
    j += space + meridiem->first;
  } else if (is_one_of(at(s, j), U"-~～") && is_digit(at(s, j + 1))) {
    if (auto end = clock_at(s, j + 1); end && is_time(s, *end)) {
      words += kRangeTo + time_words(*end);
      j += 1 + end->length;
    }
  }
  return Reading{j - i, words};
}

// ----- Codes: telephone numbers, addresses, numbers with a leading zero -----

// "415-816-3816", "010-64035547": groups of three digits or more, the last of four or more, and no range of years
// or of round numbers.
size_t phone_at(std::u32string_view s, size_t i) {
  std::vector<std::u32string_view> groups;
  size_t j = i;
  for (size_t n = digits_at(s, j); n >= 3; n = digits_at(s, j + 1)) {
    if (!groups.empty()) {
      j++;
    }
    groups.push_back(s.substr(j, n));
    j += n;
    if (at(s, j) != U'-') {
      break;
    }
  }
  if (groups.size() < 2 || groups.back().size() < 4 || groups.size() > 3 || at(s, j) == U'-' ||
      (at(s, j) == U'.' && is_digit(at(s, j + 1)))) {
    return 0;
  }
  if (groups.size() == 2 && groups[0].size() == 4 && groups[1].size() == 4) {
    char32_t next = at(s, j);
    bool years = is_year(groups[0], 1500) && is_year(groups[1], 1500);
    bool round = value_of(groups[0]) % 100 == 0 && value_of(groups[1]) % 100 == 0;
    if (years || round || (next != 0 && (next > 0x7F || is_letter(next)))) {
      return 0;
    }
  }
  return j - i;
}

std::u32string code_words(std::u32string_view code) {
  std::u32string words;
  for (char32_t c : code) {
    words += c == U'-' ? std::u32string(U" ") : to_digits(std::u32string(1, c));
  }
  return words;
}

// A cue of a table ends at i, maybe followed by a link: "尾号是"
template <typename Cues>
bool cue_before(std::u32string_view s, size_t i, const Cues& cues) {
  auto cue_at = [&](std::u32string_view head) {
    return std::ranges::any_of(cues, [&](const auto& cue) { return head.ends_with(cue); });
  };
  return cue_at(s.substr(0, i)) || std::ranges::any_of(kCueLinks, [&](const auto& link) {
           return s.substr(0, i).ends_with(link) && cue_at(s.substr(0, i - link.size()));
         });
}

std::optional<Reading> read_code(std::u32string_view s, size_t i) {
  // "(310)726-7887"
  if (at(s, i) == U'(' && !is_digit(before(s, i))) {
    size_t n = digits_at(s, i + 1);
    size_t j = i + 2 + n;
    size_t space = at(s, j) == U' ' ? 1 : 0;
    if ((n == 3 || n == 4) && at(s, j - 1) == U')') {
      if (size_t phone = phone_at(s, j + space); phone > 0) {
        return Reading{j + space + phone - i, U"(" + to_digits(s.substr(i + 1, n)) + U")" + (space ? U" " : U"") +
                                                  code_words(s.substr(j + space, phone))};
      }
    }
    return std::nullopt;
  }
  // After one or two capitals, a code: "CA1234", "G2501"
  size_t capitals = 0;
  while (capitals < 3 && i > capitals && at(s, i - capitals - 1) >= U'A' && at(s, i - capitals - 1) <= U'Z') {
    capitals++;
  }
  if (size_t n = digits_at(s, i); capitals >= 1 && capitals <= 2 && !is_letter(before(s, i - capitals)) &&
                                  (n == 3 || n == 4) && !(at(s, i + n) == U'.' && is_digit(at(s, i + n + 1)))) {
    return Reading{n, to_digits(s.substr(i, n))};
  }
  if (!is_digit(at(s, i)) || !free_before(s, i) || is_one_of(before(s, i), U"-/") ||
      (before(s, i) == U':' && is_digit(before(s, i - 1)))) {
    return std::nullopt;
  }
  size_t n = digits_at(s, i);
  char32_t next = at(s, i + n);
  if (size_t phone = phone_at(s, i); phone > 0) {
    return Reading{phone, code_words(s.substr(i, phone))};
  }
  // A mobile number, "13800138000"
  if (n == 11 && at(s, i) == U'1' && digit_value(at(s, i + 1)) >= 3 && !(next == U'.' && is_digit(at(s, i + n + 1)))) {
    return Reading{n, to_digits(s.substr(i, n))};
  }
  // After a word such as 尾号
  if (n >= 3 && cue_before(s, i, kDigitCues)) {
    size_t j = i + n;
    while (at(s, j) == U'-' && digits_at(s, j + 1) > 0) {
      j += 1 + digits_at(s, j + 1);
    }
    return Reading{j - i, code_words(s.substr(i, j - i))};
  }
  // An address or a section, "127.0.0.1", "8.2.4"
  if (next == U'.' && digits_at(s, i + n + 1) > 0) {
    size_t j = i + n;
    size_t groups = 1;
    while (at(s, j) == U'.' && digits_at(s, j + 1) > 0) {
      j += 1 + digits_at(s, j + 1);
      groups++;
    }
    return groups >= 3 ? std::optional(Reading{j - i, to_digits(s.substr(i, j - i))}) : std::nullopt;
  }
  // A leading zero, "007", and a generation, "80后"
  bool generation = n == 2 && digit_value(at(s, i + 1)) == 0 && is_one_of(next, U"后後");
  bool grouped = is_one_of(before(s, i), U" ,") && (is_digit(before(s, i - 1)) || before(s, i - 1) == U',');
  if (n >= 2 && (digit_value(at(s, i)) == 0 || generation) && !grouped &&
      !(next == U',' && digits_at(s, i + n + 1) == 3)) {
    return Reading{n, to_digits(s.substr(i, n))};
  }
  return std::nullopt;
}

// ----- Money, units, and percentages -----

// "$1.5万" -> "一点五万美元"
std::optional<Reading> read_money(std::u32string_view s, size_t i) {
  for (const auto& [symbol, name] : kCurrencies) {
    if (!starts_with(s, i, symbol) || (is_letter(symbol[0]) && is_letter(before(s, i)))) {
      continue;
    }
    size_t j = i + symbol.size();
    if (at(s, j) == U' ') {
      j++;
    }
    size_t n = number_at(s, j);
    if (n == 0) {
      return std::nullopt;
    }
    auto number = to_count(s.substr(j, n));
    if (!number) {
      return std::nullopt;
    }
    size_t scale = longest_at(s, j + n, kScales, [](const auto& w) -> std::u32string_view { return w; });
    // "$500元" names its currency once
    std::u32string currency = is_one_of(at(s, j + n + scale), U"元圓圆块塊") ? U"" : name;
    return Reading{j + n + scale - i, *number + std::u32string(s.substr(j + n, scale)) + currency};
  }
  return std::nullopt;
}

// The unit starting at i, if a letter or digit doesn't continue it.
const Unit* unit_at(std::u32string_view s, size_t i) {
  const Unit* found = nullptr;
  for (const auto& unit : kUnits) {
    if ((!found || unit.symbol.size() > found->symbol.size()) && starts_with(s, i, unit.symbol)) {
      char32_t next = at(s, i + unit.symbol.size());
      if (!is_letter(next) && !is_digit(next)) {
        found = &unit;
      }
    }
  }
  return found;
}

// "25kg" -> "二十五千克", "5-6mm" -> "五到六毫米", "4.4×4.1 cm" -> "四点四乘四点一厘米", "-5%" -> "负百分之五"
std::optional<Reading> read_measure(std::u32string_view s, size_t i) {
  bool negative = at(s, i) == U'-' && !is_digit(before(s, i)) && !is_letter(before(s, i));
  size_t start = negative ? i + 1 : i;
  // After a letter, only a percentage: "GDP10%"
  bool after_letter = !negative && is_letter(before(s, i));
  if (!is_digit(at(s, start)) || (!negative && !after_letter && !free_before(s, i))) {
    return std::nullopt;
  }

  // The numbers of the quantity: one, a range of two, or a product
  std::vector<std::u32string_view> numbers = {s.substr(start, number_at(s, start))};
  size_t j = start + numbers[0].size();
  char32_t joint = 0;
  if (is_one_of(at(s, j), U"-~～×") && number_at(s, j + 1) > 0 && !negative) {
    joint = at(s, j);
    while (at(s, j) == joint && number_at(s, j + 1) > 0 && (joint == U'×' || numbers.size() < 2)) {
      numbers.push_back(s.substr(j + 1, number_at(s, j + 1)));
      j += 1 + numbers.back().size();
    }
  }

  auto unit_after = [&](size_t end) -> std::pair<const Unit*, size_t> {
    if (const Unit* unit = unit_at(s, end)) {
      return {unit, end + unit->symbol.size()};
    }
    if (at(s, end) == U' ') {
      // A unit of one letter is a word after a space: "230 W Cermak Road"
      if (const Unit* unit = unit_at(s, end + 1); unit && !(unit->symbol.size() == 1 && is_letter(unit->symbol[0]))) {
        return {unit, end + 1 + unit->symbol.size()};
      }
    }
    return {nullptr, end};
  };
  auto [unit, end] = unit_after(j);
  if (!unit && numbers.size() > 1) {
    // Only the first number has a unit: "5-6" before nothing is a range for a later pass
    numbers.resize(1);
    std::tie(unit, end) = unit_after(start + numbers[0].size());
  }
  if (!unit || (after_letter && !unit->word.empty())) {
    return std::nullopt;
  }

  auto quantity = [&](std::u32string_view number) { return unit->counted ? to_count(number) : to_cardinal(number); };
  std::u32string words = negative ? std::u32string(1, kNegative) : U"";
  words += unit->per;
  for (size_t k = 0; k < numbers.size(); k++) {
    auto number = quantity(numbers[k]);
    if (!number) {
      return std::nullopt;
    }
    if (k > 0) {
      words += joint == U'×' ? U"乘" : kRangeTo;
    }
    words += *number;
  }
  words += unit->word;

  // "10%-20%" -> "百分之十到百分之二十"
  if (numbers.size() == 1 && is_one_of(at(s, end), U"-~～")) {
    size_t n = number_at(s, end + 1);
    if (auto number = quantity(s.substr(end + 1, n)); n > 0 && number && unit_at(s, end + 1 + n) == unit) {
      words += kRangeTo + unit->per + *number + unit->word;
      end += 1 + n + unit->symbol.size();
    }
  }
  return Reading{end - i, words};
}

// ----- Expressions, fractions, ranges -----

// "1+2=3" -> "一加二等于三"; a minus counts only in an equation.
std::optional<Reading> read_math(std::u32string_view s, size_t i) {
  size_t start = at(s, i) == U'-' && !is_digit(before(s, i)) && !is_letter(before(s, i)) ? i + 1 : i;
  if (!is_digit(at(s, start)) || !free_before(s, i)) {
    return std::nullopt;
  }
  size_t j = start + number_at(s, start);
  auto words = to_cardinal(s.substr(i, j - i));
  // Each operation: its symbol, its end, and the words up to it
  std::vector<std::tuple<std::u32string_view, size_t, std::u32string>> operations;
  while (words) {
    size_t k = at(s, j) == U' ' ? j + 1 : j;
    auto op = std::ranges::max_element(
        kOperators, {}, [&](const auto& o) { return starts_with(s, k, o.symbol) ? o.symbol.size() : 0; });
    if (!starts_with(s, k, op->symbol)) {
      break;
    }
    k += op->symbol.size();
    if (at(s, k) == U' ') {
      k++;
    }
    size_t n = number_at(s, k);
    auto operand = to_cardinal(s.substr(k, n));
    if (n == 0 || !operand) {
      break;
    }
    j = k + n;
    *words += op->word + *operand;
    operations.emplace_back(op->symbol, j, *words);
  }
  // Without an equals sign, a minus is a dash: keep the operations before the first one
  if (std::ranges::none_of(operations, [](const auto& o) { return std::get<0>(o) == U"="; })) {
    operations.erase(std::ranges::find_if(operations, [](const auto& o) { return std::get<0>(o) == U"-"; }),
                     operations.end());
  }
  if (operations.empty()) {
    return std::nullopt;
  }
  return Reading{std::get<1>(operations.back()) - i, std::get<2>(operations.back())};
}

// "1/3" -> "三分之一", "-1/3" -> "负三分之一", "2009/10" -> "二零零九到一零"
std::optional<Reading> read_fraction(std::u32string_view s, size_t i) {
  bool negative = at(s, i) == U'-' && !is_digit(before(s, i)) && !is_letter(before(s, i));
  size_t start = negative ? i + 1 : i;
  if (!is_digit(at(s, start)) || !free_before(s, i) || before(s, i) == U'/') {
    return std::nullopt;
  }
  size_t a = digits_at(s, start);
  size_t j = start + a;
  bool spaced = at(s, j) == U' ';
  if (at(s, j + spaced) != U'/' || (spaced && at(s, j + 2) != U' ')) {
    return std::nullopt;
  }
  j += spaced ? 3 : 1;
  size_t b = digits_at(s, j);
  if (b == 0 || (at(s, j + b) == U'/' && is_digit(at(s, j + b + 1)))) {
    return std::nullopt;
  }
  auto numerator = s.substr(start, a);
  auto denominator = s.substr(j, b);
  uint64_t first = value_of(numerator);
  uint64_t second = value_of(denominator);
  if (!negative && !spaced && is_year(numerator, 1500) &&
      ((b == 2 && second == (first + 1) % 100) || (b == 4 && second == first + 1))) {
    return Reading{j + b - i, to_digits(numerator) + kRangeTo + to_digits(denominator)};
  }
  auto top = to_cardinal(numerator);
  auto bottom = to_cardinal(denominator);
  if (!top || !bottom) {
    return Reading{j + b - i, std::nullopt};
  }
  return Reading{j + b - i, (negative ? std::u32string(1, kNegative) : U"") + *bottom + kFraction + *top};
}

// "3-5" -> "三到五", "1995-2002" -> "一九九五到二零零二"; a second number below the first is no range.
std::optional<Reading> read_range(std::u32string_view s, size_t i) {
  if (!is_digit(at(s, i)) || !free_before(s, i) || before(s, i) == U'-') {
    return std::nullopt;
  }
  size_t a = number_at(s, i);
  size_t j = i + a;
  if (!is_one_of(at(s, j), U"-~～－—")) {
    return std::nullopt;
  }
  size_t b = number_at(s, j + 1);
  size_t end = j + 1 + b;
  if (b == 0 || is_letter(at(s, end)) || (at(s, end) == U'-' && is_digit(at(s, end + 1)))) {
    return std::nullopt;
  }
  auto from = s.substr(i, a);
  auto to = s.substr(j + 1, b);
  uint64_t first = value_of(from.substr(0, from.find(U'.')));
  uint64_t second = value_of(to.substr(0, to.find(U'.')));
  if ((is_year(from, 1500) && is_year(to, 1500) && second > first && !(first % 100 == 0 && second % 100 == 0)) ||
      (is_year(from) && b == 2 && (second > first % 100 || second == (first + 1) % 100))) {
    return Reading{end - i, to_digits(from) + kRangeTo + to_digits(to)};
  }
  // A score: "23-0", "比分为1-2"
  bool address = is_one_of(at(s, end), U"号號室座楼樓") || (b > 1 && to[0] == U'0');
  bool cued = cue_before(s, i, kScoreCues) ||
              longest_at(s, end, kScoreResults, [](const auto& w) -> std::u32string_view { return w; }) > 0;
  if (!address && a <= 3 && b <= 3 && ((second < first && a <= 2 && b <= 2) || cued)) {
    return Reading{end - i, *to_cardinal(from) + kRatio + *to_cardinal(to)};
  }
  if (second < first) {
    return std::nullopt;
  }
  auto low = to_cardinal(from);
  auto high = to_cardinal(to);
  if (!low || !high) {
    return std::nullopt;
  }
  return Reading{end - i, *low + kRangeTo + *high};
}

// "2个" -> "两个"
std::optional<Reading> read_two(std::u32string_view s, size_t i) {
  if (!is_digit(at(s, i)) || digit_value(at(s, i)) != 2 || !free_before(s, i) || is_one_of(before(s, i), U"第-:/") ||
      is_digit(at(s, i + 1)) || (is_one_of(at(s, i + 1), U".,") && is_digit(at(s, i + 2)))) {
    return std::nullopt;
  }
  size_t word = longest_at(s, i + 1, kTwoBefore, [](const auto& w) -> std::u32string_view { return w; });
  if (word == 0 || (s.substr(i + 1, word) == U"年" && is_one_of(at(s, i + 2), U"级級"))) {
    return std::nullopt;
  }
  return Reading{1, kTwo};
}

// Any other number: "-12.5" -> "负十二点五"
std::optional<Reading> read_number(std::u32string_view s, size_t i) {
  if (at(s, i) == U'-' && is_one_of(before(s, i), U"万萬亿億") && is_digit(at(s, i + 1))) {
    return Reading{1, kRangeTo}; // "3万-5万"
  }
  bool sign =
      at(s, i) == U'-' && !is_digit(before(s, i)) && !is_letter(before(s, i)) && !is_one_of(before(s, i), kDateMarkers);
  if (sign && starts_with(s, i + 1, kPercent)) {
    return Reading{1, std::u32string(1, kNegative)}; // "-百分之1"
  }
  size_t start = sign ? i + 1 : i;
  size_t n = number_at(s, start);
  if (n == 0) {
    return std::nullopt;
  }
  return Reading{start + n - i, to_cardinal(s.substr(i, start + n - i))};
}

// --------------------
// ----- Language -----
// --------------------

/**
 * Spells out Arabic numbers in Chinese, after cn2an.transform(text, "an2cn"). In order, it reads dates (years digit
 * by digit), times and scores, codes (telephone numbers, addresses, leading zeros) digit by digit, money, units and
 * percentages, expressions, fractions, ranges, a 2 before a measure word as 两, and plain numbers.
 * Full-width digits count as digits.
 */
class Mandarin final : public Language {
public:
  Mandarin() : Language({.characters = &kCharacters, .symbols = &kSymbols}) {}

  void read_numbers(Document& document) const override {
    // Each pass rewrites a draft in turn: later passes see the words of earlier ones
    Document draft(document.text());
    for (auto read : {read_date, read_time, read_code, read_money, read_measure, read_math, read_fraction, read_range,
                      read_two, read_number}) {
      substitute(draft, read);
    }

    // The changes of the draft are the edits of the document
    for (auto& [span, words] : draft.changes()) {
      document.replace(span, std::move(words));
    }
  }
};
} // namespace

const Language& language() {
  static const Mandarin language;
  return language;
}

} // namespace verbalis::languages::zh
