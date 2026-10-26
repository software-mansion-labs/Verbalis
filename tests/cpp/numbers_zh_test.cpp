#include "test.h"
#include "verbalize.h"

namespace verbalis::test {


// Expected values are apply(cn2an, text, "an2cn") outputs.
REGISTER_TEST(numbers_zh_cardinal_test) {

  ASSERT_EQUALS(U"零", verbalize("zh", U"0", only(&Options::numbers)));
  ASSERT_EQUALS(U"十二", verbalize("zh", U"12", only(&Options::numbers)));
  ASSERT_EQUALS(U"一百", verbalize("zh", U"100", only(&Options::numbers)));
  ASSERT_EQUALS(U"一百一十", verbalize("zh", U"110", only(&Options::numbers)));
  ASSERT_EQUALS(U"一千零一", verbalize("zh", U"1001", only(&Options::numbers)));
  ASSERT_EQUALS(U"一万零一十", verbalize("zh", U"10010", only(&Options::numbers)));
  ASSERT_EQUALS(U"二万零五百", verbalize("zh", U"20500", only(&Options::numbers)));
  ASSERT_EQUALS(U"十万", verbalize("zh", U"100000", only(&Options::numbers)));
  ASSERT_EQUALS(U"一千零二十万三千零四十", verbalize("zh", U"10203040", only(&Options::numbers)));
  ASSERT_EQUALS(U"一亿", verbalize("zh", U"100000000", only(&Options::numbers)));
  ASSERT_EQUALS(U"十亿零一万", verbalize("zh", U"1000010000", only(&Options::numbers)));
  ASSERT_EQUALS(U"七千五百九十", verbalize("zh", U"07５90", only(&Options::numbers)));
  ASSERT_EQUALS(U"零点五", verbalize("zh", U"0.5", only(&Options::numbers)));
  ASSERT_EQUALS(U"三点一四一五九二六五三五八九七九三二",
                verbalize("zh", U"3.14159265358979323846", only(&Options::numbers)));
  ASSERT_EQUALS(U"他有三个苹果。", verbalize("zh", U"他有3个苹果。", only(&Options::numbers)));

  // More than 16 digits are left as they are.
  ASSERT_EQUALS(U"12345678901234567", verbalize("zh", U"12345678901234567", only(&Options::numbers)));

  return true;
}

REGISTER_TEST(numbers_zh_special_forms_test) {

  ASSERT_EQUALS(U"二零二四年五月一日", verbalize("zh", U"2024年5月1日", only(&Options::numbers)));
  ASSERT_EQUALS(U"三分之一", verbalize("zh", U"1/3", only(&Options::numbers)));
  ASSERT_EQUALS(U"百分之负五", verbalize("zh", U"-5%", only(&Options::numbers)));
  ASSERT_EQUALS(U"二十五摄氏度", verbalize("zh", U"25℃", only(&Options::numbers)));
  ASSERT_EQUALS(U"三到五个", verbalize("zh", U"3-5个", only(&Options::numbers)));
  ASSERT_EQUALS(U"一点五到三点五公斤", verbalize("zh", U"1.5-3.5公斤", only(&Options::numbers)));

  // Without a measure word, a dash is a minus sign.
  ASSERT_EQUALS(U"九十九a一负六", verbalize("zh", U"9９a1-6", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
