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

namespace verbalis::languages::hi {

namespace {
using namespace text;
using namespace numbers;

// ------------------------
// ----- Number words -----
// ------------------------

// Every number up to 99 is its own word; then "<digit> सौ", and the Indian scales (हज़ार, लाख, करोड़, अरब, ...)
std::u32string cardinal(uint64_t n) {
  if (n < 100) {
    return kCardinals.at(static_cast<int32_t>(n));
  }
  if (n < 1000) {
    uint64_t rest = n % 100;
    return kCardinals.at(static_cast<int32_t>(n / 100)) + U" सौ" + (rest ? U" " + cardinal(rest) : U"");
  }

  // Hindi keeps the "एक" of every scale: एक हज़ार, एक लाख
  for (auto it = kLargeCardinals.rbegin(); it != kLargeCardinals.rend(); ++it) {
    auto base = static_cast<uint64_t>(it->first);
    if (n >= base) {
      uint64_t low = n % base;
      return cardinal(n / base) + U" " + it->second + (low ? U" " + cardinal(low) : U"");
    }
  }
  return U"";
}

// A year in hundreds: 1999 -> उन्नीस सौ निन्यानवे, 2101 -> इक्कीस सौ एक; but 2024 -> दो हज़ार चौबीस
std::u32string year(uint64_t n) {
  bool hundreds = (n >= 1100 && n < 2000) || (n >= 2100 && n < 10000 && n / 100 % 10 != 0);
  if (!hundreds) {
    return cardinal(n);
  }
  uint64_t rest = n % 100;
  return cardinal(n / 100) + U" सौ" + (rest ? U" " + cardinal(rest) : U"");
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.push_back(cardinal(c - U'0'));
  }
  return join(words);
}

// An English ordinal up to 99, as said in Hindi (7th -> सेवंथ); empty above
std::u32string english_ordinal(uint64_t n) {
  if (n < 20) {
    return kEnglishOrdinals[n];
  }
  if (n < 100) {
    return n % 10 ? kEnglishTens[n / 10] + U" " + kEnglishOrdinals[n % 10] : kEnglishTenths[n / 10];
  }
  return U"";
}

// An amount and a half: 1.5 -> डेढ़, 2.5 -> ढाई, 3.5 -> साढ़े तीन
std::optional<std::u32string> and_a_half(const Number& number) {
  uint64_t n = value(number);
  if (number.fraction.find_last_not_of(U'0') != 0 || number.fraction[0] != U'5' || n == 0) {
    return std::nullopt;
  }
  return n == 1 ? U"डेढ़" : n == 2 ? U"ढाई" : U"साढ़े " + cardinal(n);
}

bool is_devanagari(char32_t c) {
  return c >= 0x0900 && c < 0x0980 && unicode::isalpha(c);
}

// Whether any of the few words before a match is one of given words: नंबर पे कॉल करो 1374-309988
bool after_any_of(const Match& match, const std::unordered_set<std::u32string>& words, size_t k) {
  for (size_t i = 1; i <= k; ++i) {
    if (words.contains(match.context.before(i))) {
      return true;
    }
  }
  return false;
}

// --------------------
// ----- Patterns -----
// --------------------

// टी20, 5जी, है1: a number joined to a Devanagari word, which the generic patterns leave as written
class JoinedNumberPattern : public Pattern {
public:
  JoinedNumberPattern() : Pattern(Kind::NUMBER) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    bool before = position > 0 && is_devanagari(text[position - 1]);
    if (!unicode::isdigit(text[position]) || (!before && !is_clear_before(text, position, U".,"))) {
      return std::nullopt;
    }
    for (auto& number : numbers_at(text, position, kNotation.group_separators, kNotation.decimal_separators)) {
      // A zero after a letter marks an abbreviation, as a dot: डॉ0, ए0 जे0
      size_t end = number.span.end;
      bool after = end < text.size() && is_devanagari(text[end]);
      if (before && number.integer == U"0" && number.fraction.empty()) {
        return std::nullopt;
      }
      if (after || (before && is_clear_after(text, end))) {
        return Match{.kind = Kind::NUMBER, .span = number.span, .numbers = {number}};
      }
    }
    return std::nullopt;
  }
};

