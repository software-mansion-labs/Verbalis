#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// French readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_fr_test) {

  ASSERT_EQUALS(
      U"Le premier janvier deux mille vingt à dix-huit heures trente, vingt et une personnes, trois euros cinquante.",
      verbalize("fr", U"Le 1er janvier 2020 à 18h30, 21 personnes, 3,50 €.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Au vingtième siècle, au zéro six douze trente-quatre cinquante-six soixante-dix-huit.",
                verbalize("fr", U"Au XXe siècle, au 06 12 34 56 78.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Elle a vingt et un ans et trente et une heures de travail.",
                verbalize("fr", U"Elle a 21 ans et 31 heures de travail.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Les années quatre-vingt et deux cent mille habitants, quatre-vingts pages.",
                verbalize("fr", U"Les années 80 et 200 000 habitants, 80 pages.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Le cinq novembre et le trois quarts du gâteau, puis un demi litre.",
                verbalize("fr", U"Le 05/11 et le 3/4 du gâteau, puis 1/2 litre.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Rendez-vous à dix-huit heures trente ou quatorze heures, pas un virgule treize mètres.",
                verbalize("fr", U"Rendez-vous à 18.30 ou 14h, pas 1.13 mètres.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il a gagné trois millions d'euros et dix mille abonnés, quatre mille vidéo.",
                verbalize("fr", U"Il a gagné 3 millions d'euros et 10k abonnés, 4K vidéo.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Une pièce de douze fois quatre = quarante-huit et un écran de mille neuf cent vingt par mille quatre-vingts.",
      verbalize("fr", U"Une pièce de 12 x 4 = 48 et un écran de 1920x1080.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il est arrivé deuxième, puis première et seconde, les troisièmes.",
                verbalize("fr", U"Il est arrivé 2e, puis 1re et 2nde, les 3èmes.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Il fait moins cinq degrés Celsius, vingt-cinq kilomètres par heure et un virgule cinq kilogramme de farine.",
      verbalize("fr", U"Il fait -5 °C, 25 km/h et 1,5 kg de farine.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"soixante-quinze centimes et quatre-vingt-dix-neuf centimes, une livre et deux virgule cinq livres, trois "
      U"millions d'euros.",
      verbalize("fr", U"0,75 € et 99 centimes, 1 £ et 2,5 £, 3M€.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Le quatorze juillet mille sept cent quatre-vingt-neuf et le premier mai deux mille vingt-quatre.",
                verbalize("fr", U"Le 14 juillet 1789 et le 1.5.2024.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il a cent un ans et soixante et onze euros, quatre-vingt-onze pommes, quatre-vingt-une voitures.",
                verbalize("fr", U"Il a 101 ans et 71 euros, 91 pommes, 81 voitures.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Il était quatorze heures vingt ce mercredi.",
                verbalize("fr", U"Il était 14 h 20 ce mercredi.", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
