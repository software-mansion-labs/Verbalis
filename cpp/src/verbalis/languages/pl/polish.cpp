#include "constants.h"

#include <verbalis/languages/language.h>
#include <verbalis/numbers/scanning.h>
#include <verbalis/text/context.h>
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

namespace verbalis::languages::pl {

namespace {
using namespace text;
using namespace numbers;

// Whether a number reads as a count or as an ordinal (years, days of a month, headings)
enum class Form { CARDINAL = 0, ORDINAL };

// Inflection of a number: its form, and the case and gender its words agree with
struct Inflection {
  Form form = Form::CARDINAL; // An ordinal for years, days of a month and headings
  Case grammatical_case = Case::NOMINATIVE;
  Gender gender = Gender::MASCULINE;
  bool plural = false; // lata 70., 18 urodziny: an ordinal in the plural
};

// What the words around a match say about reading its numbers: the inflection of its first number, and more
struct Cues : Inflection {
  Case amount_case = Case::NOMINATIVE; // The case of an amount (5 kg): of its number and its noun
  bool numbered = false;               // numerem 3x38: the "x" of a document number stands for a slash
};

size_t index(Case c) {
  return static_cast<size_t>(c);
}

bool starts_with_any(std::u32string_view word, const std::vector<std::u32string>& stems) {
  return std::ranges::any_of(stems, [word](const auto& stem) { return word.starts_with(stem); });
}

// ------------------------
// ----- Number words -----
// ------------------------

// The column of kCardinals: nominative and accusative, genitive and locative, dative, instrumental
size_t column(Case c) {
  switch (c) {
    case Case::NOMINATIVE:
    case Case::ACCUSATIVE:
      return 0;
    case Case::GENITIVE:
    case Case::LOCATIVE:
      return 1;
    case Case::DATIVE:
      return 2;
    default:
      return 3;
  }
}

// The form of a noun after a number: 1 / 2-4 / 5+ in the nominative, the plural of the case otherwise
const std::u32string& form(const Noun& noun, uint64_t n, Case c, bool fraction = false) {
  if (fraction) {
    return noun.singular[index(Case::GENITIVE)];
  }
  if (n == 1) {
    return noun.singular[index(c)];
  }
  if (c == Case::NOMINATIVE || c == Case::ACCUSATIVE) {
    bool few = n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14);
    return noun.plural[index(few ? Case::NOMINATIVE : Case::GENITIVE)];
  }
  return noun.plural[index(c)];
}

std::u32string below_thousand(uint64_t n, Case c, Gender g) {
  if (n == 0) {
    return std::u32string(kCardinals.at(0)[column(c)]);
  }

  uint64_t r = n % 100;
  std::vector<std::u32string> words;
  for (uint64_t part : {n - r, r < 20 ? r : r - r % 10, r < 20 ? 0 : r % 10}) {
    if (part == 1) {
      words.push_back(U"jeden"); // Inside a compound number "jeden" does not inflect
    } else if (part == 2 && g == Gender::FEMININE &&
               (c == Case::NOMINATIVE || c == Case::ACCUSATIVE || c == Case::INSTRUMENTAL)) {
      words.push_back(c == Case::INSTRUMENTAL ? U"dwiema" : U"dwie");
    } else if (part) {
      words.emplace_back(kCardinals.at(part)[column(c)]);
    }
  }
  return join(words);
}

std::u32string cardinal(uint64_t n, Case c = Case::NOMINATIVE, Gender g = Gender::MASCULINE) {
  if (n == 1) {
    return std::u32string(kOne[static_cast<size_t>(g)][index(c)]);
  }

  std::vector<std::u32string> parts;
  for (auto [scale, noun] : {std::pair{1000000000ULL, &kBillion}, {1000000ULL, &kMillion}, {1000ULL, &kThousand}}) {
    uint64_t q = n / scale;
    n %= scale;
    if (q) {
      parts.push_back(q == 1 ? form(*noun, q, c) : cardinal(q, c) + U" " + form(*noun, q, c));
    }
  }
  if (n || parts.empty()) {
    parts.push_back(below_thousand(n, c, g));
  }
  return join(parts);
}

// An ordinal stem with an adjective ending: drug + y -> drugi, trzec + a -> trzecia
std::u32string adjective(std::u32string_view stem, std::u32string_view ending) {
  std::u32string e(ending);
  if (stem.ends_with(U'g') || stem.ends_with(U'k')) {
    e = e[0] == U'y' ? U"i" + e.substr(1) : e[0] == U'e' ? U"i" + e : e;
  } else if (stem.ends_with(U'c')) {
    e = e[0] == U'y' ? U"i" + e.substr(1) : U"i" + e;
  }
  return std::u32string(stem) + e;
}

// The form of a number inside a compound: czterdziestopięcio(metrowe), stu(letni)
std::u32string combining(uint64_t n) {
  uint64_t h = n - n % 100, r = n % 100;
  if (h >= 1000) {
    return cardinal(n);
  }
  std::u32string words(h ? kCombining.at(h) : U"");
  if (r) {
    words += r < 20 || r % 10 == 0 ? std::u32string(kCombining.at(r)) :
                                     std::u32string(kCombining.at(r - r % 10)) + std::u32string(kCombining.at(r % 10));
  }
  return words;
}

// Only the last two parts inflect: dwa tysiące dwudziestym drugim, tysiąc osiemsetnym
std::u32string ordinal(uint64_t n, Case c = Case::NOMINATIVE, Gender g = Gender::MASCULINE, bool plural = false) {
  std::u32string_view e = kAdjectiveEndings[plural ? 3 : static_cast<size_t>(g)][index(c)];

  uint64_t r = n % 100;
  if (r) {
    std::vector<std::u32string> words = {n - r ? cardinal(n - r) : U""};
    if (r < 20 || r % 10 == 0) {
      words.push_back(adjective(kOrdinalStems.at(r), e));
    } else {
      words.push_back(adjective(kOrdinalStems.at(r - r % 10), e));
      words.push_back(adjective(kOrdinalStems.at(r % 10), e));
    }
    return join(words);
  }

  uint64_t h = n % 1000;
  if (h) {
    std::vector<std::u32string> words = {n - h ? cardinal(n - h) : U"", adjective(kOrdinalStems.at(h), e)};
    return join(words);
  }

  uint64_t t = n / 1000;
  return adjective((t == 1 ? U"" : combining(t)) + U"tysięczn", e);
}

std::u32string digits(std::u32string_view text) {
  std::vector<std::u32string> words;
  for (char32_t c : text) {
    words.emplace_back(c == U'1' ? U"jeden" : kCardinals.at(c - U'0')[0]);
  }
  return join(words);
}

// -------------------
// ----- Context -----
// -------------------

// A noun's case, gender and number, guessed from its ending (as seen after an ordinal): sesji, sesję, roku
struct NounForm {
  Case grammatical_case = Case::NOMINATIVE;
  Gender gender = Gender::MASCULINE;
  bool plural = false;
  bool plural_only = false; // urodziny: never a form a count takes
};

// Whether a word is made only of given characters (and is not empty).
bool made_of(std::u32string_view word, std::u32string_view characters) {
  return !word.empty() && std::ranges::all_of(word, [&](char32_t c) { return strings::contains(characters, c); });
}

constexpr std::u32string_view kDigits = U"0123456789";

const std::unordered_set<std::u32string>& genitive_plurals() {
  static const std::unordered_set<std::u32string> forms = [] {
    std::unordered_set<std::u32string> result;
    for (const auto* table : {&kUnits, &kCurrencies, &kScales}) {
      for (const auto& [key, noun] : *table) {
        // Not one which is also a nominative (procent, euro): trzydzieści cztery procent
        if (noun.plural[index(Case::GENITIVE)] != noun.plural[index(Case::NOMINATIVE)]) {
          result.insert(noun.plural[index(Case::GENITIVE)]);
        }
      }
    }
    return result;
  }();
  return forms;
}

// A noun that a number counts: osób, metrów, złotych (as opposed to a verb, or nothing after a year)
bool counted(std::u32string_view word) {
  std::u32string w(word);
  return kCountNouns.contains(w) || genitive_plurals().contains(w) || word.ends_with(U"ów") || word.ends_with(U"ych") ||
         word.ends_with(U"ich");
}

/**
 * The words before a number within its clause (lowercased), and the first word after it, skipping a range
 * or coordination it belongs to (20-30, 3 a 368) and the złote of an amount of grosze (39 532 złotych 20 groszy).
 */
std::pair<std::vector<std::u32string>, std::u32string> surroundings(const Context& context) {
  std::u32string_view head = context.head();
  size_t clause = 0;
  for (size_t i = 0; i + 1 < head.size(); ++i) {
    if (strings::contains(std::u32string_view(U",;:.!?"), head[i]) && unicode::isspace(head[i + 1])) {
      clause = i + 1;
    }
  }

  std::vector<std::u32string> before;
  for (auto word : words_of(head.substr(clause))) {
    before.push_back(strings::to_lower(stripped(word)));
  }
  auto range_start = [](std::u32string_view w) {
    return w.size() > 1 && unicode::isdigit(w[0]) && w.back() == U'-' &&
           made_of(w.substr(0, w.size() - 1), U"0123456789,. ");
  };
  while (!before.empty() && range_start(before.back())) {
    before.pop_back();
  }
  while (before.size() >= 2 && kConjunctions.contains(before.back()) &&
         made_of(before[before.size() - 2], U"0123456789,.")) {
    before.resize(before.size() - 2);
  }
  if (!before.empty() && kSigns.contains(before.back())) {
    before.pop_back(); // do minus siedmiu stopni, od +12°C
  }
  // do nawet siedmiuset, przez około cztery: the preposition before a particle governs the number
  if (before.size() >= 2 && kParticles.contains(before.back()) &&
      kPrepositionCases.contains(before[before.size() - 2])) {
    before.pop_back();
  }
  if (!before.empty() && kCurrencyWords.contains(before.back())) {
    before.pop_back();
    while (!before.empty() && made_of(before.back(), kDigits)) {
      before.pop_back();
    }
  }

  // The first word after, beyond the end of a range (-30)
  std::u32string next;
  std::u32string_view tail = context.tail();
  if (tail.empty() || !strings::contains(text::kPunctuation, tail[0])) {
    auto after = words_of(tail);
    for (size_t i = 0; i < after.size(); ++i) {
      std::u32string_view w = stripped(after[i]);
      if (!(w.size() > 1 && w[0] == U'-' && made_of(w.substr(1), U"0123456789,."))) {
        next = strings::to_lower(w);
        break;
      }
    }
  }
  return {before, next};
}

// The context of the number this one continues after a comma (140 g mąki, 130 g mąki): both share a case.
std::optional<Context> enumerated(const Context& context) {
  std::u32string_view head = context.head();
  size_t end = head.find_last_not_of(U" \t\n\r");
  if (end == std::u32string_view::npos) {
    return std::nullopt;
  }
  // spośród 510 ocen i 68 recenzji: a conjunction, as a comma
  if (std::u32string w = context.before(); kListConjunctions.contains(w) && unicode::isalpha(head[end])) {
    end = end + 1 - w.size();
  } else if (head[end] != U',') {
    return std::nullopt;
  }

  // The last number before the comma, at most four words before it
  std::u32string_view text = head.substr(0, end);
  std::optional<Span> number;
  for (size_t i = 0; i < text.size();) {
    if (unicode::isdigit(text[i]) && is_clear_before(text, i, U".,")) {
      number = Span{i, digits_end(text, i)};
      i = number->end;
    } else {
      i++;
    }
  }
  if (!number || words_of(text.substr(number->end)).size() > 4) {
    return std::nullopt;
  }
  // A score or a range, which reads "6 do 0" by now, ends in the genitive of its "do", which the next one doesn't
  // share: 6:0, 7:6 (sześć do zera, siedem do sześciu)
  Context earlier(text, *number);
  if (earlier.before() == U"do" && made_of(earlier.before(2), kDigits)) {
    return std::nullopt;
  }
  return earlier;
}

// The case of a number governed by an amount noun, a genitive verb or a negation right before it.
std::optional<Case> governed(const std::vector<std::u32string>& before) {
  std::u32string w1 = before.empty() ? U"" : before.back();
  for (const auto& stem : kAmountNouns) {
    if (w1.starts_with(stem) && kNounEndings.contains(std::u32string(w1.substr(stem.size())))) {
      return Case::GENITIVE; // od kwoty trzystu tysięcy
    }
  }

  std::vector<std::u32string> window(before.end() - static_cast<std::ptrdiff_t>(std::min<size_t>(before.size(), 3)),
                                     before.end());
  std::vector<std::u32string> verbs;
  std::ranges::copy_if(window, std::back_inserter(verbs), [](const auto& w) { return !kClitics.contains(w); });
  if (!verbs.empty() && starts_with_any(verbs.back(), kGenitiveVerbs)) {
    return Case::GENITIVE; // potrzebowali stu czterdziestu gramów, brakuje nam trzech osób
  }

  // nie powinno przekraczać pięciu gramów, but not: nie jest to pięć, nie więcej niż pięć
  auto no = std::ranges::find(window, U"nie");
  if (no != window.end() && std::none_of(no + 1, window.end(), [](const auto& w) {
        return kCopulas.contains(w) || w == U"niż" || w == U"tylko";
      })) {
    return Case::GENITIVE;
  }
  return std::nullopt;
}

// The forms of the nouns of kOrdinalNouns: each form with its possible case, gender, and number
const std::unordered_map<std::u32string, std::vector<NounForm>>& ordinal_nouns() {
  static const auto forms = [] {
    std::unordered_map<std::u32string, std::vector<NounForm>> result;
    for (const auto& [gender, list] : kOrdinalNouns) {
      Noun n = noun(list);
      bool plural_only = n.singular == n.plural; // urodziny
      for (size_t c = 0; c < 6; ++c) {
        if (!plural_only) {
          result[n.singular[c]].push_back({static_cast<Case>(c), gender, false});
        }
        result[n.plural[c]].push_back({static_cast<Case>(c), gender, true, plural_only});
      }
    }
    return result;
  }();
  return forms;
}

// The cases a preposition gives a noun, in order of likelihood: w rundzie (locative), z miejsca (genitive)
std::vector<Case> preposition_cases(std::u32string_view preposition) {
  std::u32string p(preposition);
  if (kLocativePrepositions.contains(p)) {
    return {Case::LOCATIVE, Case::ACCUSATIVE};
  }
  if (p == U"przed" || p == U"nad" || p == U"pod" || p == U"między" || p == U"pomiędzy" || p == U"za") {
    return {Case::INSTRUMENTAL, Case::ACCUSATIVE};
  }
  if (auto it = kPrepositionCases.find(p); it != kPrepositionCases.end()) {
    return {it->second == Case::INSTRUMENTAL ? Case::GENITIVE : it->second, it->second}; // z miejsca, ze stron
  }
  return {Case::NOMINATIVE, Case::ACCUSATIVE, Case::GENITIVE, Case::LOCATIVE, Case::DATIVE, Case::INSTRUMENTAL};
}

NounForm noun_form(std::u32string_view word, std::u32string_view preposition) {
  // A known noun: the singular form its preposition makes likeliest, then the plural (w ramach fazy, not fazy)
  if (auto it = ordinal_nouns().find(std::u32string(word)); it != ordinal_nouns().end()) {
    for (bool plural : {false, true}) {
      for (Case c : preposition_cases(preposition)) {
        for (const NounForm& form : it->second) {
          if (form.grammatical_case == c && form.plural == plural) {
            return form;
          }
        }
      }
    }
    return it->second.front();
  }

  bool locative = kLocativePrepositions.contains(std::u32string(preposition));
  bool instrumental = preposition == U"z" || preposition == U"ze" || preposition == U"przed" || preposition == U"nad" ||
                      preposition == U"pod";

  const std::vector<std::pair<std::u32string_view, NounForm>> endings = {
      {U"ami", {Case::INSTRUMENTAL, Gender::MASCULINE, true}},
      {U"ach", {Case::LOCATIVE, Gender::MASCULINE, true}},
      {U"ych", {Case::GENITIVE, Gender::MASCULINE, true}},
      {U"ów", {Case::GENITIVE, Gender::MASCULINE, true}},
      {U"iem", {Case::INSTRUMENTAL, Gender::MASCULINE}},
      {U"em", {Case::INSTRUMENTAL, Gender::MASCULINE}},
      {U"ym", {locative ? Case::LOCATIVE : Case::INSTRUMENTAL, Gender::MASCULINE}},
      {U"ą", {instrumental ? Case::INSTRUMENTAL : Case::ACCUSATIVE, Gender::FEMININE}},
      {U"ę", {Case::ACCUSATIVE, Gender::FEMININE}},
      {U"a", {Case::NOMINATIVE, Gender::FEMININE}},
      {U"i", {locative ? Case::LOCATIVE : Case::GENITIVE, Gender::FEMININE}},
      {U"y", {locative ? Case::LOCATIVE : Case::GENITIVE, Gender::FEMININE}},
      {U"u", {locative ? Case::LOCATIVE : Case::GENITIVE, Gender::MASCULINE}},
      {U"ie", {Case::LOCATIVE, Gender::MASCULINE}},
      {U"e", {Case::NOMINATIVE, Gender::NEUTER}},
      {U"o", {Case::NOMINATIVE, Gender::NEUTER}}};
  for (const auto& [ending, form] : endings) {
    if (word.ends_with(ending)) {
      return form;
    }
  }
  return {};
}

// The form, case and gender of a number from its context: an ordinal for years, days and headings.
Inflection inflection_of(const Context& context, uint64_t n, bool decimal, bool year_abbreviation = false) {
  auto [before, next] = surroundings(context);
  std::u32string w1 = before.empty() ? U"" : before.back();
  std::u32string w2 = before.size() < 2 ? U"" : before[before.size() - 2];
  std::u32string w3 = before.size() < 3 ? U"" : before[before.size() - 3];
  std::u32string noun = year_abbreviation ? U"roku" : next; // 2020 r.

  // do lat 3, od lat 5: an age in the case of the preposition before "lat" (but z lat 1991-2020 are years)
  if (w1 == U"lat" && kPrepositionCases.contains(w2) && n < 100) {
    return {Form::CARDINAL, kPrepositionCases.at(w2)};
  }

  // 1990-1994: the end of a range of years stays a cardinal, as its start (but od 2020 do 2022 reads ordinals)
  auto year = [](std::u32string_view w) { return w.size() == 4 && made_of(w, kDigits); };
  if (w1 == U"do" && year(w2) && is_year_value(n) && w3 != U"od" && noun != U"roku" && noun != U"r") {
    return {Form::CARDINAL, Case::NOMINATIVE};
  }

  // 1. Years, days and headings are ordinals
  bool locative = kLocativePrepositions.contains(w1);
  if ((noun == U"roku" || noun == U"rok" || noun == U"rokiem") && !decimal) {
    Case c = noun == U"rok"                                             ? Case::NOMINATIVE :
             noun == U"rokiem" || (year_abbreviation && w1 == U"przed") ? Case::INSTRUMENTAL :
             locative                                                   ? Case::LOCATIVE :
                                                                          Case::GENITIVE;
    return {Form::ORDINAL, c};
  }
  if (w1 == U"dnia") {
    return {Form::ORDINAL, Case::GENITIVE}; // z dnia 1.
  }
  if (auto hour = kHourWords.find(w1); hour != kHourWords.end() && n <= 24 && !decimal && !counted(noun)) {
    return {Form::ORDINAL, hour->second, Gender::FEMININE}; // o godzinie piętnastej
  }
  // w (samym) roku 2016; but roku 76 pracowników counts
  if ((w1 == U"roku" || w1 == U"rok") && !counted(noun)) {
    bool w2_locative =
        kLocativePrepositions.contains(w2) || (w2.ends_with(U"ym") && kLocativePrepositions.contains(w3) && n >= 1000);
    return {Form::ORDINAL, w1 == U"rok" ? Case::NOMINATIVE : w2_locative ? Case::LOCATIVE : Case::GENITIVE};
  }
  // od pierwszego do piątego lipca: the first day of a range, before "do" and a day of a month
  auto after = words_of(context.tail());
  bool month_after_range =
      noun == U"do" && after.size() >= 3 && made_of(stripped(after[1]), kDigits) &&
      std::ranges::find(kMonthsGenitive, strings::to_lower(stripped(after[2]))) != kMonthsGenitive.end();
  if ((std::ranges::find(kMonthsGenitive, noun) != kMonthsGenitive.end() || month_after_range) && n >= 1 && n <= 31) {
    // od pierwszego kwietnia, po pierwszym maja, przed dwudziestym czwartym grudnia, na szóstego grudnia, był
    // dwudziestego października; 11 listopada to Święto: a date opening its sentence before a copula is its subject
    bool subject = before.empty() && after.size() >= 2 && kCopulas.contains(strings::to_lower(stripped(after[1])));
    Case c = subject        ? Case::NOMINATIVE :
             w1 == U"po"    ? Case::LOCATIVE :
             w1 == U"przed" ? Case::INSTRUMENTAL :
                              Case::GENITIVE;
    return {Form::ORDINAL, c};
  }
  if (kMonthWords.contains(w1) && is_year_value(n)) {
    return {Form::ORDINAL, Case::GENITIVE};
  }
  // października 18, 2017: a day after its month, and the year after them
  auto written = words_of(context.head());
  auto month_at = [&](size_t back) {
    return written.size() >= back &&
           std::ranges::find(kMonthsGenitive, strings::to_lower(stripped(written[written.size() - back]))) !=
               kMonthsGenitive.end();
  };
  bool day_after_month = month_at(1) && n >= 1 && n <= 31 && context.tail().starts_with(U',');
  bool year_after_day =
      context.head().ends_with(U", ") && is_year_value(n) && month_at(2) && made_of(stripped(written.back()), kDigits);
  if (day_after_month || year_after_day) {
    return {Form::ORDINAL, Case::GENITIVE};
  }
  if ((w1 == U"w" || w1 == U"we") && is_year_value(n) && !genitive_plurals().contains(noun)) {
    return {Form::ORDINAL, Case::LOCATIVE};
  }
  if (auto it = kYearPrepositions.find(w1); it != kYearPrepositions.end() && is_year_value(n, 1900) && !counted(noun)) {
    return {Form::ORDINAL, it->second}; // od dwa tysiące dwudziestego do dwa tysiące dwudziestego drugiego
  }
  if (starts_with_any(w1, kOrdinalHeadings) && n < 100 && !counted(noun)) {
    NounForm heading = noun_form(w1, w2);
    return {Form::ORDINAL, heading.grammatical_case, heading.gender}; // w punkcie drugim, część druga
  }
  // w 4 lidze, na 6 kroku, 22 urodziny: before a form of a noun no count takes (a singular, or a noun with no
  // singular), a number is an ordinal agreeing with it; but nr 4 turnieju is a label
  if (auto it = ordinal_nouns().find(noun);
      it != ordinal_nouns().end() && n >= 2 && !decimal && !w1.starts_with(kNumberStem) && w1 != kNumberAbbreviation &&
      std::ranges::none_of(it->second, [](const NounForm& form) { return form.plural && !form.plural_only; })) {
    NounForm form = noun_form(noun, w1);
    return {Form::ORDINAL, form.grammatical_case, form.gender, form.plural};
  }

  // 2. The case of a cardinal: from a preposition, a governing word or a number it continues
  Inflection result;
  if ((w1 == U"pod" || w1 == U"na") && kCallVerbs.contains(w2)) {
    result.grammatical_case = Case::NOMINATIVE; // zadzwoń pod sto dwanaście: a number to call
  } else if (auto it = kPrepositionCases.find(w1); it != kPrepositionCases.end()) {
    result.grammatical_case = it->second;
  } else if (auto complex = kComplexPrepositions.find(w1);
             complex != kComplexPrepositions.end() && complex->second == w2) {
    result.grammatical_case = Case::GENITIVE;
  } else if (auto governing = decimal || kCopulas.contains(noun) ? std::nullopt : governed(before)) {
    result.grammatical_case = *governing;
  } else if (auto earlier = enumerated(context)) {
    result.grammatical_case = inflection_of(*earlier, 0, false).grammatical_case;
  }

  // 3. The noun after it, as it agrees with the number, or with the end of the range it starts: 4-5 godzin agrees
  // with pięć, so cztery-pięć godzin
  std::u32string_view tail = context.tail();
  if (tail.size() > 1 && tail[0] == U'-' && unicode::isdigit(tail[1])) {
    n = parse_value(tail.substr(1, digits_end(tail, 1) - 1)).value_or(n);
  }
  bool few = n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14);
  bool nominative = result.grammatical_case == Case::NOMINATIVE || result.grammatical_case == Case::ACCUSATIVE;
  if ((noun.ends_with(U"ami") || noun.ends_with(U"mi")) && noun.size() > 4) {
    result.grammatical_case = Case::INSTRUMENTAL;
  } else if (noun.ends_with(U"ach") && noun.size() > 4) {
    result.grammatical_case = Case::LOCATIVE;
  } else if (noun.ends_with(U"om") && noun.size() > 4 && !decimal) {
    result.grammatical_case = Case::DATIVE; // pomogliśmy czterem rodzinom
  } else if (nominative && n >= 2 && n % 1000 != 0 && kVirileNouns.contains(noun)) {
    result.grammatical_case = Case::GENITIVE; // piętnastu pracowników, but trzydzieści osiem milionów mieszkańców
  } else if (nominative && few && counted(noun) && !kSameInBothCases.contains(noun)) {
    result.grammatical_case = Case::GENITIVE; // trzech osób: the noun is genitive, so the number is too
  }

