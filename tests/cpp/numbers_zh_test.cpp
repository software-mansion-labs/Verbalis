#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

namespace {
std::u32string numbers(std::u32string_view text) {
  return verbalize("zh", text, only(&Options::numbers));
}
} // namespace

REGISTER_TEST(numbers_zh_cardinal_test) {

  ASSERT_EQUALS(U"零", numbers(U"0"));
  ASSERT_EQUALS(U"十二", numbers(U"12"));
  ASSERT_EQUALS(U"一百", numbers(U"100"));
  ASSERT_EQUALS(U"一百一十", numbers(U"110"));
  ASSERT_EQUALS(U"一千零一", numbers(U"1001"));
  ASSERT_EQUALS(U"一万零一十", numbers(U"10010"));
  ASSERT_EQUALS(U"十万", numbers(U"100000"));
  ASSERT_EQUALS(U"一千零二十万三千零四十", numbers(U"10203040"));
  ASSERT_EQUALS(U"一亿", numbers(U"100000000"));
  ASSERT_EQUALS(U"十亿零一万", numbers(U"1000010000"));
  ASSERT_EQUALS(U"一千亿零一千", numbers(U"100000001000"));
  ASSERT_EQUALS(U"零点五", numbers(U"0.5"));
  ASSERT_EQUALS(U"三点一四一五九二六五三五八九七九三二", numbers(U"3.14159265358979323846"));
  ASSERT_EQUALS(U"他有三个苹果。", numbers(U"他有3个苹果。"));
  ASSERT_EQUALS(U"二百五十万人", numbers(U"2,500,000人"));

  // 两 before thousands, a leading group, and a measure word; 二 elsewhere
  ASSERT_EQUALS(U"两万零五百", numbers(U"20500"));
  ASSERT_EQUALS(U"一万两千", numbers(U"12000"));
  ASSERT_EQUALS(U"二百", numbers(U"200"));
  ASSERT_EQUALS(U"两个人", numbers(U"2个人"));
  ASSERT_EQUALS(U"两万人", numbers(U"2万人"));
  ASSERT_EQUALS(U"第二名", numbers(U"第2名"));
  ASSERT_EQUALS(U"二年级", numbers(U"2年级"));

  // More than 16 digits are left as they are.
  ASSERT_EQUALS(U"12345678901234567", numbers(U"12345678901234567"));

  return true;
}

REGISTER_TEST(numbers_zh_special_forms_test) {

  ASSERT_EQUALS(U"二零二四年五月一日", numbers(U"2024年5月1日"));
  ASSERT_EQUALS(U"二零二零年五月十九日", numbers(U"2020 年 5 月19 日"));
  ASSERT_EQUALS(U"二十年后", numbers(U"20年后"));
  ASSERT_EQUALS(U"一八年九月", numbers(U"18年9月"));
  ASSERT_EQUALS(U"二零零八年八月八日", numbers(U"2008-08-08"));
  ASSERT_EQUALS(U"二零零八年八月", numbers(U"08/2008"));
  ASSERT_EQUALS(U"公元前二三三四年", numbers(U"2334bc"));
  ASSERT_EQUALS(U"三分之一", numbers(U"1/3"));
  ASSERT_EQUALS(U"负三分之一", numbers(U"-1/3"));
  ASSERT_EQUALS(U"二零零九到一零赛季", numbers(U"2009/10赛季"));
  ASSERT_EQUALS(U"负百分之五", numbers(U"-5%"));
  ASSERT_EQUALS(U"百分之六十到七十", numbers(U"60-70%"));
  ASSERT_EQUALS(U"二十五摄氏度", numbers(U"25℃"));
  ASSERT_EQUALS(U"每小时十公里", numbers(U"10km/h"));
  ASSERT_EQUALS(U"五到六毫米", numbers(U"5-6mm"));
  ASSERT_EQUALS(U"三到五个", numbers(U"3-5个"));
  ASSERT_EQUALS(U"一点五到三点五公斤", numbers(U"1.5-3.5公斤"));
  ASSERT_EQUALS(U"一九九五到二零零二", numbers(U"1995-2002"));
  ASSERT_EQUALS(U"一点五万美元", numbers(U"$1.5万"));
  ASSERT_EQUALS(U"五百元礼卡", numbers(U"$500元礼卡"));
  ASSERT_EQUALS(U"一加二等于三", numbers(U"1+2=3"));
  ASSERT_EQUALS(U"五减三等于二", numbers(U"5-3=2"));

  // Times, and pairs that are no time: ratios and scores
  ASSERT_EQUALS(U"两点零二分", numbers(U"2:02"));
  ASSERT_EQUALS(U"十一点", numbers(U"11:00"));
  ASSERT_EQUALS(U"上午一点零二分三十六秒", numbers(U"1:02:36 am"));
  ASSERT_EQUALS(U"十点到二十二点", numbers(U"10:00-22:00"));
  ASSERT_EQUALS(U"二比零", numbers(U"2:0"));
  ASSERT_EQUALS(U"一比五万", numbers(U"1:50,000"));
  ASSERT_EQUALS(U"二十三比零", numbers(U"23-0"));
  ASSERT_EQUALS(U"一比三落后", numbers(U"1-3落后"));

  // Codes, digit by digit
  ASSERT_EQUALS(U"四一五 八一六 三八一六", numbers(U"415-816-3816"));
  ASSERT_EQUALS(U"一三八零零一三八零零零", numbers(U"13800138000"));
  ASSERT_EQUALS(U"尾号一七零二", numbers(U"尾号1702"));
  ASSERT_EQUALS(U"一二七点零点零点一", numbers(U"127.0.0.1"));
  ASSERT_EQUALS(U"零零七", numbers(U"007"));
  ASSERT_EQUALS(U"八零后", numbers(U"80后"));
  ASSERT_EQUALS(U"CA一二三四", numbers(U"CA1234"));

  // A dash after a letter or a digit is no minus sign.
  ASSERT_EQUALS(U"COVID-十九", numbers(U"COVID-19"));
  ASSERT_EQUALS(U"九十九a一-六", numbers(U"9９a1-6"));

  // A day and a month in parentheses
  ASSERT_EQUALS(U"昨日 (十月二十四日) 她", verbalize("zh", U"昨日（24/10）她"));

  return true;
}

} // namespace verbalis::test
