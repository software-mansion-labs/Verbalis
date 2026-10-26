#include "test.h"
#include "verbalize.h"

namespace verbalis::test {


REGISTER_TEST(numbers_hi_cardinal_test) {

  // Integers and decimals
  ASSERT_EQUALS(U"शून्य", verbalize("hi", U"0", only(&Options::numbers)));
  ASSERT_EQUALS(U"इक्कीस", verbalize("hi", U"21", only(&Options::numbers)));
  ASSERT_EQUALS(U"सत्तर", verbalize("hi", U"70", only(&Options::numbers)));
  ASSERT_EQUALS(U"इकहत्तर", verbalize("hi", U"71", only(&Options::numbers)));
  ASSERT_EQUALS(U"अस्सी", verbalize("hi", U"80", only(&Options::numbers)));
  ASSERT_EQUALS(U"निन्यानवे", verbalize("hi", U"99", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौ", verbalize("hi", U"100", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौ तेईस", verbalize("hi", U"123", only(&Options::numbers)));
  ASSERT_EQUALS(U"दो सौ", verbalize("hi", U"200", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक हज़ार", verbalize("hi", U"1000", only(&Options::numbers)));
  ASSERT_EQUALS(U"दो हज़ार", verbalize("hi", U"2000", only(&Options::numbers)));
  ASSERT_EQUALS(U"पाँच हज़ार", verbalize("hi", U"5000", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक लाख", verbalize("hi", U"100000", only(&Options::numbers)));
  ASSERT_EQUALS(U"दस लाख", verbalize("hi", U"1000000", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक करोड़", verbalize("hi", U"10000000", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौ तेईस दशमलव चार पाँच", verbalize("hi", U"123.45", only(&Options::numbers)));
  ASSERT_EQUALS(U"शून्य दशमलव शून्य पाँच", verbalize("hi", U"0.05", only(&Options::numbers)));

  // Fractions
  ASSERT_EQUALS(U"आधा", verbalize("hi", U"1/2", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक तिहाई", verbalize("hi", U"1/3", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक चौथाई", verbalize("hi", U"1/4", only(&Options::numbers)));
  ASSERT_EQUALS(U"तीन चौथाई", verbalize("hi", U"3/4", only(&Options::numbers)));
  ASSERT_EQUALS(U"तीन बटा दस", verbalize("hi", U"3/10", only(&Options::numbers)));

  // Currency
  ASSERT_EQUALS(U"एक डॉलर", verbalize("hi", U"1$", only(&Options::numbers)));
  ASSERT_EQUALS(U"पचास डॉलर", verbalize("hi", U"50$", only(&Options::numbers)));
  ASSERT_EQUALS(U"दो पाउंड", verbalize("hi", U"2£", only(&Options::numbers)));
  ASSERT_EQUALS(U"क़ीमत निन्यानवे यूरो है।", verbalize("hi", U"क़ीमत 99€ है।", only(&Options::numbers)));

  return true;
}

REGISTER_TEST(numbers_hi_ordinal_test) {

  // The suffix gives the form: masculine (वाँ), feminine (वीं) or plural (वें)
  ASSERT_EQUALS(U"पहला", verbalize("hi", U"1वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"पहली", verbalize("hi", U"1वीं", only(&Options::numbers)));
  ASSERT_EQUALS(U"पहले", verbalize("hi", U"1वें", only(&Options::numbers)));
  ASSERT_EQUALS(U"दूसरा", verbalize("hi", U"2वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"तीसरा", verbalize("hi", U"3वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"चौथा", verbalize("hi", U"4वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"पाँचवाँ", verbalize("hi", U"5वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"पाँचवीं", verbalize("hi", U"5वीं", only(&Options::numbers)));
  ASSERT_EQUALS(U"छठा", verbalize("hi", U"6वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"दसवाँ", verbalize("hi", U"10वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"बारहवाँ", verbalize("hi", U"12वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"इक्कीसवाँ", verbalize("hi", U"21वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौवाँ", verbalize("hi", U"100वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"यह पाँचवीं बार है।", verbalize("hi", U"यह 5वीं बार है।", only(&Options::numbers)));

  return true;
}

REGISTER_TEST(numbers_hi_date_test) {

  ASSERT_EQUALS(U"सत्ताईस मार्च दो हज़ार छब्बीस", verbalize("hi", U"27.03.2026", only(&Options::numbers)));
  ASSERT_EQUALS(U"आज सत्ताईस मार्च दो हज़ार छब्बीस है।", verbalize("hi", U"आज 27-03-2026 है।", only(&Options::numbers)));
  ASSERT_EQUALS(U"आज सत्ताईस मार्च दो हज़ार छब्बीस है।", verbalize("hi", U"आज 2026-03-27 है।", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
