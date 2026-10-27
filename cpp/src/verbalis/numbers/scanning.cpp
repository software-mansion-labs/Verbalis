#include "scanning.h"

#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>
#include <array>

namespace verbalis::numbers {

using namespace text;


Number digits_at(std::u32string_view text, size_t position) {
  size_t end = digits_end(text, position);
  return {.span = {position, end}, .integer = std::u32string(text.substr(position, end - position))};
}

std::vector<Number> numbers_at(std::u32string_view text, size_t position, std::u32string_view group,
                               std::u32string_view decimal) {
  size_t run = digits_end(text, position) - position;
  if (run == 0) {
    return {};
  }

  // Integer parts: 1-3 digits followed by whole groups (1 000 000), then a plain run of digits
  std::vector<Number> integers;
  if (run <= 3) {
    size_t end = position + run;
    while (end + 3 < text.size() && strings::contains(group, text[end]) && digits_end(text, end + 1) >= end + 4) {
      end += 4;
    }
    // A group may not be followed by a digit, nor by a separator and a digit
    bool complete = end > position + run && !is_digit_after(text, end) &&
                    !(end < text.size() && strings::contains(group, text[end]) && is_digit_after(text, end + 1));
    if (complete) {
      std::u32string digits;
      std::ranges::copy_if(text.substr(position, end - position), std::back_inserter(digits), unicode::isdigit);
      integers.push_back({.span = {position, end}, .integer = digits, .grouped = true});
    }
  }
  integers.push_back(digits_at(text, position));

  // Each with its fraction first, then without it
  std::vector<Number> numbers;
  for (const Number& integer : integers) {
    size_t end = integer.span.end;
    if (end < text.size() && strings::contains(decimal, text[end]) && is_digit_after(text, end + 1)) {
      Number number = integer;
      number.span.end = digits_end(text, end + 1);
      number.fraction = text.substr(end + 1, number.span.end - end - 1);
      number.decimal_mark = text[end];

      // A fraction shaped like a group is one: the 1.980 of 1.980,6
      if (!integer.grouped && integer.integer.size() <= 3 && number.fraction.size() == 3 &&
          strings::contains(group, number.decimal_mark)) {
        number = {.span = number.span, .integer = integer.integer + number.fraction, .grouped = true};
      }
      numbers.push_back(number);
    }
    numbers.push_back(integer);
  }
  return numbers;
}

std::optional<uint64_t> parse_value(std::u32string_view digits) {
  if (digits.empty() || digits.size() > 18) {
    return std::nullopt;
  }

  uint64_t value = 0;
  for (char32_t c : digits) {
    value = value * 10 + static_cast<uint64_t>(c - U'0');
  }
  return value;
}

uint64_t value(const Number& number) {
  return parse_value(number.integer).value_or(0);
}

bool is_one(const Number& number) {
  return number.integer == U"1" && number.fraction.empty();
}

std::optional<uint64_t> code_value(std::u32string_view digits, size_t longest) {
  if ((digits.size() > 1 && digits[0] == U'0') || digits.size() > longest) {
    return std::nullopt;
  }
  return parse_value(digits);
}

bool fits_clock(const Match& time) {
  return time.numbers.size() < 2 || (value(time.numbers[0]) <= 24 && value(time.numbers[1]) <= 59);
}

std::optional<std::u32string> named_fraction(const Fractions& fractions, const Match& fraction) {
  auto it = fractions.find({value(fraction.numbers[0]), value(fraction.numbers[1])});
  if (it == fractions.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::u32string spaced_suffix(const Match& match) {
  return match.suffix.empty() ? U"" : U" " + std::u32string(match.suffix);
}

bool is_spoken_digits(const Number& number, size_t longest) {
  const auto& i = number.integer;
  return (i.size() > 1 && i[0] == U'0') || (i.size() >= longest && !number.grouped && !i.ends_with(U"000"));
}

std::u32string to_digits(uint64_t value) {
  std::u32string digits;
  do {
    digits.insert(digits.begin(), static_cast<char32_t>(U'0' + value % 10));
    value /= 10;
  } while (value > 0);
  return digits;
}

std::optional<uint64_t> roman_value(std::u32string_view numeral) {
  auto digit = [](char32_t c) -> uint64_t {
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
      case U'D':
        return 500;
      case U'M':
        return 1000;
      default:
        return 0;
    }
  };
  if (numeral.empty() || numeral.size() > 15) {
    return std::nullopt;
  }

  int64_t value = 0;
  for (size_t i = 0; i < numeral.size(); ++i) {
    auto d = static_cast<int64_t>(digit(numeral[i]));
    if (d == 0) {
      return std::nullopt;
    }
    value += i + 1 < numeral.size() && d < static_cast<int64_t>(digit(numeral[i + 1])) ? -d : d;
  }

  // Only the canonical spelling of its value: IV, not IIII; XC, not LXL
  static constexpr std::array<std::pair<uint64_t, std::u32string_view>, 13> kNumerals = {
      {{1000, U"M"}, {900, U"CM"}, {500, U"D"}, {400, U"CD"}, {100, U"C"}, {90, U"XC"}, {50, U"L"}, {40, U"XL"},
       {10, U"X"}, {9, U"IX"}, {5, U"V"}, {4, U"IV"}, {1, U"I"}}};
  std::u32string canonical;
  for (auto rest = static_cast<uint64_t>(std::max<int64_t>(value, 0)); const auto& [numeral_value, symbol] : kNumerals) {
    for (; rest >= numeral_value; rest -= numeral_value) {
      canonical += symbol;
    }
  }
  return value > 0 && value < 4000 && canonical == numeral ? std::optional<uint64_t>(value) : std::nullopt;
}

std::optional<RomanRole> roman_role(const Match& match, const RomanCues& cues) {
  // A name is capitalized as written: Henry VIII, but not "the henry viii"
  std::u32string_view head = match.context.head();
  size_t start = head.size();
  while (start > 0 && unicode::isspace(head[start - 1])) {
    start--;
  }
  while (start > 0 && !unicode::isspace(head[start - 1])) {
    start--;
  }
  bool capitalized = start < head.size() && unicode::isupper(head[start]);

  // A name or noun before it belongs to it only with nothing but spaces between: not "na piętrze, II liga"
  size_t last = head.size();
  while (last > 0 && unicode::isspace(head[last - 1])) {
    last--;
  }
  bool adjacent = last > 0 && unicode::isalpha(head[last - 1]);

  // Paul V. Halvorsen: one letter with a dot before a capitalized word is an initial, not a regnal number
  std::u32string_view tail = match.context.tail();
  size_t next = !tail.empty() && tail[0] == U'.' ? 1 : 0;
  while (next < tail.size() && unicode::isspace(tail[next])) {
    next++;
  }
  bool dotted = match.suffix == U"." || (!tail.empty() && tail[0] == U'.');
  const Span& numeral = match.numbers[0].span;
  bool initial = numeral.end - numeral.begin == 1 && dotted && next < tail.size() && unicode::isupper(tail[next]);

  std::u32string previous = match.context.before();
  if (adjacent && capitalized && !initial && cues.names.contains(previous)) {
    return RomanRole::NAME;
  }
  if (adjacent && cues.before.contains(previous)) {
    return RomanRole::BEFORE;
  }
  if (cues.after.contains(strings::to_lower(match.context.after()))) {
    return RomanRole::AFTER;
  }
  return std::nullopt;
}

std::vector<std::u32string_view> code_parts(std::u32string_view code) {
  auto is_mark = [](char32_t c) { return c == U'\'' || c == U'.' || c == U'-'; };

  std::vector<std::u32string_view> parts;
  for (size_t i = 0; i < code.size();) {
    size_t end = i + 1;
    if (unicode::isdigit(code[i])) {
      // 264, 2.5
      end = digits_end(code, i);
      if (end + 1 < code.size() && code[end] == U'.' && unicode::isdigit(code[end + 1])) {
        end = digits_end(code, end + 1);
      }
    } else if (code[i] == U'.' && i + 1 < code.size() && unicode::isdigit(code[i + 1])) {
      // .264
      end = digits_end(code, i + 1);
    } else if (!is_mark(code[i])) {
      while (end < code.size() && !unicode::isdigit(code[end]) && !is_mark(code[end])) {
        end++;
      }
    }
    parts.push_back(code.substr(i, end - i));
    i = end;
  }
  return parts;
}

} // namespace verbalis::numbers
