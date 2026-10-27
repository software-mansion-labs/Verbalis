#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// Portuguese readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_pt_test) {

  ASSERT_EQUALS(
      U"No dia vinte e quatro de março de dois mil e vinte e dois e sete de novembro de dois mil e dezoito, às catorze "
      U"horas e trinta e às vinte e uma horas.",
      verbalize("pt", U"No dia 24/03/2022 e 07-11-2018, às 14h30 e às 21h.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Ligue para zero oitocentos, um dois três quatro cinco seis ou três quatro cinco seis, sete oito nove zero, de "
      U"dois mil e vinte e um a dois mil e vinte e dois.",
      verbalize("pt", U"Ligue para 0800-123456 ou 3456-7890, de 2021-2022.", only(&Options::numbers)));
  ASSERT_EQUALS(U"São vinte e duas mil pessoas, dois mil reais e um vírgula cinco milhão de reais.",
                verbalize("pt", U"São 22 mil pessoas, 2 mil reais e R$ 1,5 milhão.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Custa vinte e nove reais e noventa centavos e um real e cinquenta centavos, trinta e sete mil quinhentos e "
      U"dezessete reais e cinquenta reais.",
      verbalize("pt", U"Custa R$ 29,90 e R$ 1,00 e R$ 0,50, R$ 37,517 e R$ 50 reais.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Tem vinte e um anos, vinte e uma casas e primeiro de maio, às duas da tarde.",
                verbalize("pt", U"Tem 21 anos, 21 casas e 1 de maio, às 2 da tarde.", only(&Options::numbers)));
  ASSERT_EQUALS(U"O primeiro e a segunda, artigo oitenta e o décimo primeiro lugar.",
                verbalize("pt", U"O 1º e a 2ª, artigo 80º e o 11º lugar.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Entre dois mil e dezesseis a dois mil e vinte e dois e dez a quinze quilômetros, o placar quatro-três-três e o "
      U"A trezentos e vinte duzentos.",
      verbalize("pt", U"Entre 2016-2022 e 10-15 km, o placar 4-3-3 e o A320-200.", only(&Options::numbers)));
  ASSERT_EQUALS(U"São três milhões de habitantes e um bilhão, quatro K e quinhentos mil seguidores.",
                verbalize("pt", U"São 3 mi de habitantes e 1 bi, 4K e 500k seguidores.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Subiu vinte e cinco por cento e três vírgula cinco por cento ao ano, um vírgula cinco quilômetro e dois quilos "
      U"e um grama.",
      verbalize("pt", U"Subiu 25% e 3,5% ao ano, 1,5 km e 2 kg e 1 g.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"São um milhão de pessoas, dois mil e trezentos e um milhão e duzentos mil, dois mil e um e mil quinhentos e "
      U"vinte.",
      verbalize("pt", U"São 1.000.000 de pessoas, 2.300 e 1.200.000, 2.001 e 1.520.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Às treze horas, nove horas e uma hora.",
                verbalize("pt", U"Às 13:00, 9h e 1h.", only(&Options::numbers)));
  ASSERT_EQUALS(U"três bilhões de dólares e dois vírgula cinco milhões de euros, uma libra e duas libras.",
                verbalize("pt", U"US$ 3 bilhões e € 2,5 milhões, £ 1 e £ 2.", only(&Options::numbers)));
  ASSERT_EQUALS(U"O evento de trinta de janeiro de dois mil e vinte e três.",
                verbalize("pt", U"O evento de 2023-01-30.", only(&Options::numbers)));

  ASSERT_EQUALS(U"Às catorze horas e trinta minutos e cinco segundos, à uma hora e cinco e às três horas da tarde.",
                verbalize("pt", U"Às 14:30:05, à 1h05 e às 15h da tarde.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Pelas vinte e uma horas e trinta, às vinte horas e trinta e um ponto setenta e um.",
                verbalize("pt", U"Pelas 21.30h, às 20:30 hs e 1.71.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"No dia quinze de setembro, quinta-feira (primeiro de março), três de abril, quinze de março de dois mil e "
      U"vinte e quatro e trinta e um de dezembro de dois mil e vinte e cinco.",
      verbalize("pt", U"No dia 15/9, quinta-feira (1/3), 03/04, 2024.03.15 e 12/31/2025.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"três oitavos, cinco treze avos, um quarto do salário, meia xícara, uma e meia xícara e três/seis, seis/dois.",
      verbalize("pt", U"3/8, 5/13, 1/4 do salário, 1/2 xícara, 1 1/2 xícara e 3/6, 6/2.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Ligue um um, três três três três, quatro quatro quatro quatro extensão doze, cinco cinco cinco, um "
      U"dois três quatro ou zero oitocentos, sete seis cinco quatro três dois um; quinhentos-mil.",
      verbalize("pt", U"Ligue 11 3333-4444 ext. 12, 555-1234 ou 0800-7654321; 500-1000."));
  ASSERT_EQUALS(
      U"Em dois mil e vinte havia cento e nove mil trezentos e noventa e nove nados vivos, duas irmãs e um "
      U"milhão oitocentos e onze mil seiscentos e cinco reais; ligue através do sete zero sete, dois quatro "
      U"seis, sete zero sete.",
      verbalize("pt", U"Em 2020 havia 109 399 nados vivos, 2 irmãs e R$ 1811605; ligue através do 707 246 707."));
  ASSERT_EQUALS(U"duzentos gramas em trinta minutos e quarenta e cinco segundos, trinta segundos e os anos noventa.",
                verbalize("pt", U"200 g em 30 min e 45 s, 30s e os anos 90s."));

  return true;
}

} // namespace verbalis::test
