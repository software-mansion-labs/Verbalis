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

  // A power digit after a unit after a number is a superscript, and a ring after a number a degree
  ASSERT_EQUALS(U"345m², 900 cm³, the m2, A3, 180°C",
                verbalize("en-us", U"345m2, 900 cm3, the m2, A3, 180˚C", only(&Options::normalize)));

  // A hyphen spaced from the number before it only is a range
  ASSERT_EQUALS(U"1989-1990", verbalize("en-us", U"1989 -1990", only(&Options::normalize)));

  // Between numbers, a range: 2020 – 2022
  ASSERT_EQUALS(U"2020-2022, but 5 - 3", verbalize("en-us", U"2020 – 2022, but 5 - 3", only(&Options::normalize)));

  return true;
}

REGISTER_TEST(normalize_entities_test) {

  // Named, with or without the semicolon, and by number
  ASSERT_EQUALS(
      U"Computación, número, 'x' - y",
      verbalize("es", U"Computaci&oacuten, n&uacute;mero, &#8216;x&#x2019; &ndash; y", only(&Options::normalize)));
  ASSERT_EQUALS(U" Historial 60 m²", verbalize("es", U"&nbspHistorial 60 m&sup2;", only(&Options::normalize)));

  // Not an entity: a short name needs its semicolon, and an unknown name stays
  ASSERT_EQUALS(U"AT&T, M&ltd, B&B, &egreave",
                verbalize("en-us", U"AT&T, M&ltd, B&B, &egreave", only(&Options::normalize)));

  return true;
}

} // namespace verbalis::test
