#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// Dates glued to letters, as in scraped headings (Amora01/09/2015): each shape a pattern reads, and the neighbouring
// shapes it leaves to other patterns. The reading is spaced apart from the letters it touches.
REGISTER_TEST(glued_date_test) {

  // DatePattern: letters may touch a date with a four-digit year, before it, after it, or both
  ASSERT_EQUALS(U"Amora primeiro de setembro de dois mil e quinze a",
                verbalize("pt", U"Amora01/09/2015 a", only(&Options::numbers)));
  ASSERT_EQUALS(U"quattro agosto duemilasei L'UE finanzia",
                verbalize("it", U"04/08/2006L'UE finanzia", only(&Options::numbers)));
  ASSERT_EQUALS(U"trinta e um de outubro de dois mil e seis Teve lugar",
                verbalize("pt", U"31-10-2006Teve lugar", only(&Options::numbers)));
  ASSERT_EQUALS(U"veinticinco de febrero de dos mil quince Respecto",
                verbalize("es", U"25/Febrero/2015Respecto", only(&Options::numbers)));
  ASSERT_EQUALS(U"veintitrés de noviembre de dos mil catorce domingo",
                verbalize("es", U"23-nov-2014domingo", only(&Options::numbers)));
  ASSERT_EQUALS(U"Le vingt-sept octobre deux mille six La crémation",
                verbalize("fr", U"Le27/10/2006La crémation", only(&Options::numbers)));
  ASSERT_EQUALS(U"Am ersten September zweitausendfünfzehn war",
                verbalize("de", U"Am01.09.2015 war", only(&Options::numbers)));
  ASSERT_EQUALS(U"Am ersten September zweitausendfünfzehn war",
                verbalize("de", U"Am 01.09.2015war", only(&Options::numbers)));
  ASSERT_EQUALS(U"Amora primeiro de setembro de dois mil e quinze a",
                verbalize("pt", U"Amora01/09/2015a", only(&Options::numbers)));
  ASSERT_EQUALS(U"Dnia piętnastego sierpnia dwa tysiące dwudziestego czwartego abc",
                verbalize("pl", U"Dnia 15.08.2024abc", only(&Options::numbers)));
  // with a suffix after the year, which may also touch a two-digit year
  ASSERT_EQUALS(U"Dnia piętnastego sierpnia dwa tysiące dwudziestego roku przyszło",
                verbalize("pl", U"Dnia15.08.2020 r. przyszło", only(&Options::numbers)));
  ASSERT_EQUALS(U"Dnia piętnastego sierpnia dwa tysiące dwudziestego roku przyszło",
                verbalize("pl", U"Dnia15.08.2020r. przyszło", only(&Options::numbers)));
  ASSERT_EQUALS(U"W czwartek (dwudziestego drugiego sierpnia dwa tysiące dwudziestego czwartego roku) około",
                verbalize("pl", U"W czwartek (22.08.24r.) około", only(&Options::numbers)));
  // not with a two-digit year (even one written out as 2024), nor without a year
  ASSERT_EQUALS(U"Amora zero um/zero nove/quinze a", verbalize("pt", U"Amora01/09/15 a", only(&Options::numbers)));
  ASSERT_EQUALS(U"zero um/zero nove/quinze a", verbalize("pt", U"01/09/15a", only(&Options::numbers)));
  ASSERT_EQUALS(U"Abc dwanaście zero pięć dwadzieścia cztery r",
                verbalize("pl", U"Abc12.05.24 r", only(&Options::numbers)));
  ASSERT_EQUALS(U"Dnia piętnaście zero osiem dwadzieścia cztery abc",
                verbalize("pl", U"Dnia 15.08.24abc", only(&Options::numbers)));
  ASSERT_EQUALS(U"Amora dezenove.ago a", verbalize("pt", U"Amora19.ago a", only(&Options::numbers)));

  // Polish GluedDatePattern: a day glued to the letters before it or to its month in the genitive, then the year
  ASSERT_EQUALS(U"Jean osiemnastego listopada dwa tysiące piętnastego piętnasta czterdzieści sześć",
                verbalize("pl", U"Jean18 listopada 2015 15:46", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pandas szesnastego czerwca dwa tysiące siedemnastego jedenasta dwadzieścia jeden",
                verbalize("pl", U"Pandas16 czerwca 2017 11:21", only(&Options::numbers)));
  ASSERT_EQUALS(U"Jutro (siedemnastego stycznia), jak",
                verbalize("pl", U"Jutro (17stycznia), jak", only(&Options::numbers)));
  ASSERT_EQUALS(U"dzisiejsza (dwudziestego czwartego maja dwa tysiące osiemnastego) aktualizacja",
                verbalize("pl", U"dzisiejsza (24maja 2018) aktualizacja", only(&Options::numbers)));
  ASSERT_EQUALS(U"Jean osiemnastego listopada", verbalize("pl", U"Jean18listopada", only(&Options::numbers)));
  // a year touching letters stays out of the date
  ASSERT_EQUALS(U"Jean osiemnastego listopada dwa tysiące piętnaście r",
                verbalize("pl", U"Jean18 listopada 2015r", only(&Options::numbers)));
  // not a month touching letters, three digits, or a day past 31
  ASSERT_EQUALS(U"Jean osiemnaście listopadaX", verbalize("pl", U"Jean18 listopadaX", only(&Options::numbers)));
  ASSERT_EQUALS(U"sto osiemnaście listopada", verbalize("pl", U"118listopada", only(&Options::numbers)));
  ASSERT_EQUALS(U"Jean trzydzieści dwa listopada", verbalize("pl", U"Jean32 listopada", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
