#pragma once

#include "language.h"

#include <string>
#include <string_view>
#include <vector>

namespace verbalis::languages {

// The language with a code, or nullptr if it isn't supported.
const Language* find(std::string_view code);

// The supported language codes.
const std::vector<std::string>& codes();

} // namespace verbalis::languages
