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

namespace verbalis::languages::pt {

namespace {
using namespace text;
using namespace numbers;

// What the words around a match say about reading its numbers
struct Cues {
  Gender gender = Gender::MASCULINE; // Of the noun after: duas pessoas; an hour is feminine (às duas)
  bool day = false;                  // 1 de maio: "1" before "de" and a month is "primeiro"
  bool heading = false;              // artigo 80º: an ordinal after a legal heading reads as a cardinal
  bool lettered = false;             // A320-200, 3-4x: a range touching letters is a plain hyphen
  bool named = false;                // R$ 50 reais: the currency is already named after the amount
};

bool is_ascii_letter(char32_t c) {
  return unicode::is_ascii(c) && unicode::isalpha(c);
}

// ------------------------
// ----- Number words -----
// ------------------------

std::u32string unit(uint64_t n, Gender gender) {
  if (gender == Gender::FEMININE && (n == 1 || n == 2)) {
    return n == 1 ? U"uma" : U"duas";
  }
  return std::u32string(kOnes[n]);
}

std::u32string below_hundred(uint64_t n, Gender gender) {
  if (n < 20) {
    return unit(n, gender);
  }
  return std::u32string(kTens[n / 10]) + (n % 10 ? U" e " + unit(n % 10, gender) : U"");
}

std::u32string below_thousand(uint64_t n, Gender gender) {
  uint64_t h = n / 100, r = n % 100;
  if (h == 0) {
    return below_hundred(r, gender);
  }
  if (n == 100) {
    return U"cem";
  }

  // duzentas pessoas
  std::u32string head(kHundreds[h]);
  if (gender == Gender::FEMININE && h > 1) {
    head.replace(head.size() - 2, 2, U"as");
  }
  return head + (r ? U" e " + below_hundred(r, gender) : U"");
}

// "e" joins the last group below 100 or of round hundreds: dois mil e vinte, mil quinhentos e vinte, mil duzentos e um
std::u32string cardinal(uint64_t n, Gender gender = Gender::MASCULINE) {
  std::vector<std::pair<uint64_t, std::u32string>> groups;
  for (auto [scale, one, many] :
       {std::tuple{1000000000000ULL, U"trilhão", U"trilhões"}, std::tuple{1000000000ULL, U"bilhão", U"bilhões"},
        std::tuple{1000000ULL, U"milhão", U"milhões"}}) {
    if (uint64_t q = n / scale) {
      groups.emplace_back(q, cardinal(q) + U" " + (q == 1 ? one : many));
    }
    n %= scale;
  }

  // Thousands agree with the noun: duas mil pessoas
  if (uint64_t q = n / 1000) {
    groups.emplace_back(q, q == 1 ? U"mil" : below_thousand(q, gender) + U" mil");
  }
  n %= 1000;
  if (n || groups.empty()) {
    groups.emplace_back(n, below_thousand(n, gender));
  }

  std::u32string words = groups[0].second;
  for (size_t i = 1; i < groups.size(); ++i) {
    uint64_t last = groups[i].first;
    bool joined = i + 1 == groups.size() && (last < 100 || last % 100 == 0);
    words += (joined ? U" e " : U" ") + groups[i].second;
  }
  return words;
}

// décimo terceiro, vigésima primeira, milésimo
std::u32string ordinal(uint64_t n, bool feminine) {
  char32_t ending = feminine ? U'a' : U'o';
  std::vector<std::u32string> words;
  if (uint64_t thousands = n / 1000) {
    words.push_back((thousands == 1 ? U"" : cardinal(thousands) + U" ") + U"milésim" + ending);
  }
  for (auto [scale, stems] : {std::pair{uint64_t{100}, &kOrdinalHundreds}, std::pair{uint64_t{10}, &kOrdinalTens},
                              std::pair{uint64_t{1}, &kOrdinalOnes}}) {
    if (uint64_t digit = n / scale % 10) {
      words.push_back(std::u32string((*stems)[digit]) + ending);
    }
  }
  return join(words);
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.push_back(unit(c - U'0', Gender::MASCULINE));
  }
  return join(words);
}

// um milhão de reais, but dez mil reais
std::u32string with_de(const std::u32string& words) {
  bool scale = std::ranges::any_of(kScaleWords, [&](const auto& w) { return words.ends_with(w); });
  return scale ? words + U" de" : words;
}

// A number of a given gender, as before a currency or a unit
std::u32string number_with(const Number& number, Gender gender) {
  if (number.fraction.empty()) {
    auto n = parse_value(number.integer);
    return !n || is_spoken_digits(number, 7) ? digits(number.integer) : cardinal(*n, gender);
  }

  // The separator is read as written: dois vírgula cinco, dois ponto cinco
  std::u32string fraction;
  if (number.fraction.size() > 2) {
    fraction = digits(number.fraction);
  } else {
    std::vector<std::u32string> words;
    size_t zeros = number.fraction.find_first_not_of(U'0');
    words.insert(words.end(), std::min(zeros, number.fraction.size()), U"zero");
    if (zeros != std::u32string::npos) {
      words.push_back(cardinal(parse_value(number.fraction).value_or(0)));
    }
    fraction = join(words);
  }
  std::u32string head = number.integer.empty() ? U"" : cardinal(value(number), gender) + U" ";
  return head + (number.decimal_mark == U',' ? U"vírgula " : U"ponto ") + fraction;
}

// An amount with its scale: vinte e duas mil pessoas, três milhões
std::u32string scaled(const Number& number, std::u32string_view scale, Gender gender) {
  const Forms& forms = kScales.at(std::u32string(scale));
  if (forms.one == U"mil") {
    return number_with(number, gender) + U" mil";
  }
  return number_with(number, Gender::MASCULINE) + U" " + (value(number) == 1 ? forms.one : forms.many);
}

// -------------------
// ----- Context -----
// -------------------

bool feminine(std::u32string_view noun) {
  std::u32string word = strings::to_lower(noun);
  std::u32string singular = word;
  if (word.ends_with(U"ões") || word.ends_with(U"ães")) {
    singular = word.substr(0, word.size() - 3) + U"ão";
  } else if (word.ends_with(U"res") || word.ends_with(U"zes") || word.ends_with(U"les")) {
    singular.resize(word.size() - 2);
  } else if (word.ends_with(U's')) {
    singular.pop_back();
  }

  if (kMasculineInA.contains(singular) || kMasculineInA.contains(word)) {
    return false;
  }
  bool feminine_ending = std::ranges::any_of(kFeminineEndings, [&](const auto& e) { return singular.ends_with(e); });
  return kFeminineNouns.contains(singular) || kFeminineNouns.contains(word) || feminine_ending;
}

// The gender of the noun after a number; an hour is feminine: às duas, duas da tarde
Gender gender_of(const Context& context) {
  std::u32string word = context.after();
  if ((word == U"da" || word == U"de") && kDayParts.contains(strings::to_lower(context.after(2)))) {
    return Gender::FEMININE;
  }
  if (word.empty() || !unicode::isalpha(word[0]) || kStopWords.contains(strings::to_lower(word))) {
    return kHourWords.contains(context.before()) ? Gender::FEMININE : Gender::MASCULINE;
  }
  return feminine(word) ? Gender::FEMININE : Gender::MASCULINE;
}

// A lowercase noun follows: 500k seguidores
bool counted(const Context& context) {
  std::u32string word = context.after();
  bool lowercase_word = word.size() > 2 && std::ranges::all_of(word, [](char32_t c) { return unicode::islower(c); });
  return lowercase_word && !kStopWords.contains(word);
}

// A range touching letters: A320-200, 3-4x
bool lettered(const Match& range, const Context& context) {
  std::u32string_view head = context.head(), tail = context.tail();
  size_t before = range.numbers[0].span.begin, after = range.numbers[1].span.end - range.span.end;
  return (before > 0 && is_ascii_letter(head[before - 1])) || (after < tail.size() && is_ascii_letter(tail[after]));
}

// A proper fraction with a small denominator, read as one without a word after it: 3/8, 5/13, 1/100
bool small_fraction(uint64_t a, uint64_t b) {
  return a < b && (b < 20 || b == 100 || b == 1000);
}

// The numbers of a slashed pair which a text starts with (6/2), or ends with if from_end; nullopt if none
std::optional<std::pair<uint64_t, uint64_t>> slashed(std::u32string_view text, bool from_end) {
  size_t start = 0;
  if (from_end) {
    size_t end = text.size();
    start = end;
    while (start > 0 && unicode::isdigit(text[start - 1])) {
      start--;
    }
    if (start == end || start < 2 || text[start - 1] != U'/' || !unicode::isdigit(text[start - 2])) {
      return std::nullopt;
    }
    start -= 2;
    while (start > 0 && unicode::isdigit(text[start - 1])) {
      start--;
    }
  }
  size_t slash = digits_end(text, start);
  size_t end = digits_end(text, slash + 1);
  if (slash == start || slash >= text.size() || text[slash] != U'/' || end == slash + 1) {
    return std::nullopt;
  }
  auto a = parse_value(text.substr(start, slash - start));
  auto b = parse_value(text.substr(slash + 1, end - slash - 1));
  if (!a || !b) {
    return std::nullopt;
  }
  return std::pair{*a, *b};
}

// A phone number written with spaces only: after a word for one (ligue para 213 456 789), or nine digits that start
// as a Portuguese number does (2, 3, 7, 8, 9) and count nothing after them (através do 707 246 707 (dias úteis)), but
// not 213 456 789 pessoas or 300 000 000 €
bool spaced_phone(const Match& phone) {
  const Context& context = phone.context;
  if (kPhoneWords.contains(context.before())) {
    return true;
  }
  std::u32string_view tail = context.tail();
  size_t next = tail.starts_with(U' ') ? 1 : 0;
  std::u32string word = strings::to_lower(context.after());
  bool counts = next < tail.size() && tail[next] != U'(' && !word.empty() && !kStopWords.contains(word);
  const auto& numbers = phone.numbers;
  bool national = numbers.size() == 3 && strings::contains(std::u32string_view(U"23789"), numbers[0].integer[0]);
  return national && numbers.back().integer != U"000" && !counts;
}

// A round number before "s" which is a decade: anos 90s, década de 80s, '80s, 1980s
bool decade(const Match& match) {
  const Number& number = match.numbers.back();
  uint64_t n = value(number);
  std::u32string word = match.context.before();
  bool marked = word == U"anos" || word == U"década" || (word == U"de" && match.context.before(2) == U"década") ||
                match.text.starts_with(U'\'') || match.context.head().ends_with(U'\'');
  return n % 10 == 0 && ((number.integer.size() == 2 && marked) || (number.integer.size() == 4 && is_year_value(n)));
}

// A day and a month marked as a date by the word before them, or by parentheses: dia 15/9, quinta-feira (16/3);
// after "de" if they are no fraction: de 31/12, but de 1/3
bool dated(const Match& date) {
  std::u32string word = date.context.before();
  bool parenthesized = date.context.head().ends_with(U'(') && date.context.tail().starts_with(U')');
  bool after_de = word == U"de" && !small_fraction(value(date.numbers[0]), value(date.numbers[1]));
  return kDateWords.contains(word) || word.ends_with(U"-feira") || parenthesized || after_de;
}

// A fraction without a word after it: 3/8, 5/13, 1/100. Not a law number (Lei 15/98), a grade (18/20), or a score in
// a list of them (3/6, 6/2 e 7/6; 22/24 e 12/15).
bool bare_fraction(const Match& fraction) {
  const auto& numbers = fraction.numbers;
  bool leading_zero = numbers.front().integer[0] == U'0' || numbers.back().integer[0] == U'0';
  auto [a, b] = fraction_of(fraction);
  if (numbers.size() != 2 || leading_zero || !small_fraction(a, b) || kLawWords.contains(fraction.context.before())) {
    return false;
  }
  std::u32string_view head = fraction.context.head(), tail = fraction.context.tail();
  for (std::u32string_view joiner : {U", ", U" e "}) {
    std::optional<std::pair<uint64_t, uint64_t>> next, previous;
    if (tail.starts_with(joiner)) {
      next = slashed(tail.substr(joiner.size()), false);
    }
    if (head.ends_with(joiner)) {
      previous = slashed(head.substr(0, head.size() - joiner.size()), true);
    }
    for (const auto& pair : {next, previous}) {
      if (pair && !small_fraction(pair->first, pair->second)) {
        return false;
      }
    }
  }
  return true;
}

Cues cues_of(const Match& match) {
  const Context& context = match.context;
  Cues cues{.gender = gender_of(context),
            .day = strings::to_lower(context.after()) == U"de" &&
                   std::ranges::find(kMonths, strings::to_lower(context.after(2))) != kMonths.end(),
            .heading = kHeadings.contains(context.before())};
  if (match.kind == Kind::RANGE) {
    cues.lettered = lettered(match, context);
  }

  // R$ 50 reais
  if (match.kind == Kind::MONEY) {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    std::u32string next = strings::to_lower(context.after());
    cues.named = next == currency.one || next == currency.many;
  }
  return cues;
}

// ---------------------
// ----- Patterns -----
// ---------------------

// 2024/03/15, 2024.03.15, 03/15/2024: a date written year first, or month first where the day can't be a month.
// Its numbers are ordered as day, month, year.
class ReorderedDatePattern : public Pattern {
public:
  ReorderedDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t first_end = digits_end(text, position);
    if (!is_clear_before(text, position, U".,/-") || first_end == position || first_end >= text.size()) {
      return std::nullopt;
    }
    std::vector<Number> parts = {digits_at(text, position)};
    char32_t separator = text[first_end];
    if (separator != U'/' && separator != U'.') {
      return std::nullopt;
    }
    for (size_t end = parts[0].span.end; parts.size() < 3; end = parts.back().span.end) {
      if (end + 1 >= text.size() || text[end] != separator || !unicode::isdigit(text[end + 1])) {
        return std::nullopt;
      }
      parts.push_back(digits_at(text, end + 1));
    }
    size_t end = parts[2].span.end;
    if (!is_clear_after(text, end) || is_digit_after(text, end, U".,/-")) {
      return std::nullopt;
    }

