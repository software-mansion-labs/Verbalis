#include "scanning.h"

#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>
#include <array>
#include <bit>

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
    // Every group after the same separator: 1.000.000, but the 1.234 of 1.234,56
    size_t end = position + run;
    char32_t separator = end < text.size() ? text[end] : 0;
    while (end + 3 < text.size() && text[end] == separator && strings::contains(group, separator) &&
           digits_end(text, end + 1) >= end + 4) {
      end += 4;
    }
    // Indian groups: pairs before the last three digits (3,94,227, 12,34,56,789)
    if (end == position + run && run <= 2) {
      size_t pairs = end;
      while (pairs + 3 < text.size() && text[pairs] == U',' && digits_end(text, pairs + 1) == pairs + 3) {
        pairs += 3;
      }
      if (pairs > end && pairs + 3 < text.size() && text[pairs] == U',' && digits_end(text, pairs + 1) == pairs + 4) {
        end = pairs + 4;
      }
    }
    // A group may not be followed by a digit, nor by its separator and a digit
    bool complete = end > position + run && !is_digit_after(text, end) &&
                    !(end < text.size() && text[end] == separator && is_digit_after(text, end + 1));
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

std::pair<uint64_t, uint64_t> fraction_of(const Match& fraction) {
  size_t n = fraction.numbers.size();
  return {value(fraction.numbers[n - 2]), value(fraction.numbers[n - 1])};
}

