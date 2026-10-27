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

namespace verbalis::languages::es {

namespace {
using namespace text;
using namespace numbers;

// How "uno" ends a number: alone (veintiuno), before a masculine noun (veintiún años) or a feminine one (veintiuna)
enum class One { UNO = 0, UN, UNA };

// ------------------------
// ----- Number words -----
// ------------------------

std::u32string below_hundred(uint64_t n, One one) {
  std::u32string words(n < 30 ? kOnes[n] : kTens[n / 10]);
  if (n >= 30 && n % 10) {
    words += U" y " + std::u32string(kOnes[n % 10]);
  }

  // veintiún años, treinta y una personas
  if (words.ends_with(U"uno") && one != One::UNO) {
    words.resize(words.size() - 3);
    words += one == One::UNA ? U"una" : (words == U"veinti" ? U"ún" : U"un");
  }
  return words;
}

std::u32string below_thousand(uint64_t n, One one) {
  uint64_t h = n / 100, r = n % 100;
  if (h == 0) {
    return below_hundred(r, one);
  }
  if (n == 100) {
    return U"cien";
  }

  // doscientas personas
  std::u32string head(kHundreds[h]);
  if (one == One::UNA && h > 1) {
    head.replace(head.size() - 2, 2, U"as");
  }
  return head + (r ? U" " + below_hundred(r, one) : U"");
}

// A cardinal, ending with the given form of "uno": doscientas mil personas, veintiún años
std::u32string cardinal(uint64_t n, One one = One::UNO) {
  if (n < 1000) {
    return below_thousand(n, one);
  }
  if (n < 1000000) {
    uint64_t t = n / 1000, r = n % 1000;
    std::u32string head = t == 1 ? U"mil" : below_thousand(t, one == One::UNA ? One::UNA : One::UN) + U" mil";
    return head + (r ? U" " + below_thousand(r, one) : U"");
  }

  bool billions = n >= 1000000000000ULL;
  uint64_t scale = billions ? 1000000000000ULL : 1000000ULL, q = n / scale, r = n % scale;
  std::u32string head = q == 1 ? (billions ? U"un billón" : U"un millón") :
                                 cardinal(q, One::UN) + (billions ? U" billones" : U" millones");
  return head + (r ? U" " + cardinal(r, one) : U"");
}

// centésima trigésima séptima
std::u32string ordinal(uint64_t n, bool feminine) {
  std::vector<std::u32string> words;
  for (auto [scale, stems] :
       {std::pair{100, &kOrdinalHundreds}, std::pair{10, &kOrdinalTens}, std::pair{1, &kOrdinalOnes}}) {
    if (uint64_t digit = n / scale % 10) {
      words.push_back(std::u32string((*stems)[digit]) + (feminine ? U"a" : U"o"));
    }
  }
  return join(words);
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    if (unicode::isdigit(c)) {
      words.emplace_back(kOnes[c - U'0']);
    }
  }
  return join(words);
}

// A number ending with a given form of "uno", as before a currency or a unit
std::u32string number_with(const Number& number, One one) {
  if (number.fraction.empty()) {
    auto n = parse_value(number.integer);
    return !n || is_spoken_digits(number, 7) ? digits(number.integer) : cardinal(*n, one);
  }

  // The separator is read as written: tres coma cinco, tres punto cinco
  std::u32string fraction;
  if (number.fraction.size() > 2) {
    fraction = digits(number.fraction);
  } else {
    std::vector<std::u32string> words;
    size_t zeros = number.fraction.find_first_not_of(U'0');
    words.insert(words.end(), std::min(zeros, number.fraction.size()), U"cero");
    if (zeros != std::u32string::npos) {
      words.push_back(cardinal(parse_value(number.fraction).value_or(0), one));
    }
    fraction = join(words);
  }
  std::u32string head = number.integer.empty() ? U"" : cardinal(value(number), one == One::UN ? One::UNO : one) + U" ";
  return head + (number.decimal_mark == U',' ? U"coma " : U"punto ") + fraction;
}

// -------------------
// ----- Context -----
// -------------------

bool feminine(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun);
  std::u32string singular = word;
  if (word.ends_with(U"iones")) {
    singular = word.substr(0, word.size() - 5) + U"ión";
  } else if (word.ends_with(U"as") || word.ends_with(U"es")) {
    singular.pop_back();
  }

