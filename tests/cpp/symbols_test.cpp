#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

REGISTER_TEST(symbols_test) {
  Options options = only(&Options::symbols);
  options.symbol_words = {{"&", "and"}, {"+", "plus"}, {"=", "equals"}, {"*", ""}};

  // Symbols read as words, separated by single spaces
  ASSERT_EQUALS(U"Tom and Jerry", verbalize("en-us", U"Tom & Jerry", options));
  ASSERT_EQUALS(U"R and B", verbalize("en-us", U"R&B", options));
  ASSERT_EQUALS(U"plus equals x", verbalize("en-us", U"+= x", options));

  // No space before punctuation, and a symbol without words leaves one space
  ASSERT_EQUALS(U"a plus, b", verbalize("en-us", U"a+, b", options));
  ASSERT_EQUALS(U"a b", verbalize("en-us", U"a * b", options));

  return true;
}

REGISTER_TEST(symbols_conditions_test) {
  // Spanish "+" after a name
  ASSERT_EQUALS(U"Google plus y C más más", verbalize("es", U"Google+ y C++", only(&Options::symbols)));

  // German "§" after a word which takes its plural
  ASSERT_EQUALS(U"im Paragrafen 5 und Paragraf 6", verbalize("de", U"im § 5 und § 6", only(&Options::symbols)));

  return true;
}

} // namespace verbalis::test
