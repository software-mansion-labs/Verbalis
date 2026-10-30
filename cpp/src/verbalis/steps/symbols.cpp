#include "symbols.h"

#include <verbalis/text/context.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>

namespace verbalis::steps {

using namespace text;


namespace {
// The words of a symbol before numbers are read: those of its first reading whose condition holds, if that one has a
// condition. After: those of its first reading without one.
std::optional<std::u32string> read(const std::vector<Reading>& readings, const Context& context, bool conditional) {
  auto reading =
      std::ranges::find_if(readings, [&](const Reading& r) { return !r.when || (conditional && r.when(context)); });
  if (reading == readings.end() || (conditional && !reading->when)) {
    return std::nullopt;
  }
  return reading->words;
}
} // namespace

SymbolReader::SymbolReader(Readings symbols) : symbols_(std::move(symbols)) {
  for (const auto& [symbol, readings] : symbols_) {
    keys_.push_back(symbol);
  }
}

void SymbolReader::apply(Document& document, bool conditional) const {
  std::u32string_view text = document.text();

  for (size_t start = 0; start < text.size();) {
    // 1. A run of symbols, with the whitespaces around it: " & ", "++"
    size_t symbol = start;
    while (symbol < text.size() && unicode::isspace(text[symbol])) {
      symbol++;
    }
    // Each symbol in its own context: "Google+", but "tiene +1". A symbol it can't read ends the run, and a run
    // without a symbol read stays as written: "<i>", "Año~"
    std::vector<std::u32string> words;
    size_t end = symbol;
    for (auto keys = keys_at(text, end, keys_); !keys.empty(); keys = keys_at(text, end, keys_)) {
      Context context(text, {end, end + keys.front().size()});
      auto reading = read(symbols_.at(std::u32string(keys.front())), context, conditional);
      if (!reading) {
        break;
      }
      if (!reading->empty()) {
        words.push_back(*reading);
      }
      end += keys.front().size();
    }
    if (end == symbol) {
      start = symbol > start ? symbol : start + 1;
      continue;
    }
    while (end < text.size() && unicode::isspace(text[end])) {
      end++;
    }

    // 2. The words, separated from the text around by single spaces (none before punctuation).
    // A run right after another one already has its space: "™ &" -> "trademark and"
    bool left = start > 0 && !unicode::isspace(text[start - 1]);
    std::u32string replacement;
    if (words.empty()) {
      replacement = left && end < text.size() ? U" " : U"";
    } else {
      bool right = end < text.size() && !strings::contains(kPunctuation, text[end]);
      replacement = (left ? U" " : U"") + join(words) + (right ? U" " : U"");
    }
    document.replace({start, end}, replacement);
    start = end;
  }
}

} // namespace verbalis::steps
