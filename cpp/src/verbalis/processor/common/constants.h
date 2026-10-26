#pragma once

#include <string_view>

namespace verbalis::processor::constants {

// Punctuation stripped from the words around a match
inline constexpr std::u32string_view kPunctuation = U".,;:!?\"'()[]«»„“”…";

} // namespace verbalis::processor::constants
