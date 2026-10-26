#include "registry.h"

namespace verbalis::languages {

// Each language's folder defines its instance; add a language here.
// clang-format off
namespace de { const Language& language(); }
namespace en { const Language& language(); }
namespace es { const Language& language(); }
namespace fr { const Language& language(); }
namespace hi { const Language& language(); }
namespace it { const Language& language(); }
namespace pl { const Language& language(); }
namespace pt { const Language& language(); }
namespace zh { const Language& language(); }
// clang-format on

namespace {
struct Entry {
  const char* code;
  const Language& (*language)();
};

const Entry kLanguages[] = {
    {"de", de::language}, {"en", en::language}, {"en-gb", en::language}, {"en-us", en::language},
    {"es", es::language}, {"fr", fr::language}, {"hi", hi::language},    {"it", it::language},
    {"pl", pl::language}, {"pt", pt::language}, {"zh", zh::language},
};
} // namespace

const Language* find(std::string_view code) {
  for (const auto& entry : kLanguages) {
    if (code == entry.code) {
      return &entry.language();
    }
  }
  return nullptr;
}

const std::vector<std::string>& codes() {
  static const std::vector<std::string> codes = [] {
    std::vector<std::string> result;
    for (const auto& entry : kLanguages) {
      result.emplace_back(entry.code);
    }
    return result;
  }();
  return codes;
}

} // namespace verbalis::languages
