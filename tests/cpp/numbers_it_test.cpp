#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// Italian readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_it_test) {

  ASSERT_EQUALS(U"Il ventiquattro marzo duemilaventidue alle ventuno e trenta, e alle nove e cinque o alle tredici.",
                verbalize("it", U"Il 24/03/2022 alle 21.30, e alle 9:05 o alle 13:00.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ha ventuno anni, ventotto euro e primo maggio.",
                verbalize("it", U"Ha 21 anni, 28 euro e 1 maggio.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Negli anni ottanta e novanta, ma non 'ottantacinque.",
                verbalize("it", U"Negli anni '80 e '90, ma non '85.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Costa dieci euro e cinquanta e uno euro o tre milioni di euro e diecimila euro.",
                verbalize("it", U"Costa 10,50 € e 1,00 € o 3 mln € e 10 mila €.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Ci sono settantaquattromila persone e uno virgola cinque milioni di abitanti, quattro K e quarantamila.",
      verbalize("it", U"Ci sono 74 mila persone e 1,5 milioni di abitanti, 4K e 40K.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pesa uno chilo e ventitré chili, uno chilometro e centotto chilometri orari.",
                verbalize("it", U"Pesa 1 kg e 23 kg, 1 km e 108 km/h.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il primo e la seconda, il ventunesimo e la trentatreesima.",
                verbalize("it", U"Il 1º e la 2ª, il 21esimo e la 33esima.", only(&Options::numbers)));
  ASSERT_EQUALS(U"La versione due punto settantanove e uno punto tredici, il risultato due-uno e tre a due.",
                verbalize("it", U"La versione 2.79 e 1.13, il risultato 2-1 e 3:2.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Sono un milione di abitanti e duemilatrecentoquarantacinque persone, milletré e millecentotto e milleottanta.",
      verbalize("it", U"Sono 1.000.000 di abitanti e 2.345 persone, 1003 e 1108 e 1080.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Nel duemilaventitré e nel millenovecentonovantanove, ventitré e trentatré e centotré.",
                verbalize("it", U"Nel 2023 e nel 1999, 23 e 33 e 103.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ha un milione e due miliardi e milleuno e undicimila.",
                verbalize("it", U"Ha 1000000 e 2000000000 e 1001 e 11000.", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