  // jedna godzina, jednej godziny, but jednego stopnia (a masculine genitive in -a)
  bool nominative_feminine = noun.ends_with(U'a') && result.grammatical_case == Case::NOMINATIVE;
  bool genitive_feminine = (noun.ends_with(U'y') || noun.ends_with(U'i')) && result.grammatical_case == Case::GENITIVE;
  // na 2 równe części: the noun after an adjective
  bool adjective = noun.ends_with(U'e') || noun.ends_with(U"ych") || noun.ends_with(U"ich");
  std::u32string head_noun = adjective && after.size() >= 2 ? strings::to_lower(stripped(after[1])) : noun;
  if (((nominative_feminine || genitive_feminine) && n == 1) || starts_with_any(head_noun, kFemininePlurals)) {
    result.gender = Gender::FEMININE;
  } else if (noun == U"euro") {
    result.gender = Gender::NEUTER;
  }
  return result;
}

// The word governing a dot ordinal: the one before it, or before the first ordinal of "lat 50. i 60."
std::u32string ordinal_cue(const Context& context) {
  std::u32string w1 = context.before();
  auto head = words_of(context.head());
  if (kListConjunctions.contains(w1) && head.size() > 1) {
    std::u32string_view previous = head[head.size() - 2];
    if (previous.ends_with(U'.') && made_of(previous.substr(0, previous.size() - 1), kDigits)) {
      return context.before(3);
    }
  }
  return w1;
}

