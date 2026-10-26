#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// German readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_de_test) {

  ASSERT_EQUALS(U"Am ersten Januar zweitausendzwanzig um achtzehn Uhr dreißig kostete es zwölf Euro neunundneunzig.",
                verbalize("de", U"Am 1. Januar 2020 um 18:30 Uhr kostete es 12,99 €.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Er wird in zweiter und dritter Lesung beraten.",
                verbalize("de", U"Er wird in 2. und 3. Lesung beraten.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Nur eine Person kam, eintausendneunhundertfünfundvierzig Einwohner, im Jahr neunzehnhundertfünfundvierzig.",
      verbalize("de", U"Nur 1 Person kam, 1945 Einwohner, im Jahr 1945.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Die neunziger Jahre und die Neunziger, achtprozentige Lösung, Vier-Zimmer-Wohnung und dreißigjährige.",
      verbalize("de", U"Die 90er Jahre und die 90er, 8%ige Lösung, 4-Zimmer-Wohnung und 30-jährige.",
                only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Ruf null drei null eins zwei drei vier fünf sechs sieben an, am zwölften März ist es so weit, drei Viertel und "
      U"fünf durch sieben der Stimmen.",
      verbalize("de", U"Ruf 030 1234567 an, am 12. 3. ist es so weit, 3/4 und 5/7 der Stimmen.",
                only(&Options::numbers)));
  ASSERT_EQUALS(
      U"ein bis zwei Werktage, mit einer Stimme, Seite eins und fünf Kilogramm Mehl für einen Euro, zwei Komma fünf "
      U"Millionen Euro.",
      verbalize("de", U"1 bis 2 Werktage, mit 1 Stimme, Seite 1 und 5 kg Mehl für 1 €, 2,5 Mio. Euro.",
                only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Es ist eintausendneunhundertzwanzig mal eintausendachtzig und zehn bis fünfzehn Kilometer entfernt, zwanzig "
      U"Prozent mehr.",
      verbalize("de", U"Es ist 1920 mal 1080 und 10-15 km entfernt, 20 % mehr.", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