    auto fits = [&](size_t i, uint64_t low, uint64_t high) {
      return parts[i].integer.size() <= 2 && value(parts[i]) >= low && value(parts[i]) <= high;
    };
    Match date{.kind = Kind::DATE, .span = {position, end}, .separator = separator};
    if (parts[0].integer.size() == 4 && fits(1, 1, 12) && fits(2, 1, 31)) {
      date.numbers = {parts[2], parts[1], parts[0]};
    } else if (separator == U'/' && fits(0, 1, 12) && fits(1, 13, 31) && parts[2].integer.size() == 4) {
      date.numbers = {parts[1], parts[0], parts[2]};
    } else {
      return std::nullopt;
    }
    return date;
  }
};

// --------------------
// ----- Language -----
// --------------------

class Portuguese final : public PatternLanguage {
public:
  Portuguese() : PatternLanguage({.terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<DatePattern>(kDotDateFormat));
    add_pattern(std::make_unique<ReorderedDatePattern>());
    add_pattern(std::make_unique<SecondsPattern>());
    add_pattern(std::make_unique<PrimePattern>(
        kNotation, PrimeFormat{.cues = kPrimeCues, .minutes = kMinuteCues, .heights = kHeightCues}));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kOrdinalSigns, kOrdinalSignFormat));
    add_pattern(std::make_unique<SuffixPattern>(Kind::DECADE, kDecadeSuffixes, kDecadeFormat));
    add_pattern(std::make_unique<RomanPattern>(false, kRomanMarkers));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    // 08 de abril: a day, despite its leading zero
    if (auto day = unpadded_day(number, match, kMonths, U"de")) {
      return read_number(*day, match);
    }
    Cues cues = cues_of(match);
    if (number.fraction.empty() && cues.day && number.integer == U"1") {
      return U"primeiro";
    }
    // 1234567890 alone: the whole text is a number, not a code
    if (auto n = parse_value(number.integer); n && number.fraction.empty() && number.integer[0] != U'0' &&
                                              match.context.head().empty() && match.context.tail().empty()) {
      return cardinal(*n, Gender::MASCULINE);
    }
    // Em 2020 havia, de 1904 a 1949: a year counts nothing after it
    bool year = (kYearWords.contains(match.context.before()) || !counted(match.context)) &&
                number.integer.size() == 4 && !number.grouped && is_year_value(value(number));
    // 1200-871 Lisboa: the second part of a postal code
    std::u32string_view head = match.context.head();
    bool postal = number.integer.size() == 3 && head.size() >= 5 && head.ends_with(U'-') &&
                  std::ranges::all_of(head.substr(head.size() - 5, 4), unicode::isdigit);
    return number_with(number, number.fraction.empty() && !year && !postal ? cues.gender : Gender::MASCULINE);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    if (set_score(match)) {
      return read_set_score(match);
    }
    uint64_t day = value(match.numbers[0]);
    std::u32string words = (day == 1 ? U"primeiro" : cardinal(day)) + U" de " + kMonths[value(match.numbers[1]) - 1];
    if (match.numbers.size() == 3) {
      return words + U" de " + cardinal(value(match.numbers[2]));
    }

    // Without a year, a zero-padded, marked or named date: 03/04, dia 15/9, quinta-feira (16/3), 19.ago; but 1/4 do
    // salário, 3/4
    bool padded = match.numbers[0].integer[0] == U'0' || match.numbers[1].integer[0] == U'0';
    bool named = std::ranges::any_of(match.text, unicode::isalpha);
    if (!named &&
        (match.separator != U'/' || (!padded && (is_marked_fraction(match, kFractionWords) || !dated(match))))) {
      return std::nullopt;
    }
    return words;
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // 2021-2022, 20 2021-2022, 1685-1750: a range of years; 500-1000: a range of round numbers; 213 456 789 pessoas:
    // a count
    if (match.kind == Kind::PHONE) {
      auto year = [](const Number& group) { return group.integer.size() == 4 && is_year_value(value(group)); };
      const auto& numbers = match.numbers;
      size_t n = numbers.size();
      uint64_t from = value(numbers[n - 2]), to = value(numbers[n - 1]);
      bool years = year(numbers[n - 2]) && year(numbers[n - 1]) && (from >= 1900 || to > from);
      bool round = n == 2 && numbers[0].integer.ends_with(U'0') && numbers[1].integer.ends_with(U'0');
      bool spaced = match.separator == U' ' && match.text.find(U'-') == std::u32string_view::npos;
      if (years || round || (spaced && !spaced_phone(match))) {
        return std::nullopt;
      }
    }

    std::vector<std::u32string> groups;
    for (const auto& group : match.numbers) {
      groups.push_back(group.integer == U"0800" ? U"zero oitocentos" : digits(group.integer));
    }
    return join(groups, U", ");
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    // 1/2 litro: meio litro; 1/2 xícara, 1 1/2 xícara: meia xícara, uma e meia xícara
    if (auto words = named_fraction(kFractions, match)) {
      return *words == U"meio" && cues_of(match).gender == Gender::FEMININE ? U"meia" : *words;
    }
    auto [a, b] = fraction_of(match);
    if (b < 2 || a >= b || !(is_marked_fraction(match, kFractionWords) || bare_fraction(match))) {
      return set_score(match) ? std::optional(read_set_score(match)) : std::nullopt;
    }
    // um quinto, dois terços, um centésimo, três onze avos
    if (b > 10 && b != 100 && b != 1000) {
      return cardinal(a) + U" " + cardinal(b) + U" avos";
    }
    std::u32string denominator = b == 2 ? U"meio" : b == 3 ? U"terço" : ordinal(b, false);
    return cardinal(a) + U" " + denominator + (a == 1 ? U"" : U"s");
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    // A dot makes a time only after an hour word or before "h": às 21.30, 21.30h, but 1.71
    bool decimal = match.separator == U'.' && match.suffix.empty() && !kHourWords.contains(match.context.before());
    if (!fits_clock(match) || decimal) {
      return std::nullopt;
    }

    // 15h da tarde: três horas da tarde; 21h da noite: nove horas da noite
    uint64_t hours = value(match.numbers[0]);
    std::u32string part = match.context.after() == U"da" ? strings::to_lower(match.context.after(2)) : U"";
    if ((part == U"tarde" && hours >= 13 && hours <= 18) || (part == U"noite" && hours >= 19 && hours <= 23)) {
      hours -= 12;
    }

    // 14h30: catorze horas e trinta; 14:30:05: catorze horas e trinta minutos e cinco segundos
    std::u32string words = cardinal(hours, Gender::FEMININE) + (hours == 1 ? U" hora" : U" horas");
    uint64_t minutes = match.numbers.size() > 1 ? value(match.numbers[1]) : 0;
    if (match.numbers.size() == 3) {
      uint64_t seconds = value(match.numbers[2]);
      return words + U" e " + cardinal(minutes) + (minutes == 1 ? U" minuto e " : U" minutos e ") + cardinal(seconds) +
             (seconds == 1 ? U" segundo" : U" segundos");
    }
    return minutes ? words + U" e " + cardinal(minutes) : words;
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    auto role = roman_role(match, kRomanCues);
    if (!role) {
      return std::nullopt;
    }
    std::u32string previous = match.context.before();
    switch (*role) {
      case RomanRole::NAME: { // Dom Pedro segundo, Dona Maria segunda, but Bento dezesseis
        bool feminine = kFeminineNames.contains(previous);
        return n <= 10 ? ordinal(n, feminine) : cardinal(n, feminine ? Gender::FEMININE : Gender::MASCULINE);
      }
      case RomanRole::AFTER: // a Segunda Guerra, o terceiro congresso
        return ordinal(n, kFeminineRomanNouns.contains(strings::to_lower(match.context.after())));
      default: // século quinto, Vaticano segundo, but século vinte e um; capítulo quatro
        return (previous.starts_with(U"século") || previous == U"vaticano") && n <= 10 ? ordinal(n, false) :
                                                                                         cardinal(n);
    }
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    if (n == 0 || (match.suffix == U"°" && !is_ordinal_degree(match, kOrdinalArticles, kOrdinalNouns))) {
      return std::nullopt; // 39° 2' are degrees
    }
    // 2° takes the gender of its article or noun: na 5° edição
    bool feminine_degree =
        match.suffix.ends_with(U'°') &&
        (kOrdinalArticles.contains(match.context.before()) ? kFeminineOrdinalArticles.contains(match.context.before()) :
                                                             feminine(match.context.after()));
    return n >= 10 && cues_of(match).heading ? cardinal(n) :
                                               ordinal(n, match.suffix.ends_with(U'ª') || feminine_degree);
  }

  // anos 90s: anos noventa; anos 1980s: anos mil novecentos e oitenta
  std::optional<std::u32string> read_decade(const Match& match) const override {
    if (!decade(match)) {
      return std::nullopt;
    }
    return cardinal(value(match.numbers[0]));
  }

  std::optional<std::u32string> read_compound(const Match& match) const override {
    return number_with(match.numbers[0], Gender::MASCULINE) + U"-" + std::u32string(match.suffix);
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Forms& currency = kCurrencies.at(std::u32string(match.unit));
    Gender gender = currency.one == U"libra" ? Gender::FEMININE : Gender::MASCULINE;

    // R$ 37,517: prices have two decimals, so three digits are a thousands group
    Number amount = number;
    if (number.decimal_mark == U',' && !number.integer.empty() && number.integer.size() <= 3 &&
        number.fraction.size() == 3) {
      amount = {.integer = number.integer + number.fraction};
    }
    // R$ 1811605: an amount, however long, is not a code
    amount.grouped = amount.grouped || !amount.integer.starts_with(U'0');

    if (cues_of(match).named) {
      return match.scale.empty() ? number_with(amount, gender) : scaled(amount, match.scale, gender);
    }
    if (!match.scale.empty()) {
      return with_de(scaled(amount, match.scale, gender)) + U" " + currency.many;
    }
    // 66,23 euros: a written word keeps the decimal reading, without a zero fraction (3,00 euros: três euros)
    if (match.unit == U"euros") {
      if (amount.fraction.find_first_not_of(U'0') == std::u32string::npos) {
        amount = {.integer = amount.integer, .grouped = amount.grouped};
      }
      return with_de(number_with(amount, gender)) + U" " + currency.many;
    }

    // A price: vinte e nove reais e noventa centavos
    if (amount.fraction.size() == 2 && amount.decimal_mark == U',') {
      uint64_t cents = parse_value(amount.fraction).value_or(0);
      std::u32string centavos = cardinal(cents) + (cents == 1 ? U" centavo" : U" centavos");
      if (value(amount) == 0) {
        return centavos;
      }
      Number whole{.integer = amount.integer, .grouped = amount.grouped};
      std::u32string words =
          with_de(number_with(whole, gender)) + U" " + (amount.integer == U"1" ? currency.one : currency.many);
      return cents ? words + U" e " + centavos : words;
    }
    return with_de(number_with(amount, gender)) + U" " + (is_one(amount) ? currency.one : currency.many);
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    // 4K, 2K: a resolution; but 500k is quinhentos mil
    if ((match.scale == U"k" || match.scale == U"K") && value(match.numbers.back()) < 10 && !counted(match.context)) {
      return std::nullopt;
    }
    return scaled(number, match.scale, cues_of(match).gender);
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return number_with(number, Gender::MASCULINE) + U" por cento" + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    // o 2° lugar, na 5° edição: an ordinal, read by its own pattern
    if (is_ordinal_degree(match, kOrdinalArticles, kOrdinalNouns)) {
      return std::nullopt;
    }
    // 45 s, 30s, but anos 90s
    if (match.unit == U"s" && !match.text.ends_with(U" s") && decade(match)) {
      return std::nullopt;
    }
    const Forms& unit = kUnits.at(std::u32string(match.unit));
    std::u32string_view first_word = std::u32string_view(unit.one).substr(0, unit.one.find(U' '));
    // 1,90 m: um vírgula noventa metro, a decimal reads masculine, and the unit agrees with its integer part
    Gender gender = feminine(first_word) && number.fraction.empty() ? Gender::FEMININE : Gender::MASCULINE;
    bool one = is_one(number) || (number.integer == U"1" && !number.fraction.empty());
    return number_with(number, gender) + U" " + (one ? unit.one : unit.many);
  }

  std::u32string read_range(const Number& from, const Number& to, const Match& match) const override {
    // 2016-2022 and 10-15 read "a"; formations and splits (4-3-3, 60-40) keep "-", but scores read "a": venceu 5-2
    if (is_score(match, kSportWords)) {
      return joiners().score;
    }
    bool plain = from.fraction.empty() && to.fraction.empty() && !from.grouped && !to.grouped;
    bool rising = from.integer.size() == to.integer.size() && value(to) > value(from);
    return plain && rising && !cues_of(match).lettered ? joiners().range : U"-";
  }

