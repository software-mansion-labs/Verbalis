#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// Phone numbers: international ones (+48 ...) in every language, and each language's own formats.
// Formats written with spaces are counts unless a word says they are phone numbers (Kup 600 123 456 jabłek).
REGISTER_TEST(numbers_phone_test) {
  ASSERT_EQUALS(
      U"Call five five five, one two three, four five six seven, five five five, one two three, four five six seven or "
      U"five five five, one two three, four five six seven.",
      verbalize("en-us", U"Call (555) 123-4567, 555.123.4567 or 555 123 4567.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Call plus one five five five, one two three, four five six seven or plus forty-four zero, two zero, seven nine "
      U"four six, zero nine five eight.",
      verbalize("en-us", U"Call +1 555 123 4567 or +44 (0)20 7946 0958.", only(&Options::numbers)));

  ASSERT_EQUALS(
      U"Zadzwoń na plus czterdzieści osiem sześćset sto dwadzieścia trzy czterysta pięćdziesiąt sześć lub dwadzieścia "
      U"dwa sto dwadzieścia trzy czterdzieści pięć sześćdziesiąt siedem.",
      verbalize("pl", U"Zadzwoń na +48 600 123 456 lub 22 123 45 67.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Infolinia: dwadzieścia dwa sto dwadzieścia trzy czterdzieści pięć sześćdziesiąt siedem, telefony sześćdziesiąt "
      U"osiem trzysta dwadzieścia cztery dwadzieścia dwa pięćdziesiąt pięć lub osiemset jeden trzysta dwadzieścia "
      U"siedem czterysta sześćdziesiąt dwa.",
      verbalize("pl", U"Infolinia: 22 123 45 67, telefony 68-324-22-55 lub 801-327-462.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Kup sześćset milionów sto dwadzieścia trzy tysiące czterysta pięćdziesiąt sześć jabłek.",
                verbalize("pl", U"Kup 600 123 456 jabłek.", only(&Options::numbers)));

  ASSERT_EQUALS(U"Appelez le plus trente-trois six douze trente-quatre cinquante-six soixante-dix-huit.",
                verbalize("fr", U"Appelez le +33 6 12 34 56 78.", only(&Options::numbers)));

  ASSERT_EQUALS(
      U"Llame al seis cero cero uno dos tres cuatro cinco seis o al más treinta y cuatro nueve uno dos tres cuatro "
      U"cinco seis siete ocho.",
      verbalize("es", U"Llame al 600 123 456 o al +34 912 345 678.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Son seiscientos ciento veintitrés cuatrocientos cincuenta y seis euros.",
                verbalize("es", U"Son 600 123 456 euros.", only(&Options::numbers)));

  ASSERT_EQUALS(
      U"Chiama il tre tre tre uno due tre quattro cinque sei sette o il zero sei uno due tre quattro cinque sei sette "
      U"otto.",
      verbalize("it", U"Chiama il 333 1234567 o il 06 1234 5678.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Chiama il più trentanove tre tre tre uno due tre quattro cinque sei sette.",
                verbalize("it", U"Chiama il +39 333 123 4567.", only(&Options::numbers)));

  ASSERT_EQUALS(
      U"Ligue para um um, nove oito sete seis cinco, quatro três dois um ou mais cinquenta e cinco um um, nove oito "
      U"sete seis cinco, quatro três dois um.",
      verbalize("pt", U"Ligue para (11) 98765-4321 ou +55 11 98765-4321.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ligue para dois um três, quatro cinco seis, sete oito nove.",
                verbalize("pt", U"Ligue para 213 456 789.", only(&Options::numbers)));

  ASSERT_EQUALS(U"Telefon: plus neunundvierzig drei null eins zwei drei vier fünf sechs sieben.",
                verbalize("de", U"Telefon: +49 30 1234567.", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
