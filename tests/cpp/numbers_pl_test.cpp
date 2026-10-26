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
  ASSERT_EQUALS(U"W latach dwa tysiące dziesięć-dwa tysiące dwanaście zginęło tysiąc osób, w tym dwie kobiety.",
                verbalize("pl", U"W latach 2010-2012 zginęło 1 000 osób, w tym 2 kobiety.", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
