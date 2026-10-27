#include "trim.h"

#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

namespace verbalis::steps {

using namespace text;

namespace {
// Whether a position starts punctuation which takes no space before it: "a ," -> "a,", but "a ..." and the point of
// "about .5 kg" stay
bool is_closing_punctuation(std::u32string_view text, size_t position) {
  if (position >= text.size()) {
    return false;
  }
  bool ellipsis = text.substr(position).starts_with(U"...");
  bool decimal = position + 1 < text.size() && unicode::isdigit(text[position + 1]);
  return text::strings::contains(std::u32string_view(U",!?;:"), text[position]) ||
         (text[position] == U'.' && !ellipsis && !decimal);
}
} // namespace

void trim(Document& document) {
  std::u32string_view text = document.text();

  for (size_t i = 0; i < text.size();) {
    if (!text::unicode::isspace(text[i])) {
      i++;
      continue;
    }

    // Find the whole run of whitespaces
    size_t end = i;
    bool newline = false;
    while (end < text.size() && text::unicode::isspace(text[end])) {
      newline |= text[end] == U'\n';
      end++;
    }

    // Edges and spaces before punctuation lose the run entirely, the middle keeps a single character
    bool closing = !newline && is_closing_punctuation(text, end);
    std::u32string replacement = i == 0 || end == text.size() || closing ? U"" : newline ? U"\n" : U" ";
    if (text.substr(i, end - i) != replacement) {
      document.replace({i, end}, replacement);
    }

    i = end;
  }
}

} // namespace verbalis::steps
