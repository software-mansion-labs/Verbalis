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
      U"fünf Siebtel der Stimmen.",
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
  ASSERT_EQUALS(U"Es fiel um minus ein Drittel und minus zwei ein Drittel Grad.",
                verbalize("de", U"Es fiel um -1/3 und -2 1/3 Grad.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Er zahlte achtzehntausendachthundertachtzig Euro für eine Billiarde zweihundert Billionen, PLZ vier null "
      U"vier sieben vier.",
      verbalize("de", U"Er zahlte 18880 € für 1200000000000000, PLZ 40474.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Es kostet zwei Dollar, zwei Euro zwanzig und zwei Komma zwei eins Millionen Euro.",
                verbalize("de", U"Es kostet $2,000, 2,200 € und €2,21 millionen.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Um zwei Uhr zwei Minuten dreiundzwanzig Sekunden lief er zwei Stunden acht Minuten neunundzwanzig "
      U"Sekunden.",
      verbalize("de", U"Um 02:02:23 Uhr lief er 2:08:29.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ruf null vier eins eins eins zwei drei vier eins zwei drei vier an.",
                verbalize("de", U"Ruf (0411) 1234-1234 an.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Es wog neunzig Millionen Gramm, ein einhalb Fuß lang, bei zweiundvierzigtausend pro Quadratmeter.",
                verbalize("de", U"Es wog 90 Millionen g, 1 1/2 ft lang, bei 42000/m².", only(&Options::numbers)));
  ASSERT_EQUALS(U"Mister Bean und Professor Doktor Klein, zum Beispiel am zweiten Januar zweitausenddrei.",
                verbalize("de", U"Mr. Bean und Prof. Dr. Klein, z.B. am 2. Jan. 2003."));
  ASSERT_EQUALS(
      U"Er kam am dritten, traf am dritten Mai zum zwanzigsten Mal und siegte am dritten.",
      verbalize("de", U"Er kam am 3., traf am 3ten Mai zum 20sten Mal und siegte am 3.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Er wurde zwei. Er kam von neunzehnhundertdreiundachtzig.",
                verbalize("de", U"Er wurde 2. Er kam von 1983.", only(&Options::numbers)));
  ASSERT_EQUALS(U"am zweiten März zweitausenddrei", verbalize("de", U"am 2. mär. 2003"));
  ASSERT_EQUALS(U"Es kostet einen Euro neunundsiebzig, er lief einen Kilometer, aber ein Euro ist viel.",
                verbalize("de", U"Es kostet 1,79 €, er lief 1 km, aber 1 Euro ist viel.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"In der Saison zweitausenddreiundzwanzig vierundzwanzig kostet er zwölf Euro fünfzig pro Quadratmeter.",
      verbalize("de", U"In der Saison 2023/24 kostet er 12,50 €/m².", only(&Options::numbers)));
  ASSERT_EQUALS(U"Er trank einen halben Liter, nach einer halben Stunde, mit einem Achtel der Leute.",
                verbalize("de", U"Er trank 1/2 l, nach 1/2 Stunde, mit 1/8 der Leute.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ruf null acht neun neun acht sieben sechs an, Leistung eintausendvierhundert Megawatt.",
                verbalize("de", U"Ruf (089) 98 76 an, Leistung 1.400 MW.", only(&Options::numbers)));

  // A unit or currency per unit; not alternatives (400GB/800GB) nor 24/7
  ASSERT_EQUALS(
      U"Gold kostet drei Dollar pro Unze, Sand fünf Kilogramm pro Quadratmeter, und vierhundert "
      U"Gigabyte/achthundert Gigabyte laufen vierundzwanzig durch sieben.",
      verbalize("de", U"Gold kostet 3 $/oz, Sand 5 kg/m², und 400GB/800GB laufen 24/7.", only(&Options::numbers)));

  // A pair like a clock time among words of sport is a score, unless a time word comes before it
  ASSERT_EQUALS(
      U"Die Füchse siegten mit siebenunddreißig zu neunundzwanzig (zwanzig zu dreizehn), Anpfiff um fünfzehn Uhr "
      U"einundzwanzig.",
      verbalize("de", U"Die Füchse siegten mit 37:29 (20:13), Anpfiff um 15:21.", only(&Options::numbers)));

  // A spaced date after a weekday, with a space before its year, or in parentheses
  ASSERT_EQUALS(U"Am Freitag, siebter Juni zweitausendneunzehn und (sechzehnter Dezember).",
                verbalize("de", U"Am Freitag, 7. 6. 2019 und (16. 12.)."));

  return true;
}

} // namespace verbalis::test
