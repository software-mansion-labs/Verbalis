#include "scanning.h"

#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>

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
