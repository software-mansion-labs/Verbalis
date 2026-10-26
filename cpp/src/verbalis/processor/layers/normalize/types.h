#pragma once

#include <string>
#include <vector>

namespace verbalis::processor::normalize {

// A class of characters written in one canonical form: all quotes as ".
struct CharacterClass {
  std::u32string characters; // « » „ “ ” ‟ ˝
  std::u32string canonical;  // "; empty to remove them
};

using Characters = std::vector<CharacterClass>;

} // namespace verbalis::processor::normalize
