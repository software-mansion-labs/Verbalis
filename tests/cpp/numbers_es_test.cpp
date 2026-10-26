#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// Spanish readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_es_test) {

  ASSERT_EQUALS(U"La Ley dos de dos mil dieciocho entró en vigor el catorce de julio de dos mil veintitrés.",
                verbalize("es", U"La Ley 2/2018 entró en vigor el 14/07/2023.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Llame al seis cero cero uno dos tres cuatro cinco seis o al nueve uno dos tres cuatro cinco seis siete ocho.",
      verbalize("es", U"Llame al 600-123-456 o al 912-345-678.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Tiene veintiún años y treinta y una personas, una casa y veintiuna mesas.",
                verbalize("es", U"Tiene 21 años y 31 personas, 1 casa y 21 mesas.", only(&Options::numbers)));
  ASSERT_EQUALS(U"A las dieciocho treinta o a las nueve quince, pero uno punto trece metros.",
                verbalize("es", U"A las 18:30 o a las 9.15, pero 1.13 metros.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Cuesta cinco dólares con cincuenta o tres euros con noventa y nueve y una libra, son cinco mil dólares.",
      verbalize("es", U"Cuesta $5,50 o 3,99 € y 1 £, son $5.000 dólares.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ganó tres millones de euros y dos mil quinientos millones.",
                verbalize("es", U"Ganó 3 millones de euros y 2.500 millones.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Tiene diez mil seguidores y una tarjeta cuatro K.",
                verbalize("es", U"Tiene 10k seguidores y una tarjeta 4K.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Mide un kilómetro y veintiún kilómetros, uno coma cinco kilogramos de harina, una hectárea.",
                verbalize("es", U"Mide 1 km y 21 km, 1,5 kg de harina, 1 ha.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Llegó primero y segunda, el tercer puesto y el primer día, la quinta.",
                verbalize("es", U"Llegó 1º y 2ª, el 3er puesto y el 1er día, la 5ta.", only(&Options::numbers)));
  ASSERT_EQUALS(U"En los ochentas y los noventas hubo cambios.",
                verbalize("es", U"En los 80s y los 90's hubo cambios.", only(&Options::numbers)));
  ASSERT_EQUALS(U"La versión tres punto ocho punto diez y el resultado dos-uno, el marcador tres a dos.",
                verbalize("es", U"La versión 3.8.10 y el resultado 2-1, el marcador 3:2.", only(&Options::numbers)));
  ASSERT_EQUALS(U"En el minuto noventa más seis y dos más dos.",
                verbalize("es", U"En el minuto 90+6 y 2 + 2.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Alcanzó mil novecientos ochenta,6 millones de dólares.",
                verbalize("es", U"Alcanzó 1.980,6 millones de dólares.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Son las una cinco y la una.", verbalize("es", U"Son las 1:05 y la 1.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pagó mil doscientos treinta y cuatro,56 € por dos libros.",
                verbalize("es", U"Pagó 1.234,56 € por 2 libros.", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
