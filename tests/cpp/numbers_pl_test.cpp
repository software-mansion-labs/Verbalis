#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// Polish readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_pl_test) {

  ASSERT_EQUALS(
      U"Urodził się w tysiąc dziewięćset osiemdziesiątym piątym roku, pierwszego stycznia o osiemnastej trzydzieści.",
      verbalize("pl", U"Urodził się w 1985 roku, 1 stycznia o 18:30.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Otwieram pięćdziesiątą piątą sesję, około dwudziestu-trzydziestu minut, z trzema kolegami.",
                verbalize("pl", U"Otwieram 55. sesję, około 20-30 minut, z 3 kolegami.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"To pięćset dwadzieścia cztery kilometry, jedenaście tysięcy trzysta czterdzieści dwa złote i "
      U"czterdziestopięciometrowe mieszkania z lat siedemdziesiątych.",
      verbalize("pl", U"To 524 km, 11 342 zł i 45-metrowe mieszkania z lat 70-tych.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Od kwoty trzystu tysięcy, nie powinno przekraczać pięciu gramów.",
                verbalize("pl", U"Od kwoty 300 tysięcy, nie powinno przekraczać 5 g.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Dnia piętnastego sierpnia dwa tysiące dwudziestego roku przyszło dwie osoby, a w dwa tysiące dwudziestym roku "
      U"było ich czterysta osiemdziesiąt.",
      verbalize("pl", U"Dnia 15.08.2020 r. przyszło 2 osoby, a w 2020 r. było ich 480.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Kosztuje dwanaście złotych dziewięćdziesiąt dziewięć groszy albo trzy miliony złotych, czyli dwadzieścia "
      U"procent więcej niż półtora roku temu.",
      verbalize("pl", U"Kosztuje 12,99 zł albo 3 mln zł, czyli 20% więcej niż 1,5 roku temu.",
                only(&Options::numbers)));
  ASSERT_EQUALS(U"Z lat siedemdziesiątych i osiemdziesiątych, w punkcie trzecim i rozdziale sześćset czternaście.",
                verbalize("pl", U"Z lat 70. i 80., w punkcie 3 i rozdziale 614.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Potrzebowali stu czterdziestu gramów mąki, stu trzydziestu gramów cukru.",
                verbalize("pl", U"Potrzebowali 140 g mąki, 130 g cukru.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Zadzwoń pod numer sześćset sto dwadzieścia trzy czterysta pięćdziesiąt sześć albo o siódmej piętnaście, "
      U"numerem trzy przez trzydzieści osiem.",
      verbalize("pl", U"Zadzwoń pod numer 600 123 456 albo o 7.15, numerem 3x38.", only(&Options::numbers)));
  ASSERT_EQUALS(U"W latach dwa tysiące dziesięć do dwa tysiące dwanaście zginęło tysiąc osób, w tym dwie kobiety.",
                verbalize("pl", U"W latach 2010-2012 zginęło 1 000 osób, w tym 2 kobiety.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Profesor Nowak mieszka przy ulicy Długiej pięć mieszkanie cztery, o godzinie piętnastej jedzie "
      U"dziewięćdziesiąt kilometrów na godzinę.",
      verbalize("pl", U"Prof. Nowak mieszka przy ul. Długiej 5 m. 4, o godz. 15 jedzie 90 km/h."));
  ASSERT_EQUALS(
      U"Hołownia zmierzy się, jak się okazało, w dziesięciu metrach sześciennych, o pięć punktów "
      U"procentowych.",
      verbalize("pl", U"Ho³ownia zmierzy siê, jak siÄ™ okazaÅ‚o, w 10 m³, o 5 pkt proc."));

  ASSERT_EQUALS(U"Kandydat zdobył dwie trzecie głosów, około jednej piątej wyjechało, w jednej ósmej finału.",
                verbalize("pl", U"Kandydat zdobył 2/3 głosów, około 1/5 wyjechało, w 1/8 finału."));
  ASSERT_EQUALS(U"Czekał półtorej godziny, ma dwa i pół roku, ulica Marszałkowska dziesięć łamane przez dwanaście.",
                verbalize("pl", U"Czekał 1 1/2 godziny, ma 2 1/2 roku, ul. Marszałkowska 10/12."));

  ASSERT_EQUALS(U"W trzeciej rundzie, na trzecim piętrze, z pierwszego miejsca, w ramach drugiej fazy.",
                verbalize("pl", U"W 3. rundzie, na 3. piętrze, z 1. miejsca, w ramach 2. fazy."));

  ASSERT_EQUALS(
      U"Dzwoń na pięćset trzy dwieście dwadzieścia dwa sto jedenaście, zadzwoń pod sto dwanaście, telefon "
      U"pięćdziesiąt osiem trzysta czterdzieści pięćdziesiąt zero zero.",
      verbalize("pl", U"Dzwoń na 503 222 111, zadzwoń pod 112, tel. 58 340 50 00."));

  ASSERT_EQUALS(
      U"Zatrudniamy piętnastu pracowników, cena wzrosła z czterdziestu złotych do pięćdziesięciu złotych, "
      U"temperatura spadła do minus siedmiu stopni, po pierwszym maja, kosztuje dziewięćdziesiąt dziewięć groszy.",
      verbalize("pl",
                U"Zatrudniamy 15 pracowników, cena wzrosła z 40 zł do 50 zł, temperatura spadła do -7 "
                U"stopni, po 1 maja, kosztuje 0,99 zł."));

  // "na" takes the accusative: na sekundę, na dobę
  ASSERT_EQUALS(U"Łącze dziesięć megabajtów na sekundę, dwadzieścia cztery godziny na dobę, dwie tony na hektar.",
                verbalize("pl", U"Łącze 10 MB/s, 24h/dobę, 2 t/ha.", only(&Options::numbers)));

  // A pair like a clock time among words of sport is a score, unless a time word comes before it
  ASSERT_EQUALS(U"Siatkarki objęły prowadzenie dwadzieścia do szesnastu, mecz o dwudziestej szesnaście.",
                verbalize("pl", U"Siatkarki objęły prowadzenie 20:16, mecz o 20:16.", only(&Options::numbers)));

  // A number before a form of a noun no count takes is an ordinal; a label (nr 4) stays a cardinal
  ASSERT_EQUALS(
      U"W czwartej lidze, w trzydziestej piątej minucie, na szóstym kroku, moje dwudzieste drugie urodziny, "
      U"numer cztery turnieju.",
      verbalize("pl", U"W 4 lidze, w 35 minucie, na 6 kroku, moje 22 urodziny, nr 4 turnieju."));

  // The case of a cardinal past a particle, after a conjunction, and an adjective before a feminine noun
  ASSERT_EQUALS(
      U"Do nawet siedmiuset złotych, spośród pięciuset dziesięciu ocen i sześćdziesięciu ośmiu recenzji, "
      U"na dwie równe części.",
      verbalize("pl", U"Do nawet 700 zł, spośród 510 ocen i 68 recenzji, na 2 równe części."));

  // Years after "przed" and "w samym roku"; dot ordinals after an adverb and after "został"
  ASSERT_EQUALS(
      U"Przed tysiąc dziewięćset dziewięćdziesiątym dziewiątym rokiem, w samym roku dwa tysiące szesnastym, "
      U"dwudziestym pierwszym najbardziej zakorkowanym, został jedenastym kierowcą.",
      verbalize("pl", U"Przed 1999 r., w samym roku 2016, 21. najbardziej zakorkowanym, został 11. kierowcą."));

  // cz. 2 is a heading; ~ after "około" isn't read twice
  ASSERT_EQUALS(U"Kuchnia część druga, około stu osiemdziesięciu złotych.",
                verbalize("pl", U"Kuchnia cz. 2, około ~180 zł."));

  // Dates in more shapes: a day and a month in parentheses or zero-padded (not the version 1.10), a two-digit year,
  // slashes, a day glued to its month, and a day after its month
  ASSERT_EQUALS(U"Wtorek (drugiego stycznia), dzień dwudziestego dziewiątego czerwca i wersja jeden dziesięć.",
                verbalize("pl", U"Wtorek (2.01), dzień 29.06 i wersja 1.10."));
  ASSERT_EQUALS(
      U"Ukazał się trzydziestego maja dwa tysiące dwudziestego trzeciego roku, ważna do trzydziestego "
      U"pierwszego lipca dwa tysiące osiemnastego.",
      verbalize("pl", U"Ukazał się 30.05.23 r., ważna do 31/07/2018."));
  ASSERT_EQUALS(U"Jutro (siedemnastego stycznia) i kwietnia czternastego, dwa tysiące siedemnastego.",
                verbalize("pl", U"Jutro (17stycznia) i kwietnia 14, 2017."));

  return true;
}

} // namespace verbalis::test