// The gender of a counted noun, for the number before it: dwie tony, dwa metry
Gender gender_of(const Noun& noun) {
  const auto& nominative = noun.singular[index(Case::NOMINATIVE)];
  return nominative.ends_with(U'a') ? Gender::FEMININE : nominative == U"euro" ? Gender::NEUTER : Gender::MASCULINE;
}

// A decimal number in a case: dwa przecinek pięć, półtora, dwa i pół
std::u32string decimal(const Number& number, Case c) {
  // 801.04, 4.0: a dotted code rather than a decimal (but 0.05 is a decimal)
  if (number.decimal_mark == U'.' && number.integer != U"0" &&
      (number.fraction.size() == 2 || number.fraction == U"0")) {
    uint64_t f = parse_value(number.fraction).value_or(0);
    return cardinal(value(number)) + U" " + (number.fraction[0] == U'0' ? digits(number.fraction) : cardinal(f));
  }

  bool zero = !number.integer.empty(); // .5 has no zero before its point
  if (number.integer == U"1" && number.fraction == U"5") {
    return U"półtora";
  }
  std::u32string head = zero ? cardinal(value(number), c) + U" " : U"";
  if (number.fraction == U"5" && value(number) > 0) {
    return head + U"i pół"; // dwa i pół, but zero przecinek pięć
  }
  if (number.fraction.size() > 3) {
    return head + U"przecinek " + digits(number.fraction); // dwa przecinek siedem jeden osiem dwa osiem
  }

  std::vector<std::u32string> words = {U"przecinek"};
  size_t zeros = number.fraction.find_first_not_of(U'0');
  words.insert(words.end(), std::min(zeros, number.fraction.size()), U"zero");
  if (zeros != std::u32string::npos) {
    words.push_back(cardinal(parse_value(number.fraction).value_or(0), c));
  }
  return head + join(words);
}

