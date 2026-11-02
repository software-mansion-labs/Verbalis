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
       {std::tuple{1000000000000000000ULL, U"Trillion", U"Trillionen"},
        std::tuple{1000000000000000ULL, U"Billiarde", U"Billiarden"},
        std::tuple{1000000000000ULL, U"Billion", U"Billionen"}, std::tuple{1000000000ULL, U"Milliarde", U"Milliarden"},
        std::tuple{1000000ULL, U"Million", U"Millionen"}}) {
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
  if (n == 0) {
    return words + U"t" + std::u32string(ending); // nullter
  }
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

// "halb" after "ein", by case and gender: ein halber Meter, einen halben Liter, eine halbe Stunde
std::u32string half(Case c, Gender g) {
  if (c == Case::DATIVE || c == Case::GENITIVE || (c == Case::ACCUSATIVE && g == Gender::MASCULINE)) {
    return U"halben";
  }
  return g == Gender::MASCULINE ? U"halber" : g == Gender::FEMININE ? U"halbe" : U"halbes";
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
  // über 1/4, unter 3 Euro: "more than", "less than" before an amount, not prepositions
  std::u32string before = context.before();
  cues.grammatical_case = before == U"über" || before == U"unter" ? Case::NOMINATIVE : case_after(before);
  cues.gender = gender_of(noun);

  // kostet 1 Euro, lief 1 km: after a verb, an amount is its object (einen Euro); but ist 1 Euro, als 1 Euro
  auto head = words_of(context.head());
  std::u32string_view last = head.empty() ? U"" : stripped(head.back());
  bool amount = match.kind == Kind::MONEY || match.kind == Kind::UNIT || match.kind == Kind::FRACTION;
  bool lowercase_word = !last.empty() && std::ranges::all_of(last, unicode::islower);
  if (amount && cues.grammatical_case == Case::NOMINATIVE && lowercase_word &&
      !kNominativeWords.contains(std::u32string(last))) {
    cues.grammatical_case = Case::ACCUSATIVE;
  }

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

// 2.3, 02.03: a day and a month without a dot after them, as the whole text
class BareDatePattern : public Pattern {
public:
  BareDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t dot = digits_end(text, position);
    if (position > 0 || dot == position || dot > 2 || dot >= text.size() || text[dot] != U'.') {
      return std::nullopt;
    }
    size_t end = digits_end(text, dot + 1);
    if (end != text.size() || end - dot - 1 > 2 || end == dot + 1) {
      return std::nullopt;
    }
    Number day = digits_at(text, 0), month = digits_at(text, dot + 1);
    if (value(day) < 1 || value(day) > 31 || value(month) < 1 || value(month) > 12) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DATE, .span = {0, end}, .numbers = {day, month}, .separator = U'.'};
  }
};

// 2003-03: a year and its month; not a range of years, whose end is later (2003-04)
class YearMonthPattern : public Pattern {
public:
  YearMonthPattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t hyphen = digits_end(text, position);
    if (!is_clear_before(text, position, U".,-/") || hyphen != position + 4 || hyphen >= text.size() ||
        text[hyphen] != U'-') {
      return std::nullopt;
    }
    size_t end = digits_end(text, hyphen + 1);
    if (end != hyphen + 3 || !is_clear_after(text, end) || is_digit_after(text, end, U".-/")) {
      return std::nullopt;
    }
    Number year = digits_at(text, position), month = digits_at(text, hyphen + 1);
    if (value(month) < 1 || value(month) > 12 || value(month) > value(year) % 100) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DATE, .span = {position, end}, .numbers = {year, month}, .separator = U'-'};
  }
};