// चंद्रयान-III, भाग-XCIX: a Roman numeral joined to the word it numbers by a hyphen
class HyphenRomanPattern : public Pattern {
public:
  HyphenRomanPattern() : Pattern(Kind::ROMAN) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (position < 2 || text[position - 1] != U'-' || !unicode::isalpha(text[position - 2])) {
      return std::nullopt;
    }
    // The numeral as if it started the text, which the hyphen before it would otherwise rule out
    auto roman = roman_.match(text.substr(position), 0);
    if (!roman) {
      return std::nullopt;
    }
    for (Span* span : {&roman->span, &roman->numbers[0].span}) {
      *span = {span->begin + position, span->end + position};
    }
    return roman;
  }

private:
  RomanPattern roman_;
};

// 2^3: the power of a number (only the caret and the exponent are matched)
class PowerPattern : public Pattern {
public:
  PowerPattern() : Pattern(Kind::SCORE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (position == 0 || text[position] != U'^' || !unicode::isdigit(text[position - 1]) ||
        !is_digit_after(text, position + 1)) {
      return std::nullopt;
    }
    Number exponent = digits_at(text, position + 1);
    if (!is_clear_after(text, exponent.span.end)) {
      return std::nullopt;
    }
    return Match{.kind = Kind::SCORE, .span = {position, exponent.span.end}, .numbers = {exponent}, .separator = U'^'};
  }
};

// --------------------
// ----- Language -----
// --------------------

