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

  // Conditions look at the text before numbers are read: a digit is not a name, a number word is not one either
  ASSERT_EQUALS(U"el cincuenta por ciento más uno, LAB cuatro plus", verbalize("es", U"el 50%+1, LAB4+"));

  // A comparison or approximation sign before a number
  ASSERT_EQUALS(U"p less than 0.05, approximately dollar 145",
                verbalize("en-us", U"p<0.05, ~$145", only(&Options::symbols)));
  ASSERT_EQUALS(U"THI less than 84", verbalize("en-us", U"THI< 84", only(&Options::symbols)));

  // A symbol whose readings all decline stays exactly as written
  for (std::u32string text : {U"<i>a</i> 5", U"Año~", U"2018~2019", U"a -> 5", U"Traum<3.", U"<27>", U"a > 9h",
                              U"100<P<250", U"<sup>2</sup>"}) {
    ASSERT_EQUALS(text, verbalize("en-us", text, only(&Options::symbols)));
  }

  return true;
}

} // namespace verbalis::test