// A number counting a noun, both in a case: pięćset dwadzieścia cztery kilometry
std::u32string amount(const Number& number, const Noun& noun, Case c) {
  std::u32string words;
  if (!number.fraction.empty()) {
    words = decimal(number, c);
  } else {
    auto n = parse_value(number.integer);
    words = !n || is_spoken_digits(number, 7) ? digits(number.integer) : cardinal(*n, c, gender_of(noun));
  }
  return words + U" " + form(noun, value(number), c, !number.fraction.empty());
}

bool is_zloty(std::u32string_view currency) {
  return currency == U"zł" || currency == U"PLN";
}

Cues cues_of(const Match& match) {
  const Context& context = match.context;
  Cues cues;

  switch (match.kind) {
    case Kind::DATE:
      cues.grammatical_case = kCopulas.contains(context.before()) ? Case::NOMINATIVE : Case::GENITIVE;
      return cues;

    case Kind::TIME: {
      // o osiemnastej trzydzieści: the hour is a feminine ordinal (godzina) in the case of the word before, or of the
      // word before the first time of "między 10:00 a 12:00"
      std::u32string w1 = context.before();
      std::u32string w2 = context.before(2);
      if (kConjunctions.contains(w1) && !w2.empty() && unicode::isdigit(w2.back()) &&
          (strings::contains(w2, U':') || strings::contains(w2, U'.'))) {
        w1 = context.before(3);
      }
      auto hour = kHourWords.find(w1);
      cues.grammatical_case = hour != kHourWords.end()                                      ? hour->second :
                              w1 == U"o" || w1 == U"po"                                     ? Case::LOCATIVE :
                              w1 == U"do" || w1 == U"od" || w1 == U"około" || w1 == U"koło" ? Case::GENITIVE :
                              w1 == U"przed" || w1 == U"między" || w1 == U"pomiędzy"        ? Case::INSTRUMENTAL :
                              w1 == U"na" || w1 == U"za"                                    ? Case::ACCUSATIVE :
                                                                                              Case::NOMINATIVE;
      return cues;
    }

    case Kind::ORDINAL: {
      // "29. sesji": an ordinal agreeing with the noun after it, the decade of "lat 70.", or a heading
      cues.form = Form::ORDINAL;
      std::u32string w1 = ordinal_cue(context);
      if (auto it = kDecadeWords.find(w1); it != kDecadeWords.end()) {
        cues.grammatical_case = it->second;
        cues.plural = true;
      } else if (starts_with_any(w1, kHeadings) && value(match.numbers[0]) < 100) {
        cues.grammatical_case = noun_form(w1, context.before(2)).grammatical_case; // Punkt 3., w punkcie 5.
      } else {
        // 21. najbardziej zakorkowanym: the word after an adverb of degree
        auto words = words_of(context.tail());
        size_t i = !words.empty() && kDegreeAdverbs.contains(strings::to_lower(stripped(words[0]))) ? 1 : 0;
        std::u32string word = words.size() > i ? strings::to_lower(stripped(words[i])) : U"";
        NounForm next = noun_form(word, w1);
        // został 11. kierowcą: an instrumental after a form of "zostać" or "być", in the gender of the verb
        std::u32string verb = kCopulas.contains(w1) || w1.starts_with(kBecomeStem) ? w1 : context.before(2);
        if (word.ends_with(U'ą') && (kCopulas.contains(verb) || verb.starts_with(kBecomeStem))) {
          next.grammatical_case = Case::INSTRUMENTAL;
          next.gender = verb.ends_with(U"ła") ? Gender::FEMININE :
                        verb.ends_with(U"ło") ? Gender::NEUTER :
                        verb.ends_with(U'ł')  ? Gender::MASCULINE :
                                                next.gender;
        }
        cues.grammatical_case = next.grammatical_case;
        cues.gender = next.gender;
        cues.plural = next.plural;
      }
      return cues;
    }

    case Kind::DIMENSION: {
      // numerem 3x38: a document number, where "x" stands for a slash
      std::u32string_view head = context.head();
      size_t digits = head.size();
      while (digits > 0 && unicode::isdigit(head[digits - 1])) {
        digits--;
      }
      size_t word_end = digits;
      while (word_end > 0 && unicode::isspace(head[word_end - 1])) {
        word_end--;
      }
      size_t word_begin = word_end;
      while (word_begin > 0 && unicode::isword(head[word_begin - 1])) {
        word_begin--;
      }
      std::u32string_view word = head.substr(word_begin, word_end - word_begin);
      cues.numbered =
          digits < head.size() && word_end < digits && (word.starts_with(kNumberStem) || word == kNumberAbbreviation);
      return cues;
    }

    default:
      break;
  }

  // A number: its own inflection (the start of a range for amounts), and the case of what it counts
  if (!match.numbers.empty()) {
    const Number& first = match.numbers.front();
    bool year_abbreviation = match.suffix == U"r.";
    static_cast<Inflection&>(cues) = first.fraction.empty() ?
                                         inflection_of(context, value(first), false, year_abbreviation) :
                                         inflection_of(context, 0, true, year_abbreviation);

    const Number& last = match.numbers.back();
    bool grosze = match.kind == Kind::MONEY && match.scale.empty() && last.fraction.size() == 2 && is_zloty(match.unit);
    cues.amount_case = inflection_of(context, 0, !last.fraction.empty() && !grosze).grammatical_case;

    // w 10 m³, w 100 proc.: in an amount, locative; but w 10 min (within ten minutes) stays accusative
    std::u32string w1 = context.before();
    bool measure = match.kind == Kind::PERCENT || (match.kind == Kind::UNIT && !kTimeUnits.contains(match.unit));
    if ((w1 == U"w" || w1 == U"we") && measure && cues.amount_case == Case::ACCUSATIVE) {
      cues.amount_case = Case::LOCATIVE;
      // w 10-15 proc.: w dziesięciu do piętnastu procent, the start of the range too
      if (match.numbers.size() == 2 && cues.grammatical_case == Case::ACCUSATIVE) {
        cues.grammatical_case = Case::LOCATIVE;
      }
    }
  }
  return cues;
}