// Hindi reads its numbers without the gender of the counted noun so far.
class Hindi final : public PatternLanguage {
public:
  Hindi() : PatternLanguage({.characters = &kCharacters, .terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<SecondsPattern>(Keys{}, true));
    add_pattern(std::make_unique<PowerPattern>());
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, keys_of(kOrdinalSuffixes)));
    add_pattern(std::make_unique<SuffixPattern>(Kind::ORDINAL, kEnglishOrdinalSuffixes));
    add_pattern(std::make_unique<RomanOrdinalPattern>(keys_of(kOrdinalSuffixes)));
    add_pattern(std::make_unique<HyphenRomanPattern>());
    add_pattern(std::make_unique<RomanPattern>(false, kRomanOrdinalSuffixes));
    add_pattern(std::make_unique<JoinedNumberPattern>());
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    // An address of four parts digit by digit: 192.168.1.1 -> एक नौ दो डॉट एक छह आठ डॉट एक डॉट एक
    bool address = match.kind == Kind::VERSION && match.numbers.size() == 4 &&
                   std::ranges::all_of(match.numbers, [](const Number& part) { return value(part) <= 255; });
    if (number.fraction.empty() && (address || is_spoken_digits(number, match))) {
      return digits(number.integer);
    }
    // The fraction is read digit by digit: as written in a number (97.0 -> सत्तानवे दशमलव शून्य), without its
    // trailing zeros in an amount (5.50 लाख -> पाँच दशमलव पाँच लाख, 888.60 रुपये)
    std::u32string_view fraction = number.fraction;
    if (match.kind != Kind::NUMBER || kCurrencyWords.contains(match.context.after())) {
      fraction = fraction.substr(0, fraction.find_last_not_of(U'0') + 1);
    }
    std::u32string integer = cardinal(value(number));
    return fraction.empty() ? integer : integer + U" दशमलव " + digits(fraction);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    // A380: तीन सौ अस्सी, 5000mAh: पाँच हज़ार, census2011: दो हज़ार ग्यारह, but ABCDE1234F digit by digit
    auto n = code_value(text, 4);
    bool number = n && (*n < 1000 || *n % 100 == 0 || (*n >= 1900 && *n < 2100));
    return number ? cardinal(*n) : digits(text);
  }

  std::u32string read_code_letters(std::u32string_view letters) const override {
    // Capitals are spelled (ABC-123: ए बी सी), unless they make a word (COVID-19); other letters stay
    bool capitals =
        letters.size() <= 5 && std::ranges::all_of(letters, [](char32_t c) { return c >= U'A' && c <= U'Z'; });
    if (!capitals || kCodeWords.contains(std::u32string(letters))) {
      return std::u32string(letters);
    }
    std::vector<std::u32string> words;
    for (char32_t c : letters) {
      words.push_back(kLetters[c - U'A']);
    }
    return join(words);
  }

  std::u32string read_country_code(std::u32string_view text) const override {
    return digits(text); // +91: प्लस नौ एक
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // Digit by digit. Without a trunk zero (1374 309988), only after a word of phones: नंबर पे कॉल करो
    bool zero = match.text.starts_with(U'0') || match.text.starts_with(U"(0");
    if (match.kind == Kind::PHONE && !zero && !after_any_of(match, kPhoneCues, 6)) {
      return std::nullopt;
    }
    // The digits of a prefix too: 0 (80) 26411128
    std::vector<std::u32string> words;
    std::u32string_view prefix =
        match.kind == Kind::PHONE ? match.text.substr(0, match.numbers[0].span.begin - match.span.begin) : U"";
    for (char32_t c : prefix) {
      if (unicode::isdigit(c)) {
        words.push_back(cardinal(c - U'0'));
      }
    }
    for (const auto& group : match.numbers) {
      words.push_back(digits(group.integer));
    }
    return join(words);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // 06-05: a day and a month only with a leading zero; 10-15 is a range
    if (match.numbers.size() == 2) {
      const auto& day = match.numbers[0].integer;
      const auto& month = match.numbers[1].integer;
      if (match.separator == U'.' || day.size() != 2 || month.size() != 2 || (day[0] != U'0' && month[0] != U'0')) {
        return std::nullopt;
      }
      return cardinal(value(match.numbers[0])) + U" " + kMonths[value(match.numbers[1])];
    }
    // सत्ताईस मार्च दो हज़ार छब्बीस, दो जुलाई उन्नीस सौ सत्तर
    return cardinal(value(match.numbers[0])) + U" " + kMonths[value(match.numbers[1])] + U" " +
           year(value(match.numbers[2]));
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    // Common proper fractions have their own nouns, others are "<a> बटा <b>": 14/3 -> चौदह बटा तीन
    // Numbers with a leading zero are codes: गाड़ी संख्या 03281/03282
    auto coded = [](const Number& number) { return number.integer.size() > 1 && number.integer[0] == U'0'; };
    if (coded(match.numbers[match.numbers.size() - 2]) || coded(match.numbers.back())) {
      return std::nullopt;
    }
    auto [a, b] = fraction_of(match);
    if (a == 1 && b == 2) {
      return U"आधा";
    }
    if ((b == 3 || b == 4) && a < b) {
      return cardinal(a) + (b == 3 ? U" तिहाई" : U" चौथाई");
    }
    return cardinal(a) + U" बटा " + cardinal(b);
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    return ordinal(value(match.numbers[0]), match.suffix);
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    // LIII वा: an ordinal; कक्षा XII, चंद्रयान-III: a number
    uint64_t n = value(match.numbers[0]);
    if (!match.suffix.empty()) {
      return ordinal(n, match.suffix);
    }
    if (!match.context.head().ends_with(U'-') && !roman_role(match, kRomanCues)) {
      return std::nullopt;
    }
    return cardinal(n);
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    // आठ बजे, सात बजकर इक्कीस मिनट; 10:30 बजे: दस तीस बजे; 03:50 मिनट: तीन बजकर पचास मिनट
    if (match.numbers.size() < 2 || !fits_clock(match)) {
      return std::nullopt;
    }
    // 8.30 only before बजे or after सुबह, शाम: otherwise a decimal
    std::u32string next = match.context.after();
    if (match.separator == U'.' && next != U"बजे" && !kDayTimes.contains(match.context.before())) {
      return std::nullopt;
    }
    uint64_t minutes = value(match.numbers[1]);
    std::u32string hours = cardinal(value(match.numbers[0]));
    if (match.numbers.size() == 3) {
      // बारह बजकर दस मिनट दस सेकंड
      uint64_t seconds = value(match.numbers[2]);
      if (seconds > 59) {
        return std::nullopt;
      }
      return hours + U" बजकर " + (minutes ? cardinal(minutes) + U" मिनट " : U"") + cardinal(seconds) + U" सेकंड";
    }
    if (next == U"बजे") {
      return minutes ? hours + U" " + cardinal(minutes) : hours;
    }
    if (!minutes) {
      return hours + U" बजे";
    }
    return hours + U" बजकर " + cardinal(minutes) + (next == U"मिनट" ? U"" : U" मिनट");
  }

  std::optional<std::u32string> read_plain(const Match& match) const override {
    // A year after सन or a month, or before में, का, तक: 1999 में -> उन्नीस सौ निन्यानवे में. Not 3200 से ज्यादा.
    const Number& number = match.numbers[0];
    bool dated = kYearsAfter.contains(match.context.before()) || kYearsBefore.contains(match.context.after());
    bool hundreds = value(number) >= 1100 && value(number) < 2000 && number.integer.size() == 4;
    std::u32string words = hundreds && number.fraction.empty() && !number.grouped && dated ? year(value(number)) :
                                                                                             read_number(number, match);

    // A number joined to a word gets spaces: टी20 -> टी बीस
    std::u32string_view head = match.context.head();
    std::u32string_view tail = match.context.tail();
    bool before = !head.empty() && is_devanagari(head.back());
    bool after = !tail.empty() && is_devanagari(tail.front());
    return (before ? U" " : U"") + words + (after ? U" " : U"");
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    // एक रुपया, पाँच रुपये, एक लाख रुपये; $50 अरब डॉलर: the currency is already named
    std::u32string currency = kCurrencies.at(std::u32string(match.unit));
    bool one = match.unit == U"₹" && number.integer == U"1" && match.scale.empty();

    // ₹9.50: नौ रुपये पचास पैसे; ₹5.00: पाँच रुपये. Three decimals or more stay a decimal: ₹2.123
    std::u32string_view fraction = number.fraction;
    fraction = fraction.substr(0, fraction.find_last_not_of(U'0') + 1);
    auto cents = kCents.find(std::u32string(match.unit));
    if (!number.fraction.empty() && fraction.size() <= 2 && match.scale.empty() && cents != kCents.end()) {
      uint64_t n = fraction.empty() ? 0 : parse_value(fraction).value_or(0) * (fraction.size() == 1 ? 10 : 1);
      std::u32string whole = cardinal(value(number)) + U" " + (one ? U"रुपया" : currency);
      std::u32string small = cardinal(n) + U" " + (n == 1 && match.unit == U"₹" ? U"पैसा" : cents->second);
      return value(number) == 0 && n ? small : whole + (n ? U" " + small : U"");
    }

    std::u32string words =
        read_number(number, match) + (match.scale.empty() ? U"" : U" " + std::u32string(match.scale));
    if (match.context.after() == currency || match.context.after() == U"रुपए") {
      return words;
    }
    return words + U" " + (one && number.fraction.empty() ? U"रुपया" : currency);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    // 1st: an ordinal, not a stone
    if (match.unit == U"st" && match.text.find(U' ') == std::u32string_view::npos) {
      return std::nullopt;
    }
    // 1.5 yr: डेढ़ साल
    if (auto half = and_a_half(number); half && match.unit == U"yr") {
      return *half + U" " + kUnits.at(U"yr");
    }
    return read_number(number, match) + U" " + kUnits.at(std::u32string(match.unit));
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return read_number(number, match) + U" प्रतिशत";
  }

  std::optional<std::u32string> read_joiner(const Match& match) const override {
    // 3^2: तीन स्क्वेर्ड, 2^3: दो क्यूब, 4^5: चार टु द पावर पाँच
    if (match.separator == U'^') {
      uint64_t n = value(match.numbers[0]);
      return n == 2 ? U" स्क्वेर्ड" : n == 3 ? U" क्यूब" : U" टु द पावर " + cardinal(n);
    }
    return PatternLanguage::read_joiner(match);
  }

  std::u32string read_range(const Number& from, const Number& to, const Match& match) const override {
    // 10-20-30, 1-800-555: a chain of numbers, 66-4: no range down, and 12-12: a tie; but 297-272 ई. पू. is one, and
    // 1920-26, with a shortened year
    if (match.kind == Kind::RANGE) {
      std::u32string_view head = match.context.head();
      std::u32string_view tail = match.context.tail();
      size_t a = from.integer.size(), b = to.integer.size();
      bool chain =
          (head.size() > a + 1 && head[head.size() - a - 1] == U'-' && unicode::isdigit(head[head.size() - a - 2])) ||
          (tail.size() > b + 1 && tail[b] == U'-' && unicode::isdigit(tail[b + 1]));
      bool down = value(to) < value(from) && b < a && !(a == 4 && b == 2);
      bool tied = value(to) == value(from); // 12-12 अंक: a tie, बारह-बारह
      if (chain || down || tied) {
        return U"-";
      }
    }
    return PatternLanguage::read_range(from, to, match);
  }

