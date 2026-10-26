#include "constants.h"

#include <verbalis/languages/language.h>
#include <verbalis/numbers/scanning.h>
#include <verbalis/text/scan.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::de {

namespace {
using namespace text;
using namespace numbers;

// How the ending of an ordinal follows the word before it: am dritten (weak), ein zweiter (mixed), dritter (strong)
enum class Declension { WEAK = 0, MIXED, STRONG };

// What the words around a match say about reading its numbers
struct Cues {
  Case grammatical_case = Case::NOMINATIVE; // Given by the word before: mit einer Stimme, am dritten
  Gender gender = Gender::MASCULINE;        // Of the noun after: eine Stunde, der dritte Platz
  Declension declension = Declension::STRONG;
  bool article = false;    // "1" before a noun is said like an article: eine Stunde
  bool year = true;        // A 4-digit number reads as a year (1998 Abi), unless it counts a noun (1945 Einwohner)
  bool capitalized = true; // A decade is a noun (die Neunziger), but an adjective before Jahre (neunziger Jahre)
};

// ------------------------
// ----- Number words -----
// ------------------------

std::u32string cardinal(uint64_t n);

// A cardinal as the first part of a compound: ein, einundzwanzig, einhundertein
std::u32string stem(uint64_t n) {
  std::u32string words = cardinal(n);
  return words.ends_with(U"eins") ? words.substr(0, words.size() - 1) : words;
}

std::u32string cardinal(uint64_t n) {
  if (n < 20) {
    return std::u32string(kOnes[n]);
  }
  if (n < 100) {
    return (n % 10 ? stem(n % 10) + U"und" : U"") + std::u32string(kTens[n / 10]);
  }
  if (n < 1000) {
    return stem(n / 100) + U"hundert" + (n % 100 ? cardinal(n % 100) : U"");
  }
  if (n < 1000000) {
    return stem(n / 1000) + U"tausend" + (n % 1000 ? cardinal(n % 1000) : U"");
  }
  for (auto [scale, one, many] :
       {std::tuple{1000000000ULL, U"Milliarde", U"Milliarden"}, std::tuple{1000000ULL, U"Million", U"Millionen"}}) {
    if (n >= scale) {
      uint64_t q = n / scale, r = n % scale;
      std::u32string head = q == 1 ? U"eine " + std::u32string(one) : cardinal(q) + U" " + std::u32string(many);
      return head + (r ? U" " + cardinal(r) : U"");
    }
  }
  return U"";
}

// 1100-1999 in hundreds (neunzehnhundertneunzig); later years read like cardinals
std::u32string year(uint64_t n) {
  if (n >= 1100 && n <= 1999) {
    return cardinal(n / 100) + U"hundert" + (n % 100 ? cardinal(n % 100) : U"");
  }
  return cardinal(n);
}

std::u32string ordinal(uint64_t n, std::u32string_view ending) {
  std::u32string words = cardinal(n);
  uint64_t r = n % 100;
  if (r > 0 && r < 20) {
    std::u32string one(kOnes[r]);
    auto it = kOrdinalStems.find(one);
    std::u32string base = words.substr(0, words.size() - one.size());
    return base + (it != kOrdinalStems.end() ? it->second : one) + U"t" + std::u32string(ending);
  }
  return words + U"st" + std::u32string(ending);
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.emplace_back(kOnes[c - U'0']);
  }
  return join(words);
}

// "1" said like an article, by case and gender: eine Stunde, mit einem Mal
std::u32string article(Case c, Gender g) {
  static const std::array<std::array<std::u32string_view, 3>, 4> forms = {{
      {U"ein", U"eine", U"ein"},      // Nominative
      {U"eines", U"einer", U"eines"}, // Genitive
      {U"einem", U"einer", U"einem"}, // Dative
      {U"einen", U"eine", U"ein"},    // Accusative
  }};
  size_t row = c == Case::GENITIVE ? 1 : c == Case::DATIVE ? 2 : c == Case::ACCUSATIVE ? 3 : 0;
  return std::u32string(forms[row][static_cast<size_t>(g)]);
}

// The adjective ending of an ordinal: am dritten, der dritte, ein zweiter, dritter
std::u32string_view ending(const Cues& cues) {
  static const std::array<std::array<std::u32string_view, 3>, 4> strong = {{
      {U"er", U"e", U"es"},  // Nominative
      {U"en", U"er", U"en"}, // Genitive
      {U"em", U"er", U"em"}, // Dative
      {U"en", U"e", U"es"},  // Accusative
  }};
  switch (cues.declension) {
    case Declension::WEAK:
      return cues.grammatical_case == Case::NOMINATIVE ? U"e" : U"en";
    case Declension::MIXED:
      return cues.gender == Gender::NEUTER ? U"es" : U"er";
    default:
      Case c = cues.grammatical_case;
      size_t row = c == Case::GENITIVE ? 1 : c == Case::DATIVE ? 2 : c == Case::ACCUSATIVE ? 3 : 0;
      return strong[row][static_cast<size_t>(cues.gender)];
  }
}

