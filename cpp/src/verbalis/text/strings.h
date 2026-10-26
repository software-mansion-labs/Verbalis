#pragma once

#include "unicode.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace verbalis::text::strings {

// Whether a string contains a character.
template <typename StringT>
bool contains(const StringT& str, char32_t c) {
  return std::ranges::find(str, c) != std::ranges::end(str);
}

// A string in lowercase.
inline std::u32string to_lower(std::u32string_view str) {
  std::u32string lower(str);
  std::ranges::transform(lower, lower.begin(), [](char32_t c) { return unicode::tolower(c); });
  return lower;
}

// The parts of a string between occurrences of a character, empty parts included.
inline std::vector<std::u32string_view> split(std::u32string_view str, char32_t separator) {
  std::vector<std::u32string_view> parts;
  for (size_t start = 0; start < str.size();) {
    size_t end = std::min(str.find(separator, start), str.size());
    parts.push_back(str.substr(start, end - start));
    start = end + 1;
  }
  return parts;
}

} // namespace verbalis::text::strings
