#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

// English readings follow the Echo verbalizer; ISO dates are read by Verbalis only.
REGISTER_TEST(numbers_en_test) {

  // Race times, plural units before an adverb, and four-digit seasons
  ASSERT_EQUALS(
      U"won in one minute three point nine one seconds, sixty million dollars annually, twenty kilometers "
      U"northwest, a three million dollar settlement, the twenty twenty-four to twenty-five budget",
      verbalize("en-us",
                U"won in 1:03.91, $60 million annually, 20 km northwest, a $3 million settlement, the 2024-25 "
                U"budget"));
  ASSERT_EQUALS(U"The meeting is at ten thirty, about twenty to twenty-five minutes.",
                verbalize("en-us", U"The meeting is at 10:30, about 20-25 minutes.", only(&Options::numbers)));
  ASSERT_EQUALS(U"He was born on March third, nineteen ninety, and paid one point five million dollars.",
                verbalize("en-us", U"He was born on March 3, 1990, and paid $1.5 million.", only(&Options::numbers)));
  ASSERT_EQUALS(U"About one thousand nine hundred ninety-nine people, a five dollar bill and SCP six oh six's.",
                verbalize("en-us", U"About 1999 people, a $5 bill and SCP-606's.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"It costs four dollars and ninety-nine cents or ninety-nine cents, maybe fifteen euros or fifty dollars.",
      verbalize("en-us", U"It costs $4.99 or 99 cents, maybe €15 or 50$.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"The twenty-first century began in two thousand one; the nineteen nineties and eighties were fun.",
      verbalize("en-us", U"The 21st century began in 2001; the 1990s and '80s were fun.", only(&Options::numbers)));
  ASSERT_EQUALS(U"A seventy-five millimeter gun fired three rounds at seven p.m., ten to fifteen kilometers away.",
                verbalize("en-us", U"A 75mm gun fired 3 rounds at 7pm, 10-15 km away.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Version three point eight point ten runs on ten dot zero dot zero dot fifty-one with nineteen twenty by ten "
      U"eighty screens.",
      verbalize("en-us", U"Version 3.8.10 runs on 10.0.0.51 with 1920x1080 screens.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Call one five five five, one two three, four five six seven or five five five, one two three four now.",
      verbalize("en-us", U"Call 1-555-123-4567 or 555-1234 now.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"Temperatures of minus five degrees Celsius, twenty percent humidity and one half of the three/seven votes.",
      verbalize("en-us", U"Temperatures of -5 °C, 20% humidity and 1/2 of the 3/7 votes.", only(&Options::numbers)));
  ASSERT_EQUALS(U"The score was two to one and five minus three equals two.",
                verbalize("en-us", U"The score was 2:1 and 5 - 3 equals 2.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"MP three, COVID nineteen, F sixteens and H two sixty-four are codes; a twenty-one-year-old won.",
      verbalize("en-us", U"MP3, COVID-19, F-16s and H.264 are codes; a 21-year-old won.", only(&Options::numbers)));
  ASSERT_EQUALS(U"It's one thirteen not one thirteen, and two plus two.",
                verbalize("en-us", U"It's 1.13 not 1:13, and 2 + 2.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"On the twenty-seventh of March twenty twenty-six and the twenty-seventh of March twenty twenty-six we met.",
      verbalize("en-us", U"On 2026-03-27 and 27.03.2026 we met.", only(&Options::numbers)));
  ASSERT_EQUALS(U"We counted one million people and point five of them.",
                verbalize("en-us", U"We counted 1,000,000 people and .5 of them.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"It costs one cent, twenty dollars per month, fifty cents per hour and twenty dollars and fifty cents.",
      verbalize("en-us", U"It costs $.01, $20/mo, $0.5/hr and $20.5.", only(&Options::numbers)));
  ASSERT_EQUALS(
      U"It is thirty billion yen, seven point four five million pounds and seventy-six point three trillion US "
      U"dollars.",
      verbalize("en-us", U"It is ¥30b, £7.45m and US$76.3 trillion.", only(&Options::numbers)));
  ASSERT_EQUALS(U"Open two p.m. to five p.m., two percent to ten percent, fifty dollars to one hundred dollars.",
                verbalize("en-us", U"Open 2pm-5pm, 2%-10%, $50-$100.", only(&Options::numbers)));
  ASSERT_EQUALS(U"So one minus two equals five, two times eight is sixteen, and apt five b is three meters tall.",
                verbalize("en-us", U"So 1-2=5, 2*8 is 16, and apt 5b is 3 m tall."));
  ASSERT_EQUALS(
      U"On the twenty-fifth of july twenty twelve, January fifteenth, the third of July twenty sixteen and in the "
      U"second quarter of twenty-two.",
      verbalize("en-us", U"On 25 july 2012, jan. 15, 2016/07/03 and in 2Q22."));
  ASSERT_EQUALS(U"The one thousandth fan, one quarter, nineteen sixties to eighties and three forty AD.",
                verbalize("en-us", U"The 1,000th fan, 1/4th, 1960s-80s and 340 AD."));
  ASSERT_EQUALS(
      U"For ten to thirty seconds, five minus three equals two, and ten thousand one hundred eleven residents.",
      verbalize("en-us", U"For 10 - 30 seconds, 5 - 3 = 2, and 10111 residents."));
  ASSERT_EQUALS(U"The four K screen and five hundred thousand fans, three billion dollars.",
                verbalize("en-us", U"The 4K screen and 500k fans, 3 bn dollars.", only(&Options::numbers)));

  // A range rising across a digit before a plural noun, or of days; bowling figures keep "-"
  ASSERT_EQUALS(U"Children aged one to fifty-nine months, July fifth to twelfth, but he took three-fifty-three.",
                verbalize("en", U"Children aged 1-59 months, July 5-12, but he took 3-53.", only(&Options::numbers)));

  // Prime marks: feet and inches, unless a quotation closes
  ASSERT_EQUALS(
      U"He is five feet nine inches tall, the room is twenty-six feet eight inches by seventeen feet five "
      U"inches, fly over four hundred feet, not 'Hera Pheri three'.",
      verbalize("en-us", U"He is 5'9\" tall, the room is 26'8 x 17'5, fly over 400', not 'Hera Pheri 3'.",
                only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
