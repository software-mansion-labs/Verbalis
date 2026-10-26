#include "test.h"
#include "verbalize.h"

namespace verbalis::test {


REGISTER_TEST(normalize_characters_test) {

  // One form of each class
  ASSERT_EQUALS(U"\"Hello\", 'quoted' \"text\" and \"more\"",
                verbalize("en-us", U"„Hello“, ‘quoted’ «text» and “more”", only(&Options::normalize)));
  ASSERT_EQUALS(U"It's the '90s...", verbalize("en-us", U"It’s the ’90s…", only(&Options::normalize)));

  // Invisible characters are removed
  ASSERT_EQUALS(U"wordword", verbalize("en-us", U"word​word﻿", only(&Options::normalize)));

  return true;
}

REGISTER_TEST(normalize_dashes_test) {

  // Inside a word or a number
  ASSERT_EQUALS(U"Szczurek-Żelazko, 1990-2000, a-b",
                verbalize("en-us", U"Szczurek–Żelazko, 1990—2000, a--b", only(&Options::normalize)));

  // Between words, a run of dashes with its spaces
  ASSERT_EQUALS(U"Warszawa - Kraków - Gdańsk",
                verbalize("en-us", U"Warszawa – Kraków  –  — Gdańsk", only(&Options::normalize)));
  ASSERT_EQUALS(U"a - b", verbalize("en-us", U"a ---- b", only(&Options::normalize)));

  // Between numbers, a range: 2020 – 2022
  ASSERT_EQUALS(U"2020-2022, but 5 - 3", verbalize("en-us", U"2020 – 2022, but 5 - 3", only(&Options::normalize)));

  return true;
}

} // namespace verbalis::test