private:
  // Whether a number is read digit by digit: a leading zero, twelve digits or more, a mobile number (98765
  // 43210), a code after its word (पिन कोड 110023), or the postal code ending an address (रांची, झारखंड 736557)
  static bool is_spoken_digits(const Number& number, const Match& match) {
    const auto& i = number.integer;
    // 04 जून: a day before a month, despite its leading zero
    std::u32string next = match.context.after();
    bool day = i.size() == 2 && !next.empty() && std::ranges::find(kMonths, next) != kMonths.end();
    if (!day && numbers::is_spoken_digits(number, 12)) {
      return true;
    }
    if (match.kind != Kind::NUMBER || number.grouped || i.size() < 4) {
      return false;
    }
    bool mobile = i.size() == 10 && i[0] >= U'6';
    std::u32string before = match.context.before();
    bool code = kCodeCues.contains(before) ||
                (before == U"अंक" && (match.context.before(2) == U"आखिरी" || match.context.before(2) == U"अंतिम"));
    std::u32string_view tail = match.context.tail();
    bool last = std::ranges::all_of(tail, [](char32_t c) { return unicode::isspace(c) || !unicode::isalnum(c); });
    bool postal = (i.size() == 5 || i.size() == 6) && !i.ends_with(U"000") && last && !before.empty() &&
                  unicode::isalpha(before.back()) && !kCountCues.contains(before);
    return mobile || code || postal;
  }

  // The ordinal of a number by its suffix: 5वीं -> पाँचवीं, 2री -> दूसरी, 7th -> सेवंथ
  static std::u32string ordinal(uint64_t n, std::u32string_view suffix) {
    if (std::ranges::find(kEnglishOrdinalSuffixes, suffix) != kEnglishOrdinalSuffixes.end()) {
      std::u32string words = english_ordinal(n);
      return words.empty() ? cardinal(n) + kOrdinalEndings[0] : words;
    }
    // The suffix gives the form: masculine (वाँ, रा), feminine (वीं, री) or plural (वें, रे)
    size_t form = kOrdinalSuffixes.at(std::u32string(suffix));

    // पहला, दूसरा, तीसरा, चौथा, छठा: irregular
    if (auto it = kOrdinalsSpecial.find(static_cast<int32_t>(n)); n < 10 && it != kOrdinalsSpecial.end()) {
      return it->second[form];
    }
    // The regular ending as written (5वां, 5वाँ), or by the form of an irregular one
    bool regular = suffix.starts_with(U'व');
    return cardinal(n) + (regular ? std::u32string(suffix) : kOrdinalEndings[form]);
  }
};
} // namespace

const Language& language() {
  static const Hindi language;
  return language;
}

} // namespace verbalis::languages::hi
