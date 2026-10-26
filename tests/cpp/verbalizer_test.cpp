#include "test.h"

#include <verbalis/verbalizer.h>

#include <algorithm>
#include <stdexcept>

namespace verbalis::test {

REGISTER_TEST(verbalizer_process_test) {
  Verbalizer english("en-us");
  ASSERT_EQUALS(std::string("I bought three tickets for forty-nine dollars."),
                english.process("I bought 3 tickets for $49."));

  Verbalizer polish("pl");
  ASSERT_EQUALS(std::string("Mam dwa koty."), polish.process("Mam 2 koty."));
  return true;
}

REGISTER_TEST(verbalizer_changes_test) {
  // Offsets count code points: the emoji before "5" is one, though it takes four bytes in UTF-8
  Verbalizer english("en-us");
  Result result = english.process_with_changes("🍎 5 apples & pears");

  ASSERT_EQUALS(std::string("🍎 five apples and pears"), result.text);
  ASSERT_EQUALS(size_t(2), result.changes.size());
  ASSERT_EQUALS(size_t(2), result.changes[0].start);
  ASSERT_EQUALS(size_t(3), result.changes[0].end);
  ASSERT_EQUALS(std::string("five"), result.changes[0].replacement);
  ASSERT_EQUALS(std::string(" and "), result.changes[1].replacement); // Symbols bring their own spacing
  return true;
}

REGISTER_TEST(verbalizer_options_test) {
  Verbalizer no_numbers("en-us", {.numbers = false});
  ASSERT_EQUALS(std::string("3 cats and 2 dogs"), no_numbers.process("3 cats & 2 dogs"));

  Verbalizer custom("en-us", {.abbreviations = {{"approx.", "approximately"}}, .symbol_words = {{"™", "trademark"}}});
  ASSERT_EQUALS(std::string("approximately ten Acme trademark"), custom.process("approx. 10 Acme™"));
  return true;
}

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