std::optional<std::u32string> named_fraction(const Fractions& fractions, const Match& fraction) {
  auto it = fractions.find(fraction_of(fraction));
  if (it == fractions.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool is_ordinal_degree(const Match& unit, const std::unordered_set<std::u32string>& articles,
                       const std::unordered_set<std::u32string>& nouns) {
  static const std::unordered_set<std::u32string> not_names = {
      U"Norte", U"Sur",  U"Sul",   U"Nord",  U"Sud",     U"Este",       U"Est",   U"Leste",
      U"Oeste", U"Oest", U"Ovest", U"Ouest", U"Celsius", U"Fahrenheit", U"Kelvin"};
  if ((unit.unit != U"°" && unit.suffix != U"°") || unit.numbers.size() != 1 || !unit.numbers[0].fraction.empty()) {
    return false;
  }
  std::u32string after = unit.context.after();
  bool name = after.size() >= 3 && unicode::isupper(after[0]) && !not_names.contains(after);
  return name || articles.contains(unit.context.before()) || nouns.contains(strings::to_lower(after));
}

bool is_marked_fraction(const Match& fraction, const std::unordered_set<std::u32string>& words) {
  if (fraction.numbers.size() == 3) {
    return true;
  }
  // An inch has binary fractions: 3/8"
  auto [a, b] = fraction_of(fraction);
  std::u32string_view tail = fraction.context.tail();
  bool inch = tail.starts_with(U'"') || tail.starts_with(U"''") || tail.starts_with(U'″');
  if (inch && b <= 64 && std::has_single_bit(b)) {
    return true;
  }

  // A common denominator, not a year or a number of a series: 1/3 of, but 40/2023 du, 18/20 da
  std::u32string word = strings::to_lower(fraction.context.after());
  if (size_t apostrophe = word.find(U'\''); apostrophe != std::u32string::npos) {
    word.resize(apostrophe + 1);
  }
  bool common = b <= 10 || b == 12 || b == 16 || b == 100 || b == 1000;
  return common && words.contains(word);
}

std::u32string spaced_suffix(const Match& match) {
  return match.suffix.empty() ? U"" : U" " + std::u32string(match.suffix);
}

bool is_spoken_digits(const Number& number, size_t longest) {
  const auto& i = number.integer;
  return (i.size() > 1 && i[0] == U'0') || (i.size() >= longest && !number.grouped && !i.ends_with(U"000"));
}

std::optional<Number> unpadded_day(const Number& number, const Match& match, std::span<const std::u32string> months,
                                   std::u32string_view of) {
  const auto& i = number.integer;
  if (i.size() != 2 || i[0] != U'0' || i[1] == U'0' || !number.fraction.empty()) {
    return std::nullopt;
  }
  std::u32string next = strings::to_lower(match.context.after());
  if (!of.empty() && next == of) {
    next = strings::to_lower(match.context.after(2));
  }
  if (next.empty() || std::ranges::find(months, next) == months.end()) {
    return std::nullopt;
  }
  Number day = number;
  day.integer.erase(0, 1);
  return day;
}

bool is_year_value(uint64_t value, uint64_t from, uint64_t to) {
  return value >= from && value <= to;
}

namespace {
constexpr std::u32string_view kElisions = U"'’´";
} // namespace

std::u32string bare_word(std::u32string_view word) {
  std::u32string lower = strings::to_lower(stripped(word));
  return lower.substr(lower.find_last_of(kElisions) + 1);
}

bool is_score(const Match& range, const Keys& sport_words) {
  if (range.kind != Kind::RANGE) {
    return false;
  }
  const Number& from = range.numbers[0];
  const Number& to = range.numbers[1];
  std::u32string_view head = range.context.head(), tail = range.context.tail();
  size_t start = head.size() - from.integer.size(), end = to.integer.size();

  // Not part of a code, a decimal, or a time: 14-2b, 4,5-5,5, 18:30-20:30
  bool glued_before = start > 0 && (unicode::isalpha(head[start - 1]) || strings::contains(U".,:-", head[start - 1]));
  bool glued_after =
      end < tail.size() && (unicode::isalpha(tail[end]) || (strings::contains(U".,:", tail[end]) &&
                                                            end + 1 < tail.size() && unicode::isdigit(tail[end + 1])));
  if (from.integer.size() > 3 || to.integer.size() > 3 || glued_before || glued_after) {
    return false;
  }
  bool elided = start > 0 && strings::contains(kElisions, head[start - 1]);
  return (value(to) <= value(from) || elided) && mentions(range.context, sport_words);
}

bool mentions(const Context& context, const Keys& stems) {
  auto any = [&](std::u32string_view text) {
    return std::ranges::any_of(words_of(text), [&](std::u32string_view word) {
      std::u32string bare = bare_word(word); // dell'Atalanta, (Sabato)
      return std::ranges::any_of(stems, [&](const auto& stem) { return bare.starts_with(stem); });
    });
  };
  return any(context.head()) || any(context.tail());
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
  static constexpr std::array<std::pair<uint64_t, std::u32string_view>, 13> kNumerals = {{{1000, U"M"},
                                                                                          {900, U"CM"},
                                                                                          {500, U"D"},
                                                                                          {400, U"CD"},
                                                                                          {100, U"C"},
                                                                                          {90, U"XC"},
                                                                                          {50, U"L"},
                                                                                          {40, U"XL"},
                                                                                          {10, U"X"},
                                                                                          {9, U"IX"},
                                                                                          {5, U"V"},
                                                                                          {4, U"IV"},
                                                                                          {1, U"I"}}};
  std::u32string canonical;
  for (auto rest = static_cast<uint64_t>(std::max<int64_t>(value, 0));
       const auto& [numeral_value, symbol] : kNumerals) {
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

size_t double_prime_at(std::u32string_view text, size_t position) {
  if (position < text.size() && strings::contains(kDoublePrimes, text[position])) {
    return 1;
  }
  bool two = position + 1 < text.size() && strings::contains(kPrimes, text[position]) &&
             strings::contains(kPrimes, text[position + 1]);
  return two ? 2 : 0;
}

namespace {
// The quotations open before a mark: curly ones („ “ « … ” ») counted, a straight one toggled.
struct Quotes {
  int curly = 0;
  bool straight = false;
};

// Whether the mark at a position, right after a digit, marks inches or seconds, given the quotations open before it
bool inch_mark_in(std::u32string_view text, size_t position, const Quotes& quotes) {
  size_t next = position + double_prime_at(text, position);
  if (next < text.size() && strings::contains(kClosingQuotes, text[next])) {
    return true; // 55"": inches, then the quotation mark
  }
  // The number before the mark, with its decimals: 10.1"
  size_t start = position;
  while (start > 0 &&
         (unicode::isdigit(text[start - 1]) || ((text[start - 1] == U'.' || text[start - 1] == U',') && start > 1 &&
                                                unicode::isdigit(text[start - 2]) && unicode::isdigit(text[start])))) {
    --start;
  }
  size_t feet = start > 0 && unicode::isspace(text[start - 1]) ? start - 1 : start;
  if (feet > 1 && strings::contains(kPrimes, text[feet - 1]) && unicode::isdigit(text[feet - 2])) {
    return true; // 5'5": the inches of feet and inches
  }
  if (quotes.curly > 0 || quotes.straight) {
    return false; // "Redmi 7", „7×7″: the mark closes the quotation
  }
  if (text[position] == U'″') {
    return true;
  }
  // Redmi 7”: a small whole number after a capitalized name (not an acronym: UHD 110'') is a model, and the mark
  // closes a quotation opened before the text
  std::u32string_view digits = text.substr(start, position - start);
  size_t word_end = start;
  while (word_end > 0 && unicode::isspace(text[word_end - 1])) {
    --word_end;
  }
  size_t word_start = word_end;
  while (word_start > 0 && unicode::isalpha(text[word_start - 1])) {
    --word_start;
  }
  std::u32string_view name = text.substr(word_start, word_end - word_start);
  bool named =
      word_end < start && !name.empty() && unicode::isupper(name[0]) && std::ranges::any_of(name, unicode::islower);
  bool small = !digits.empty() && digits.size() <= 2 && std::ranges::all_of(digits, unicode::isdigit) &&
               (digits.size() == 1 || digits[0] == U'1' || digits[0] == U'0');
  return !(named && small);
}

// The quotations open before a position, and whether the mark there (if any) marks inches or seconds
std::pair<Quotes, bool> scan_quotes(std::u32string_view text, size_t position) {
  Quotes quotes;
  for (size_t i = 0; i < position;) {
    char32_t c = text[i];
    size_t length = double_prime_at(text, i);
    bool digit = i > 0 && unicode::isdigit(text[i - 1]);
    if (length > 0 && digit) {
      if (!inch_mark_in(text, i, quotes)) { // „7×7″, "Redmi 7": the mark closes the open quotation
        quotes.curly > 0 ? --quotes.curly : (quotes.straight = false, 0);
      }
      i += length;
      continue;
    }
    if (c == U'„' || c == U'«' || (c == U'“' && quotes.curly == 0)) {
      quotes.curly++;
    } else if ((c == U'“' || c == U'”' || c == U'»' || c == U'″') && quotes.curly > 0) {
      quotes.curly--;
    } else if ((c == U'"' || length == 2) && (!digit || quotes.straight)) {
      quotes.straight = !quotes.straight; // also ``text`` as two apostrophes
    }
    i += length == 2 ? 2 : 1;
  }
  bool inch = position > 0 && position < text.size() && double_prime_at(text, position) > 0 &&
              unicode::isdigit(text[position - 1]) && inch_mark_in(text, position, quotes);
  return {quotes, inch};
}
} // namespace

bool is_inch_mark(std::u32string_view text, size_t position) {
  return scan_quotes(text, position).second;
}

bool in_double_quotation(std::u32string_view text, size_t position) {
  Quotes quotes = scan_quotes(text, position).first;
  return quotes.curly > 0 || quotes.straight;
}

} // namespace verbalis::numbers