// 030 1234567, 0800-123 45 67, (0411) 1234-1234: an area code starting with 0, then the number, optionally in groups.
class AreaCodePattern : public Pattern {
public:
  AreaCodePattern() : Pattern(Kind::PHONE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    bool parenthesized = text[position] == U'(';
    size_t area = parenthesized ? position + 1 : position;
    if (!is_clear_before(text, position, U".,") || area >= text.size() || text[area] != U'0') {
      return std::nullopt;
    }

    // The area code and the number: written together (0301234567), or apart (030 1234567, 030/1234567, (030) 1234567)
    size_t end = digits_end(text, area);
    size_t length = end - area;
    if (parenthesized) {
      size_t number = end < text.size() && text[end] == U')' ? skip_space(text, end + 1) : end;
      if (length < 3 || number == end || digits_end(text, number) < number + 2) {
        return std::nullopt;
      }
      end = digits_end(text, number);
    } else if (length < 6) {
      bool apart = length >= 3 && end < text.size() && (text[end] == U' ' || text[end] == U'/' || text[end] == U'-') &&
                   digits_end(text, end + 1) >= end + 4;
      if (!apart) {
        return std::nullopt;
      }
      end = digits_end(text, end + 1);
    }

    // Then any further groups: 0800-123 45 67, (030) 4664 - 911300. Each could end the number, if nothing continues it.
    std::vector<size_t> ends = {end};
    while (true) {
      size_t next = ends.back() + (text.substr(ends.back()).starts_with(U" - ") ? 3 : 1);
      bool separator = ends.back() < text.size() && (text[ends.back()] == U' ' || text[ends.back()] == U'-');
      if (!separator || next >= text.size() || !unicode::isdigit(text[next])) {
        break;
      }
      ends.push_back(digits_end(text, next));
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

// 90 Millionen g, 1 1/2 ft: a scale or a fraction, then a unit after an optional space.
class UnitAfterPattern : public Pattern {
public:
  explicit UnitAfterPattern(std::unique_ptr<Pattern> base)
      : Pattern(base->kind()), base_(std::move(base)), units_(keys_of(kUnits)) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    auto found = base_->match(text, position);
    if (!found) {
      return std::nullopt;
    }
    size_t start = skip_space(text, found->span.end);
    for (auto unit : keys_at(text, start, units_)) {
      if (is_clear_after(text, start + unit.size())) {
        found->span.end = start + unit.size();
        found->unit = unit;
        return found;
      }
    }
    return std::nullopt;
  }

private:
  std::unique_ptr<Pattern> base_;
  Keys units_;
};

// --------------------
// ----- Language -----
// --------------------

class German final : public PatternLanguage {
public:
  German() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<BareDatePattern>());
    add_pattern(std::make_unique<YearMonthPattern>());
    add_pattern(std::make_unique<AreaCodePattern>());
    add_pattern(std::make_unique<SecondsPattern>(kSecondsSuffixes, true));
    add_pattern(std::make_unique<PrimePattern>(kNotation, PrimeFormat{.cues = kPrimeCues, .heights = kHeightCues}));
    add_pattern(std::make_unique<UnitAfterPattern>(std::make_unique<ScalePattern>(kNotation)));
    add_pattern(std::make_unique<UnitAfterPattern>(std::make_unique<FractionPattern>()));
    add_pattern(std::make_unique<DotOrdinalPattern>(true));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<SuffixPattern>(Kind::DECADE, kDecadeSuffixes, kDecadeFormat));
    add_pattern(std::make_unique<RomanPattern>(true));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (!number.fraction.empty()) {
      std::u32string head = number.integer.empty() ? U"" : cardinal(value(number)) + U" ";
      return head + U"Komma " + digits(number.fraction);
    }

