#include "languages.h"

#include "lang/de/constants.h"
#include "lang/de/num2word.h"
#include "lang/en/constants.h"
#include "lang/en/num2word.h"
#include "lang/es/constants.h"
#include "lang/es/num2word.h"
#include "lang/fr/constants.h"
#include "lang/fr/num2word.h"
#include "lang/hi/constants.h"
#include "lang/hi/num2word.h"
#include "lang/it/constants.h"
#include "lang/it/num2word.h"
#include "lang/pl/constants.h"
#include "lang/pl/num2word.h"
#include "lang/pt/constants.h"
#include "lang/pt/num2word.h"
#include "lang/zh/constants.h"
#include "lang/zh/num2word.h"
#include "processor/preprocessor.h"

namespace verbalis {

namespace {
using processor::Config;
using processor::Preprocessor;

// Each supported language: its code and how its preprocessing is built. Add a language here.
struct Entry {
  const char* code;
  Preprocessor (*build)(const Config&);
};

#define VERBALIS_LANGUAGE(ns)                                                                                         \
  [](const Config& config) {                                                                                          \
    return processor::make_preprocessor<ns::Num2Word>(                                                                \
        config, {.terms = &ns::constants::term2word::kTerms, .symbols = &ns::constants::sym2word::kSymbols});        \
  }

const Entry kLanguages[] = {
    {"de", VERBALIS_LANGUAGE(de)},
    {"en", VERBALIS_LANGUAGE(en)},
    {"en-gb", VERBALIS_LANGUAGE(en)},
    {"en-us", VERBALIS_LANGUAGE(en)},
    {"es", VERBALIS_LANGUAGE(es)},
    {"fr", VERBALIS_LANGUAGE(fr)},
    {"hi",
     [](const Config& config) {
       return processor::make_preprocessor<hi::Num2Word>(config, {.characters = &hi::constants::normalize::kCharacters,
                                                                  .terms = &hi::constants::term2word::kTerms,
                                                                  .symbols = &hi::constants::sym2word::kSymbols});
     }},
    {"it", VERBALIS_LANGUAGE(it)},
    {"pl", VERBALIS_LANGUAGE(pl)},
    {"pt", VERBALIS_LANGUAGE(pt)},
    {"zh",
     [](const Config& config) {
       return processor::make_preprocessor<zh::Num2Word>(config, {.characters = &zh::constants::normalize::kCharacters});
     }},
};

#undef VERBALIS_LANGUAGE
} // namespace

std::optional<Preprocessor> make_preprocessor(std::string_view language, const Config& config) {
  for (const auto& entry : kLanguages) {
    if (language == entry.code) {
      return entry.build(config);
    }
  }
  return std::nullopt;
}

const std::vector<std::string>& language_codes() {
  static const std::vector<std::string> codes = [] {
    std::vector<std::string> result;
    for (const auto& entry : kLanguages) {
      result.emplace_back(entry.code);
    }
    return result;
  }();
  return codes;
}

} // namespace verbalis
