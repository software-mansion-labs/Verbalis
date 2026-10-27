#include "normalize.h"

#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

namespace verbalis::steps {

using namespace text;


namespace {
// Whether a character is a dash or a hyphen
bool is_dash(char32_t c) {
  return c == U'-' || strings::contains(kDashes, c);
}

// The end of a dash starting at a position: a dash character or two hyphens and more (--); the position if none
size_t dash_end(std::u32string_view text, size_t position) {
  if (position < text.size() && strings::contains(kDashes, text[position])) {
    return position + 1;
  }
  size_t end = position;
  while (end < text.size() && text[end] == U'-') {
    end++;
  }
  return end - position >= 2 ? end : position;
}

// A character lost in decoding, which says nothing: the replacement character, also as UTF-8 read as Latin-1
// (dï¿½clarant -> dclarant)
std::optional<Document::Edit> read_lost(std::u32string_view text, size_t position) {
  for (std::u32string_view lost : {std::u32string_view(U"ï¿½"), std::u32string_view(U"\ufffd")}) {
    if (text.substr(position).starts_with(lost)) {
      return Document::Edit{.span = {position, position + lost.size()}, .text = U""};
    }
  }
  return std::nullopt;
}

// A fraction character standing apart, written with digits for the fraction readings: ¾ szklanki -> 3/4, 2½ -> 2 1/2;
// nullopt next to a letter, where it is likely a mis-encoded letter (dï¿½clarant)
std::optional<std::u32string> vulgar_fraction(std::u32string_view text, size_t position) {
  static const std::unordered_map<char32_t, std::u32string> fractions = {
      {U'½', U"1/2"}, {U'¼', U"1/4"}, {U'¾', U"3/4"}, {U'⅓', U"1/3"}, {U'⅔', U"2/3"}};
  auto it = fractions.find(text[position]);
  bool letter_before = position > 0 && unicode::isalpha(text[position - 1]);
  bool letter_after = position + 1 < text.size() && unicode::isalpha(text[position + 1]);
  if (it == fractions.end() || letter_before || letter_after) {
    return std::nullopt;
  }
  bool digit_before = position > 0 && unicode::isdigit(text[position - 1]);
  return (digit_before ? U" " : U"") + it->second;
}

// Whether an ordinal sign at a position is a degree: 18ºC, 12 ºF
bool is_degree(std::u32string_view text, size_t position) {
  size_t before = position;
  while (before > 0 && unicode::isspace(text[before - 1])) {
    before--;
  }
  size_t unit = position + 1;
  return text[position] == U'º' && before > 0 && unicode::isdigit(text[before - 1]) && unit < text.size() &&
         (text[unit] == U'C' || text[unit] == U'F') && (unit + 1 == text.size() || !unicode::isword(text[unit + 1]));
}

// The characters of the Windows-1252 bytes 0x80 to 0x9F; an undefined byte reads as the control character of its value
constexpr std::u32string_view kWindows1252 = U"€\u0081‚ƒ„…†‡ˆ‰Š‹Œ\u008dŽ\u008f\u0090‘’“”•–—˜™š›œ\u009džŸ";

// The Windows-1252 byte a character was read from, or -1 for none
int byte_of(char32_t c) {
  if (c >= 0x80 && c <= 0xff) {
    return static_cast<int>(c);
  }
  size_t i = kWindows1252.find(c);
  return i == std::u32string_view::npos ? -1 : static_cast<int>(0x80 + i);
}

// A character of UTF-8 read as Windows-1252 at a position (Ä™ -> ę), or nullopt
std::optional<Document::Edit> read_utf8(std::u32string_view text, size_t position) {
  int lead = byte_of(text[position]);
  size_t length = lead >= 0xc2 && lead <= 0xdf ? 2 :
                  lead >= 0xe0 && lead <= 0xef ? 3 :
                  lead >= 0xf0 && lead <= 0xf4 ? 4 :
                                                 0;
  if (length == 0 || position + length > text.size()) {
    return std::nullopt;
  }
  char32_t c = lead & (0x7f >> length);
  for (size_t k = 1; k < length; ++k) {
    int byte = byte_of(text[position + k]);
    if (byte < 0x80 || byte > 0xbf) {
      return std::nullopt;
    }
    c = (c << 6) | (byte & 0x3f);
  }
  if ((length == 3 && c < 0x800) || (length == 4 && (c < 0x10000 || c > 0x10ffff))) {
    return std::nullopt;
  }
  return Document::Edit{.span = {position, position + length}, .text = std::u32string(1, c)};
}
} // namespace

Normalizer::Normalizer(const Characters& characters) {
  for (const auto& [members, canonical, letter] : characters) {
    for (char32_t c : members) {
      canonical_[c] = canonical;
      if (letter) {
        letters_.insert(c);
        utf8_ = true;
      }
    }
  }
}

void Normalizer::apply(Document& document) const {
  std::u32string_view text = document.text();

  for (size_t i = 0; i < text.size();) {
    auto edit = utf8_ ? read_utf8(text, i) : std::nullopt;
    if (!edit) {
      edit = read_lost(text, i);
    }
    if (!edit) {
      edit = read_dash(text, i);
    }
    if (edit) {
      document.replace(edit->span, std::move(edit->text));
      i = edit->span.end;
      continue;
    }
    if (is_degree(text, i)) {
      document.replace({i, i + 1}, U"°");
    } else if (auto fraction = vulgar_fraction(text, i)) {
      document.replace({i, i + 1}, std::move(*fraction));
    } else if (auto it = canonical_.find(text[i]);
               it != canonical_.end() && (!letters_.contains(text[i]) || is_letter(text, i))) {
      document.replace({i, i + 1}, it->second);
    }
    i++;
  }
}

bool Normalizer::is_letter(std::u32string_view text, size_t position) const {
  auto letter = [&](size_t i) { return unicode::isalpha(text[i]) || letters_.contains(text[i]); };
  if (position + 1 < text.size() && letter(position + 1)) {
    return true;
  }
  size_t start = position;
  while (start > 0 && letter(start - 1)) {
    start--;
  }
  std::u32string_view word = text.substr(start, position - start);
  return !word.empty() && word != U"m" && word != U"cm" && word != U"dm" && word != U"km" && word != U"mm";
}

std::optional<Document::Edit> Normalizer::read_dash(std::u32string_view text, size_t position) const {
  size_t end = dash_end(text, position);
  if (end == position) {
    return std::nullopt;
  }

  // 1. Inside a word or a number: Szczurek–Żelazko, 1990–2000, a--b
  if (position > 0 && unicode::isword(text[position - 1]) && end < text.size() && unicode::isword(text[end])) {
    return Document::Edit{.span = {position, end}, .text = U"-"};
  }

  // 2. Elsewhere, the whole run of dashes with the spaces around it: "a – b", "a – — b", "a ---- b"
  size_t begin = position;
  while (begin > 0 && unicode::isspace(text[begin - 1])) {
    begin--;
  }
  for (size_t next = end; next < text.size();) {
    while (next < text.size() && unicode::isspace(text[next])) {
      next++;
    }
    if (next == text.size() || !is_dash(text[next])) {
      break;
    }
    while (next < text.size() && is_dash(text[next])) {
      next++;
    }
    end = next;
  }
  while (end < text.size() && unicode::isspace(text[end])) {
    end++;
  }

  // Between numbers it stays a range: 2020 – 2022 -> 2020-2022
  bool range = begin > 0 && unicode::isdigit(text[begin - 1]) && end < text.size() && unicode::isdigit(text[end]);
  return Document::Edit{.span = {begin, end}, .text = range ? U"-" : U" - "};
}

} // namespace verbalis::steps
