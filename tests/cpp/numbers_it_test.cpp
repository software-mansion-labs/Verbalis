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
  ASSERT_EQUALS(U"Costa dieci euro e cinquanta centesimi e un euro o tre milioni di euro e diecimila euro.",
                verbalize("it", U"Costa 10,50 € e 1,00 € o 3 mln € e 10 mila €.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Ci sono settantaquattromila persone e uno virgola cinque milioni di abitanti, quattro K e quarantamila.",
      verbalize("it", U"Ci sono 74 mila persone e 1,5 milioni di abitanti, 4K e 40K.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pesa un chilo e ventitré chili, un chilometro e centotto chilometri orari.",
                verbalize("it", U"Pesa 1 kg e 23 kg, 1 km e 108 km/h.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il primo e la seconda, il ventunesimo e la trentatreesima.",
                verbalize("it", U"Il 1º e la 2ª, il 21esimo e la 33esima.", only(&Options::numbers)));
  ASSERT_EQUALS(U"La versione due punto settantanove e uno punto tredici, il risultato due a uno e tre a due.",
                verbalize("it", U"La versione 2.79 e 1.13, il risultato 2-1 e 3:2.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Sono un milione di abitanti e duemilatrecentoquarantacinque persone, milletré e millecentotto e milleottanta.",
      verbalize("it", U"Sono 1.000.000 di abitanti e 2.345 persone, 1003 e 1108 e 1080.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Nel duemilaventitré e nel millenovecentonovantanove, ventitré e trentatré e centotré.",
                verbalize("it", U"Nel 2023 e nel 1999, 23 e 33 e 103.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ha un milione e due miliardi e milleuno e undicimila.",
                verbalize("it", U"Ha 1000000 e 2000000000 e 1001 e 11000.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Costa cinquanta centesimi, un centesimo, tre euro e cinquanta centesimi, due sterline e ottanta penny, "
      U"quattro virgola due sterline.",
      verbalize("it", U"Costa 0,50 €, 0,01 €, 3.50 €, 2,80 £, 4,2 £.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Ha un figlio e una figlia, uno studente, un' ora, ma l'articolo uno comma due e uno su tre.",
                verbalize("it", U"Ha 1 figlio e 1 figlia, 1 studente, 1 ora, ma l'articolo 1 comma 2 e 1 su 3.",
                          only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Il quattro virgola tre per cento, ventinove virgola otto gradi e pesava tre virgola due chili, ma "
      U"Python tre punto dodici e la versione due punto uno.",
      verbalize("it", U"Il 4.3%, 29.8°C e pesava 3.2 chili, ma Python 3.12 e la versione 2.1.",
                only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Il primo gennaio, il secondo posto, la terza edizione, l'undicesima edizione, ma novanta gradi.",
      verbalize("it", U"Il 1° gennaio, il 2° posto, la 3° edizione, l'11° edizione, ma 90°.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Il diciassettesimo Meeting, terzo lotto, quinta tappa, del secondo battaglione, a trenta gradi.",
      verbalize("it", U"Il 17° Meeting, 3° lotto, 5° tappa, del 2.° battaglione, a 30°.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Alle diciassette e quindici minuti e ventisei secondi, il ventiquattro marzo duemilaventidue.",
                verbalize("it", U"Alle 17:15:26, il 24.03.2022.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Dura un'ora, centocinquantacinque giorni, quindici ampere, cinque chilometri al secondo, mezzo litro.",
      verbalize("it", U"Dura 1 h, 155 d, 15 A, 5 km/s, 1/2 litro.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il signor Rossi, la dottoressa Neri e l'avvocato Bianchi in piazza Grande, eccetera.",
                verbalize("it", U"Il Sig. Rossi, la Dott.ssa Neri e l'avv. Bianchi in p.za Grande, ecc."));

  // A falling or tied range among words of sport, or after an elided article, is a score
  ASSERT_EQUALS(U"Il Milan ha vinto due a uno dopo l'uno a uno, con trecentocinquanta-quattrocentocinquanta grammi.",
                verbalize("it", U"Il Milan ha vinto 2-1 dopo l'1-1, con 350-450 grammi.", only(&Options::numbers)));

  // Prime marks: a time joined by "e", and a lone 19″ in inches
  ASSERT_EQUALS(
      U"Ha chiuso in un minuto e dodici secondi e settantasei centesimi, con venticinque secondi di vantaggio.",
      verbalize("it", U"Ha chiuso in 1'12\"76, con 25” di vantaggio.", only(&Options::numbers)));
  ASSERT_EQUALS(U"In due ore undici minuti e tredici secondi, su uno schermo da diciannove pollici.",
                verbalize("it", U"In 2h11'13\", su uno schermo da 19″.", only(&Options::numbers)));
  ASSERT_EQUALS(U"È alto cinque piedi e cinque pollici.", verbalize("it", U"È alto 5'5\".", only(&Options::numbers)));

  // 1 after a participle counts its noun; a range takes the gender of a plural in -e
  ASSERT_EQUALS(U"Inclusa una bevanda e una-due volte al giorno.",
                verbalize("it", U"Inclusa 1 bevanda e 1-2 volte al giorno.", only(&Options::numbers)));

  // A day and a month without a year after an article, unless they may be a fraction or a decimal
  ASSERT_EQUALS(
      U"Dal ventuno settembre al ventisei settembre, il due terzi dei voti, al ventiquattro virgola tre per "
      U"cento, del venticinque febbraio.",
      verbalize("it", U"Dal 21/9 al 26/9, il 2/3 dei voti, al 24.3%, del 25.02."));

  return true;
}

} // namespace verbalis::test
