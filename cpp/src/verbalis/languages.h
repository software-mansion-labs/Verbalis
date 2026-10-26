#pragma once

#include "processor/config.h"
#include "processor/processor.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace verbalis {

// The preprocessing of a language, built for a configuration, or nullopt if the language isn't supported.
std::optional<processor::Preprocessor> make_preprocessor(std::string_view language, const processor::Config& config);

// The supported language codes, sorted.
const std::vector<std::string>& language_codes();

} // namespace verbalis
