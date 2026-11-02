#include "readings.h"
#include "scan.h"
#include "strings.h"
#include "unicode.h"

namespace verbalis::text {


std::optional<std::u32string> read(const std::vector<Reading>& readings, const Context& context) {
  for (const auto& reading : readings) {
    if (!reading.when || reading.when(context)) {
      return reading.words;
    }
  }
  return std::nullopt;
}

bool after_name(const Context& context) {
  // Digits right after the letters count with them: LAB4+, but not 50%+ or HDR 10+
  std::u32string_view head = context.head();
  size_t end = head.size();
  while (end > 0 && unicode::isdigit(head[end - 1])) {
    end--;
  }
  return end >= 2 && unicode::isalpha(head[end - 1]) && unicode::isalpha(head[end - 2]);
}

bool before_number(const Context& context) {
  std::u32string_view tail = context.tail();
  return is_digit_after(tail, skip_space(tail, 0));
}

bool before_value(const Context& context) {
  std::u32string_view head = context.head();
  std::u32string_view tail = context.tail();
  // Not an arrow (->, =>), between numbers (2018~2019, a range), or in a tag or a chain (<sup>2, </i> 10, 100<P<250)
  if (!head.empty() && (unicode::isdigit(head.back()) || head.back() == U'-' || head.back() == U'=')) {
    return false;
  }
  for (size_t i = head.size(); i > 0 && !unicode::isspace(head[i - 1]); --i) {
    if (head[i - 1] == U'<') {
      return false;
    }
  }
  // A space after only without one before: THI< 84, (< 60%), but not "octobre > 9h", "ME ~ 8pm"; a spaced <, ≤, ≥,
  // or ≈ only compares: creatinina < 30
  size_t i = 0;
  if (i < tail.size() && unicode::isspace(tail[i])) {
    if (!head.empty() && unicode::isspace(head.back()) && (context.text() == U">" || context.text() == U"~")) {
      return false;
    }
    i++;
  }
  if (i < tail.size() && strings::contains(std::u32string_view(U"$€£"), tail[i])) {
    i++;
  }
  size_t number = i;
  while (i < tail.size() &&
         (unicode::isdigit(tail[i]) || (i > number && strings::contains(std::u32string_view(U".,"), tail[i]) &&
                                        i + 1 < tail.size() && unicode::isdigit(tail[i + 1])))) {
    i++;
  }
  // Not a tag or a citation (<27>), nor a heart (<3, but x < 3 compares)
  bool closed = i < tail.size() && tail[i] == U'>';
  bool heart = context.text() == U"<" && number == 0 && tail.substr(number, i - number) == U"3";
  return i > number && !closed && !heart;
}

bool before_name(const Context& context) {
  std::u32string word = context.after();
  return !word.empty() && unicode::isupper(word[0]);
}

Condition after_any(std::unordered_set<std::u32string> words) {
  return [words = std::move(words)](const Context& context) { return words.contains(context.before()); };
}

} // namespace verbalis::text
