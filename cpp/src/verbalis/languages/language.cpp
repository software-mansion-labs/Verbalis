#include "language.h"

#include <verbalis/numbers/scanning.h>
#include <verbalis/text/scan.h>
#include <verbalis/text/unicode.h>

#include <algorithm>

namespace verbalis::languages {

using text::Context;
using text::join;
using numbers::code_parts;
namespace unicode = text::unicode;

PatternLanguage::PatternLanguage(Tables tables, const Notation& notation, Joiners joiners)
    : Language(tables), joiners_(std::move(joiners)) {
  for (auto& pattern : numbers::generic_patterns(notation)) {
    patterns_.push_back({std::move(pattern), true});
  }
}

void PatternLanguage::add_pattern(std::unique_ptr<Pattern> pattern) {
  // After every pattern of an earlier kind, and every pattern of its own kind which is not generic
  Kind kind = pattern->kind();
  auto it = std::ranges::find_if(patterns_, [kind](const Entry& entry) {
    return entry.pattern->kind() > kind || (entry.pattern->kind() == kind && entry.generic);
  });
  patterns_.insert(it, {std::move(pattern), false});
}

void PatternLanguage::read_numbers(Document& document) const {
  // A draft rewritten by each pattern in turn: later patterns see the words of earlier ones
  Document draft(document.text());

  for (const auto& [pattern, generic] : patterns_) {
    std::u32string_view text = draft.text();

    for (size_t position = 0; position < text.size();) {
      auto match = pattern->match(text, position);
      if (!match) {
        position++;
        continue;
      }
      match->text = text.substr(match->span.begin, match->span.end - match->span.begin);
      match->context = Context(text, match->span);

      if (auto words = read_match(*match); words && *words != match->text) {
        draft.replace(match->span, std::move(*words));
      }
      position = match->span.end;
    }
    draft.commit();
  }

  // Each change of the draft is an edit of the document
  for (auto& [span, words] : draft.changes()) {
    document.replace(span, std::move(words));
  }
}

std::optional<std::u32string> PatternLanguage::read_match(const Match& match) const {
  switch (match.kind) {
    case Kind::DATE:
      return read_date(match);
    case Kind::INTERNATIONAL_PHONE:
      return read_international_phone(match);
    case Kind::PHONE:
      return read_phone(match);
    case Kind::VERSION:
      return read_version(match);
    case Kind::DIFFERENCE:
    case Kind::SUM:
    case Kind::RANGE:
    case Kind::DIMENSION:
    case Kind::SIGN:
    case Kind::SCORE:
      return read_joiner(match);
    case Kind::FRACTION:
      return read_fraction(match);
    case Kind::TIME:
      return read_time(match);
    case Kind::MONEY:
      return read_amount(match, &PatternLanguage::read_money);
    case Kind::SCALE:
      return read_amount(match, &PatternLanguage::read_scale);
    case Kind::PERCENT:
      return read_amount(match, &PatternLanguage::read_percent);
    case Kind::UNIT:
      return read_amount(match, &PatternLanguage::read_unit);
    case Kind::ORDINAL:
      return read_ordinal(match);
    case Kind::DECADE:
      return read_decade(match);
    case Kind::COMPOUND:
      return read_compound(match);
    case Kind::CODE:
      return read_code(match);
    case Kind::NUMBER:
      return read_plain(match);
  }
  return std::nullopt;
}

// ----------------------------
// ----- Default readings -----
// ----------------------------

std::optional<std::u32string> PatternLanguage::read_date(const Match& match) const {
  return read_each(match);
}

std::optional<std::u32string> PatternLanguage::read_phone(const Match& match) const {
  std::vector<std::u32string> words;
  for (const auto& group : match.numbers) {
    words.push_back(read_code_digits(group.integer));
  }
  return join(words);
}

std::optional<std::u32string> PatternLanguage::read_fraction(const Match& match) const {
  return read_each(match);
}

std::optional<std::u32string> PatternLanguage::read_time(const Match& match) const {
  return read_each(match);
}

std::optional<std::u32string> PatternLanguage::read_ordinal(const Match& match) const {
  return read_number(match.numbers[0], match);
}

std::optional<std::u32string> PatternLanguage::read_decade(const Match& match) const {
  return read_number(match.numbers[0], match);
}

std::optional<std::u32string> PatternLanguage::read_compound(const Match& match) const {
  return read_number(match.numbers[0], match) + U"-" + std::u32string(match.suffix);
}

std::optional<std::u32string> PatternLanguage::read_plain(const Match& match) const {
  return read_number(match.numbers[0], match);
}

std::optional<std::u32string> PatternLanguage::read_money(const Number& amount, const Match& match) const {
  return with_word(amount, match, match.unit);
}

std::optional<std::u32string> PatternLanguage::read_scale(const Number& amount, const Match& match) const {
  return with_word(amount, match, match.scale);
}

std::optional<std::u32string> PatternLanguage::read_percent(const Number& amount, const Match& match) const {
  return with_word(amount, match, U"%");
}

std::optional<std::u32string> PatternLanguage::read_unit(const Number& amount, const Match& match) const {
  return with_word(amount, match, match.unit);
}

std::optional<std::u32string> PatternLanguage::read_joiner(const Match& match) const {
  switch (match.kind) {
    case Kind::DIFFERENCE:
      return U" " + joiners_.minus + U" ";
    case Kind::SUM:
      return U" " + joiners_.plus + U" ";
    case Kind::RANGE:
      return read_range(match.numbers[0], match.numbers[1], match);
    case Kind::DIMENSION:
      return U" " + joiners_.by + U" ";
    case Kind::SIGN:
      return joiners_.minus + U" ";
    case Kind::SCORE:
      return joiners_.score;
    default:
      return std::nullopt; // Not a joiner
  }
}

std::u32string PatternLanguage::read_range(const Number& /*from*/, const Number& /*to*/, const Match& /*match*/) const {
  return joiners_.range;
}

// ---------------------------
// ----- Shared readings -----
// ---------------------------

std::u32string PatternLanguage::read_each(const Match& match, std::u32string_view separator) const {
  std::vector<std::u32string> words;
  for (const auto& number : match.numbers) {
    words.push_back(read_number(number, match));
  }
  return join(words, separator);
}

std::u32string PatternLanguage::read_version(const Match& match) const {
  // 1.2.3.4 is an address, read with dots; 1.2.3 a version, read with points
  return read_each(match, U" " + (match.numbers.size() == 4 ? joiners_.dot : joiners_.point) + U" ");
}

std::u32string PatternLanguage::with_word(const Number& amount, const Match& match, std::u32string_view word) const {
  return read_number(amount, match) + U" " + std::u32string(word);
}

std::optional<std::u32string> PatternLanguage::read_international_phone(const Match& match) const {
  // plus forty-eight, then the groups as the language reads a phone number
  Match groups = match;
  groups.numbers.erase(groups.numbers.begin());
  groups.text = match.text.substr(groups.numbers.front().span.begin - match.span.begin);
  auto words = read_phone(groups);
  if (!words) {
    return std::nullopt;
  }
  return joiners_.plus + U" " + read_code_digits(match.numbers[0].integer) + U" " + *words;
}

std::u32string PatternLanguage::read_code(const Match& match) const {
  // Letters stay, digit runs read as numbers: MP3, COVID-19, L'A380, F-16s
  std::vector<std::u32string> out;
  bool glue = false;
  std::u32string_view previous;

  for (auto part : code_parts(match.text)) {
    if (part == U"'" && !out.empty()) {
      out.back() += part; // L'A380
      glue = true;
      continue;
    }

    std::optional<std::u32string> word;
    if (part == U"-" || part == U".") {
      word = std::nullopt;
    } else if (unicode::isdigit(part.front()) && part.find(U'.') == std::u32string_view::npos) {
      word = read_code_digits(part);
    } else if (part.front() == U'.' && !out.empty()) {
      word = read_code_digits(part.substr(1)); // H.264
    } else if (unicode::isdigit(part.back())) {
      // 2.5 inside a code, or .5 starting it
      size_t dot = part.find(U'.');
      Number number{.integer = std::u32string(part.substr(0, dot)),
                    .fraction = std::u32string(part.substr(dot + 1)),
                    .decimal_mark = U'.'};
      word = read_number(number, match);
    } else if (bool after_digits = !previous.empty() && unicode::isdigit(previous.back());
               auto words = part == U"s" && after_digits ? plural(out.back()) : std::nullopt) {
      out.back() = *words; // F-16s
    } else {
      word = std::u32string(part);
    }

    if (word) {
      if (glue && !out.empty()) {
        out.back() += *word;
      } else {
        out.push_back(*word);
      }
    }
    glue = false;
    previous = part;
  }
  return join(out);
}

std::optional<std::u32string> PatternLanguage::read_amount(const Match& match, Amount reading) const {
  Number last = match.numbers.back();

  // A unit reads its number without leading zeros: 05 km
  while (match.kind == Kind::UNIT && last.integer.size() > 1 && last.integer[0] == U'0') {
    last.integer.erase(0, 1);
  }

  auto words = (this->*reading)(last, match);
  if (!words || match.numbers.size() == 1) {
    return words;
  }
  return read_number(match.numbers[0], match) + read_range(match.numbers[0], last, match) + *words;
}

} // namespace verbalis::languages