// -------------------
// ----- Context -----
// -------------------

Gender gender_of(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun);

  // The longest known ending decides, then the regular ones
  size_t longest = 0;
  Gender gender = Gender::MASCULINE;
  for (const auto& [key, key_gender] : kGenders) {
    if (key.size() > longest && word.ends_with(key)) {
      longest = key.size();
      gender = key_gender;
    }
  }
  if (longest > 0) {
    return gender;
  }

  auto ends_with_any = [&](const std::vector<std::u32string>& endings) {
    return std::ranges::any_of(endings, [&](const auto& e) { return word.ends_with(e); });
  };
  return ends_with_any(kFeminineEndings) ? Gender::FEMININE :
         ends_with_any(kNeuterEndings)   ? Gender::NEUTER :
                                           Gender::MASCULINE;
}

Case case_after(std::u32string_view word) {
  std::u32string w(word);
  return kDative.contains(w)     ? Case::DATIVE :
         kAccusative.contains(w) ? Case::ACCUSATIVE :
         kGenitive.contains(w)   ? Case::GENITIVE :
                                   Case::NOMINATIVE;
}

bool capitalized(std::u32string_view word) {
  return !word.empty() && unicode::isupper(word[0]);
}

bool all_digits(std::u32string_view word) {
  return !word.empty() && std::ranges::all_of(word, unicode::isdigit);
}

// A number right after a capitalized noun is its label: Seite 1, Platz 1
bool labelled(const Context& context) {
  auto head = words_of(context.head());
  return !head.empty() && capitalized(head.back()) &&
         !(head.size() == 1 || strings::contains(std::u32string_view(U".!?"), head[head.size() - 2].back()));
}

// The ending of an ordinal from the words before it, and the noun it qualifies.
void resolve_ending(const Context& context, std::u32string_view noun, Cues& cues) {
  std::vector<std::u32string> before;
  for (auto word : words_of(context.head())) {
    before.push_back(strings::to_lower(stripped(word)));
  }

  // "in 2. und 3. Lesung", "vom 1. 5. bis 3. 5.": both take the ending of the first
  static const std::array<std::u32string_view, 4> joining = {U"und", U"oder", U"bis", U"sowie"};
  while (before.size() >= 2 && std::ranges::find(joining, before.back()) != joining.end() &&
         all_digits(before[before.size() - 2])) {
    before.resize(before.size() - 2);
    while (!before.empty() && all_digits(before.back())) {
      before.pop_back();
    }
  }
  std::u32string w1 = before.empty() ? U"" : before.back();
  std::u32string w2 = before.size() < 2 ? U"" : before[before.size() - 2];

  cues.gender = gender_of(noun);
  if (kWeak.contains(w1) || (w1 == U"der" && (kDative.contains(w2) || kGenitive.contains(w2)))) {
    cues.declension = Declension::WEAK;
    cues.grammatical_case = Case::DATIVE;
  } else if (kWeakNominative.contains(w1)) {
    cues.declension = Declension::WEAK;
    cues.grammatical_case = Case::NOMINATIVE;
  } else if (kMixed.contains(w1)) {
    cues.declension = Declension::MIXED;
  } else {
    cues.declension = Declension::STRONG;
    cues.grammatical_case = case_after(w1);
  }
}