// lat 80.-90., latach 20. - 30.: a range of decades after a word for years. Its numbers are both decades.
class DecadeRangePattern : public Pattern {
public:
  DecadeRangePattern() : Pattern(Kind::ORDINAL) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,-") ||
        !kDecadeWords.contains(strings::to_lower(Context(text, {position, position}).before()))) {
      return std::nullopt;
    }
    size_t first = digits_end(text, position);
    if (first != position + 2 || first >= text.size() || text[first] != U'.') {
      return std::nullopt;
    }
    size_t dash = skip_space(text, first + 1);
    if (dash >= text.size() || (text[dash] != U'-' && text[dash] != U'–')) {
      return std::nullopt;
    }
    size_t second = skip_space(text, dash + 1), end = digits_end(text, second);
    if (end != second + 2 || end >= text.size() || text[end] != U'.') {
      return std::nullopt;
    }
    Number a = digits_at(text, position), b = digits_at(text, second);
    if (value(a) % 10 || value(b) % 10 || value(a) == 0 || value(b) == 0) {
      return std::nullopt;
    }
    // A dot closing both the decade and the sentence stays with the sentence; a century after it continues the
    // sentence (lat 80.-90. XX wieku)
    size_t next = skip_space(text, end + 1), roman = next;
    while (roman < text.size() && strings::contains(std::u32string_view(U"IVXLCDM"), text[roman])) {
      roman++;
    }
    bool century = roman > next && is_clear_after(text, roman);
    size_t span_end = is_sentence_end(text, end + 1, true) && !century ? end : end + 1;
    return Match{.kind = Kind::ORDINAL, .span = {position, span_end}, .numbers = {a, b}, .suffix = U"."};
  }
};

// 1/2 kg, 3/4 l: a fraction of a unit, read with the unit's genitive (pół kilograma, trzy czwarte litra)
class FractionUnitPattern : public Pattern {
public:
  FractionUnitPattern() : Pattern(Kind::FRACTION), units_(keys_of(kUnits)) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t slash = digits_end(text, position);
    if (!is_clear_before(text, position, U".,/") || slash == position || slash >= text.size() || text[slash] != U'/') {
      return std::nullopt;
    }
    size_t end = digits_end(text, slash + 1);
    if (end == slash + 1) {
      return std::nullopt;
    }
    size_t unit = skip_space(text, end);
    for (auto key : keys_at(text, unit, units_)) {
      if (is_clear_after(text, unit + key.size())) {
        return Match{.kind = Kind::FRACTION,
                     .span = {position, unit + key.size()},
                     .numbers = {digits_at(text, position), digits_at(text, slash + 1)},
                     .unit = key,
                     .separator = U'/'};
      }
    }
    return std::nullopt;
  }

private:
  Keys units_;
};

// 04.02., 23.07-22.08, (2.01), 29.06: a day and a month of two digits (the day of one before a padded month),
// without a year: before a closing dot, as a range of two such dates (its numbers are then both days and months), in
// parentheses, or with a two-digit day and a zero-padded month
class YearlessDatePattern : public Pattern {
public:
  YearlessDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    if (!is_clear_before(text, position, U".,-/")) {
      return std::nullopt;
    }
    auto date_at = [&](size_t at) -> std::optional<std::pair<Number, Number>> {
      size_t dot = digits_end(text, at);
      if (dot == at || dot > at + 2 || dot + 3 > text.size() || text[dot] != U'.' ||
          digits_end(text, dot + 1) != dot + 3) {
        return std::nullopt;
      }
      Number day = digits_at(text, at), month = digits_at(text, dot + 1);
      bool padded = day.integer.size() == 2 || month.integer[0] == U'0'; // 4.02, but not the version 1.10
      if (value(day) < 1 || value(day) > 31 || value(month) < 1 || value(month) > 12 || !padded) {
        return std::nullopt;
      }
      return std::pair{day, month};
    };
    auto first = date_at(position);
    if (!first) {
      return std::nullopt;
    }
    size_t end = first->second.span.end;
    // 23.07-22.08: a range
    if (end < text.size() && text[end] == U'-') {
      if (auto second = date_at(end + 1); second && is_clear_after(text, second->second.span.end) &&
                                          !is_digit_after(text, second->second.span.end, U".")) {
        return Match{.kind = Kind::DATE,
                     .span = {position, second->second.span.end},
                     .numbers = {first->first, first->second, second->first, second->second},
                     .separator = U'-'};
      }
      return std::nullopt;
    }
    // 04.02.: a closing dot, not followed by a year
    if (end < text.size() && text[end] == U'.' && !is_digit_after(text, end + 1) && is_clear_after(text, end + 1)) {
      size_t span_end = is_sentence_end(text, end + 1, true) ? end : end + 1;
      return Match{.kind = Kind::DATE,
                   .span = {position, span_end},
                   .numbers = {first->first, first->second},
                   .separator = U'.'};
    }
    // (21.11), 29.06
    bool parenthesized = position > 0 && text[position - 1] == U'(' && end < text.size() && text[end] == U')';
    bool padded = first->first.integer.size() == 2 && first->second.integer[0] == U'0';
    if ((parenthesized || padded) && is_clear_after(text, end) && !is_digit_after(text, end, U".,")) {
      return Match{
          .kind = Kind::DATE, .span = {position, end}, .numbers = {first->first, first->second}, .separator = U'.'};
    }
    return std::nullopt;
  }
};

// Jean18 listopada, 17stycznia, 24maja 2018: a day glued to the letters before it or to its month, the month in the
// genitive, and the year if written
class GluedDatePattern : public Pattern {
public:
  GluedDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t day_end = digits_end(text, position);
    if (day_end == position || day_end > position + 2 || (position > 0 && unicode::isdigit(text[position - 1]))) {
      return std::nullopt;
    }
    size_t month = skip_space(text, day_end);
    bool glued = (position > 0 && unicode::isalpha(text[position - 1])) || month == day_end;
    auto name = std::ranges::find_if(kMonthsGenitive, [&](auto m) {
      return text.substr(month).starts_with(m) && is_clear_after(text, month + m.size());
    });
    Number day = digits_at(text, position);
    if (!glued || name == kMonthsGenitive.end() || value(day) < 1 || value(day) > 31) {
      return std::nullopt;
    }
    size_t month_end = month + name->size();
    Match date{.kind = Kind::DATE,
               .span = {position, month_end},
               .numbers = {day, {.integer = to_digits(static_cast<uint64_t>(name - kMonthsGenitive.begin()) + 1)}},
               .separator = U' '};
    if (size_t year = skip_space(text, month_end);
        year > month_end && digits_end(text, year) == year + 4 && is_clear_after(text, year + 4)) {
      date.numbers.push_back(digits_at(text, year));
      date.span.end = year + 4;
    }
    return date;
  }
};