  if (kFeminineNouns.contains(word) || kFeminineNouns.contains(singular)) {
    return true;
  }
  bool feminine_ending = std::ranges::any_of(kFeminineEndings, [&](const auto& e) { return singular.ends_with(e); });
  return !kMasculineInA.contains(singular) && feminine_ending;
}

// The form of "uno" from the noun after a number: veintiún años, veintiuna personas; a la 1
One one_of(const Context& context) {
  std::u32string word = context.after();
  std::u32string previous = context.before();
  if (previous == U"la" || previous == U"las") {
    return One::UNA;
  }
  if (word.empty() || !unicode::isalpha(word[0]) || kStopWords.contains(strings::to_lower(word))) {
    return One::UNO;
  }
  return feminine(word) ? One::UNA : One::UN;
}

// A lowercase noun follows: 10k seguidores
bool counted(const Context& context) {
  std::u32string word = context.after();
  bool lowercase_word = word.size() > 2 && std::ranges::all_of(word, [](char32_t c) { return unicode::islower(c); });
  return lowercase_word && !kStopWords.contains(word);
}

// What the words around a match say about reading its numbers
struct Cues {
  One one = One::UNO; // From the noun after the number
  bool named = false; // $5.000 dólares: the currency is already named after the amount
};

Cues cues_of(const Match& match) {
  // A decade or a compound stands alone: los 80s, 4-carriles
  bool alone = match.kind == Kind::DECADE || match.kind == Kind::COMPOUND;
  Cues cues{.one = alone ? One::UNO : one_of(match.context)};

  // $5.000 dólares
  if (match.kind == Kind::MONEY) {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    std::u32string next = strings::to_lower(match.context.after());
    cues.named = next == currency.one || next == currency.many || next == U"pesos";
  }
  return cues;
}

// --------------------
// ----- Patterns -----
// --------------------

// Ley 2/2018: the number of a law with its year. The law word stays, its numbers are the number and the year.
class LawPattern : public Pattern {
public:
  LawPattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    // A law word, then spaces
    size_t word_end = position;
    while (word_end > 0 && unicode::isspace(text[word_end - 1])) {
      word_end--;
    }
    size_t word_begin = word_end;
    while (word_begin > 0 && unicode::isword(text[word_begin - 1])) {
      word_begin--;
    }
    if (word_end == position || !kLaws.contains(strings::to_lower(text.substr(word_begin, word_end - word_begin)))) {
      return std::nullopt;
    }

    // Its number, then a year of 19xx or 20xx
    Number number = digits_at(text, position);
    size_t year = number.span.end + 1;
    if (number.span.end == position || number.integer.size() > 3 || year >= text.size() ||
        !strings::contains(std::u32string_view(U"./-"), text[year - 1])) {
      return std::nullopt;
    }
    Number year_number = digits_at(text, year);
    if (year_number.integer.size() != 4 ||
        !(year_number.integer.starts_with(U"19") || year_number.integer.starts_with(U"20"))) {
      return std::nullopt;
    }
    size_t end = year_number.span.end;
    if (!is_clear_after(text, end) || is_digit_after(text, end, U".,/")) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DATE, .span = {position, end}, .numbers = {number, year_number}};
  }
};

// --------------------
// ----- Language -----
// --------------------

