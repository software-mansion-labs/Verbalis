// What is specific to the C++ API. The shared cases (outputs, options, changes) are in tests/parity/api.jsonl.
#include "test.h"

#include <verbalis/verbalizer.h>

#include <algorithm>
#include <stdexcept>

namespace verbalis::test {

REGISTER_TEST(verbalizer_languages_test) {
  const auto& languages = Verbalizer::languages();
  for (const char* code : {"de", "en-us", "en-gb", "es", "fr", "hi", "it", "pl", "pt", "zh"}) {
    bool supported = std::find(languages.begin(), languages.end(), code) != languages.end();
    ASSERT_EQUALS(true, supported);
  }

  bool thrown = false;
  try {
    Verbalizer unknown("xx");
  } catch (const std::invalid_argument&) {
    thrown = true;
  }
  ASSERT_EQUALS(true, thrown);
  return true;
}

} // namespace verbalis::test