// 5.VI, 5.VI.2024: a date with its month in Roman numerals
class RomanDatePattern : public Pattern {
public:
  RomanDatePattern() : Pattern(Kind::DATE) {}

  std::optional<Match> match(std::u32string_view text, size_t position) const override {
    size_t day_end = digits_end(text, position);
    if (!is_clear_before(text, position, U".,") || day_end == position || day_end > position + 2 ||
        day_end >= text.size() || (text[day_end] != U'.' && text[day_end] != U' ')) {
      return std::nullopt;
    }
    size_t month = day_end + 1; // 5.VI, or 15 X
    size_t month_end = month;
    while (month_end < text.size() && strings::contains(std::u32string_view(U"IVX"), text[month_end])) {
      month_end++;
    }
    auto month_value = roman_value(text.substr(month, month_end - month));
    uint64_t day = value(digits_at(text, position));
    if (!month_value || *month_value > 12 || day < 1 || day > 31) {
      return std::nullopt;
    }
    // 15 X: after a space, not V or I alone (12 V is volts), nor an edition (15 X wyd.)
    std::u32string_view numeral = text.substr(month, month_end - month);
    if (text[day_end] == U' ' && (numeral == U"V" || numeral == U"I" || text.substr(month_end).starts_with(U" wyd"))) {
      return std::nullopt;
    }

    Match date{.kind = Kind::DATE,
               .numbers = {digits_at(text, position), {.span = {month, month_end}, .integer = to_digits(*month_value)}},
               .separator = U'.'};
    // With a year first (5.VI.2024), then without it (5.VI)
    if (month_end + 5 <= text.size() && text[month_end] == U'.' && digits_end(text, month_end + 1) == month_end + 5 &&
        is_clear_after(text, month_end + 5)) {
      date.numbers.push_back(digits_at(text, month_end + 1));
      date.span = {position, month_end + 5};
      return date;
    }
    if (!is_clear_after(text, month_end) || is_digit_after(text, month_end, U".")) {
      return std::nullopt;
    }
    date.span = {position, month_end};
    return date;
  }
};

// --------------------
// ----- Language -----
// --------------------

/**
 * Polish numbers inflect for case and gender, so most readings first find the case a number takes:
 * from the preposition before it (od pięciu), the noun after it (z trzema kolegami), a governing noun or verb
 * (od kwoty trzystu tysięcy), or the year and date patterns which make it an ordinal (w 1985 roku).
 */
class Polish final : public PatternLanguage {
public:
  Polish()
      : PatternLanguage({.characters = &kCharacters, .terms = &kTerms, .symbols = &kSymbols}, kNotation, kJoiners) {
    add_pattern(std::make_unique<DatePattern>(kDateFormat));
    add_pattern(std::make_unique<YearlessDatePattern>());
    add_pattern(std::make_unique<GluedDatePattern>());
    add_pattern(std::make_unique<RomanDatePattern>());
    add_pattern(std::make_unique<FractionUnitPattern>());
    add_pattern(std::make_unique<PrimePattern>(kNotation, PrimeFormat{.cues = kPrimeCues, .heights = kHeightCues}));
    for (const auto& format : kPhoneFormats) {
      add_pattern(std::make_unique<PhonePattern>(format));
    }
    add_pattern(std::make_unique<DecadeRangePattern>());
    add_pattern(std::make_unique<DotOrdinalPattern>(true, true));
    add_pattern(std::make_unique<SuffixPattern>(Kind::NUMBER, kYearSuffixes, kYearFormat));
    add_pattern(std::make_unique<RomanPattern>(false, kCenturyAbbreviations));
  }

protected:
  std::u32string read_number(const Number& number, const Match& match) const override {
    Cues cues = cues_of(match);
    if (!number.fraction.empty()) {
      return decimal(number, cues.grammatical_case);
    }

    auto n = parse_value(number.integer);
    if (!n || is_spoken_digits(number, 7)) {
      return digits(number.integer);
    }
    if (cues.form == Form::ORDINAL) {
      return ordinal(*n, cues.grammatical_case, cues.gender, cues.plural);
    }
    return cardinal(*n, cues.grammatical_case, cues.gender);
  }

  std::u32string read_code_digits(std::u32string_view text) const override {
    auto n = code_value(text, 6);
    return n ? cardinal(*n) : digits(text);
  }

  std::optional<std::u32string> read_date(const Match& match) const override {
    // 23.07-22.08: dwudziestego trzeciego lipca do dwudziestego drugiego sierpnia
    if (match.numbers.size() == 4) {
      return ordinal(value(match.numbers[0]), cues_of(match).grammatical_case) + U" " +
             std::u32string(kMonthsGenitive[value(match.numbers[1]) - 1]) + U" do " +
             ordinal(value(match.numbers[2]), Case::GENITIVE) + U" " +
             std::u32string(kMonthsGenitive[value(match.numbers[3]) - 1]);
    }
    std::vector<std::u32string> words = {ordinal(value(match.numbers[0]), cues_of(match).grammatical_case),
                                         std::u32string(kMonthsGenitive[value(match.numbers[1]) - 1])};
    if (match.numbers.size() == 3) { // 5.VI has no year
      words.push_back(ordinal(value(match.numbers[2]), Case::GENITIVE));
    }
    if (!match.suffix.empty()) {
      words.push_back(U"roku");
    }
    return join(words);
  }

  std::optional<std::u32string> read_phone(const Match& match) const override {
    // 600 123 456 is a count, unless a word says it is a phone number: numer, tel., zadzwoń pod
    std::u32string w1 = match.context.before();
    bool called = (w1 == U"pod" || w1 == U"na") && kCallVerbs.contains(match.context.before(2));
    if (match.kind == Kind::PHONE && match.separator == U' ' && !kPhoneWords.contains(w1) && !called) {
      return std::nullopt;
    }
    // 31-123 Kraków: a postal code only before a capitalized town or after "kod"; else a range (20-300 zł)
    std::u32string town = match.context.after();
    bool postal = (!town.empty() && unicode::isupper(town[0])) || w1 == U"kod" || w1 == U"pocztowy";
    if (match.numbers.size() == 2 && match.numbers[0].integer.size() == 2 && !postal) {
      return std::nullopt;
    }

    // In groups: sześćset sto dwadzieścia trzy, a leading zero apart (020: zero dwadzieścia, 00: zero zero); a
    // longer one digit by digit
    std::vector<std::u32string> words;
    for (const auto& group : match.numbers) {
      std::u32string_view g = group.integer;
      if (g.size() > 3) {
        words.push_back(digits(g));
        continue;
      }
      size_t zeros = std::min(g.find_first_not_of(U'0'), g.size());
      words.insert(words.end(), zeros, U"zero");
      if (zeros < g.size()) {
        words.push_back(cardinal(parse_value(g.substr(zeros)).value_or(0)));
      }
    }
    return join(words);
  }

  std::optional<std::u32string> read_time(const Match& match) const override {
    const auto& minutes = match.numbers[1].integer;
    bool round = minutes == U"00" || minutes == U"15" || minutes == U"30" || minutes == U"45";
    bool decimal = match.separator == U'.' && !round && !kTimeWords.contains(match.context.before());
    // prowadzenie 20:16: a score, read by its own pattern; but o 20:16, mecz (poniedziałek, 18:00)
    bool score = match.separator == U':' && match.suffix.empty() && minutes != U"00" &&
                 !kTimeWords.contains(match.context.before()) && mentions(match.context, kSportWords);
    if (!fits_clock(match) || decimal || score) {
      return std::nullopt;
    }

    std::u32string hours = ordinal(value(match.numbers[0]), cues_of(match).grammatical_case, Gender::FEMININE);
    if (value(match.numbers[1]) == 0) {
      return hours;
    }
    return hours + U" " + (minutes[0] == U'0' ? digits(minutes) : cardinal(value(match.numbers[1])));
  }

