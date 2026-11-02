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
  ASSERT_EQUALS(U"A las dieciocho y media o a las nueve y cuarto, pero uno punto trece metros.",
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
  ASSERT_EQUALS(
      U"Quedó cuarto en la primera ronda, el primer día, y vigesimoprimero en la decimotercera.",
      verbalize("es", U"Quedó 4.º en la 1.ª ronda, el 1.er día, y 21.º en la 13.ª.", only(&Options::numbers)));
  // ° is an ordinal by its article or noun, else degrees
  ASSERT_EQUALS(
      U"El segundo lugar, la sexta fecha, primer piso, el trigésimo tercer Campeonato, a treinta grados y "
      U"dieciocho°cincuenta y dos'.",
      verbalize("es", U"El 2° lugar, la 6° fecha, 1° piso, el 33°Campeonato, a 30° y 18°52'.",
                only(&Options::numbers)));
  ASSERT_EQUALS(U"Abre de nueve de la mañana a diecinueve horas y cierra a las cinco y dos de la tarde.",
                verbalize("es", U"Abre de 9 a. m. a 19 h y cierra a las 5:02 p. m.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Costó cincuenta y cinco centavos y tres billones de dólares.",
                verbalize("es", U"Costó $0,55 y US$3 billones.", only(&Options::numbers)));
  ASSERT_EQUALS(U"En los ochentas y los noventas hubo cambios.",
                verbalize("es", U"En los 80s y los 90's hubo cambios.", only(&Options::numbers)));
  ASSERT_EQUALS(U"La versión tres punto ocho punto diez y el resultado dos a uno, el marcador tres a dos.",
                verbalize("es", U"La versión 3.8.10 y el resultado 2-1, el marcador 3:2.", only(&Options::numbers)));
  ASSERT_EQUALS(U"En el minuto noventa más seis y dos más dos.",
                verbalize("es", U"En el minuto 90+6 y 2 + 2.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Alcanzó mil novecientos ochenta coma seis millones de dólares.",
                verbalize("es", U"Alcanzó 1.980,6 millones de dólares.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Son las una y cinco y la una.", verbalize("es", U"Son las 1:05 y la 1.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pagó mil doscientos treinta y cuatro euros con cincuenta y seis por dos libros.",
                verbalize("es", U"Pagó 1.234,56 € por 2 libros.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Son ochocientas sesenta mil personas y cinco mil dos coma uno tres dos.",
                verbalize("es", U"Son 860 000 personas y 5.002,132.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Cuesta diez dólares, diez centavos, un euro con cinco céntimos y un millón de pesos.",
                verbalize("es", U"Cuesta $10,00, $0,1, 1,05 € y $1.000.000 pesos.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pagó veintiún córdobas, dos dólares estadounidenses y cincuenta quetzales, en el Q tres.",
                verbalize("es", U"Pagó 21 NIO, USD 2 y Q50, en el Q3.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Añade media taza, dos tercios de la harina, un dieciseisavo y cinco sextas partes.",
                verbalize("es", U"Añade 1/2 taza, 2/3 de la harina, 1/16 y 5/6 partes.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Recorrió medio kilómetro, un metro y medio, menos dos y un tercio y tres octavos de pulgada.",
                verbalize("es", U"Recorrió 1/2 km, 1 1/2 m, -2 1/3 y 3/8\".", only(&Options::numbers)));
  ASSERT_EQUALS(U"Juegan el quince de junio y del tres de mayo al siete de mayo, abierto veinticuatro siete.",
                verbalize("es", U"Juegan el 15/06 y del 3/5 al 7/5, abierto 24/7.", only(&Options::numbers)));
  ASSERT_EQUALS(U"un millonésimo, un dosmilésimo, uno sobre ciento uno y la tesis ciento diecisiete/dos mil cuatro.",
                verbalize("es", U"1/1.000.000, 1/2000, 1/101 y la tesis 117/2004.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Hizo nueve coma cinco segundos a ciento veinte kilómetros por hora y menos ocho grados centígrados.",
                verbalize("es", U"Hizo 9,5 s a 120 kph y -8° c.", only(&Options::numbers)));

  // A falling or tied range among words of sport is a score; any other keeps its hyphen
  ASSERT_EQUALS(U"Argentina ganó dos a cero y empató uno a uno; vivió diez-diecisiete años.",
                verbalize("es", U"Argentina ganó 2-0 y empató 1-1; vivió 10-17 años.", only(&Options::numbers)));

  // A hyphen after a word joins it to the number; a minus needs no word before it, or a time zone: GMT -4
  ASSERT_EQUALS(U"Viajó por Portugal -dos mil quince a dos mil diecisiete-, con COVID -diecinueve y GMT menos cuatro.",
                verbalize("es", U"Viajó por Portugal -2015 a 2017-, con COVID -19 y GMT -4.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Fue el cinco de mayo, detenido durante más de diez minutos, gol (treinta y seis minutos PT).",
      verbalize("es", U"Fue el 05 de mayo, detenido durante más de 10', gol (36' PT).", only(&Options::numbers)));

  // The noun after a second number sets the gender; a year, a numbered item, or a bracket takes none
  ASSERT_EQUALS(
      U"Para cuatrocientas y quinientas entrevistas, doscientos dias; en mil novecientos setenta y uno publicó, Arte "
      U"número uno (ESEA).",
      verbalize("es", U"Para 400 y 500 entrevistas, 200 dias; en 1971 publicó, Arte número 1 (ESEA).",
                only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
