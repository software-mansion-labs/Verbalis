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

// centésima trigésima séptima; undécimo, duodécimo, and one word from 13 to 29: decimoctavo, vigesimoprimera
std::u32string ordinal(uint64_t n, bool feminine) {
  std::u32string ending = feminine ? U"a" : U"o";
  // milésimo, dosmilésimo, quinientasmilésima primera: the thousands as one word
  if (n >= 1000) {
    std::u32string head = n / 1000 == 1 ? U"" : below_thousand(n / 1000, feminine ? One::UNA : One::UN);
    std::erase(head, U' ');
    head += U"milésim" + ending;
    return n % 1000 ? head + U" " + ordinal(n % 1000, feminine) : head;
  }
  std::vector<std::u32string> words;
  if (uint64_t h = n / 100 % 10) {
    words.push_back(std::u32string(kOrdinalHundreds[h]) + ending);
  }
  uint64_t r = n % 100;
  if (r == 11 || r == 12) {
    words.push_back((r == 11 ? U"undécim" : U"duodécim") + ending);
  } else if (r > 12 && r < 30 && r % 10) {
    std::u32string head = r < 20 ? U"decimo" : U"vigesimo";
    std::u32string unit = std::u32string(kOrdinalOnes[r % 10]) + ending;
    if (unit.starts_with(U'o')) {
      head.pop_back();
    }
    words.push_back(head + unit);
  } else {
    for (auto [scale, stems] : {std::pair{uint64_t{10}, &kOrdinalTens}, std::pair{uint64_t{1}, &kOrdinalOnes}}) {
      if (uint64_t digit = r / scale % 10) {
        words.push_back(std::u32string((*stems)[digit]) + ending);
      }
    }
  }
  return join(words);
}

// The denominator of a fraction from 11 to 99: onceavo, dieciseisavo, veintiunavo, treintavo, treintaiunavo
std::u32string avo(uint64_t n) {
  std::u32string words = cardinal(n);
  if (size_t y = words.find(U" y "); y != std::u32string::npos) {
    words.replace(y, 3, U"i");
  }
  for (auto [accented, plain] : {std::pair{U'é', U'e'}, std::pair{U'ó', U'o'}}) {
    std::ranges::replace(words, accented, plain);
  }
  if (words.ends_with(U"uno") || words.ends_with(U'a')) {
    words.pop_back();
  }
  return words + U"avo";
}

// The first word of a name: dólar of dólar estadounidense
std::u32string_view first_word(std::u32string_view name) {
  return name.substr(0, name.find(U' '));
}