Cues cues_of(const Match& match) {
  const Context& context = match.context;
  Cues cues;

  switch (match.kind) {
    case Kind::DATE:
      resolve_ending(context, U"Monat", cues);
      return cues;

    case Kind::ORDINAL: {
      // The noun is the first capitalized word after it: der 3. Platz
      std::u32string_view noun;
      for (auto word : words_of(context.tail())) {
        if (capitalized(word)) {
          size_t end = word.find_last_not_of(U".,;:!?");
          noun = word.substr(0, end == std::u32string_view::npos ? 0 : end + 1);
          break;
        }
      }
      resolve_ending(context, noun, cues);
      return cues;
    }

    case Kind::DECADE: {
      std::u32string next = context.after();
      cues.capitalized = next != U"Jahre" && next != U"Jahren" && next != U"Jahrgang";
      return cues;
    }

    default:
      break;
  }

  // "1" before a noun is said like an article, in the case of the word before it: mit einer Stimme
  std::u32string next = context.after();
  std::u32string noun = next;
  if ((next == U"bis" || next == U"oder") && all_digits(context.after(2))) {
    // "1 bis 2 Werktage": gendered by the plural noun
    noun = context.after(3);
    if (noun.ends_with(U"en")) {
      noun.pop_back();
    } else if (noun.ends_with(U'e')) {
      noun.pop_back();
    }
  }
  cues.article = capitalized(noun) && noun != U"Quadrat" && (noun != next || !labelled(context));
  cues.grammatical_case = case_after(context.before());
  cues.gender = gender_of(noun);

  // 1998 Abi gemacht, im Jahr 1945; but 1945 Einwohner, 1920 mal 1080
  bool month = std::ranges::find(kMonths, next) != kMonths.end();
  bool measure = next.ends_with(U'e') || next.ends_with(U'n') || next.ends_with(U"er") || next.ends_with(U's') ||
                 kMeasureWords.contains(next);
  bool counted = capitalized(next) && measure && !month;
  std::u32string previous = context.before();
  bool times = next == U"mal" || previous == U"mal";
  cues.year = !times && (kYearCues.contains(previous) || !counted);
  return cues;
}

// --------------------
// ----- Patterns -----
// --------------------

// 030 1234567, 0800-123 45 67: an area code starting with 0, then the number, optionally in groups.
class AreaCodePattern : public Pattern {
public:
  AreaCodePattern() : Pattern(Kind::PHONE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,") || text[position] != U'0') {
      return std::nullopt;
    }

    // The area code and the number: written together (0301234567), or apart (030 1234567, 030/1234567)
    size_t end = digits_end(text, position);
    size_t length = end - position;
    if (length < 6) {
      bool apart = length >= 3 && end < text.size() && (text[end] == U' ' || text[end] == U'/' || text[end] == U'-') &&
                   digits_end(text, end + 1) >= end + 4;
      if (!apart) {
        return std::nullopt;
      }
      end = digits_end(text, end + 1);
    }

    // Then any further groups: 0800-123 45 67. Each could end the number, if nothing continues it there.
    std::vector<size_t> ends = {end};
    while (ends.back() + 1 < text.size() && (text[ends.back()] == U' ' || text[ends.back()] == U'-') &&
           unicode::isdigit(text[ends.back() + 1])) {
      ends.push_back(digits_end(text, ends.back() + 1));
    }
    for (auto it = ends.rbegin(); it != ends.rend(); ++it) {
      // Not followed by a digit, even after a word character, a dot or a comma
      bool continued =
          is_digit_after(text, *it) || (!is_clear_after(text, *it, U".,") && is_digit_after(text, *it + 1));
      if (!continued) {
        Match phone{.kind = Kind::PHONE, .span = {position, *it}};
        for (size_t i = position; i < *it;) {
          size_t digits = digits_end(text, i);
          if (digits > i) {
            phone.numbers.push_back(digits_at(text, i));
            i = digits;
          } else {
            i++;
          }
        }
        return phone;
      }
    }
    return std::nullopt;
  }
};

// --------------------
// ----- Language -----
// --------------------

