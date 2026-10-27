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

namespace verbalis::languages::it {

namespace {
using namespace text;
using namespace numbers;

// ------------------------
// ----- Number words -----
// ------------------------

// ventuno, ventotto: a ten drops its vowel before uno and otto
std::u32string below_hundred(uint64_t n) {
  if (n < 20) {
    return std::u32string(kOnes[n]);
  }
  std::u32string tens(kTens[n / 10]);
  uint64_t unit = n % 10;
  if (unit == 1 || unit == 8) {
    tens.pop_back();
  }
  return tens + (unit ? std::u32string(kOnes[unit]) : U"");
}

// centotto, novecentottanta: a hundred drops its vowel before otto and ottanta
std::u32string below_thousand(uint64_t n) {
  uint64_t h = n / 100, r = n % 100;
  std::u32string head = h == 0 ? U"" : (h == 1 ? U"cento" : std::u32string(kOnes[h]) + U"cento");
  if (!head.empty() && (r == 8 || (r >= 80 && r <= 89))) {
    head.pop_back();
  }
  return head + (r || head.empty() ? below_hundred(r) : U"");
}

// ventitré, centotré, milletré: a compound ending in tre takes the accent
std::u32string accented(std::u32string_view words) {
  std::vector<std::u32string> out;
  for (auto word : strings::split(words, U' ')) {
    std::u32string w(word);
    if (w.size() > 3 && w.ends_with(U"tre")) {
      w.replace(w.size() - 3, 3, U"tré");
    }
    out.push_back(w);
  }
  return join(out);
}

// One word below a million (duemilaventidue), then un milione duecentomila
std::u32string cardinal(uint64_t n) {
  std::vector<std::u32string> words;
  for (auto [scale, one, many] :
       {std::tuple{1000000000ULL, U"un miliardo", U"miliardi"}, std::tuple{1000000ULL, U"un milione", U"milioni"}}) {
    if (uint64_t q = n / scale) {
      words.push_back(q == 1 ? std::u32string(one) : cardinal(q) + U" " + many);
    }
    n %= scale;
  }

  uint64_t thousands = n / 1000;
  n %= 1000;
  std::u32string head = thousands == 0 ? U"" : (thousands == 1 ? U"mille" : below_thousand(thousands) + U"mila");
  if (!head.empty() || n || words.empty()) {
    words.push_back(head + (n || head.empty() ? below_thousand(n) : U""));
  }
  return accented(join(words));
}

// primo ... decimo, then the cardinal with -esimo: undicesimo, ventitreesimo, millesimo
std::u32string ordinal(uint64_t n, char32_t ending) {
  if (n <= 10) {
    std::u32string word(kOrdinals[n]);
    word.back() = ending;
    return word;
  }

  std::u32string words = cardinal(n);
  if (words.ends_with(U"tré")) {
    words.back() = U'e';
  }
  if (words.ends_with(U"tre") || words.ends_with(U"sei")) {
    return words + U"esim" + ending;
  }
  if (words.ends_with(U"mila")) {
    return words.substr(0, words.size() - 4) + U"millesim" + ending;
  }
  return words.substr(0, words.size() - 1) + U"esim" + ending;
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.emplace_back(kOnes[c - U'0']);
  }
  return join(words);
}

// An integer as a cardinal, or digit by digit if it reads as a code: 0039, 1034500
std::u32string integer_words(const Number& number) {
  auto n = parse_value(number.integer);
  return !n || is_spoken_digits(number, 7) ? digits(number.integer) : cardinal(*n);
}

// -------------------
// ----- Context -----
// -------------------

bool is_vowel(char32_t c) {
  return strings::contains(std::u32string_view(U"aeiouàèéìòù"), c);
}

bool is_feminine(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun);
  if (word.ends_with(U'a')) {
    return !kMasculineInA.contains(word);
  }
  if (word.ends_with(U"ione")) {
    return !kMasculineInIone.contains(word);
  }
  return word.ends_with(U"tà") || word.ends_with(U"tù") || word.ends_with(U"si") || word.ends_with(U"ie") ||
         kFeminineInE.contains(word);
}

