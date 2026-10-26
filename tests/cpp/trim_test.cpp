#include "test.h"
#include "verbalize.h"

namespace verbalis::test {

REGISTER_TEST(trim_whitespace_test) {

  // Consecutive whitespaces
  ASSERT_EQUALS(U"hello world", verbalize("en-us", U"hello    world", only(&Options::trim)));
  ASSERT_EQUALS(U"a b c", verbalize("en-us", U"a  b   c", only(&Options::trim)));

  // A run with a line break becomes one line break
  ASSERT_EQUALS(U"line1\nline2", verbalize("en-us", U"line1\n\t line2", only(&Options::trim)));
  ASSERT_EQUALS(U"line1\nline2", verbalize("en-us", U"line1\n\n\nline2", only(&Options::trim)));

  // No space before punctuation, but before an ellipsis
  ASSERT_EQUALS(U"Hello, world! Ok? Yes; no: maybe. And ...",
                verbalize("en-us", U"Hello , world ! Ok ? Yes ; no : maybe . And ...", only(&Options::trim)));

  return true;
}

REGISTER_TEST(trim_ends_test) {

  // Leading and trailing whitespaces
  ASSERT_EQUALS(U"trimmed", verbalize("en-us", U"   trimmed   ", only(&Options::trim)));
  ASSERT_EQUALS(U"multiple words trimmed", verbalize("en-us", U"\n  multiple words trimmed\t ", only(&Options::trim)));

  // Empty or only whitespace input
  ASSERT_EQUALS(U"", verbalize("en-us", U"   ", only(&Options::trim)));
  ASSERT_EQUALS(U"", verbalize("en-us", U"", only(&Options::trim)));

  return true;
}

} // namespace verbalis::test
