#include "test.h"
#include "verbalize.h"

namespace verbalis::test {


REGISTER_TEST(terms_address_test) {

  // Domains, hosts and paths
  ASSERT_EQUALS(U"Visit example dot com.", verbalize("en-us", U"Visit example.com.", only(&Options::terms)));
  ASSERT_EQUALS(U"w w w dot example dot co dot uk slash news slash index dot html",
                verbalize("en-us", U"www.example.co.uk/news/index.html", only(&Options::terms)));
  ASSERT_EQUALS(U"(see blog dot example dot io slash)",
                verbalize("en-us", U"(see https://blog.example.io/)", only(&Options::terms)));

  // Emails
  ASSERT_EQUALS(U"Write to jan dot kowalski at firma dot pl!",
                verbalize("en-us", U"Write to jan.kowalski@firma.pl!", only(&Options::terms)));

  // Not addresses: no known domain, a number, an abbreviation
  ASSERT_EQUALS(U"file.txt, 3.5 and U.S.", verbalize("en-us", U"file.txt, 3.5 and U.S.", only(&Options::terms)));

  return true;
}

REGISTER_TEST(terms_abbreviation_test) {

  // Readings by context: before a number, before a name
  ASSERT_EQUALS(U"See page 12 and chapter 3, not p. a.",
                verbalize("en-us", U"See p. 12 and ch. 3, not p. a.", only(&Options::terms)));
  ASSERT_EQUALS(U"Saint Louis is on Main Street.",
                verbalize("en-us", U"St. Louis is on Main St.", only(&Options::terms)));
  ASSERT_EQUALS(U"Doctor Smith, Mister Brown and Dr. no",
                verbalize("en-us", U"Dr. Smith, Mr. Brown and Dr. no", only(&Options::terms)));

  // A dot ending the text stays
  ASSERT_EQUALS(U"apples, pears et cetera.", verbalize("en-us", U"apples, pears etc.", only(&Options::terms)));

  // Polish, with a number right after the abbreviation
  ASSERT_EQUALS(U"numer 5, numer 7 i około 20 osób, między innymi Jan",
                verbalize("pl", U"nr 5, nr7 i ok. 20 osób, m.in. Jan", only(&Options::terms)));

  return true;
}

} // namespace verbalis::test