// A fraction: un quinto, dos tercios, tres medios, un onceavo, un ducentésimo, un dosmilésimo, un millonésimo;
// uno sobre ciento uno. Before "partes", feminine: cinco sextas, una tercera
std::u32string fraction_words(uint64_t a, uint64_t b, bool parts = false) {
  // The ordinal of a round thousand or million is one word: dosmilésimo, cienmillonésimo
  auto round = [](uint64_t n, std::u32string_view ending) {
    std::u32string words = n == 1 ? U"" : cardinal(n, One::UN);
    std::erase(words, U' ');
    return words + std::u32string(ending);
  };
  std::u32string denominator;
  if (b <= 10) {
    denominator = b == 2 ? U"medio" : b == 3 ? U"tercio" : ordinal(b, false);
  } else if (b < 100) {
    denominator = avo(b);
  } else if (b < 1000 && b % 100 == 0) {
    denominator = ordinal(b, false);
  } else if (b % 1000000 == 0) {
    denominator = round(b / 1000000, U"millonésimo");
  } else if (b % 1000 == 0 && b < 1000000) {
    denominator = round(b / 1000, U"milésimo");
  } else {
    return cardinal(a) + U" sobre " + cardinal(b);
  }
  if (parts) {
    denominator = denominator == U"tercio" ? U"tercera" : denominator.substr(0, denominator.size() - 1) + U"a";
  }
  return cardinal(a, parts ? One::UNA : One::UN) + U" " + denominator + (a == 1 ? U"" : U"s");
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

// A grouped number too long to hold, by its groups of six digits: cien mil trillones dos
std::u32string long_cardinal(std::u32string_view digits_, One one) {
  static constexpr std::array<std::pair<std::u32string_view, std::u32string_view>, 5> scales = {{
      {U"", U""},
      {U"millón", U"millones"},
      {U"billón", U"billones"},
      {U"trillón", U"trillones"},
      {U"cuatrillón", U"cuatrillones"},
  }};
  std::vector<std::u32string> words;
  size_t count = (digits_.size() + 5) / 6;
  for (size_t i = count; i-- > 0;) {
    size_t end = digits_.size() - 6 * i, start = end >= 6 ? end - 6 : 0;
    uint64_t group = parse_value(digits_.substr(start, end - start)).value_or(0);
    if (group == 0) {
      continue;
    }
    if (i == 0) {
      words.push_back(cardinal(group, one));
    } else {
      auto [one_scale, many_scales] = scales[i];
      words.push_back(group == 1 ? U"un " + std::u32string(one_scale) :
                                   cardinal(group, One::UN) + U" " + std::u32string(many_scales));
    }
  }
  return join(words);
}

// A number ending with a given form of "uno", as before a currency or a unit
std::u32string number_with(const Number& number, One one) {
  if (number.fraction.empty()) {
    auto n = parse_value(number.integer);
    // 100 000 000 000 000 000 000 000: written in groups, a number even past what a 64-bit integer holds
    if (!n && number.grouped && number.integer[0] != U'0' && number.integer.size() <= 30) {
      return long_cardinal(number.integer, one);
    }
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

// A word before a numbered item, whose number is not a fraction or a date: nº 12/45, expediente 3/5
bool is_number_word(const std::u32string& word) {
  return word == U"número" || word == U"expediente" || kLaws.contains(word);
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

  if (match.kind == Kind::MONEY) {
    // $5.000 dólares, $3 millones de dólares
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    std::u32string next =
        strings::to_lower(match.context.after(!match.scale.empty() && match.context.after() == U"de" ? 2 : 1));
    cues.named = next == first_word(currency.one) || next == first_word(currency.many) || next == U"pesos";
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

// A fraction which the generic pattern leaves: with a minus (-1/3), a grouped denominator (1/100.000), or the unit
// it measures after it (1/2 km, 3 3/5 m). Its unit is the unit's key; a minus starts its text.
class FractionPattern : public Pattern {
public:
  FractionPattern() : Pattern(Kind::FRACTION) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    bool minus = text[position] == U'-' && is_clear_before(text, position, U".,") && is_digit_after(text, position + 1);
    // After a minus, the generic pattern reads the text from the number on, where a mixed number may start: -2 1/3
    size_t start = minus ? position + 1 : position, offset = minus ? start : 0;
    auto fraction = fractions_.match(text.substr(offset), start - offset);
    if (fraction) {
      fraction->span.end += offset;
      for (Number& number : fraction->numbers) {
        number.span.begin += offset;
        number.span.end += offset;
      }
    }
    bool grouped = !fraction;
    if (grouped && !(fraction = grouped_at(text, start))) {
      return std::nullopt;
    }
    fraction->span.begin = position;

    // 3/4": tres cuartos de pulgada
    for (auto mark : keys_at(text, fraction->span.end, kInchMarks)) {
      fraction->unit = mark;
      fraction->span.end += mark.size();
      return fraction;
    }
    size_t unit_start = skip_space(text, fraction->span.end);
    for (auto unit : keys_at(text, unit_start, kNotation.units)) {
      if (is_clear_after(text, unit_start + unit.size())) {
        fraction->unit = unit;
        fraction->span.end = unit_start + unit.size();
        return fraction;
      }
    }
    return minus || grouped ? fraction : std::nullopt;
  }

private:
  // 1/1.000.000
  static std::optional<Match> grouped_at(std::u32string_view text, size_t position) {
    size_t slash = digits_end(text, position);
    if (slash == position || slash + 1 >= text.size() || text[slash] != U'/' ||
        !is_clear_before(text, position, U"./")) {
      return std::nullopt;
    }
    auto numbers = numbers_at(text, slash + 1, U".", U"");
    if (numbers.empty() || !numbers[0].grouped || !is_clear_after(text, numbers[0].span.end, U"/") ||
        is_digit_after(text, numbers[0].span.end, U".,")) {
      return std::nullopt;
    }
    return Match{.kind = Kind::FRACTION,
                 .span = {position, numbers[0].span.end},
                 .numbers = {digits_at(text, position), numbers[0]},
                 .separator = U'/'};
  }

  numbers::FractionPattern fractions_;
};

// The month written as a word at a position (abril, abr.): its number and end
std::optional<std::pair<uint64_t, size_t>> month_at(std::u32string_view text, size_t position) {
  size_t end = position;
  while (end < text.size() && unicode::isalpha(text[end])) {
    end++;
  }
  if (end == position || !is_clear_before(text, position)) {
    return std::nullopt;
  }
  std::u32string word = strings::to_lower(text.substr(position, end - position));
  if (auto it = kMonthNumbers.find(word); it != kMonthNumbers.end() && is_clear_after(text, end)) {
    return std::pair{it->second, end};
  }
  if (auto it = kMonthNumbers.find(word + U"."); end < text.size() && text[end] == U'.' && it != kMonthNumbers.end()) {
    return std::pair{it->second, end + 1};
  }
  return std::nullopt;
}

// A year of four digits at a position, after a space or a comma and a space: 10 mayo 1990, junio 11, 2000
std::optional<Number> year_at(std::u32string_view text, size_t position) {
  size_t start = skip_space(text, position < text.size() && text[position] == U',' ? position + 1 : position);
  if (start == position || digits_end(text, start) != start + 4 || !is_clear_after(text, start + 4) ||
      is_digit_after(text, start + 4, U".,")) {
    return std::nullopt;
  }
  return digits_at(text, start);
}

// 5 febrero, 10 mayo 1990, abril 30, junio 11 2000, abr. 22: a date with its month as a word and no "de" before it.
// Its numbers are the day, the month, and the year if written; its separator is a space.
class MonthDatePattern : public Pattern {
public:
  MonthDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    Match date{.kind = Kind::DATE, .span = {position, position}, .separator = U' '};
    if (unicode::isdigit(text[position])) {
      // The day, then the month
      Number day = digits_at(text, position);
      auto month =
          is_clear_before(text, position, U".,/-") ? month_at(text, skip_space(text, day.span.end)) : std::nullopt;
      if (!month || skip_space(text, day.span.end) == day.span.end || !is_day(day)) {
        return std::nullopt;
      }
      date.numbers = {day, {.integer = to_digits(month->first)}};
      date.span.end = month->second;
    } else {
      // The month, then the day: the end of a phrase, or before a year (not "en junio 2 personas", nor the time of
      // "21 de octubre 12:00")
      auto month = Context(text, {position, position}).before() == U"de" ? std::nullopt : month_at(text, position);
      size_t start = month ? skip_space(text, month->second) : 0;
      if (!month || start == month->second || start >= text.size() || !unicode::isdigit(text[start])) {
        return std::nullopt;
      }
      Number day = digits_at(text, start);
      bool closed = is_sentence_end(text, day.span.end) ||
                    (day.span.end < text.size() && strings::contains(U",.;)", text[day.span.end]));
      if (!is_day(day) || !is_clear_after(text, day.span.end) || (!closed && !year_at(text, day.span.end))) {
        return std::nullopt;
      }
      date.numbers = {day, {.integer = to_digits(month->first)}};
      date.span.end = day.span.end;
    }
    if (auto year = year_at(text, date.span.end)) {
      date.numbers.push_back(*year);
      date.span.end = year->span.end;
    }
    return date;
  }

private:
  static bool is_day(const Number& day) {
    uint64_t n = value(day);
    return day.integer.size() <= 2 && n >= 1 && n <= 31;
  }
};

// 12h02m00s, 12h02m: hours, minutes, and optionally seconds, each with its letter
class CompactTimePattern : public Pattern {
public:
  CompactTimePattern() : Pattern(Kind::TIME) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,:")) {
      return std::nullopt;
    }
    Match time{.kind = Kind::TIME, .separator = U'h'};
    size_t end = position;
    for (char32_t letter : std::u32string_view(U"hms")) {
      size_t digits = digits_end(text, end);
      if (digits == end || digits - end > 2 || (letter != U'h' && digits - end != 2) || digits >= text.size() ||
          text[digits] != letter) {
        if (letter == U's' && time.numbers.size() == 2) {
          break; // 12h02m: no seconds
        }
        return std::nullopt;
      }
      time.numbers.push_back(digits_at(text, end));
      end = digits + 1;
    }
    if (!is_clear_after(text, end)) {
      return std::nullopt;
    }
    // 12h02m00s: no seconds to say
    if (time.numbers.size() == 3 && value(time.numbers[2]) == 0) {
      time.numbers.pop_back();
    }
    time.span = {position, end};
    return time;
  }
};

// siglo xix, año mcmlxxxix, maría vii, cmxcix: a lowercase Roman numeral, after a century or a year, of three
// letters or more after a name ("parte vi" may be "I saw"), or of three letters or more as the whole text
class LowerRomanPattern : public Pattern {
public:
  LowerRomanPattern() : Pattern(Kind::ROMAN) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t end = position;
    while (end < text.size() && strings::contains(std::u32string_view(U"ivxlcdm"), text[end])) {
      end++;
    }
    if (end == position || !is_clear_before(text, position, U"'.-/") || !is_clear_after(text, end, U"'-/")) {
      return std::nullopt;
    }
    std::u32string numeral(text.substr(position, end - position));
    std::ranges::transform(numeral, numeral.begin(), [](char32_t c) { return c - U'a' + U'A'; });
    auto value = roman_value(numeral);
    std::u32string previous = Context(text, {position, end}).before();
    bool year = previous.starts_with(U"siglo") || previous == U"año";
    bool name = kRomanCues.names.contains(previous) && numeral.size() >= 3;
    bool alone = position == 0 && end == text.size() && numeral.size() >= 3; // cmxcix as the whole text
    if (!value || (numeral.size() == 1 && numeral != U"I" && numeral != U"V" && numeral != U"X") ||
        !(year || name || alone)) {
      return std::nullopt;
    }
    return Match{.kind = Kind::ROMAN,
                 .span = {position, end},
                 .numbers = {{.span = {position, end}, .integer = to_digits(*value)}}};
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
    add_pattern(std::make_unique<MonthDatePattern>());
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<FractionPattern>());
    add_pattern(std::make_unique<PrefixedMoneyPattern>(kNotation, kCurrencyWords));
    add_pattern(std::make_unique<SecondsPattern>());
    add_pattern(std::make_unique<CompactTimePattern>());
    add_pattern(std::make_unique<GroupedOrdinalPattern>(kOrdinalSigns, U". \u00a0\u202f"));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSigns, kOrdinalSignFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSuffixes));
    add_pattern(std::make_unique<SuffixPattern>(Kind::DECADE, kDecadeSuffixes, kDecadeFormat));
    add_pattern(std::make_unique<RomanPattern>());
    add_pattern(std::make_unique<LowerRomanPattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    // extensión 12: digit by digit
    if (match.context.before() == U"extensión" && number.fraction.empty() && number.integer.size() > 1) {
      return digits(number.integer);
    }
    return number_with(number, cues_of(match).one);
  }

  // +593: a country code of three digits, digit by digit; +58: cincuenta y ocho
  std::u32string read_country_code(std::u32string_view text) const override {
    return text.size() > 2 ? digits(text) : read_code_digits(text);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // A law: Ley dos de dos mil dieciocho
    if (match.separator == 0) {
      return cardinal(value(match.numbers[0])) + U" de " + cardinal(value(match.numbers[1]));
    }

    // 30-2-1990 and 30.2.1990 need a full year (3.8.10 is a version)
    uint64_t day = value(match.numbers[0]), month = value(match.numbers[1]);
    bool slash = match.separator == U'/';
    // 3/2 as the whole text, or 1 5/3: after a whole number, a fraction
    if (slash && match.numbers.size() == 2 && match.context.head().empty() && match.context.tail().empty()) {
      return std::nullopt;
    }
    auto before = words_of(match.context.head());
    if (slash && match.numbers.size() == 2 && !before.empty() && std::ranges::all_of(before.back(), unicode::isdigit) &&
        match.context.head().ends_with(U' ')) {
      return std::nullopt;
    }
    if (!slash && match.separator != U' ' && (match.numbers.size() < 3 || match.numbers[2].integer.size() != 4)) {
      return std::nullopt;
    }

    // A day and month: 15/06, el 3/5, but 3/4 and el 1/3 de are fractions, 24/7 is a phrase, nº 3/5 a number
    if (slash && match.numbers.size() == 2) {
      std::u32string previous = match.context.before();
      bool padded = match.numbers[0].integer.starts_with(U'0') || match.numbers[1].integer.starts_with(U'0');
      bool cue = kDateWords.contains(previous) && !kFractionWords.contains(strings::to_lower(match.context.after()));
      std::u32string_view tail = match.context.tail();
      bool inch = tail.starts_with(U'"') || tail.starts_with(U"''") || tail.starts_with(U'″') || tail.starts_with(U'”');
      if ((day < month && !padded && !cue) || (day == 24 && month == 7) || inch || is_number_word(previous)) {
        return std::nullopt;
      }
    }

    std::u32string words = (day == 1 ? U"primero" : cardinal(day)) + U" de " + kMonths[month - 1];
    return match.numbers.size() == 2 ? words : words + U" de " + cardinal(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // 600 123 456 is a count, unless a word says it is a phone number
    if (match.kind == Kind::PHONE && match.separator == U' ' && !kPhoneWords.contains(match.context.before())) {
      return std::nullopt;
    }
    // 1990-2000 is a range of years
    auto year = [](const Number& n) { return value(n) >= 1000 && value(n) <= 2100; };
    if (match.numbers.size() == 2 && year(match.numbers[0]) && year(match.numbers[1])) {
      return std::nullopt;
    }
    return digits(match.text);
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    // un quinto, dos tercios; 24/7, but nº 12/45 is a number
    auto [a, b] = fraction_of(match);
    if (a == 24 && b == 7) {
      return U"veinticuatro siete";
    }
    // Not a number with its year (117/2004) or a code (023/2006, 8/12-2K, SUP-REC-15/2011)
    std::u32string previous = match.context.before();
    bool year = a > 1 && b >= 1900 && b < 2100;
    bool code = match.numbers[match.numbers.size() - 2].integer.starts_with(U'0') ||
                match.context.tail().starts_with(U'-') || match.context.head().ends_with(U'-');
    bool alone = match.context.head().empty() && match.context.tail().empty(); // 3/2 alone: tres medios
    if (b < 2 || (a >= b && !alone) || year || code || is_number_word(previous)) {
      return std::nullopt;
    }
    // medio limón, media taza; el tercio de, but un tercio
    if (One one = cues_of(match).one; a == 1 && b == 2 && one != One::UNO) {
      return one == One::UNA ? U"media" : U"medio";
    }
    std::u32string next = strings::to_lower(match.context.after());
    std::u32string words = fraction_words(a, b, next == U"parte" || next == U"partes");
    bool article = previous == U"el" || previous == U"del" || previous == U"al" || previous == U"un";
    return a == 1 && article && words.starts_with(U"un ") ? words.substr(3) : words;
  }

  std::optional<std::u32string> read_mixed_fraction(const Match& match) const override {
    // -1/3: menos un tercio
    if (match.text.starts_with(U'-')) {
      Match fraction = match;
      fraction.text = match.text.substr(1);
      auto words = read_mixed_fraction(fraction);
      return words ? std::optional(U"menos " + *words) : std::nullopt;
    }

    bool mixed = match.numbers.size() == 3;
    if (!mixed && match.unit.empty()) {
      return read_fraction(match);
    }
    auto [a, b] = fraction_of(match);
    // 1 5/3: uno y cinco tercios, an improper fraction after a whole number
    if (b < 2 || (a >= b && !(mixed && match.unit.empty()))) {
      return std::nullopt;
    }

    // medio milímetro, tres quintos de milla por hora; un metro y medio, tres y tres quintos de metro, dos y media
    auto it = kUnits.find(std::u32string(match.unit));
    const Forms* unit = match.unit.empty() ? nullptr : it == kUnits.end() ? &kInch : &it->second;
    One one = unit ? (feminine(first_word(unit->one)) ? One::UNA : One::UN) : cues_of(match).one;
    bool half = a == 1 && b == 2;
    std::u32string fraction = half ? (one == One::UNA ? U"media" : U"medio") : fraction_words(a, b);
    if (!mixed) {
      // 1/2"Para: a word right after an inch mark gets a space
      bool glued = unit == &kInch && unicode::isalpha(match.context.tail().empty() ? U' ' : match.context.tail()[0]);
      return fraction + (half ? U" " : U" de ") + unit->one + (glued ? U" " : U"");
    }
    std::u32string whole = number_with(match.numbers[0], one);
    if (half && unit) {
      std::u32string words = is_one(match.numbers[0]) ? unit->one : unit->many;
      size_t per = words.find(U" por ");
      words.insert(per == std::u32string::npos ? words.size() : per, U" y " + fraction);
      return whole + U" " + words;
    }
    return whole + U" y " + fraction + (unit ? U" de " + unit->one : U"");
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    bool hour = std::ranges::find(kHourSuffixes, match.suffix) != kHourSuffixes.end();
    bool morning = std::ranges::find(kMorningSuffixes, match.suffix) != kMorningSuffixes.end();
    bool evening = std::ranges::find(kEveningSuffixes, match.suffix) != kEveningSuffixes.end();
    uint64_t hours = value(match.numbers[0]);
    std::u32string words = cardinal(hours, One::UNA);

    // 3:02:32: tres horas dos minutos y treinta y dos segundos; 12:00:00: doce
    if (match.numbers.size() == 3) {
      uint64_t minutes = value(match.numbers[1]), seconds = value(match.numbers[2]);
      if (hours > 23 || minutes > 59) {
        return std::nullopt;
      }
      if (minutes == 0 && seconds == 0) {
        return words;
      }
      auto unit = [](uint64_t n, std::u32string_view one, std::u32string_view many) {
        return cardinal(n, one == U"hora" ? One::UNA : One::UN) + U" " + std::u32string(n == 1 ? one : many);
      };
      words = unit(hours, U"hora", U"horas") + U" " + unit(minutes, U"minuto", U"minutos");
      return seconds ? words + U" y " + unit(seconds, U"segundo", U"segundos") : words;
    }

    // 19 h, 80hrs: diecinueve horas; 7 p. m. needs a clock hour
    if (match.numbers.size() < 2) {
      if (hour) {
        words += hours == 1 ? U" hora" : U" horas";
      } else if (hours == 0 || hours > 12) {
        return std::nullopt;
      }
    } else {
      // A dot makes a time only after "a las" or before a suffix: a las 18.30, 18.30 h, a 13.30, but 1.13, a 3.28
      uint64_t minutes = value(match.numbers[1]);
      std::u32string previous = match.context.before();
      // A clock time alone, or before a time zone: 12.15, 1.00 EST
      std::u32string next = match.context.after();
      bool zone = std::ranges::any_of(kTimeZones, [&](const auto& z) { return strings::to_lower(next) == z; });
      bool alone = match.context.head().empty() && match.context.tail().empty();
      bool cue = kTimeWords.contains(previous) || (previous == U"a" && minutes % 15 == 0) || zone || alone;
      bool decimal = match.separator == U'.' && match.suffix.empty() && !cue;
      if (!fits_clock(match) || decimal || ((morning || evening) && hours > 12)) {
        return std::nullopt;
      }
      // cinco y dos, doce y cuarto, trece y media; 16:00 h: dieciséis horas
      if (minutes == 0 && hour) {
        words += hours == 1 ? U" hora" : U" horas";
      } else if (minutes) {
        words += U" y " + (minutes == 15 ? U"cuarto" : minutes == 30 ? U"media" : cardinal(minutes));
      }
    }

    // 1 a. m.: una de la mañana; 12 p. m.: doce del mediodía; 9 p. m.: nueve de la noche
    if (morning) {
      words += hours == 12 ? U" de la noche" : U" de la mañana";
    } else if (evening) {
      words += hours == 12 ? U" del mediodía" : hours < 8 ? U" de la tarde" : U" de la noche";
    }
    // The dot of "a. m." or "hrs." also closes a sentence: a las 2:43 a. m. La mayoría
    if (match.suffix.ends_with(U'.') && is_sentence_end(match.context.tail(), 0, true)) {
      words += U".";
    }
    return words;
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    // V, cmxcix as the whole text: an ordinal
    if (match.context.head().empty() && match.context.tail().empty()) {
      return ordinal(n, false);
    }
    std::u32string previous = match.context.before();
    auto role = roman_role(match, kRomanCues);
    if (unicode::islower(match.text[0])) {
      role = kRomanCues.names.contains(previous) ? RomanRole::NAME : RomanRole::BEFORE; // As LowerRomanPattern found
    }

    // el XI apóstol: after an article and before a noun, a numeral of I, V and X is an ordinal
    std::u32string next = match.context.after();
    bool article = previous == U"el" || previous == U"la" || previous == U"los" || previous == U"las";
    bool noun = !next.empty() && unicode::islower(next[0]) && !kStopWords.contains(next);
    if (!role && article && noun && match.text.find_first_not_of(U"IVX") == std::u32string_view::npos) {
      std::u32string words = ordinal(n, previous == U"la" || previous == U"las");
      return words.ends_with(U"primero") || words.ends_with(U"tercero") ? words.substr(0, words.size() - 1) : words;
    }
    // Years reach 3999: año MCMLXXXIX
    if (!role || (n >= 1000 && role != RomanRole::BEFORE)) {
      return std::nullopt;
    }
    switch (*role) {
      case RomanRole::NAME: // Felipe sexto, Isabel segunda, but Alfonso trece
        return n <= 10 ? ordinal(n, kFeminineNames.contains(previous)) : cardinal(n);
      case RomanRole::AFTER: { // la Segunda Guerra, el primer congreso, el vigesimotercer festival
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
    // Ordinal words reach 999 999
    uint64_t n = value(match.numbers[0]);
    if (n == 0 || n >= 1000000 ||
        (match.suffix == U"°" && !is_ordinal_degree(match, kOrdinalArticles, kOrdinalNouns))) {
      return std::nullopt; // 18°52' are degrees
    }

    // 1.er, 21ᵉʳ: vigesimoprimer; 4.ª, 2da: feminine; 2° takes the gender of its article or noun, and 1° piso is
    // primer piso
    std::u32string_view suffix = match.suffix;
    std::u32string after = match.context.after();
    bool degree = suffix.ends_with(U'°'), article = kOrdinalArticles.contains(match.context.before());
    bool feminine_noun = article ? kFeminineOrdinalArticles.contains(match.context.before()) : feminine(after);
    std::u32string words = ordinal(n, suffix.ends_with(U'a') || suffix.ends_with(U'ª') || (degree && feminine_noun));
    bool before_noun = degree && !feminine_noun && kOrdinalNouns.contains(strings::to_lower(after));
    if (suffix.ends_with(U"er") || suffix.ends_with(U"ᵉʳ") || before_noun) {
      // 11.ᵉʳ: decimoprimer, as undécimo has no short form
      if (words.ends_with(U"undécimo")) {
        words.replace(words.size() - 8, 8, U"decimoprimero");
      }
      if (words.ends_with(U"primero") || words.ends_with(U"tercero")) {
        words.pop_back();
      }
    }
    // 33°Campeonato: trigésimo tercer Campeonato
    std::u32string_view tail = match.context.tail();
    return degree && !tail.empty() && unicode::isalpha(tail[0]) ? words + U" " : words;
  }

  std::optional<std::u32string> read_decade(const Match& match) const override {
    return read_number(match.numbers[0], match) + U"s"; // los 80s: los ochentas
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    One one = feminine(first_word(currency.one)) ? One::UNA : One::UN;
    bool named = cues_of(match).named;

    // Q1 to Q9 are quarters
    if ((match.unit == U"Q" || match.unit == U"q") && number.integer.size() == 1 && number.fraction.empty()) {
      return std::nullopt;
    }

    // A price: cinco dólares con cincuenta, cincuenta y cinco centavos, un euro con cinco céntimos, diez dólares
    auto cents = kCents.find(std::u32string(match.unit));
    size_t places = number.fraction.size();
    if (match.scale.empty() && (places == 1 || places == 2) && cents != kCents.end()) {
      uint64_t n = parse_value(number.fraction).value_or(0) * (places == 1 ? 10 : 1);
      std::u32string cent_words = cardinal(n, One::UN) + U" " + (n == 1 ? cents->second.one : cents->second.many);
      if (value(number) == 0 && n > 0) {
        return cent_words;
      }
      Number whole{.integer = number.integer, .grouped = number.grouped};
      std::u32string words = number_with(whole, one);
      if (!named) {
        words += U" " + (is_one(whole) ? currency.one : currency.many);
      }
      if (n == 0) {
        return words;
      }
      return words + U" con " + (n < 10 && !named ? cent_words : cardinal(n));
    }

    std::u32string words = match.scale.empty() ? number_with(number, one) : scaled(number, match);
    if (named) {
      // un millón de pesos
      bool of = (words.ends_with(U"llón") || words.ends_with(U"llones")) && match.context.after() != U"de";
      return of ? words + U" de" : words;
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
    // el 2° lugar, la 6°: an ordinal, read by its own pattern
    if (is_ordinal_degree(match, kOrdinalArticles, kOrdinalNouns)) {
      return std::nullopt;
    }
    // 4 s, but 6s and los 60s
    if (match.unit == U"s" && match.text.find(U' ') == std::u32string_view::npos) {
      return std::nullopt;
    }
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    // 4,51/s: cuatro coma cincuenta y uno por segundo
    One one = unit.one.starts_with(U"por ") ? One::UNO : feminine(first_word(unit.one)) ? One::UNA : One::UN;
    return number_with(number, one) + U" " + (is_one(number) ? unit.one : unit.many);
  }

private:
  // An amount with its scale: dos millones, un billón, cinco mil
  std::u32string scaled(const Number& number, const Match& match) const {
    const std::u32string& scale = kScales.at(std::u32string(match.scale));
    std::u32string word = scale == U"mil" || is_one(number) ? scale : scale.substr(0, scale.size() - 2) + U"ones";
    return number_with(number, One::UN) + U" " + word;
  }
};
} // namespace

const Language& language() {
  static const Spanish language;
  return language;
}

} // namespace verbalis::languages::es