class German final : public PatternLanguage {
public:
  German() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<AreaCodePattern>());
    add_pattern(std::make_unique<DotOrdinalPattern>());
    add_pattern(std::make_unique<SuffixPattern>(Kind::DECADE, kDecadeSuffixes, kDecadeFormat));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (!number.fraction.empty()) {
      std::u32string head = number.integer.empty() ? U"" : cardinal(value(number)) + U" ";
      return head + U"Komma " + digits(number.fraction);
    }

    auto n = parse_value(number.integer);
    if (!n || is_spoken_digits(number, 5)) {
      return digits(number.integer);
    }
    Cues cues = cues_of(match);
    if (*n == 1 && cues.article) {
      return article(cues.grammatical_case, cues.gender);
    }
    if (number.integer.size() == 4 && !number.grouped && cues.year) {
      return year(*n);
    }
    return cardinal(*n);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 4);
    return n ? year(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // "am 12. 3." is a date, "Platz 1. 2 Tore" is not
    if (match.text.find(U' ') != std::u32string_view::npos && !kDateCues.contains(match.context.before())) {
      return std::nullopt;
    }
    std::u32string words =
        ordinal(value(match.numbers[0]), ending(cues_of(match))) + U" " + kMonths[value(match.numbers[1]) - 1];
    return match.numbers.size() < 3 ? words : words + U" " + year(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    std::u32string all;
    for (const auto& group : match.numbers) {
      all += group.integer;
    }
    return digits(all);
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    return named_fraction(kFractions, match)
        .value_or(cardinal(value(match.numbers[0])) + U" durch " + cardinal(value(match.numbers[1])));
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    if (match.numbers.size() == 2) {
      bool decimal = match.separator == U'.' && match.suffix.empty();
      if (!fits_clock(match) || decimal) {
        return std::nullopt;
      }
    }
    std::u32string words = stem(value(match.numbers[0])) + U" Uhr";
    if (match.numbers.size() == 2 && value(match.numbers[1]) > 0) {
      words += U" " + cardinal(value(match.numbers[1]));
    }
    return words;
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    // "Platz 3. Danach", "Platz 1. 2 Tore": a sentence end, not an ordinal
    static const std::unordered_set<std::u32string> inflecting = [] {
      std::unordered_set<std::u32string> words = {U"und", U"oder", U"bis"};
      for (const auto* set : {&kWeak, &kWeakNominative, &kMixed, &kDative, &kAccusative, &kGenitive}) {
        words.insert(set->begin(), set->end());
      }
      return words;
    }();
    auto words = words_of(match.context.tail());
    std::u32string_view next = words.empty() ? U"" : words[0];
    bool month = std::ranges::find(kMonths, next) != kMonths.end();
    bool sentence = capitalized(next) && !month && !inflecting.contains(match.context.before());
    if (next.empty() || unicode::isdigit(next[0]) || sentence) {
      return std::nullopt;
    }
    return ordinal(value(match.numbers[0]), ending(cues_of(match)));
  }

  std::optional<std::u32string> read_decade(const Match& match) const override {
    // 90er, 90ern: neunziger, neunzigern; 1990er: neunzehnhundertneunziger
    const auto& digits = match.numbers[0].integer;
    std::u32string number = digits.size() > 1 && digits[0] == U'0' ? U"null" + cardinal(value(match.numbers[0])) :
                                                                     year(value(match.numbers[0]));
    std::u32string words = number + std::u32string(match.suffix);
    if (cues_of(match).capitalized) {
      words[0] = unicode::toupper(words[0]);
    }
    return words;
  }

  std::optional<std::u32string> read_compound(const Match& match) const override {
    // 30-jährige: dreißigjährige; 4-Zimmer-Wohnung: Vier-Zimmer-Wohnung
    const Number& number = match.numbers[0];
    std::u32string words =
        number.fraction.empty() && !number.grouped ? stem(value(number)) : read_number(number, match);
    if (capitalized(match.suffix)) {
      words[0] = unicode::toupper(words[0]);
      return words + U"-" + std::u32string(match.suffix);
    }
    return words + std::u32string(match.suffix);
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const std::u32string& noun = kCurrencies.at(std::u32string(match.unit));
    std::u32string one = article(cues_of(match).grammatical_case, gender_of(noun));
    if (!match.scale.empty()) {
      return scaled(number, match) + U" " + noun;
    }

    // A price: drei Euro fünfzig, neunundneunzig Cent
    if (number.fraction.size() == 2) {
      uint64_t cents = parse_value(number.fraction).value_or(0);
      if (value(number) == 0) {
        return cardinal(cents) + U" Cent";
      }
      Number whole{.integer = number.integer, .grouped = number.grouped};
      std::u32string words = number.integer == U"1" ? one : read_number(whole, match);
      return words + U" " + noun + U" " + cardinal(cents);
    }

    bool single = number.integer == U"1" && number.fraction.empty();
    return (single ? one : read_number(number, match)) + U" " + noun;
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    return scaled(number, match);
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    std::u32string words = read_number(number, match);
    // 8%ige: achtprozentige
    return match.suffix.empty() ? words + U" Prozent" : words + U"prozent" + std::u32string(match.suffix);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    if (number.integer == U"1" && number.fraction.empty()) {
      return article(cues_of(match).grammatical_case, gender_of(unit.one)) + U" " + unit.one;
    }
    return read_number(number, match) + U" " + unit.many;
  }

private:
  // An amount with its scale: drei Millionen, zehntausend
  std::u32string scaled(const Number& number, const Match& match) const {
    const Forms& scale = kScales.at(std::u32string(match.scale));
    if (scale.many == U"tausend") {
      return read_number(number, match) + U"tausend";
    }
    bool single = number.integer == U"1" && number.fraction.empty();
    return single ? U"eine " + scale.one : read_number(number, match) + U" " + scale.many;
  }
};
} // namespace

const Language& language() {
  static const German language;
  return language;
}

} // namespace verbalis::languages::de