  std::optional<std::u32string> read_roman(const Match& match) const override {
    uint64_t n = value(match.numbers[0]);
    std::u32string previous = match.context.before();
    // w XX czy XXI wieku: the second of two numerals takes the case of the word before the first
    std::u32string first = match.context.before(2); // lowercased: xix
    if (kConjunctions.contains(previous) && !first.empty() && made_of(first, U"ivxlcdm")) {
      previous = match.context.before(3);
    }

    // XIX w.: dziewiętnastego wieku, w dziewiętnastym wieku; opening its sentence or clause, osiemnasty wiek
    std::u32string next = strings::to_lower(match.context.after());
    bool century_after = kConjunctions.contains(next) && roman_value(match.context.after(2)) &&
                         match.context.tail().find(U" w.") != std::u32string_view::npos &&
                         strings::to_lower(match.context.after(3)) == U"w";
    if (match.suffix == U"w." || century_after) {
      if (previous.empty()) {
        return ordinal(n, Case::NOMINATIVE) + (century_after ? U"" : U" wiek");
      }
      Case c = kLocativePrepositions.contains(previous) ? Case::LOCATIVE : Case::GENITIVE;
      return ordinal(n, c) + (century_after ? U"" : U" wieku"); // XIX i XX w.: the noun once, after the second
    }

    // Jan Paweł drugi, Jana Pawła drugiego, Elżbiety drugiej: the case of the name
    static const auto names = [] {
      std::vector<std::tuple<std::u32string, Case, Gender>> forms;
      for (const auto& [line, gender] : kRegnalNames) {
        auto words = strings::split(line, U' ');
        for (size_t c = 0; c < words.size(); ++c) {
          forms.emplace_back(std::u32string(words[c]), static_cast<Case>(c), gender);
        }
      }
      return forms;
    }();
    static const RomanCues name_cues = [] {
      RomanCues cues;
      for (const auto& [form, c, gender] : names) {
        cues.names.insert(form);
      }
      return cues;
    }();
    if (roman_role(match, name_cues)) {
      for (const auto& [form, c, gender] : names) {
        if (form == previous) {
          return ordinal(n, c, gender);
        }
      }
    }

    // XXI wieku, II wojna, w II połowie; rozdział czwarty, w rozdziale czwartym: agreeing with its noun, after it first
    auto gender_of = [](std::u32string_view word) -> std::optional<Gender> {
      for (const auto& [form, gender] : kRomanNouns) {
        bool prefix = form.ends_with(U'-');
        if (prefix ? word.starts_with(form.substr(0, form.size() - 1)) : word == form) {
          return gender;
        }
      }
      return std::nullopt;
    };
    // A locative in -e after its preposition (w rozdziale, na piętrze), and the -ej of an adjective noun (Rzeczypospolitej)
    auto form_of = [](std::u32string_view noun, const std::u32string& preposition) {
      NounForm form = noun_form(noun, preposition);
      bool locative = kLocativePrepositions.contains(preposition);
      if (noun.ends_with(U"ej")) { // II Rzeczypospolitej
        form.grammatical_case = locative ? Case::LOCATIVE : Case::GENITIVE;
      } else if (form.grammatical_case == Case::NOMINATIVE && noun.ends_with(U'e') && locative) {
        form.grammatical_case = Case::LOCATIVE;
      }
      return form;
    };
    // XIX i XX wieku: the first of two numerals shares the noun of the second
    if (kConjunctions.contains(next) && roman_value(match.context.after(2))) {
      next = strings::to_lower(match.context.after(3));
    }
    if (auto gender = gender_of(next)) {
      NounForm noun = form_of(next, previous);
      return ordinal(n, noun.grammatical_case, *gender, noun.plural);
    }
    std::u32string_view head = match.context.head();
    size_t last = head.size();
    while (last > 0 && unicode::isspace(head[last - 1])) {
      last--;
    }
    if (auto gender = gender_of(previous); gender && last > 0 && unicode::isalpha(head[last - 1])) {
      NounForm noun = form_of(previous, match.context.before(2));
      return ordinal(n, noun.grammatical_case, *gender, noun.plural);
    }
    return std::nullopt;
  }

  std::optional<std::u32string> read_ordinal(const Match& match) const override {
    // An ordinal agrees with the noun after it: "29. sesji", but "Punkt 3. Dalej" ends a sentence
    std::u32string w1 = ordinal_cue(match.context);
    auto words = words_of(match.context.tail());
    std::u32string_view next = words.empty() ? U"" : stripped(words[0]);
    bool heading = starts_with_any(w1, kHeadings) && value(match.numbers[0]) < 100;
    // lat 70.: a decade; but "do lat 3." counts years (do lat trzech)
    uint64_t n = value(match.numbers[0]);
    bool decade = kDecadeWords.contains(w1) && n % 10 == 0 && n >= 10 && n < 100;
    if (kDecadeWords.contains(w1) && !decade) {
      return std::nullopt;
    }
    if (!decade && !heading && (next.empty() || !unicode::islower(next[0]))) {
      return std::nullopt;
    }

    Cues cues = cues_of(match);
    std::u32string reading = ordinal(n, cues.grammatical_case, cues.gender, cues.plural);
    if (match.numbers.size() == 2) {
      reading += U"-" + ordinal(value(match.numbers[1]), cues.grammatical_case, cues.gender, cues.plural); // 80.-90.
    }
    return reading;
  }

  std::optional<std::u32string> read_compound(const Match& match) const override {
    const Number& number = match.numbers[0];
    if (!number.fraction.empty()) {
      return read_number(number, match) + U"-" + std::u32string(match.suffix);
    }

    // lat 70-tych: siedemdziesiątych - the first gender and case taking the ending of the suffix
    if (auto it = kOrdinalSuffixes.find(std::u32string(match.suffix)); it != kOrdinalSuffixes.end()) {
      for (size_t g = 0; g < kAdjectiveEndings.size(); ++g) {
        for (size_t c = 0; c < kAdjectiveEndings[g].size(); ++c) {
          if (kAdjectiveEndings[g][c] == it->second) {
            Gender gender = g == 3 ? Gender::MASCULINE : static_cast<Gender>(g);
            return ordinal(value(number), static_cast<Case>(c), gender, g == 3);
          }
        }
      }
    }

    // 45-metrowe: czterdziestopięciometrowe
    return combining(value(number)) + std::u32string(match.suffix);
  }

  std::optional<std::u32string> read_plain(const Match& match) const override {
    if (match.suffix != U"r.") {
      return read_number(match.numbers[0], match);
    }
    // przed 1999 r.: przed tysiąc dziewięćset dziewięćdziesiątym dziewiątym rokiem
    bool instrumental = cues_of(match).grammatical_case == Case::INSTRUMENTAL;
    return read_number(match.numbers[0], match) + (instrumental ? U" rokiem" : U" roku");
  }

  std::optional<std::u32string> read_money(const Number& number, const Match& match) const override {
    const Noun& currency = kCurrencies.at(std::u32string(match.unit));
    Case c = amount_case(match);
    if (!match.scale.empty()) {
      return amount(number, kScales.at(std::u32string(match.scale)), c) + U" " + currency.plural[index(Case::GENITIVE)];
    }

    // dwanaście złotych dziewięćdziesiąt dziewięć groszy; 5,00 zł pięć złotych; 0,99 zł dziewięćdziesiąt dziewięć
    // groszy; 3,50 € trzy euro pięćdziesiąt centów
    // A price per unit keeps its unit for the end: 5,96 zł/m² pięć złotych dziewięćdziesiąt sześć groszy za metr
    std::u32string unit(match.unit.substr(0, match.unit.find(U'/')));
    std::u32string per = currency.singular[0].substr(kCurrencies.at(unit).singular[0].size());
    const Noun* hundredth = is_zloty(unit) ? &kGrosz : kCents.contains(unit) ? &kCent : nullptr;
    if (number.fraction.size() == 2 && hundredth) {
      uint64_t cents = parse_value(number.fraction).value_or(0);
      Number whole{.integer = number.integer, .grouped = number.grouped};
      std::u32string small = cardinal(cents, c) + U" " + form(*hundredth, cents, c);
      if (value(whole) == 0 && cents) {
        return small + per;
      }
      return amount(whole, kCurrencies.at(unit), c) + (cents ? U" " + small : U"") + per;
    }
    return amount(number, currency, c);
  }

  // The case of an amount, or of the end of its range after "do": 4-5 proc. cztery do pięciu procent
  Case amount_case(const Match& match) const {
    if (match.numbers.size() == 2 && read_range(match.numbers[0], match.numbers[1], match) == U" do ") {
      return Case::GENITIVE;
    }
    return cues_of(match).amount_case;
  }