// The article form of "uno" before a noun: un anno, una volta, uno studente, un'ora
std::u32string one_before(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun.substr(0, noun.find(U' ')));
  if (word.empty()) {
    return U"uno";
  }
  if (is_feminine(word)) {
    return is_vowel(word[0]) ? U"un'" : U"una";
  }
  bool impure = (word[0] == U's' && word.size() > 1 && !is_vowel(word[1])) || word[0] == U'z' || word[0] == U'x' ||
                word[0] == U'y' || word.starts_with(U"gn") || word.starts_with(U"ps") || word.starts_with(U"pn");
  return impure ? U"uno" : U"un";
}

// One of a noun: un metro, una sterlina, un'ora
std::u32string one_of(std::u32string_view noun) {
  std::u32string article = one_before(noun);
  return article + (article.ends_with(U'\'') ? U"" : U" ") + std::u32string(noun);
}

// Whether a word after a number may be a noun it counts: figli, chili, but not su, è, dell'anno, maggio
bool is_noun(std::u32string_view word) {
  return !word.empty() && unicode::islower(word[0]) && !kStopWords.contains(std::u32string(word)) &&
         word.find_first_of(U"'’") == std::u32string_view::npos && std::ranges::find(kMonths, word) == kMonths.end();
}

// The text before a match without the spaces closing it
std::u32string_view head_of(const Match& match) {
  std::u32string_view head = match.context.head();
  while (!head.empty() && unicode::isspace(head.back())) {
    head.remove_suffix(1);
  }
  return head;
}

// Whether a decimal with a dot measures the noun after it (pesava 72.3 chili), unlike a version (Python 3.12 introduce)
bool measures_noun(const Match& match) {
  std::u32string_view head = head_of(match);
  size_t word = head.find_last_of(U" \t\n");
  word = word == std::u32string_view::npos ? 0 : word + 1;
  std::u32string previous = match.context.before();
  // A capitalized word before it names the version (Python 3.12), unless it is an article opening a sentence (Il 72.3)
  bool name = word < head.size() && (unicode::isupper(head[word]) || unicode::isdigit(head[word])) &&
              !kStopWords.contains(previous) && !kCountWords.contains(previous);
  bool percent =
      match.context.after() == U"percento" || (match.context.after() == U"per" && match.context.after(2) == U"cento");
  return (is_noun(match.context.after()) || percent) && !name && !kVersionWords.contains(previous);
}

// Whether a 1 counts the noun after it: ha 1 figlio, but articolo 1 comma 2, 1 su 3
bool counts_noun(const Match& match) {
  if (!is_noun(match.context.after())) {
    return false;
  }
  std::u32string_view head = head_of(match);
  std::u32string previous = match.context.before();
  bool participle = previous.size() > 4 && (previous.ends_with(U"ato") || previous.ends_with(U"ito") ||
                                            previous.ends_with(U"uto") || previous.ends_with(U"ati"));
  return head.empty() || strings::contains(std::u32string_view(U".,!?:;(\""), head.back()) ||
         kCountWords.contains(previous) || participle;
}

// --------------------
// ----- Patterns -----
// --------------------

// '80: a decade written as an apostrophe and its two digits.
class DecadePattern : public Pattern {
public:
  DecadePattern() : Pattern(Kind::DECADE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position) || text[position] != U'\'') {
      return std::nullopt;
    }

    // Two digits: a ten other than 00
    Number decade = digits_at(text, position + 1);
    if (decade.integer.size() != 2 || decade.integer[0] == U'0' || decade.integer[1] != U'0' ||
        !is_clear_after(text, decade.span.end)) {
      return std::nullopt;
    }
    return Match{.kind = Kind::DECADE, .span = {position, decade.span.end}, .numbers = {decade}};
  }
};

// --------------------
// ----- Language -----
// --------------------

