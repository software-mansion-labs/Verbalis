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
  ASSERT_EQUALS(
      U"Le millième et le millionième visiteur, un cinquième de la ville, moins quatre et un tiers.",
      verbalize("fr", U"Le 1 000e et le 1 000 000e visiteur, 1/5 de la ville, -4 1/3.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Les dix-sept, dix-huit juin, le deux mars deux mille trois, dans les années quatre-vingt.",
                verbalize("fr", U"Les 17/18 juin, le 02-03-2003, dans les 80s.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Il y a quatre cent quinze mille deux cent quatre-vingt-dix-sept habitants, sept cinq zéro zéro un Paris.",
      verbalize("fr", U"Il y a 415297 habitants, 75001 Paris.", only(&Options::numbers)));
  ASSERT_EQUALS(U"deux mille euros par mois, trois euros cinquante, un million de dollars.",
                verbalize("fr", U"2 000 €/mois, 3 € 50, 1 000 000 $.", only(&Options::numbers)));
  ASSERT_EQUALS(U"un mètre soixante-cinq, un mètre quatre-vingts, trois kilomètres et demi.",
                verbalize("fr", U"1m65, 1,80 m, 3 1/2 km.", only(&Options::numbers)));
  ASSERT_EQUALS(U"En mille neuf cent quatre-vingt, page deux cent, mille neuf cent quatre-vingts habitants.",
                verbalize("fr", U"En 1980, page 200, 1980 habitants.", only(&Options::numbers)));
  ASSERT_EQUALS(U"La version trois point huit point dix, quarante et une maisons et une propriété.",
                verbalize("fr", U"La version 3.8.10, 41 maisons et 1 propriété.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Au vingtième siècle, monsieur et madame Durand, messieurs les députés.",
                verbalize("fr", U"Au XXe s., M. et Mme Durand, MM. les députés."));
  ASSERT_EQUALS(U"monsieur Dupont et madame Martin, le docteur Roux à saint-Étienne, et cetera.",
                verbalize("fr", U"M. Dupont et Mme Martin, le Dr Roux à St-Étienne, etc."));

  ASSERT_EQUALS(
      U"Elle consomme cent vingt-cinq kilowattheures par mètre carré par an à vingt et un euros par "
      U"mégawattheure.",
      verbalize("fr", U"Elle consomme 125 kWh/m2/an à 21 €/MWh.", only(&Options::numbers)));

  // Prime marks: three parts are always a time, two only in a text about time; a lone 20" is inches
  ASSERT_EQUALS(
      U"Il a terminé en douze minutes trente-six secondes et soixante-treize centièmes, avec trois minutes "
      U"seize secondes d'avance.",
      verbalize("fr", U"Il a terminé en 12’36’’73, avec 3'16\" d'avance.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Un film de trois'seize\" et un écran plat vingt pouces, le mot \"dix\".",
                verbalize("fr", U"Un film de 3'16\" et un écran plat 20\", le mot \"10\".", only(&Options::numbers)));

  ASSERT_EQUALS(U"Le quatre juin.", verbalize("fr", U"Le 04 juin.", only(&Options::numbers)));

  // The noun after a second number sets the gender; a demonstrative is not a noun
  ASSERT_EQUALS(U"De une ou deux places, en Ligue un cette saison.",
                verbalize("fr", U"De 1 ou 2 places, en Ligue 1 cette saison.", only(&Options::numbers)));

  // A two-digit year with a leading zero, a slashed day and month after "le" (not before "des"), a padded day
  ASSERT_EQUALS(U"Le dix-huit août deux mille huit, le vingt-deux septembre, le un tiers des voix, du cinq mai.",
                verbalize("fr", U"Le 18/08/08, le 22/9, le 1/3 des voix, du 05 mai."));

  return true;
}

} // namespace verbalis::test
