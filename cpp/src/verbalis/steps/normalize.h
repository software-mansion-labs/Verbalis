#pragma once

#include <verbalis/text/document.h>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace verbalis::steps {

using text::Document;

// A class of characters written in one canonical form: all quotes as ".
struct CharacterClass {
  std::u32string characters; // « » „ “ ” ‟ ˝
  std::u32string canonical;  // "; empty to remove them
  bool letter = false;       // A letter read in the wrong encoding (zupe³nie): replaced only next to a letter
};

using Characters = std::vector<CharacterClass>;

// The character classes of Echo (echo/preprocessing/characters.py), the default of every language
inline const Characters kDefaultCharacters = {
    {U"…", U"..."},                    // Ellipsis
    {U"«»„“”‟˝", U"\""},               // Quotes
    {U"‘’‚‛´`ʼ", U"'"},                // Apostrophes
    {U"\u200b\u200c\u200d\ufeff", U""} // Invisible: zero-width space, non-joiner, joiner, byte order mark
};

// Dashes besides "-": em, en, figure dash, horizontal bar
inline constexpr std::u32string_view kDashes = U"—–‒―";

/**
 * Writes equivalent characters in one canonical form, before anything reads the text:
 * quotes as ", apostrophes as ', an ellipsis as ..., dashes as - or " - ", a degree written as an ordinal sign
 * (18ºC) as °, a fraction character apart as digits (¾ -> 3/4), and no invisible or lost characters (ï¿½).
 *
 * UTF-8 read as Windows-1252 is repaired first: siÄ™ -> się, â€™ -> '; then HTML entities: &oacute; -> ó.
 */
class Normalizer {
public:
  explicit Normalizer(const Characters& characters = kDefaultCharacters);

  void apply(Document& document) const;

private:
  // An HTML entity at a position, written as the character it stands for (&eacute; -> é), or std::nullopt
  std::optional<Document::Edit> entity(std::u32string_view text, size_t position) const;
  // A letter class character at a position stands for a letter: next to a letter, but not after a metric unit (m³)
  bool is_letter(std::u32string_view text, size_t position) const;

  std::unordered_set<char32_t> letters_; // The characters of letter classes
  /**
   * A dash at a position: "-" inside a word or a number (1990–2000), and " - " for a run of dashes with
   * the spaces around it elsewhere (a – b), except between numbers, which stay a range (2020 – 2022 -> 2020-2022).
   * @returns its edit, or std::nullopt if no dash starts there ("-" alone is a hyphen).
   */
  std::optional<Document::Edit> read_dash(std::u32string_view text, size_t position) const;

  std::unordered_map<char32_t, std::u32string> canonical_; // Of each character of the classes: ’ -> '
};

} // namespace verbalis::steps