  std::optional<std::u32string> read_scale(const Number& number, const Match& match) const override {
    return amount(number, kScales.at(std::u32string(match.scale)), amount_case(match));
  }

  std::optional<std::u32string> read_percent(const Number& number, const Match& match) const override {
    return amount(number, kPercent, amount_case(match)) + spaced_suffix(match);
  }

  std::optional<std::u32string> read_unit(const Number& number, const Match& match) const override {
    const Noun& unit = kUnits.at(std::u32string(match.unit));
    return amount(number, unit, amount_case(match));
  }

  std::optional<std::u32string> read_fraction(const Match& match) const override {
    // Only where it divides a noun after it (2/3 głosów), after "ułamek", or after a whole number: 10/12 may be a
    // house number, and 7/10 a score
    auto [a, b] = fraction_of(match);
    // 1/2 kg pół kilograma, 3/4 l trzy czwarte litra: a unit's genitive after the fraction
    if (!match.unit.empty()) {
      if (b < 2 || a == 0 || a >= b) {
        return std::nullopt;
      }
      Case c = fraction_case(match.context);
      bool direct = c == Case::NOMINATIVE || c == Case::ACCUSATIVE;
      const std::u32string& unit = kUnits.at(std::u32string(match.unit)).singular[index(Case::GENITIVE)];
      return (a == 1 && b == 2 && direct ? U"pół" : fraction_words(a, b, c)) + U" " + unit;
    }
    // ulica Marszałkowska 10/12: a house number, dziesięć łamane przez dwanaście
    // (ulica Nowy Świat 15/17: a name of up to three words)
    bool street = std::ranges::any_of(std::array{2, 3, 4},
                                      [&](size_t k) { return kStreetWords.contains(match.context.before(k)); });
    if (match.numbers.size() == 2 && street) {
      return cardinal(a) + U" łamane przez " + cardinal(b);
    }
    std::u32string next = strings::to_lower(match.context.after());
    bool month = std::ranges::find(kMonthsGenitive, next) != kMonthsGenitive.end(); // 3/4 maja: two days
    bool noun = !next.empty() && unicode::isalpha(next[0]) && !kFractionStops.contains(next) && !month;
    bool compared = match.context.before() == U"niż" && !kFractionStops.contains(next); // większy niż 1/2
    bool marked =
        match.numbers.size() == 3 || match.context.before().starts_with(U"ułam") || compared || noun; // większy niż 1/2
    std::u32string_view head = match.context.head();
    bool decimal = head.size() >= 2 && head.back() == U',' && unicode::isdigit(head[head.size() - 2]); // 10,8/12 V
    bool common = b <= 10 || b == 12 || b == 16 || b == 20 || b == 100 || b == 1000;
    if (a == 0 || a >= b || !common || !marked || decimal) {
      return std::nullopt;
    }

    // pół szklanki; jedna trzecia, dwie trzecie, pięć ósmych; około jednej piątej, w jednej czwartej finału
    Case c = fraction_case(match.context);
    bool direct = c == Case::NOMINATIVE || c == Case::ACCUSATIVE;
    if (a == 1 && b == 2 && noun && direct) {
      return U"pół";
    }
    return fraction_words(a, b, c);
  }

  std::optional<std::u32string> read_mixed_fraction(const Match& match) const override {
    if (match.numbers.size() == 2) {
      return read_fraction(match);
    }
    // 1 1/2: półtorej godziny, półtora roku (a feminine noun's genitive ends in -y or -i); 2 1/2 roku: dwa i pół
    // roku; 3 3/4 mili: trzy i trzy czwarte mili
    auto [a, b] = fraction_of(match);
    uint64_t whole = value(match.numbers[0]);
    if (a >= b) {
      return std::nullopt;
    }
    if (whole == 1 && a == 1 && b == 2) {
      std::u32string next = match.context.after();
      return next.ends_with(U'y') || next.ends_with(U'i') ? U"półtorej" : U"półtora";
    }
    Case c = fraction_case(match.context);
    return cardinal(whole, c) + U" i " + (a == 1 && b == 2 ? U"pół" : fraction_words(a, b, c));
  }

private:
  // 8'22": osiem minut dwadzieścia dwie sekundy; 5'5": pięć stóp pięć cali
  std::u32string read_prime(const Number& number, Prime unit, const Match& /*match*/) const override {
    Number whole{.integer = to_digits(value(number))};
    return amount(whole, kPrimeUnits.at(unit), Case::NOMINATIVE);
  }
  std::optional<std::u32string> read_prime_fraction(std::u32string_view digits, const Match& /*match*/) const override {
    return cardinal(parse_value(digits).value_or(0));
  }

  // The case of a fraction, from the word before it: około jednej piątej, w jednej czwartej, zjadł jedną szóstą
  static Case fraction_case(const Context& context) {
    std::u32string before = context.before();
    if (before == U"w" || before == U"we") {
      return Case::LOCATIVE;
    }
    if (auto it = kPrepositionCases.find(before); it != kPrepositionCases.end()) {
      return it->second;
    }
    // An object after a verb: zjadł, wypił, dodaj, kup
    bool past = before.size() > 2 && before.ends_with(U'ł');
    return past || kImperatives.contains(before) ? Case::ACCUSATIVE : Case::NOMINATIVE;
  }

  // a/b in a case: jedna trzecia, dwie trzecie, pięć ósmych, jednej piątej, dwóch trzecich
  static std::u32string fraction_words(uint64_t a, uint64_t b, Case c) {
    bool direct = c == Case::NOMINATIVE || c == Case::ACCUSATIVE;
    bool few = a % 10 >= 2 && a % 10 <= 4 && (a % 100 < 12 || a % 100 > 14);
    Case denominator = a == 1 || few || !direct ? c : Case::GENITIVE;
    return cardinal(a, c, Gender::FEMININE) + U" " + ordinal(b, denominator, Gender::FEMININE, a != 1);
  }

public:
  std::optional<std::u32string> read_joiner(const Match& match) const override {
    if (match.kind == Kind::DIMENSION) {
      // 5 × 3 = 15, oblicz 5 × 3: a product; 60 × 35 cm, wymiary 20 × 30: a dimension
      static const std::unordered_set<std::u32string> math = {U"oblicz", U"policz", U"ile", U"pomnóż", U"wynik"};
      bool product = match.context.after(2) == U"=" || math.contains(match.context.before(2));
      if (product && match.text.find(U'×') != std::u32string_view::npos) {
        return U" razy ";
      }
      return cues_of(match).numbered ? U" przez " : U" na ";
    }
    // 1990 - 1994: a spaced dash between numbers is a range, unless an equation follows (5 - 3 = 2)
    if (match.kind == Kind::DIFFERENCE && match.text == U" - " && match.context.after(2) != U"=") {
      return U" do ";
    }
    return PatternLanguage::read_joiner(match);
  }

  // 15-20%, 20-300 zł, 1990-1994: piętnaście do dwudziestu procent ("do" gives the end its case). But a range
  // of hours (w godzinach 12-16: dwunasta-szesnasta), or before a noun written to agree with its end (2-3 minuty:
  // dwie-trzy minuty), keeps its hyphen.
  std::u32string read_range(const Number& from, const Number& to, const Match& match) const override {
    // A range of hours, or of clock times (9:00-17:00): dziewiąta-siedemnasta
    std::u32string_view head = match.context.head();
    size_t start = head.size();
    while (start > 0 && unicode::isdigit(head[start - 1])) {
      start--;
    }
    if (kHourWords.contains(match.context.before(2)) || (start > 0 && head[start - 1] == U':')) {
      return U"-";
    }
    auto year = [](const Number& n) { return n.integer.size() == 4 && is_year_value(value(n)); };
    auto after = words_of(match.context.tail());
    std::u32string noun =
        after.size() >= 2 && made_of(stripped(after[0]), U"0123456789,.") && after[0] == stripped(after[0]) ?
            std::u32string(stripped(after[1])) :
            U"";
    bool counted = !noun.empty() && unicode::islower(noun[0]) && !kUnits.contains(noun) && !kCurrencies.contains(noun);
    return counted && !(year(from) && year(to)) ? U"-" : U" do ";
  }
};
} // namespace

const Language& language() {
  static const Polish language;
  return language;
}

} // namespace verbalis::languages::pl
