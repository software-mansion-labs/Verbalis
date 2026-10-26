#pragma once

#include <verbalis/text/utf8.h>
#include <verbalis/verbalizer.h>

#include <string>
#include <string_view>

namespace verbalis::test {

// A text verbalized through the public API.
inline std::u32string verbalize(std::string_view language, std::u32string_view text, const Options& options = {}) {
  using text::u32_to_utf8;
  using text::utf8_to_u32;
  return utf8_to_u32(Verbalizer(language, options).process(u32_to_utf8(text)));
}

// Options with every step off.
inline const Options kNone{.normalize = false, .trim = false, .terms = false, .numbers = false, .symbols = false};

// Options with every step off but one, to test that step alone.
inline Options only(bool Options::*step) {
  Options options = kNone;
  options.*step = true;
  return options;
}

} // namespace verbalis::test
