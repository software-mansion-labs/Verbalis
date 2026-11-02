#include "normalize.h"

#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

namespace verbalis::steps {

using namespace text;


namespace {
// Whether a character is a guillemet written as ", which apply_guillemets writes, not apply
bool is_guillemet(char32_t c, std::u32string_view canonical) {
  return (c == U'«' || c == U'»') && canonical == U"\"";
}

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

// An ellipsis right before a digit, spaced from it so the number reads on its own: 114...48 -> 114... 48
std::optional<Document::Edit> read_ellipsis(std::u32string_view text, size_t position) {
  size_t end = text.substr(position).starts_with(U"...") ? position + 3 : position + (text[position] == U'…');
  if (end == position || end >= text.size() || !unicode::isdigit(text[end])) {
    return std::nullopt;
  }
  return Document::Edit{.span = {position, end}, .text = U"... "};
}

// A hyphen spaced from the number before it only, which is a range, not a minus: 1989 -1990 -> 1989-1990
std::optional<Document::Edit> read_spaced_range(std::u32string_view text, size_t position) {
  if (position == 0 || position + 2 >= text.size() || text[position] != U' ' || text[position + 1] != U'-' ||
      !unicode::isdigit(text[position - 1]) || !unicode::isdigit(text[position + 2])) {
    return std::nullopt;
  }
  return Document::Edit{.span = {position, position + 2}, .text = U"-"};
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

// A power written as a digit after a unit after a number, as its superscript: 345m2 -> 345m², 900 cm3 -> 900 cm³
std::optional<Document::Edit> read_power(std::u32string_view text, size_t position) {
  if ((text[position] != U'2' && text[position] != U'3') ||
      (position + 1 < text.size() && unicode::isalnum(text[position + 1]))) {
    return std::nullopt;
  }
  for (std::u32string_view unit : {U"km", U"cm", U"mm", U"dm", U"ft", U"m"}) {
    size_t start = position - std::min(position, unit.size());
    if (text.substr(start, position - start) != unit) {
      continue;
    }
    size_t before = start > 0 && text[start - 1] == U' ' ? start - 1 : start;
    if (before > 0 && unicode::isdigit(text[before - 1])) {
      return Document::Edit{.span = {position, position + 1}, .text = text[position] == U'2' ? U"²" : U"³"};
    }
    return std::nullopt;
  }
  return std::nullopt;
}

// Whether an ordinal sign at a position is a degree: 18ºC, 12 ºF; a ring after a number is one too: 180˚C
bool is_degree(std::u32string_view text, size_t position) {
  size_t before = position;
  while (before > 0 && unicode::isspace(text[before - 1])) {
    before--;
  }
  size_t unit = position + 1;
  if (text[position] == U'˚') {
    return before > 0 && unicode::isdigit(text[before - 1]);
  }
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
  // Only what mis-encoded text holds: Latin letters, Devanagari, and punctuation (â€™ -> ’); not a pair which is
  // itself text (Spaß», ÉTÉ»)
  bool latin = c >= 0xa0 && c <= 0x24f, devanagari = c >= 0x900 && c <= 0x97f;
  bool punctuation = (c >= 0x2010 && c <= 0x206f) || c == 0x20ac || c == 0x2122;
  if (!latin && !devanagari && !punctuation && length < 4) { // and emoji (ðŸ™‚ -> 🙂)
    return std::nullopt;
  }
  return Document::Edit{.span = {position, position + length}, .text = std::u32string(1, c)};
}

// The characters of named HTML entities, as the canonical forms of this step: &nbsp; is a space, &rsquo; is '
// clang-format off
const std::unordered_map<std::u32string, std::u32string> kEntities = {
    {U"amp", U"&"}, {U"lt", U"<"}, {U"gt", U">"}, {U"quot", U"\""}, {U"apos", U"'"}, {U"nbsp", U" "},
    {U"ndash", U"-"}, {U"mdash", U"-"}, {U"hellip", U"..."}, {U"laquo", U"\""}, {U"raquo", U"\""}, {U"ldquo", U"\""},
    {U"rdquo", U"\""}, {U"bdquo", U"\""}, {U"lsquo", U"'"}, {U"rsquo", U"'"}, {U"euro", U"€"}, {U"pound", U"£"},
    {U"yen", U"¥"}, {U"cent", U"¢"}, {U"deg", U"°"}, {U"sup1", U"¹"}, {U"sup2", U"²"}, {U"sup3", U"³"},
    {U"middot", U"·"}, {U"times", U"×"}, {U"divide", U"÷"}, {U"plusmn", U"±"}, {U"frac12", U"1/2"}, {U"frac14", U"1/4"},
    {U"frac34", U"3/4"}, {U"copy", U"©"}, {U"reg", U"®"}, {U"trade", U"™"}, {U"sect", U"§"}, {U"para", U"¶"},
    {U"ordm", U"º"}, {U"ordf", U"ª"}, {U"iexcl", U"¡"}, {U"iquest", U"¿"}, {U"szlig", U"ß"}, {U"ccedil", U"ç"},
    {U"Ccedil", U"Ç"}, {U"aelig", U"æ"}, {U"AElig", U"Æ"}, {U"oslash", U"ø"}, {U"Oslash", U"Ø"}, {U"aring", U"å"},
    {U"Aring", U"Å"}, {U"oelig", U"œ"}, {U"OElig", U"Œ"}, {U"eth", U"ð"}, {U"thorn", U"þ"},
    // Accented letters: &eacute; -> é
    {U"Aacute", U"Á"}, {U"Agrave", U"À"}, {U"Acirc", U"Â"}, {U"Auml", U"Ä"}, {U"Atilde", U"Ã"}, {U"Eacute", U"É"},
    {U"Egrave", U"È"}, {U"Ecirc", U"Ê"}, {U"Euml", U"Ë"}, {U"Iacute", U"Í"}, {U"Igrave", U"Ì"}, {U"Icirc", U"Î"},
    {U"Iuml", U"Ï"}, {U"Oacute", U"Ó"}, {U"Ograve", U"Ò"}, {U"Ocirc", U"Ô"}, {U"Ouml", U"Ö"}, {U"Otilde", U"Õ"},
    {U"Uacute", U"Ú"}, {U"Ugrave", U"Ù"}, {U"Ucirc", U"Û"}, {U"Uuml", U"Ü"}, {U"aacute", U"á"}, {U"agrave", U"à"},
    {U"acirc", U"â"}, {U"auml", U"ä"}, {U"atilde", U"ã"}, {U"eacute", U"é"}, {U"egrave", U"è"}, {U"ecirc", U"ê"},
    {U"euml", U"ë"}, {U"iacute", U"í"}, {U"igrave", U"ì"}, {U"icirc", U"î"}, {U"iuml", U"ï"}, {U"oacute", U"ó"},
    {U"ograve", U"ò"}, {U"ocirc", U"ô"}, {U"ouml", U"ö"}, {U"otilde", U"õ"}, {U"uacute", U"ú"}, {U"ugrave", U"ù"},
    {U"ucirc", U"û"}, {U"uuml", U"ü"}, {U"Ntilde", U"Ñ"}, {U"ntilde", U"ñ"}, {U"Yacute", U"Ý"}, {U"Yuml", U"Ÿ"},
    {U"yacute", U"ý"}, {U"yuml", U"ÿ"},
};
// clang-format on

// An HTML entity at a position, as its character: &iacute; -> í, &#8217; -> ', &nbsp -> " ". Corpora drop the
// semicolon (Computaci&oacuten), so a name of four letters and more reads without it.
std::optional<Document::Edit> read_entity(std::u32string_view text, size_t position) {
  if (text[position] != U'&' || position + 2 >= text.size()) {
    return std::nullopt;
  }
  size_t end = position + 1;
  std::u32string decoded;
  if (text[end] == U'#') {
    bool hex = text[end + 1] == U'x' || text[end + 1] == U'X';
    size_t digits = end = end + 1 + hex;
    char32_t c = 0;
    while (end < text.size() && end - digits < 7 &&
           (unicode::isdigit(text[end]) ||
            (hex && std::u32string_view(U"abcdefABCDEF").find(text[end]) != std::u32string_view::npos)) &&
           text[end] < 0x80) {
      c = c * (hex ? 16 : 10) + (text[end] <= U'9' ? text[end] - U'0' : (text[end] | 0x20) - U'a' + 10);
      end++;
    }
    if (end == digits || end == text.size() || text[end] != U';' || c == 0 || c > 0x10ffff ||
        (c >= 0xd800 && c <= 0xdfff)) {
      return std::nullopt;
    }
    end++;
    decoded = std::u32string(1, c);
  } else {
    size_t name_end = end;
    while (name_end < text.size() && name_end - end < 8 && text[name_end] < 0x80 && unicode::isalnum(text[name_end])) {
      name_end++;
    }
    for (; name_end > end + 1; --name_end) { // The longest name: &oacuten -> ó + n
      auto it = kEntities.find(std::u32string(text.substr(end, name_end - end)));
      bool semicolon = name_end < text.size() && text[name_end] == U';';
      if (it != kEntities.end() && (semicolon || name_end - end >= 4)) {
        decoded = it->second;
        end = name_end + semicolon;
        break;
      }
    }
    if (decoded.empty()) {
      return std::nullopt;
    }
  }
  return Document::Edit{.span = {position, end}, .text = decoded};
}
} // namespace

Normalizer::Normalizer(const Characters& characters) {
  for (const auto& [members, canonical, letter] : characters) {
    for (char32_t c : members) {
      canonical_[c] = canonical;
      if (letter) {
        letters_.insert(c);
      }
    }
  }
}

void Normalizer::apply(Document& document) const {
  std::u32string_view text = document.text();

  for (size_t i = 0; i < text.size();) {
    auto edit = read_utf8(text, i);
    if (!edit) {
      edit = entity(text, i);
    }
    if (!edit) {
      edit = read_lost(text, i);
    }
    if (!edit) {
      edit = read_ellipsis(text, i);
    }
    if (!edit) {
      edit = read_dash(text, i);
    }
    if (!edit) {
      edit = read_spaced_range(text, i);
    }
    if (!edit) {
      edit = read_power(text, i);
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
    } else if (auto it = canonical_.find(text[i]); it != canonical_.end() && !is_guillemet(text[i], it->second) &&
                                                   (!letters_.contains(text[i]) || is_letter(text, i))) {
      document.replace({i, i + 1}, it->second);
    }
    i++;
  }
}

void Normalizer::apply_guillemets(Document& document) const {
  std::u32string_view text = document.text();
  for (size_t i = 0; i < text.size(); ++i) {
    if (auto it = canonical_.find(text[i]); it != canonical_.end() && is_guillemet(text[i], it->second)) {
      document.replace({i, i + 1}, it->second);
    }
  }
}

std::optional<Document::Edit> Normalizer::entity(std::u32string_view text, size_t position) const {
  auto edit = read_entity(text, position);
  if (edit && edit->text.size() == 1) { // A character by number in its canonical form: &#8217; -> '
    char32_t c = edit->text[0];
    if (auto it = canonical_.find(c); it != canonical_.end() && !letters_.contains(c)) {
      edit->text = it->second;
    } else if (strings::contains(kDashes, c)) {
      edit->text = U"-";
    } else if (c == 0xa0) {
      edit->text = U" ";
    }
  }
  return edit;
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

  // 1. Inside a word or a number: Szczurek–Żelazko, 1990–2000; but an em dash or two hyphens before a word is a
  // pause, as with spaces around it (case 2): briefs—styles, I--test. A shorter part after them is a clitic
  // (limitou--se, elegê--los) or a mis-encoded letter (spos—b). An em dash before a number is a pause too.
  size_t word_end = end;
  while (word_end < text.size() && unicode::isalpha(text[word_end])) {
    word_end++;
  }
  bool number = end < text.size() && unicode::isdigit(text[end]); // drizzle—0.2 mm
  bool pause = text[position] == U'—' ? number || word_end - end >= 3 : end - position >= 2 && word_end - end >= 4;
  bool digits =
      position > 0 && unicode::isdigit(text[position - 1]) && end < text.size() && unicode::isdigit(text[end]);
  if (position > 0 && unicode::isword(text[position - 1]) && end < text.size() && unicode::isword(text[end]) &&
      (!pause || digits)) {
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
