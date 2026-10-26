#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

REGISTER_TEST(options_steps_test) {
  std::u32string_view text = U"  Dr. Smith’s 2 cats & e.g. example.com  ";

  ASSERT_EQUALS(U"Doctor Smith's two cats and for example example dot com", verbalize("en-us", text));
  ASSERT_EQUALS(U"  Doctor Smith's two cats and for example example dot com  ",
                verbalize("en-us", text, {.trim = false}));
  ASSERT_EQUALS(U"Dr. Smith's two cats and e.g. example.com", verbalize("en-us", text, {.terms = false}));
  ASSERT_EQUALS(U"Doctor Smith's 2 cats and for example example dot com", verbalize("en-us", text, {.numbers = false}));
  ASSERT_EQUALS(U"Doctor Smith's two cats & for example example dot com", verbalize("en-us", text, {.symbols = false}));
  ASSERT_EQUALS(U"Doctor Smith’s two cats and for example example dot com",
                verbalize("en-us", text, {.normalize = false}));

  // Nothing at all
  ASSERT_EQUALS(std::u32string(text), verbalize("en-us", text, kNone));

  return true;
}

REGISTER_TEST(options_readings_test) {
  // Readings added to the language's own, or replacing them
  Options options = {.abbreviations = {{"approx.", "approximately"}, {"Dr.", "Drive"}},
                     .symbol_words = {{"™", "trademark"}, {"&", "plus"}}};
  ASSERT_EQUALS(U"approximately five kilometers on Main Drive, Brand trademark plus more",
                verbalize("en-us", U"approx. 5 km on Main Dr., Brand™ & more", options));

  return true;
}

} // namespace verbalis::test