class Spanish final : public PatternLanguage {
public:
  Spanish() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<LawPattern>());
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSigns, kOrdinalSignFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<SuffixPattern>(Kind::DECADE, kDecadeSuffixes, kDecadeFormat));
    add_pattern(std::make_unique<RomanPattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    return number_with(number, cues_of(match).one);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // A law: Ley dos de dos mil dieciocho
    if (match.numbers.size() == 2) {
      return cardinal(value(match.numbers[0])) + U" de " + cardinal(value(match.numbers[1]));
    }

    uint64_t day = value(match.numbers[0]);
    return (day == 1 ? U"primero" : cardinal(day)) + U" de " + kMonths[value(match.numbers[1]) - 1] + U" de " +
           cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // 600 123 456 is a count, unless a word says it is a phone number
    if (match.kind == Kind::PHONE && match.separator == U' ' && !kPhoneWords.contains(match.context.before())) {
      return std::nullopt;
    }
    return digits(match.text);
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    return named_fraction(kFractions, match);
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    // Only with minutes; a dot makes a time only after "a las": a las 18.30, but 1.13
    if (match.numbers.size() < 2) {
      return std::nullopt;
    }
    bool decimal = match.separator == U'.' && !kTimeWords.contains(match.context.before());
    if (!fits_clock(match) || decimal) {
      return std::nullopt;
    }

    uint64_t minutes = value(match.numbers[1]);
    return cardinal(value(match.numbers[0]), One::UNA) + (minutes ? U" " + cardinal(minutes) : U"");
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    auto role = roman_role(match, kRomanCues);
    if (!role || n >= 1000) {
      return std::nullopt;
    }
    std::u32string previous = match.context.before();
    switch (*role) {
      case RomanRole::NAME: // Felipe sexto, Isabel segunda, but Alfonso trece
        return n <= 10 ? ordinal(n, kFeminineNames.contains(previous)) : cardinal(n);
      case RomanRole::AFTER: { // la Segunda Guerra, el primer congreso, el vigésimo tercer festival
        bool feminine = kFeminineRomanNouns.contains(strings::to_lower(match.context.after()));
        std::u32string words = ordinal(n, feminine);
        if (!feminine && (words.ends_with(U"primero") || words.ends_with(U"tercero"))) {
          words.pop_back();
        }
        return words;
      }
      default: // siglo quinto, Vaticano segundo, but siglo veintiuno; capítulo cuatro
        return (previous.starts_with(U"siglo") || previous == U"vaticano") && n <= 10 ? ordinal(n, false) : cardinal(n);
    }
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    // Ordinal words reach 999
    uint64_t n = value(match.numbers[0]);
    if (n == 0 || n >= 1000) {
      return std::nullopt;
    }

    std::u32string_view suffix = match.suffix;
    if (suffix == U"er" && (n == 1 || n == 3)) {
      return n == 1 ? U"primer" : U"tercer";
    }
    bool feminine = suffix == U"ª" || suffix == U"era" || suffix == U"ra" || suffix == U"da" || suffix == U"ta" ||
                    suffix == U"va" || suffix == U"ma" || suffix == U"na";
    return ordinal(n, feminine);
  }

  std::optional<std::u32string> read_decade(const Match& match) const override {
    return read_number(match.numbers[0], match) + U"s"; // los 80s: los ochentas
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    One one = currency.one == U"libra" ? One::UNA : One::UN;

    // A price: cinco dólares con cincuenta
    if (number.fraction.size() == 2 && match.scale.empty()) {
      Number whole{.integer = number.integer, .grouped = number.grouped};
      return number_with(whole, one) + U" " + (number.integer == U"1" ? currency.one : currency.many) + U" con " +
             cardinal(parse_value(number.fraction).value_or(0));
    }

    std::u32string words = match.scale.empty() ? number_with(number, one) : scaled(number, match);
    if (cues_of(match).named) {
      return words;
    }
    // tres millones de dólares, but cinco mil dólares
    bool of = !match.scale.empty() && kScales.at(std::u32string(match.scale)) != U"mil";
    return words + (of ? U" de " : U" ") + (is_one(number) && match.scale.empty() ? currency.one : currency.many);
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K, 2K: a letter, read as a code
    if ((match.scale == U"k" || match.scale == U"K") && !counted(match.context)) {
      return std::nullopt;
    }
    return scaled(number, match);
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return number_with(number, One::UNO) + U" por ciento" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    std::u32string_view first_word = std::u32string_view(unit.one).substr(0, unit.one.find(U' '));
    One one = feminine(first_word) ? One::UNA : One::UN;
    return number_with(number, one) + U" " + (is_one(number) ? unit.one : unit.many);
  }

private:
  // An amount with its scale: dos millones, cinco mil
  std::u32string scaled(const Number& number, const Match& match) const {
    const std::u32string& scale = kScales.at(std::u32string(match.scale));
    std::u32string word = scale == U"mil" ? U"mil" : (is_one(number) ? U"millón" : U"millones");
    return number_with(number, One::UN) + U" " + word;
  }
};
} // namespace

const Language& language() {
  static const Spanish language;
  return language;
}

} // namespace verbalis::languages::es
