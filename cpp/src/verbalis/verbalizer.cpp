#include <verbalis/verbalizer.h>

#include "languages.h"
#include "processor/common/document.h"
#include "utils/conversions.h"

#include <stdexcept>

namespace verbalis {

namespace {
// Readings added by the caller, each one unconditional
processor::Readings readings_of(const std::map<std::string, std::string>& words) {
  processor::Readings readings;
  for (const auto& [written, spoken] : words) {
    std::u32string reading = utils::conversions::utf8_to_u32(spoken);
    readings[utils::conversions::utf8_to_u32(written)] = {processor::Reading(reading.c_str())};
  }
  return readings;
}

processor::Config config_of(const Options& options) {
  return {.normalize = options.normalize,
          .trim = options.trim,
          .terms = options.terms,
          .numbers = options.numbers,
          .symbols = options.symbols,
          .sanitize = options.sanitize,
          .abbreviations = readings_of(options.abbreviations),
          .symbol_words = readings_of(options.symbol_words)};
}
} // namespace

struct Verbalizer::Impl {
  processor::Preprocessor preprocessor;
};

Verbalizer::Verbalizer(std::string_view language, const Options& options) {
  auto preprocessor = make_preprocessor(language, config_of(options));
  if (!preprocessor) {
    throw std::invalid_argument("Unsupported language: " + std::string(language));
  }
  impl_ = std::make_unique<Impl>(Impl{std::move(*preprocessor)});
}

Verbalizer::~Verbalizer() = default;
Verbalizer::Verbalizer(Verbalizer&&) noexcept = default;
Verbalizer& Verbalizer::operator=(Verbalizer&&) noexcept = default;

std::string Verbalizer::process(std::string_view text) const {
  return utils::conversions::u32_to_utf8(impl_->preprocessor.process(utils::conversions::utf8_to_u32(text)));
}

Result Verbalizer::process_with_changes(std::string_view text) const {
  processor::Document document(utils::conversions::utf8_to_u32(text));
  impl_->preprocessor.process(document);

  Result result{.text = utils::conversions::u32_to_utf8(document.text())};
  for (const auto& [span, replacement] : document.changes()) {
    result.changes.push_back({span.begin, span.end, utils::conversions::u32_to_utf8(replacement)});
  }
  return result;
}

const std::vector<std::string>& Verbalizer::languages() {
  return language_codes();
}

} // namespace verbalis