private:
  // A slashed pair among words of sets is a score, not a date or a fraction: parciais de 3/6, 6/2 e 7/6 (8/6)
  static bool set_score(const Match& match) {
    const auto& numbers = match.numbers;
    return numbers.size() == 2 && match.separator == U'/' && numbers[0].integer.size() <= 2 &&
           numbers[1].integer.size() <= 2 && mentions(match.context, kSetWords);
  }
  std::u32string read_set_score(const Match& match) const {
    return cardinal(value(match.numbers[0])) + joiners().score + cardinal(value(match.numbers[1]));
  }

  // 2'14'': dois minutos e catorze segundos; 1'12''00: um minuto e doze segundos e zero centésimos
  std::optional<std::u32string> read_primes(const Match& match) const override {
    return join(prime_words(match), U" e ");
  }
  std::u32string read_prime(const Number& number, Prime unit, const Match& /*match*/) const override {
    uint64_t n = value(number);
    const Forms& forms = kPrimeUnits.at(unit);
    Gender gender = unit == Prime::HOUR || unit == Prime::INCH ? Gender::FEMININE : Gender::MASCULINE;
    return cardinal(n, gender) + U" " + (n == 1 ? forms.one : forms.many);
  }
  std::optional<std::u32string> read_prime_fraction(std::u32string_view digits, const Match& /*match*/) const override {
    uint64_t n = parse_value(digits).value_or(0);
    const Forms& forms = kSecondFractions[digits.size() - 1];
    return cardinal(n) + U" " + (n == 1 ? forms.one : forms.many);
  }
};
} // namespace

const Language& language() {
  static const Portuguese language;
  return language;
}

} // namespace verbalis::languages::pt
