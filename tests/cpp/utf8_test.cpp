#include "test.h"
#include "verbalis/text/utf8.h"

namespace verbalis::test {

using text::utf8_to_u32;

REGISTER_TEST(utf8_valid_test) {
  ASSERT_EQUALS(std::u32string(U"aé€🍎"), utf8_to_u32("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x8D\x8E"));
  ASSERT_EQUALS(std::u32string(U""), utf8_to_u32(""));
  return true;
}

REGISTER_TEST(utf8_invalid_test) {
  // A bad continuation byte replaces only the lead byte; the "(" after it survives
  ASSERT_EQUALS(std::u32string(U"a\uFFFD(b"), utf8_to_u32("a\xC3(b"));
  // A cut-off sequence at the end: one replacement per byte
  ASSERT_EQUALS(std::u32string(U"x\uFFFD\uFFFD"), utf8_to_u32("x\xE2\x82"));
  // A stray continuation byte and an invalid lead byte
  ASSERT_EQUALS(std::u32string(U"\uFFFDa\uFFFD"), utf8_to_u32("\x80" "a\xFF"));
  // Overlong forms: "/" in two bytes, NUL in two bytes
  ASSERT_EQUALS(std::u32string(U"\uFFFD\uFFFD"), utf8_to_u32("\xC0\xAF"));
  ASSERT_EQUALS(std::u32string(U"\uFFFD\uFFFD"), utf8_to_u32("\xC0\x80"));
  // A surrogate (U+D800) and a code point past U+10FFFF
  ASSERT_EQUALS(std::u32string(U"\uFFFD\uFFFD\uFFFD"), utf8_to_u32("\xED\xA0\x80"));
  ASSERT_EQUALS(std::u32string(U"\uFFFD\uFFFD\uFFFD\uFFFD"), utf8_to_u32("\xF4\x90\x80\x80"));
  return true;
}

} // namespace verbalis::test