    // The whole number of 1 1/2: ein einhalb
    if (match.kind == Kind::FRACTION && is_one(number)) {
      return U"ein";
    }
    // A long number is a code (40474 Düsseldorf, WKN 840400), unless it is an amount: 18880 €, 42259 m²
    auto n = parse_value(number.integer);
    bool amount = match.kind == Kind::MONEY || match.kind == Kind::UNIT || match.kind == Kind::PERCENT ||
                  match.kind == Kind::SCALE;
    // 46664 alone, or after its sign: the whole text is a number, not a code
    std::u32string_view head = match.context.head();
    bool alone = (head.empty() || head == joiners().minus + U" ") && match.context.tail().empty();
    if (!n || (is_spoken_digits(number, 5) && !((amount || alone) && number.integer[0] != U'0'))) {
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
    // 2003-03: März zweitausenddrei
    if (match.separator == U'-' && match.numbers.size() == 2) {
      return std::u32string(kMonths[value(match.numbers[1]) - 1]) + U" " + year(value(match.numbers[0]));
    }
    // "am 12. 3." and "(16. 12.)" are dates, "Platz 1. 2 Tore" is not
    if (match.text.find(U' ') != std::u32string_view::npos && !kDateCues.contains(match.context.before()) &&
        !match.context.head().ends_with(U'(')) {
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
    // A half before a unit or a noun: 1/2 l einen halben Liter, 1/2 Stunde eine halbe Stunde
    Cues cues = cues_of(match);
    auto [a, b] = fraction_of(match);
    if (match.numbers.size() == 3 && a >= b) {
      return std::nullopt; // 1 5/3: the whole number and the fraction read apart
    }
    std::u32string next = match.context.after();
    std::u32string noun = match.unit.empty() ? next : kUnits.at(std::u32string(match.unit)).one;
    if (a == 1 && b == 2 && match.numbers.size() == 2 && capitalized(noun)) {
      Gender gender = gender_of(noun);
      std::u32string words = article(cues.grammatical_case, gender) + U" " + half(cues.grammatical_case, gender);
      return match.unit.empty() ? words : words + U" " + noun;
    }
    if (match.unit.empty()) {
      return fraction(match, cues.grammatical_case);
    }

    // 1 1/2 ft: ein einhalb Fuß; 3/4 m: drei Viertel Meter
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    return fraction(match, cues.grammatical_case) + U" " + (match.numbers.size() == 3 ? unit.many : unit.one);
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    if (match.numbers.size() == 3) {
      return read_seconds(match);
    }
    if (match.numbers.size() == 2) {
      bool decimal = match.separator == U'.' && match.suffix.empty();
      // mit 31:32 (15:21): a score, read by its own pattern; but um 15:21, Spiel (Montag, 18:00)
      bool score = match.separator == U':' && match.suffix.empty() && match.numbers[1].integer != U"00" &&
                   !kTimeWords.contains(match.context.before()) && mentions(match.context, kSportWords);
      if (!fits_clock(match) || decimal || score) {
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
    // 3ten, 121tem, 20ste: the ending as written
    if (match.suffix != U".") {
      std::u32string_view suffix = match.suffix;
      return ordinal(value(match.numbers[0]), suffix.substr(suffix.starts_with(U"st") ? 2 : 1));
    }

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
    // 14. Juni, (a month before a comma)
    bool month = std::ranges::find(kMonths, stripped(next)) != kMonths.end();
    // 100. Geburtstag, 2.: a text doesn't open with a sentence of a bare number
    bool start = match.context.head().empty();
    bool sentence = capitalized(next) && !month && !inflecting.contains(match.context.before()) && !start;
    // Before a comma or the end: only right after an article (in der 79., am 3.), not "Platz 23.," or "von 1983."
    bool word = !next.empty() && !strings::contains(text::kPunctuation, next[0]);
    if (!word) {
      std::u32string previous = match.context.before();
      if (!kWeak.contains(previous) && !kWeakNominative.contains(previous) && !(start && next.empty())) {
        return std::nullopt;
      }
      return ordinal(value(match.numbers[0]), ending(cues_of(match))) + (next.empty() ? U"." : U"");
    }
    if (unicode::isdigit(next[0]) || sentence) {
      return std::nullopt;
    }
    return ordinal(value(match.numbers[0]), ending(cues_of(match)));
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    auto role = roman_role(match, kRomanCues);
    if (!role) {
      return std::nullopt;
    }
    switch (*role) {
      case RomanRole::NAME: {
        // Ludwig der Vierzehnte, unter Ludwig dem Vierzehnten, für Wilhelm den Zweiten, Elisabeth die Zweite
        bool feminine = kFeminineNames.contains(match.context.before());
        bool dative = kDative.contains(match.context.before(2));
        bool accusative = kAccusative.contains(match.context.before(2));
        std::u32string article = dative     ? (feminine ? U"der" : U"dem") :
                                 accusative ? (feminine ? U"die" : U"den") :
                                              (feminine ? U"die" : U"der");
        std::u32string words = ordinal(n, dative || (accusative && !feminine) ? U"en" : U"e");
        words[0] = unicode::toupper(words[0]);
        return article + U" " + words;
      }
      case RomanRole::AFTER: {
        // im XX. Jahrhundert: an ordinal, as im 20. Jahrhundert
        Match ordinal_match = match;
        ordinal_match.kind = Kind::ORDINAL;
        return read_ordinal(ordinal_match);
      }
      default:
        return cardinal(n); // Kapitel drei
    }
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
    // 12,50 €/m²: zwölf Euro fünfzig pro Quadratmeter
    std::u32string noun = kCurrencies.at(std::u32string(match.unit));
    std::u32string per;
    if (size_t at = noun.find(U" pro "); at != std::u32string::npos) {
      if (match.span.begin < match.numbers[0].span.begin) {
        return std::nullopt; // Only after its amount: 19,87 €/l, not €/l 14,90
      }
      per = noun.substr(at);
      noun.resize(at);
    }
    return price(number, match, noun) + per;
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 90 Millionen g: neunzig Millionen Gramm
    std::u32string words = scaled(number, match);
    return match.unit.empty() ? words : words + U" " + kUnits.at(std::u32string(match.unit)).many;
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
  // An amount of a currency: drei Euro fünfzig, neunundneunzig Cent
  std::u32string price(const Number& number, const Match& match, const std::u32string& noun) const {
    std::u32string one = article(cues_of(match).grammatical_case, gender_of(noun));
    if (!match.scale.empty()) {
      return scaled(number, match) + U" " + noun;
    }

    // Decimals as cents, without trailing zeros: 2,2 € and 2,200 € read like 2,20 €
    Number amount = number;
    while (amount.fraction.size() > 2 && amount.fraction.back() == U'0') {
      amount.fraction.pop_back();
    }
    if (amount.fraction.size() == 1) {
      amount.fraction += U'0';
    }

    // A price: drei Euro fünfzig, neunundneunzig Cent, ein Cent; 2,00 €: zwei Euro
    if (amount.fraction.size() == 2) {
      uint64_t cents = parse_value(amount.fraction).value_or(0);
      if (value(amount) == 0) {
        return (cents == 1 ? article(cues_of(match).grammatical_case, Gender::MASCULINE) : cardinal(cents)) + U" Cent";
      }
      Number whole{.integer = amount.integer, .grouped = amount.grouped};
      std::u32string words = (amount.integer == U"1" ? one : read_number(whole, match)) + U" " + noun;
      return cents ? words + U" " + cardinal(cents) : words;
    }

    bool single = amount.integer == U"1" && amount.fraction.empty();
    return (single ? one : read_number(amount, match)) + U" " + noun;
  }

  // ein Fünftel, drei Achtel, ein Zwanzigstel, ein Hundertstel; else drei durch sieben. After a whole number, a half is
  // einhalb: zweieinhalb. The "ein" is the article of a neuter noun (das Drittel): von einem Achtel.
  std::u32string fraction(const Match& match, Case grammatical_case) const {
    auto [a, b] = fraction_of(match);
    if (a == 1 && b == 2 && match.numbers.size() == 3) {
      return U"einhalb";
    }
    std::u32string one = article(grammatical_case, Gender::NEUTER);
    if (auto words = named_fraction(kFractions, match)) {
      return a == 1 && b > 2 && match.numbers.size() == 2 ? one + words->substr(3) : *words;
    }

    // Seasons and school years: 2023/24, 2011/2012 read as two years
    size_t n = match.numbers.size();
    const auto& first = match.numbers[n - 2].integer;
    const auto& second = match.numbers[n - 1].integer;
    bool years = first.size() == 4 && is_year_value(a, 1900, 2099) &&
                 ((second.size() == 2 && b == (a + 1) % 100) || (second.size() == 4 && b == a + 1));
    if (years) {
      return year(a) + U" " + (second.size() == 2 ? cardinal(b) : year(b));
    }

    if (b < 3 || a >= b || !is_marked_fraction(match, kFractionWords)) {
      return cardinal(a) + U" durch " + cardinal(b);
    }
    std::u32string denominator = ordinal(b, U"el");
    if (b == 100 || b == 1000) {
      denominator.erase(0, 3); // einhundertstel
    }
    denominator[0] = unicode::toupper(denominator[0]);
    return (a == 1 ? one : cardinal(a)) + U" " + denominator;
  }

  // A clock time before "Uhr": zwei Uhr zwei Minuten dreiundzwanzig Sekunden; else a duration: zwei Stunden acht
  // Minuten neunundzwanzig Sekunden
  std::optional<std::u32string> read_seconds(const Match& match) const {
    uint64_t hours = value(match.numbers[0]), minutes = value(match.numbers[1]), seconds = value(match.numbers[2]);
    bool clock = match.suffix == U"Uhr";
    if (minutes > 59 || seconds > 59 || (clock && hours > 24)) {
      return std::nullopt;
    }
    auto count = [](uint64_t n, std::u32string_view one, std::u32string_view many) {
      return (n == 1 ? U"eine " + std::u32string(one) : cardinal(n) + U" " + std::u32string(many));
    };
    std::u32string head = clock ? stem(hours) + U" Uhr" : count(hours, U"Stunde", U"Stunden");
    return head + U" " + count(minutes, U"Minute", U"Minuten") + U" " + count(seconds, U"Sekunde", U"Sekunden");
  }

  // 1'27"23: eine Minute siebenundzwanzig Sekunden und dreiundzwanzig Hundertstel; 6'2": sechs Fuß zwei Zoll
  std::u32string read_prime(const Number& number, Prime unit, const Match& /*match*/) const override {
    uint64_t n = value(number);
    const Forms& forms = kPrimeUnits.at(unit);
    if (unit == Prime::FOOT || unit == Prime::INCH) {
      return (n == 1 ? U"ein" : cardinal(n)) + U" " + forms.many;
    }
    return n == 1 ? U"eine " + forms.one : cardinal(n) + U" " + forms.many;
  }
  std::optional<std::u32string> read_prime_fraction(std::u32string_view digits, const Match& /*match*/) const override {
    return U"und " + cardinal(parse_value(digits).value_or(0)) + U" " +
           std::u32string(kSecondFractions[digits.size() - 1]);
  }

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