class Italian final : public PatternLanguage {
public:
  Italian() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<SecondsPattern>());
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSigns, kOrdinalSignFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<DecadePattern>());
    add_pattern(std::make_unique<RomanPattern>(false, kRomanMarkers));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    if (number.fraction.empty()) {
      // 1034500 alone: the whole text is a number, not a code
      auto n = parse_value(number.integer);
      if (n && number.integer[0] != U'0' && match.context.head().empty() && match.context.tail().empty()) {
        return cardinal(*n);
      }
      if (number.integer != U"1") {
        return integer_words(number);
      }
      // 1 maggio: primo maggio; ha 1 figlio: un figlio
      if (std::ranges::find(kMonths, strings::to_lower(match.context.after())) != kMonths.end()) {
        return U"primo";
      }
      return match.kind == Kind::NUMBER && counts_noun(match) ? one_before(match.context.after()) : U"uno";
    }

    // A decimal comma reads virgola; a dot reads punto (versione 2.1), but virgola in an amount (1.5 kg, 4.3%)
    bool amount = match.kind == Kind::MONEY || match.kind == Kind::SCALE || match.kind == Kind::PERCENT ||
                  match.kind == Kind::UNIT || (match.kind == Kind::NUMBER && measures_noun(match));
    std::vector<std::u32string> fraction;
    size_t zeros = number.fraction.find_first_not_of(U'0');
    fraction.insert(fraction.end(), std::min(zeros, number.fraction.size()), U"zero");
    if (zeros != std::u32string::npos) {
      fraction.push_back(cardinal(parse_value(number.fraction).value_or(0)));
    }
    std::u32string head = number.integer.empty() ? U"" : cardinal(value(number)) + U" ";
    return head + (number.decimal_mark == U',' || amount ? U"virgola " : U"punto ") + join(fraction);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    uint64_t day = value(match.numbers[0]);
    return (day == 1 ? U"primo" : cardinal(day)) + U" " + kMonths[value(match.numbers[1]) - 1] + U" " +
           cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // Digit by digit: tre tre tre, uno due tre ...
    std::vector<std::u32string> groups;
    for (const auto& group : match.numbers) {
      groups.push_back(digits(group.integer));
    }
    return join(groups);
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    // 1/2 litro: mezzo litro, 1/2 porzione: mezza porzione
    std::u32string noun = match.context.after();
    if (fraction_of(match) == std::pair<uint64_t, uint64_t>{1, 2} && match.numbers.size() == 2 && is_noun(noun) &&
        !kFractionWords.contains(noun)) {
      return is_feminine(noun) ? U"mezza" : U"mezzo";
    }
    if (auto words = named_fraction(kFractions, match)) {
      return words;
    }
    auto [a, b] = fraction_of(match);
    if (b < 2 || a >= b || !is_marked_fraction(match, kFractionWords)) {
      return std::nullopt;
    }
    // un quinto, due terzi, tre undicesimi
    return (a == 1 ? U"un" : cardinal(a)) + U" " + ordinal(b, a == 1 ? U'o' : U'i');
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    // With a dot, a time comes after "alle" or from 13 on: alle 9.30, 21.30, but versione 2.79
    uint64_t hours = value(match.numbers[0]), minutes = value(match.numbers[1]);
    bool decimal =
        match.separator == U'.' && !kTimeWords.contains(match.context.before()) && (hours < 13 || hours > 23);
    if (!fits_clock(match) || decimal) {
      return std::nullopt;
    }

    // alle 21.30: ventuno e trenta; 17:15:26: diciassette e quindici minuti e ventisei secondi
    std::u32string hour = hours == 1 ? U"una" : cardinal(hours);
    if (match.numbers.size() == 3) {
      uint64_t seconds = value(match.numbers[2]);
      if (seconds > 59) {
        return std::nullopt;
      }
      return hour + (minutes ? U" e " + (minutes == 1 ? U"un minuto" : cardinal(minutes) + U" minuti") : U"") +
             (seconds ? U" e " + (seconds == 1 ? U"un secondo" : cardinal(seconds) + U" secondi") : U"");
    }
    return minutes ? hour + U" e " + cardinal(minutes) : hour;
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    // Giovanni ventitreesimo, Elisabetta seconda, capitolo quarto, parte seconda, la seconda guerra
    auto role = roman_role(match, kRomanCues);
    if (!role) {
      return std::nullopt;
    }
    std::u32string previous = match.context.before();
    bool feminine = *role == RomanRole::NAME   ? kFeminineNames.contains(previous) :
                    *role == RomanRole::BEFORE ? kFeminineRomanNouns.contains(previous) :
                                                 kFeminineRomanNouns.contains(strings::to_lower(match.context.after()));
    return ordinal(value(match.numbers[0]), feminine ? U'a' : U'o');
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    if (value(match.numbers[0]) == 0) {
      return std::nullopt;
    }
    // il 3° posto, la 3° edizione: ° takes the gender of the article
    char32_t ending = match.suffix == U"ª" ? U'a' : (match.suffix == U"º" ? U'o' : match.suffix.back());
    if (std::u32string article = match.context.before(); match.suffix == U"°") {
      bool elided = kElidedArticles.contains(article);
      ending = (elided ? is_feminine(match.context.after()) : kFeminineArticles.contains(article)) ? U'a' : U'o';
    }
    return ordinal(value(match.numbers[0]), ending);
  }

  std::optional<std::u32string> read_decade(const Match& match) const override {
    return cardinal(value(match.numbers[0])); // gli anni '80: gli anni ottanta
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));

    // 3 mln €: tre milioni di euro, but 10 mila €: diecimila euro
    if (!match.scale.empty()) {
      bool thousands = kScales.at(std::u32string(match.scale)).one == U"mila";
      return scaled(number, match) + (thousands ? U" " : U" di ") + currency.many;
    }

    // 10,50 €: dieci euro e cinquanta centesimi; 0,01 €: un centesimo; but 41,2 €/MWh: quarantuno virgola due euro
    auto cents_of = kCents.find(std::u32string(match.unit));
    if (number.fraction.size() == 2 && cents_of != kCents.end()) {
      Number whole{.integer = number.integer.empty() ? U"0" : number.integer, .grouped = number.grouped};
      uint64_t cents = parse_value(number.fraction).value_or(0);
      std::u32string cents_words = counted(cents, cents_of->second);
      if (whole.integer.find_first_not_of(U'0') == std::u32string::npos && cents) {
        return cents_words;
      }
      return counted(whole, currency) + (cents ? U" e " + cents_words : U"");
    }
    if (number.fraction.empty()) {
      return counted(number, currency);
    }
    return read_number(number, match) + U" " + currency.many;
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K: a resolution
    if ((match.scale == U"k" || match.scale == U"K") && value(match.numbers.back()) < 10) {
      return std::nullopt;
    }
    return scaled(number, match);
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return read_number(number, match) + U" per cento" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    // il 1° gennaio, al 90° minuto: an ordinal, read by its own pattern
    if (match.unit == U"°" && match.numbers.size() == 1 &&
        (kOrdinalArticles.contains(match.context.before()) ||
         std::ranges::find(kMonths, strings::to_lower(match.context.after())) != kMonths.end())) {
      return std::nullopt;
    }
    // 155 d: giorni, but not Italia/2007 d.17' nor 1 d'oro
    if (match.unit == U"d" && !match.context.tail().empty() &&
        strings::contains(std::u32string_view(U".'’"), match.context.tail()[0])) {
      return std::nullopt;
    }
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    return number.fraction.empty() ? counted(number, unit) : read_number(number, match) + U" " + unit.many;
  }

private:
  // A whole number before its noun: un euro, una sterlina, due euro
  static std::u32string counted(const Number& number, const Forms& noun) {
    return is_one(number) ? one_of(noun.one) : integer_words(number) + U" " + noun.many;
  }
  static std::u32string counted(uint64_t n, const Forms& noun) {
    return n == 1 ? one_of(noun.one) : cardinal(n) + U" " + noun.many;
  }

  // An amount with its scale: settantaquattromila, tre milioni
  std::u32string scaled(const Number& number, const Match& match) const {
    const Forms& scale = kScales.at(std::u32string(match.scale));

    // 74 mila: settantaquattromila
    if (scale.one == U"mila" && number.fraction.empty()) {
      return cardinal(value(number) * 1000);
    }
    return number.fraction.empty() ? counted(number, scale) : read_number(number, match) + U" " + scale.many;
  }
};
} // namespace

const Language& language() {
  static const Italian language;
  return language;
}

} // namespace verbalis::languages::it
