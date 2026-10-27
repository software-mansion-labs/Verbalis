#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/normalize.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/strings.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace verbalis::languages::pl {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::RomanCues;
using ::verbalis::numbers::SuffixFormat;

// A noun's forms after a number: singular, then plural, each in the order of Case.
struct Noun {
  std::array<std::u32string, 6> singular;
  std::array<std::u32string, 6> plural;
};

// A noun from its twelve forms, separated by spaces, or by "|" when they are made of several words.
inline Noun noun(std::u32string_view forms) {
  char32_t separator = forms.find(U'|') != std::u32string_view::npos ? U'|' : U' ';
  Noun result;
  for (size_t i = 0, start = 0; i < 12; ++i) {
    size_t end = std::min(forms.find(separator, start), forms.size());
    (i < 6 ? result.singular[i] : result.plural[i - 6]) = forms.substr(start, end - start);
    start = end + 1;
  }
  return result;
}

// A regular masculine noun from its stem and its locative: metr, metrze.
inline Noun masculine(std::u32string_view stem, std::u32string_view locative) {
  std::u32string s(stem);
  return {{s, s + U"a", s + U"owi", s, s + U"em", std::u32string(locative)},
          {s + U"y", s + U"ów", s + U"om", s + U"y", s + U"ami", s + U"ach"}};
}

// Cardinals: nominative/accusative, genitive/locative, dative, instrumental.
inline const std::map<uint64_t, std::array<std::u32string_view, 4>> kCardinals = {
  {0,   {U"zero", U"zera", U"zeru", U"zerem"}},
  {2,   {U"dwa", U"dwóch", U"dwóm", U"dwoma"}},
  {3,   {U"trzy", U"trzech", U"trzem", U"trzema"}},
  {4,   {U"cztery", U"czterech", U"czterem", U"czterema"}},
  {5,   {U"pięć", U"pięciu", U"pięciu", U"pięcioma"}},
  {6,   {U"sześć", U"sześciu", U"sześciu", U"sześcioma"}},
  {7,   {U"siedem", U"siedmiu", U"siedmiu", U"siedmioma"}},
  {8,   {U"osiem", U"ośmiu", U"ośmiu", U"ośmioma"}},
  {9,   {U"dziewięć", U"dziewięciu", U"dziewięciu", U"dziewięcioma"}},
  {10,  {U"dziesięć", U"dziesięciu", U"dziesięciu", U"dziesięcioma"}},
  {11,  {U"jedenaście", U"jedenastu", U"jedenastu", U"jedenastoma"}},
  {12,  {U"dwanaście", U"dwunastu", U"dwunastu", U"dwunastoma"}},
  {13,  {U"trzynaście", U"trzynastu", U"trzynastu", U"trzynastoma"}},
  {14,  {U"czternaście", U"czternastu", U"czternastu", U"czternastoma"}},
  {15,  {U"piętnaście", U"piętnastu", U"piętnastu", U"piętnastoma"}},
  {16,  {U"szesnaście", U"szesnastu", U"szesnastu", U"szesnastoma"}},
  {17,  {U"siedemnaście", U"siedemnastu", U"siedemnastu", U"siedemnastoma"}},
  {18,  {U"osiemnaście", U"osiemnastu", U"osiemnastu", U"osiemnastoma"}},
  {19,  {U"dziewiętnaście", U"dziewiętnastu", U"dziewiętnastu", U"dziewiętnastoma"}},
  {20,  {U"dwadzieścia", U"dwudziestu", U"dwudziestu", U"dwudziestoma"}},
  {30,  {U"trzydzieści", U"trzydziestu", U"trzydziestu", U"trzydziestoma"}},
  {40,  {U"czterdzieści", U"czterdziestu", U"czterdziestu", U"czterdziestoma"}},
  {50,  {U"pięćdziesiąt", U"pięćdziesięciu", U"pięćdziesięciu", U"pięćdziesięcioma"}},
  {60,  {U"sześćdziesiąt", U"sześćdziesięciu", U"sześćdziesięciu", U"sześćdziesięcioma"}},
  {70,  {U"siedemdziesiąt", U"siedemdziesięciu", U"siedemdziesięciu", U"siedemdziesięcioma"}},
  {80,  {U"osiemdziesiąt", U"osiemdziesięciu", U"osiemdziesięciu", U"osiemdziesięcioma"}},
  {90,  {U"dziewięćdziesiąt", U"dziewięćdziesięciu", U"dziewięćdziesięciu", U"dziewięćdziesięcioma"}},
  {100, {U"sto", U"stu", U"stu", U"stoma"}},
  {200, {U"dwieście", U"dwustu", U"dwustu", U"dwustoma"}},
  {300, {U"trzysta", U"trzystu", U"trzystu", U"trzystoma"}},
  {400, {U"czterysta", U"czterystu", U"czterystu", U"czterystoma"}},
  {500, {U"pięćset", U"pięciuset", U"pięciuset", U"pięciuset"}},
  {600, {U"sześćset", U"sześciuset", U"sześciuset", U"sześciuset"}},
  {700, {U"siedemset", U"siedmiuset", U"siedmiuset", U"siedmiuset"}},
  {800, {U"osiemset", U"ośmiuset", U"ośmiuset", U"ośmiuset"}},
  {900, {U"dziewięćset", U"dziewięciuset", U"dziewięciuset", U"dziewięciuset"}}
};

// "1" by gender (masculine, feminine, neuter), each in the order of Case.
inline const std::array<std::array<std::u32string_view, 6>, 3> kOne = {{
  {U"jeden", U"jednego", U"jednemu", U"jeden", U"jednym", U"jednym"},
  {U"jedna", U"jednej", U"jednej", U"jedną", U"jedną", U"jednej"},
  {U"jedno", U"jednego", U"jednemu", U"jedno", U"jednym", U"jednym"}
}};

// Stems of ordinals, which take adjective endings: pierwsz-y, drug-iego.
inline const std::map<uint64_t, std::u32string_view> kOrdinalStems = {
  {1, U"pierwsz"},  {2, U"drug"},    {3, U"trzec"},    {4, U"czwart"},   {5, U"piąt"},
  {6, U"szóst"},    {7, U"siódm"},   {8, U"ósm"},      {9, U"dziewiąt"}, {10, U"dziesiąt"},
  {11, U"jedenast"}, {12, U"dwunast"}, {13, U"trzynast"}, {14, U"czternast"}, {15, U"piętnast"},
  {16, U"szesnast"}, {17, U"siedemnast"}, {18, U"osiemnast"}, {19, U"dziewiętnast"}, {20, U"dwudziest"},
  {30, U"trzydziest"}, {40, U"czterdziest"}, {50, U"pięćdziesiąt"}, {60, U"sześćdziesiąt"},
  {70, U"siedemdziesiąt"}, {80, U"osiemdziesiąt"}, {90, U"dziewięćdziesiąt"},
  {100, U"setn"}, {200, U"dwusetn"}, {300, U"trzechsetn"}, {400, U"czterechsetn"}, {500, U"pięćsetn"},
  {600, U"sześćsetn"}, {700, U"siedemsetn"}, {800, U"osiemsetn"}, {900, U"dziewięćsetn"}
};

// Adjective endings by gender (masculine, feminine, neuter, plural), each in the order of Case.
inline const std::array<std::array<std::u32string_view, 6>, 4> kAdjectiveEndings = {{
  {U"y", U"ego", U"emu", U"y", U"ym", U"ym"},
  {U"a", U"ej", U"ej", U"ą", U"ą", U"ej"},
  {U"e", U"ego", U"emu", U"e", U"ym", U"ym"},
  {U"e", U"ych", U"ym", U"e", U"ymi", U"ych"}
}};

// Forms of numbers inside compounds: 45-metrowe -> czterdziestopięciometrowe.
inline const std::map<uint64_t, std::u32string_view> kCombining = {
  {1, U"jedno"},     {2, U"dwu"},        {3, U"trzy"},        {4, U"cztero"},      {5, U"pięcio"},
  {6, U"sześcio"},   {7, U"siedmio"},    {8, U"ośmio"},       {9, U"dziewięcio"},  {10, U"dziesięcio"},
  {11, U"jedenasto"}, {12, U"dwunasto"}, {13, U"trzynasto"},  {14, U"czternasto"}, {15, U"piętnasto"},
  {16, U"szesnasto"}, {17, U"siedemnasto"}, {18, U"osiemnasto"}, {19, U"dziewiętnasto"}, {20, U"dwudziesto"},
  {30, U"trzydziesto"}, {40, U"czterdziesto"}, {50, U"pięćdziesięcio"}, {60, U"sześćdziesięcio"},
  {70, U"siedemdziesięcio"}, {80, U"osiemdziesięcio"}, {90, U"dziewięćdziesięcio"},
  {100, U"stu"}, {200, U"dwustu"}, {300, U"trzystu"}, {400, U"czterystu"}, {500, U"pięćset"},
  {600, U"sześćset"}, {700, U"siedemset"}, {800, U"osiemset"}, {900, U"dziewięćset"}
};

// Hyphenated endings which make a number an ordinal (lat 70-tych), with the adjective ending they stand for.
inline const std::unordered_map<std::u32string, std::u32string_view> kOrdinalSuffixes = {
  {U"tych", U"ych"}, {U"ych", U"ych"}, {U"ty", U"y"},    {U"y", U"y"},     {U"szy", U"y"},    {U"gi", U"y"},
  {U"ci", U"y"},     {U"go", U"ego"},  {U"ego", U"ego"}, {U"tego", U"ego"}, {U"ej", U"ej"},    {U"tej", U"ej"},
  {U"ym", U"ym"},    {U"tym", U"ym"},  {U"ta", U"a"},    {U"a", U"a"},     {U"te", U"e"},     {U"e", U"e"},
  {U"mu", U"emu"},   {U"temu", U"emu"}
};

// Nouns counted by numbers
inline const Noun kThousand = noun(U"tysiąc tysiąca tysiącowi tysiąc tysiącem tysiącu tysiące tysięcy tysiącom tysiące tysiącami tysiącach");
inline const Noun kMillion = masculine(U"milion", U"milionie");
inline const Noun kBillion = masculine(U"miliard", U"miliardzie");
inline const Noun kZloty = noun(U"złoty złotego złotemu złoty złotym złotym złote złotych złotym złote złotymi złotych");
inline const Noun kGrosz = noun(U"grosz grosza groszowi grosz groszem groszu grosze groszy groszom grosze groszami groszach");
inline const Noun kPercent = noun(U"procent procent procentowi procent procentem procencie procent procent procentom procent procentami procentach");
inline const Noun kEuro = noun(U"euro euro euro euro euro euro euro euro euro euro euro euro");
inline const Noun kDollar = masculine(U"dolar", U"dolarze");
inline constexpr std::u32string_view kDegree = U"stopień stopnia stopniowi stopień stopniem stopniu stopnie stopni stopniom stopnie stopniami stopniach";

inline const std::unordered_map<std::u32string, Noun> kCurrencies = {
  {U"zł", kZloty}, {U"PLN", kZloty}, {U"€", kEuro}, {U"EUR", kEuro}, {U"$", kDollar}, {U"USD", kDollar},
  {U"£", masculine(U"funt", U"funcie")}, {U"gr", kGrosz}
};

inline const std::unordered_map<std::u32string, Noun> kScales = {
  {U"tys.", kThousand}, {U"mln", kMillion}, {U"mld", kBillion}
};

// A noun with an adjective after it, from the adjective's twelve forms: metr kwadratowy, metrów kwadratowych.
inline Noun qualified(const Noun& base, std::u32string_view adjective) {
  Noun forms = noun(adjective), result;
  for (size_t c = 0; c < 6; ++c) {
    result.singular[c] = base.singular[c] + U" " + forms.singular[c];
    result.plural[c] = base.plural[c] + U" " + forms.plural[c];
  }
  return result;
}

inline constexpr std::u32string_view kSquare = U"kwadratowy kwadratowego kwadratowemu kwadratowy kwadratowym kwadratowym kwadratowe kwadratowych kwadratowym kwadratowe kwadratowymi kwadratowych";
inline constexpr std::u32string_view kCubic = U"sześcienny sześciennego sześciennemu sześcienny sześciennym sześciennym sześcienne sześciennych sześciennym sześcienne sześciennymi sześciennych";
inline const Noun kMetre = masculine(U"metr", U"metrze");
inline const Noun kKilometre = masculine(U"kilometr", U"kilometrze");
inline const Noun kCentimetre = masculine(U"centymetr", U"centymetrze");
inline const Noun kPoint = noun(U"punkt punktu punktowi punkt punktem punkcie punkty punktów punktom punkty punktami punktach");

inline const std::unordered_map<std::u32string, Noun> kUnits = {
  {U"km", kKilometre},   {U"m", kMetre},
  {U"m²", qualified(kMetre, kSquare)},           {U"m2", qualified(kMetre, kSquare)},
  {U"km²", qualified(kKilometre, kSquare)},      {U"km2", qualified(kKilometre, kSquare)},
  {U"cm²", qualified(kCentimetre, kSquare)},     {U"m³", qualified(kMetre, kCubic)},
  {U"m3", qualified(kMetre, kCubic)},            {U"cm³", qualified(kCentimetre, kCubic)},
  {U"km/h", qualified(kKilometre, U"na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę|na godzinę")},
  {U"pkt", kPoint},   {U"pkt.", kPoint},
  {U"pkt. proc.", qualified(kPoint, U"procentowy procentowego procentowemu procentowy procentowym procentowym procentowe procentowych procentowym procentowe procentowymi procentowych")},
  {U"pkt proc.", qualified(kPoint, U"procentowy procentowego procentowemu procentowy procentowym procentowym procentowe procentowych procentowym procentowe procentowymi procentowych")},
  {U"proc.", noun(U"procent procent procentowi procent procentem procencie procent procent procentom procent procentami procentach")},
  {U"‰", noun(U"promil promila promilowi promil promilem promilu promile promili promilom promile promilami promilach")},
  {U"cm", masculine(U"centymetr", U"centymetrze")}, {U"mm", masculine(U"milimetr", U"milimetrze")},
  {U"nm", masculine(U"nanometr", U"nanometrze")},   {U"kg", masculine(U"kilogram", U"kilogramie")},
  {U"g", masculine(U"gram", U"gramie")},            {U"mg", masculine(U"miligram", U"miligramie")},
  {U"l", masculine(U"litr", U"litrze")},            {U"ml", masculine(U"mililitr", U"mililitrze")},
  {U"ha", masculine(U"hektar", U"hektarze")},       {U"W", masculine(U"wat", U"wacie")},
  {U"kW", masculine(U"kilowat", U"kilowacie")},     {U"MW", masculine(U"megawat", U"megawacie")},
  {U"V", masculine(U"wolt", U"wolcie")},            {U"GB", masculine(U"gigabajt", U"gigabajcie")},
  {U"MB", masculine(U"megabajt", U"megabajcie")},   {U"TB", masculine(U"terabajt", U"terabajcie")},
  {U"kWh", noun(U"kilowatogodzina kilowatogodziny kilowatogodzinie kilowatogodzinę kilowatogodziną kilowatogodzinie kilowatogodziny kilowatogodzin kilowatogodzinom kilowatogodziny kilowatogodzinami kilowatogodzinach")},
  {U"Hz", noun(U"herc herca hercowi herc hercem hercu herce herców hercom herce hercami hercach")},
  {U"°C", noun(U"stopień Celsjusza|stopnia Celsjusza|stopniowi Celsjusza|stopień Celsjusza|stopniem Celsjusza|stopniu Celsjusza|stopnie Celsjusza|stopni Celsjusza|stopniom Celsjusza|stopnie Celsjusza|stopniami Celsjusza|stopniach Celsjusza")},
  {U"°", noun(kDegree)},
  {U"t", noun(U"tona tony tonie tonę toną tonie tony ton tonom tony tonami tonach")},
  {U"h", noun(U"godzina godziny godzinie godzinę godziną godzinie godziny godzin godzinom godziny godzinami godzinach")},
  {U"min", noun(U"minuta minuty minucie minutę minutą minucie minuty minut minutom minuty minutami minutach")},
  {U"s", noun(U"sekunda sekundy sekundzie sekundę sekundą sekundzie sekundy sekund sekundom sekundy sekundami sekundach")}
};

// Months in the genitive, as dates read them: 27 marca
inline const std::array<std::u32string_view, 12> kMonthsGenitive = {
  U"stycznia", U"lutego", U"marca", U"kwietnia", U"maja", U"czerwca",
  U"lipca", U"sierpnia", U"września", U"października", U"listopada", U"grudnia"
};

inline const std::unordered_set<std::u32string> kMonthWords = {
  U"stycznia", U"lutego", U"marca", U"kwietnia", U"maja", U"czerwca", U"lipca", U"sierpnia", U"września",
  U"października", U"listopada", U"grudnia", U"styczeń", U"luty", U"marzec", U"kwiecień", U"maj", U"czerwiec",
  U"lipiec", U"sierpień", U"wrzesień", U"październik", U"listopad", U"grudzień"
};

// The case a preposition gives the number after it: od pięciu, z trzema
inline const std::unordered_map<std::u32string, Case> kPrepositionCases = {
  {U"od", Case::GENITIVE}, {U"do", Case::GENITIVE}, {U"bez", Case::GENITIVE}, {U"dla", Case::GENITIVE},
  {U"u", Case::GENITIVE}, {U"około", Case::GENITIVE}, {U"koło", Case::GENITIVE}, {U"obok", Case::GENITIVE},
  {U"podczas", Case::GENITIVE}, {U"według", Case::GENITIVE}, {U"wśród", Case::GENITIVE},
  {U"spośród", Case::GENITIVE}, {U"sprzed", Case::GENITIVE}, {U"spod", Case::GENITIVE}, {U"zza", Case::GENITIVE},
  {U"znad", Case::GENITIVE}, {U"oprócz", Case::GENITIVE}, {U"prócz", Case::GENITIVE}, {U"zamiast", Case::GENITIVE},
  {U"wokół", Case::GENITIVE}, {U"dookoła", Case::GENITIVE}, {U"naprzeciwko", Case::GENITIVE},
  {U"blisko", Case::GENITIVE}, {U"niedaleko", Case::GENITIVE}, {U"powyżej", Case::GENITIVE},
  {U"poniżej", Case::GENITIVE},
  {U"dzięki", Case::DATIVE}, {U"przeciw", Case::DATIVE}, {U"przeciwko", Case::DATIVE}, {U"ku", Case::DATIVE},
  {U"wbrew", Case::DATIVE},
  {U"przez", Case::ACCUSATIVE}, {U"ponad", Case::ACCUSATIVE}, {U"na", Case::ACCUSATIVE}, {U"o", Case::ACCUSATIVE},
  {U"za", Case::ACCUSATIVE}, {U"po", Case::ACCUSATIVE}, {U"w", Case::ACCUSATIVE}, {U"we", Case::ACCUSATIVE},
  {U"z", Case::INSTRUMENTAL}, {U"ze", Case::INSTRUMENTAL}, {U"nad", Case::INSTRUMENTAL},
  {U"pod", Case::INSTRUMENTAL}, {U"przed", Case::INSTRUMENTAL}, {U"między", Case::INSTRUMENTAL},
  {U"pomiędzy", Case::INSTRUMENTAL},
  {U"przy", Case::LOCATIVE}
};

inline const std::unordered_set<std::u32string> kLocativePrepositions = {U"w", U"we", U"o", U"po", U"przy", U"na"};

// Prepositional phrases which govern the genitive, with the preposition they need: w ciągu dwóch tygodni
inline const std::unordered_map<std::u32string, std::u32string> kComplexPrepositions = {
  {U"ciągu", U"w"}, {U"trakcie", U"w"}, {U"czasie", U"w"}, {U"wyniku", U"w"}, {U"przypadku", U"w"},
  {U"zakresie", U"w"}, {U"ramach", U"w"}, {U"celu", U"w"}, {U"postaci", U"w"}, {U"powodu", U"z"}
};

// Stems of amount nouns: a number after them is their genitive attribute (od kwoty trzystu tysięcy)
inline const std::vector<std::u32string> kAmountNouns = {
  U"rzęd", U"wysokoś", U"gruboś", U"średnic", U"długoś", U"szerokoś", U"głębokoś", U"wiek", U"kwot", U"kwoc",
  U"poziom", U"stan", U"strat", U"strac", U"wartoś", U"liczb", U"cen", U"koszt", U"opłat", U"kar", U"moc",
  U"wag", U"wadz", U"pojemnoś", U"powierzchn", U"prędkoś", U"temperatur", U"objętoś", U"odległoś", U"armi",
  U"grup", U"okres", U"limit", U"próg", U"prog", U"budżet", U"deficyt", U"nadwyżk", U"dochod", U"dochód",
  U"przychod", U"przychód", U"wydatk", U"wzrost", U"spad", U"zysk", U"dotacj", U"wynagrodzen", U"pensj"
};

inline const std::unordered_set<std::u32string> kNounEndings = {
  U"", U"a", U"ą", U"ę", U"y", U"i", U"u", U"e", U"ie", U"em", U"iem", U"om", U"ów", U"ami", U"ach", U"ć", U"ci",
  U"cią", U"ze", U"ia", U"ię", U"ii", U"ek", U"ku"
};

// Stems of verbs whose object is in the genitive: potrzebowali stu czterdziestu gramów
inline const std::vector<std::u32string> kGenitiveVerbs = {
  U"potrzeb", U"używ", U"użyj", U"użyć", U"wymag", U"szuk", U"poszuk", U"unik", U"brak", U"zabrak", U"życz",
  U"dotycz", U"uczy", U"nauczy", U"słucha", U"pilnu"
};

// Words skipped when looking for the verb governing a number: brakuje nam trzech osób
inline const std::unordered_set<std::u32string> kClitics = {
  U"mi", U"ci", U"mu", U"jej", U"nam", U"wam", U"im", U"się", U"sobie", U"już", U"jeszcze", U"teraz", U"tu",
  U"tam", U"aż", U"nawet"
};

inline const std::unordered_set<std::u32string> kCopulas = {
  U"jest", U"są", U"był", U"była", U"było", U"były", U"będzie", U"będą", U"to"
};

// Prepositions before a bare year (od 2020 do 2022), with the case of the year ordinal after them
inline const std::unordered_map<std::u32string, Case> kYearPrepositions = {
  {U"od", Case::GENITIVE}, {U"do", Case::GENITIVE}, {U"z", Case::GENITIVE}, {U"ze", Case::GENITIVE},
  {U"sprzed", Case::GENITIVE}, {U"około", Case::GENITIVE}, {U"przed", Case::INSTRUMENTAL},
  {U"po", Case::LOCATIVE}, {U"na", Case::ACCUSATIVE}
};

inline const std::unordered_set<std::u32string> kCountNouns = {
  U"osób", U"lat", U"dni", U"godzin", U"minut", U"sekund", U"razy", U"sztuk", U"ludzi", U"dzieci", U"tygodni",
  U"miesięcy"
};

inline const std::unordered_set<std::u32string> kCurrencyWords = {U"zł", U"złoty", U"złote", U"złotych"};

// Stems of nouns whose numbers are feminine: dwie osoby, dwie godziny
inline const std::vector<std::u32string> kFemininePlurals = {
  U"osob", U"osób", U"godzin", U"minut", U"sekund", U"złotówk", U"kobiet", U"stron", U"sztuk", U"rzecz", U"częś",
  U"noc", U"dob", U"klas", U"wersj", U"sesj", U"edycj", U"zapał", U"tablic", U"butelk", U"szklank", U"łyżk"
};

// Words before a time with a dot, which make it a time: o 18.30
inline const std::unordered_set<std::u32string> kTimeWords = {
  U"o", U"godzinie", U"godz", U"od", U"do", U"około", U"koło", U"po", U"przed", U"na", U"między", U"za"
};

// Units of time, whose amount after "w" is a span (w 10 min: within ten minutes), not a place
inline const std::unordered_set<std::u32string_view> kTimeUnits = {U"h", U"min", U"s", U"ms", U"godz.", U"sek."};

// The forms of "godzina", with the case of the hour after them: o godzinie 15 (piętnastej), do godziny 16 (szesnastej)
inline const std::unordered_map<std::u32string, Case> kHourWords = {
  {U"godzina", Case::NOMINATIVE}, {U"godziny", Case::GENITIVE}, {U"godzinie", Case::LOCATIVE},
  {U"godzinę", Case::ACCUSATIVE}, {U"godziną", Case::INSTRUMENTAL}
};

// Stems of headings: Punkt 3., w punkcie 5.; the ones of kOrdinalHeadings read as ordinals even without a dot
inline const std::vector<std::u32string> kHeadings = {
  U"podpunkt", U"punkt", U"punkc", U"rozdział", U"paragraf", U"artykuł", U"pkt"
};
inline const std::vector<std::u32string> kOrdinalHeadings = {U"podpunkt", U"punkt", U"punkc", U"paragraf", U"pkt"};

// Words before the ordinal of a decade, with its case: z lat 70., w latach 80.
inline const std::unordered_map<std::u32string, Case> kDecadeWords = {
  {U"lata", Case::NOMINATIVE},     {U"lat", Case::GENITIVE},       {U"latach", Case::LOCATIVE},
  {U"latami", Case::INSTRUMENTAL}, {U"roczników", Case::GENITIVE}, {U"rocznikach", Case::LOCATIVE}
};

inline const std::unordered_set<std::u32string> kConjunctions = {U"a", U"i", U"lub", U"albo", U"czy", U"oraz"};

// Names which take a regnal number, by case (nominative, genitive, dative, accusative, instrumental, locative)
// and gender: Jan Paweł II (drugi), Jana Pawła II (drugiego), Elżbiety II (drugiej)
inline const std::vector<std::pair<std::u32string_view, Gender>> kRegnalNames = {
  {U"jan jana janowi jana janem janie", Gender::MASCULINE},
  {U"paweł pawła pawłowi pawła pawłem pawle", Gender::MASCULINE},
  {U"benedykt benedykta benedyktowi benedykta benedyktem benedykcie", Gender::MASCULINE},
  {U"leon leona leonowi leona leonem leonie", Gender::MASCULINE},
  {U"pius piusa piusowi piusa piusem piusie", Gender::MASCULINE},
  {U"grzegorz grzegorza grzegorzowi grzegorza grzegorzem grzegorzu", Gender::MASCULINE},
  {U"karol karola karolowi karola karolem karolu", Gender::MASCULINE},
  {U"henryk henryka henrykowi henryka henrykiem henryku", Gender::MASCULINE},
  {U"ludwik ludwika ludwikowi ludwika ludwikiem ludwiku", Gender::MASCULINE},
  {U"kazimierz kazimierza kazimierzowi kazimierza kazimierzem kazimierzu", Gender::MASCULINE},
  {U"zygmunt zygmunta zygmuntowi zygmunta zygmuntem zygmuncie", Gender::MASCULINE},
  {U"władysław władysława władysławowi władysława władysławem władysławie", Gender::MASCULINE},
  {U"bolesław bolesława bolesławowi bolesława bolesławem bolesławie", Gender::MASCULINE},
  {U"mieszko mieszka mieszkowi mieszka mieszkiem mieszku", Gender::MASCULINE},
  {U"jerzy jerzego jerzemu jerzego jerzym jerzym", Gender::MASCULINE},
  {U"edward edwarda edwardowi edwarda edwardem edwardzie", Gender::MASCULINE},
  {U"ryszard ryszarda ryszardowi ryszarda ryszardem ryszardzie", Gender::MASCULINE},
  {U"wilhelm wilhelma wilhelmowi wilhelma wilhelmem wilhelmie", Gender::MASCULINE},
  {U"fryderyk fryderyka fryderykowi fryderyka fryderykiem fryderyku", Gender::MASCULINE},
  {U"aleksander aleksandra aleksandrowi aleksandra aleksandrem aleksandrze", Gender::MASCULINE},
  {U"mikołaj mikołaja mikołajowi mikołaja mikołajem mikołaju", Gender::MASCULINE},
  {U"piotr piotra piotrowi piotra piotrem piotrze", Gender::MASCULINE},
  {U"napoleon napoleona napoleonowi napoleona napoleonem napoleonie", Gender::MASCULINE},
  {U"filip filipa filipowi filipa filipem filipie", Gender::MASCULINE},
  {U"ferdynand ferdynanda ferdynandowi ferdynanda ferdynandem ferdynandzie", Gender::MASCULINE},
  {U"józef józefa józefowi józefa józefem józefie", Gender::MASCULINE},
  {U"franciszek franciszka franciszkowi franciszka franciszkiem franciszku", Gender::MASCULINE},
  {U"leopold leopolda leopoldowi leopolda leopoldem leopoldzie", Gender::MASCULINE},
  {U"klemens klemensa klemensowi klemensa klemensem klemensie", Gender::MASCULINE},
  {U"iwan iwana iwanowi iwana iwanem iwanie", Gender::MASCULINE},
  {U"elżbieta elżbiety elżbiecie elżbietę elżbietą elżbiecie", Gender::FEMININE},
  {U"katarzyna katarzyny katarzynie katarzynę katarzyną katarzynie", Gender::FEMININE},
};

// Nouns numbered by a Roman numeral before or after them, with their gender: rozdział IV (czwarty), XXI wieku
// (dwudziestego pierwszego), II wojna (druga). A form ending with "-" stands for every word it starts.
inline const std::vector<std::pair<std::u32string_view, Gender>> kRomanNouns = {
  {U"wiek-", Gender::MASCULINE}, {U"stuleci-", Gender::NEUTER}, {U"tysiącleci-", Gender::NEUTER},
  {U"wojn-", Gender::FEMININE}, {U"rzeczpospolit-", Gender::FEMININE}, {U"rzeczypospolit-", Gender::FEMININE},
  {U"kadencj-", Gender::FEMININE}, {U"edycj-", Gender::FEMININE}, {U"połow-", Gender::FEMININE},
  {U"kwartał-", Gender::MASCULINE}, {U"zjazd-", Gender::MASCULINE}, {U"kongres-", Gender::MASCULINE},
  {U"sesj-", Gender::FEMININE}, {U"festiwal-", Gender::MASCULINE}, {U"konkurs-", Gender::MASCULINE},
  {U"piętr-", Gender::NEUTER}, {U"kategori-", Gender::FEMININE}, {U"rozdzia-", Gender::MASCULINE},
  {U"liga", Gender::FEMININE}, {U"ligi", Gender::FEMININE}, {U"lidze", Gender::FEMININE}, {U"ligę", Gender::FEMININE},
  {U"klasa", Gender::FEMININE}, {U"klasy", Gender::FEMININE}, {U"klasie", Gender::FEMININE},
  {U"klasę", Gender::FEMININE}, {U"rok", Gender::MASCULINE}, {U"roku", Gender::MASCULINE},
  {U"stopnia", Gender::MASCULINE}, {U"stopień", Gender::MASCULINE}, {U"stopniu", Gender::MASCULINE},
  {U"tura", Gender::FEMININE}, {U"tury", Gender::FEMININE}, {U"turze", Gender::FEMININE}, {U"turę", Gender::FEMININE},
  {U"etap", Gender::MASCULINE}, {U"etapu", Gender::MASCULINE}, {U"etapie", Gender::MASCULINE},
  {U"faza", Gender::FEMININE}, {U"fazy", Gender::FEMININE}, {U"fazie", Gender::FEMININE}, {U"fazę", Gender::FEMININE},
  {U"grupa", Gender::FEMININE}, {U"grupy", Gender::FEMININE}, {U"grupie", Gender::FEMININE},
  {U"miejsce", Gender::NEUTER}, {U"miejsca", Gender::NEUTER}, {U"miejscu", Gender::NEUTER},
  {U"część", Gender::FEMININE}, {U"części", Gender::FEMININE}, {U"tom", Gender::MASCULINE}, {U"tomu", Gender::MASCULINE},
  {U"tomie", Gender::MASCULINE}, {U"sezon", Gender::MASCULINE}, {U"sezonu", Gender::MASCULINE},
  {U"sezonie", Gender::MASCULINE},
};

// XIX w.: the abbreviation of wiek after a Roman numeral
inline const Keys kCenturyAbbreviations = {U"w."};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U" \u00a0\u202f.", .decimal_separators = U",.", .time_separators = U":.",
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits)
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"kropka", .dot = U"kropka", .range = U"-", .by = U"na", .score = U" do ", .minus = U"minus", .plus = U"plus"
};

// 15.08.2020 r.: the year has four digits, "r." may follow it
inline const DateFormat kDateFormat = {.separators = U".", .boundaries = U".,", .short_year = false, .suffixes = {U"r."}};

// 600 123 456, 22 123 45 67 (after a word of kPhoneWords), 600-123-456, 68-324-22-55
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 3}, {3, 3}, {3, 3}}, .separators = U" ", .boundaries = U"", .closing = U""},
  {.groups = {{2, 2}, {3, 3}, {2, 2}, {2, 2}}, .separators = U" ", .boundaries = U"", .closing = U""},
  {.groups = {{3, 3}, {3, 3}, {3, 3}}, .separators = U"-", .boundaries = U".,-", .closing = U"-"},
  {.groups = {{2, 2}, {3, 3}, {2, 2}, {2, 2}}, .separators = U"-", .boundaries = U".,-", .closing = U"-"}
};

// Words before a phone number written with spaces, which else reads as a count: numer 600 123 456
inline const std::unordered_set<std::u32string> kPhoneWords = {
  U"numer", U"numerem", U"numeru", U"tel", U"telefon", U"telefonu", U"telefonem", U"infolinia", U"faks", U"fax"
};

// 2020 r.: a year with "roku" after it
inline const Keys kYearSuffixes = {U"r."};
inline const SuffixFormat kYearFormat = {.space = true, .sentence_dot = true};

using ::verbalis::text::Readings;

using ::verbalis::steps::Characters;

// Echo's classes, and Polish letters read as Latin-1 from Windows-1250 or ISO 8859-2: zupe³nie -> zupełnie
inline const Characters kCharacters = {
  {U"…",                U"..."}, // Ellipsis
  {U"«»„“”‟˝",          U"\""},  // Quotes
  {U"‘’‚‛´`ʼ",          U"'"},   // Apostrophes
  {U"​‌‍﻿", U""}, // Invisible: zero-width space, non-joiner, joiner, byte order mark
  {U"³", U"ł", true},   {U"£", U"Ł", true},   {U"¹±", U"ą", true},  {U"¥¡", U"Ą", true},
  {U"œ¶\u009c", U"ś", true}, {U"Œ¦\u008c", U"Ś", true}, {U"¿", U"ż", true}, {U"¯", U"Ż", true},
  {U"Ÿ¼\u009f", U"ź", true}, {U"\u008f¬", U"Ź", true}, {U"ê", U"ę", true}, {U"Ê", U"Ę", true},
  {U"æ", U"ć", true},   {U"Æ", U"Ć", true},   {U"ñ", U"ń", true},   {U"Ñ", U"Ń", true}
};

inline const Readings kSymbols = {
  {U"&", {U"end"}},        {U"+", {U"plus"}},           {U"=", {U"równa się"}}, {U"%", {U"procent"}},  {U"°", {U"stopni"}},
  {U"#", {U""}},           {U"/", {U""}},               {U"²", {U"do kwadratu"}}, {U"³", {U"do sześcianu"}},
  {U"π", {U"pi"}},         {U"×", {U"razy"}},           {U"€", {U"euro"}},     {U"$", {U"dolarów"}},   {U"@", {U"małpa"}},
  {U"±", {U"plus minus"}}, {U"‰", {U"promili"}},        {U"§", {U"paragraf"}}
};

using ::verbalis::text::before_name;
using ::verbalis::text::before_number;
using ::verbalis::text::Condition;
using ::verbalis::text::Context;
using ::verbalis::text::Reading;
using ::verbalis::steps::Terms;

// Prepositions before an inflected abbreviation, by the case they give it: przy ul. Długiej, od ul. Długiej
inline const std::unordered_set<std::u32string> kLocativeWords = {U"w", U"we", U"na", U"o", U"po", U"przy"};
inline const std::unordered_set<std::u32string> kGenitiveWords = {
  U"od", U"do", U"z", U"ze", U"u", U"obok", U"koło", U"około", U"ok", U"wzdłuż", U"dla", U"spod", U"sprzed", U"zza",
  U"naprzeciwko", U"blisko", U"niedaleko", U"wokół", U"bez", U"róg", U"rogu", U"okolicach", U"okolice",
  // A title after the name of a street or a patron: ul. ks. Popiełuszki, im. prof. Nowaka
  U"ul", U"al", U"pl", U"os", U"ulica", U"ulicy", U"aleja", U"alei", U"plac", U"placu", U"im", U"imienia", U"pw"};
inline const std::unordered_set<std::u32string> kInstrumentalWords = {U"pod", U"nad", U"przed", U"między", U"pomiędzy"};

/**
 * The case of an abbreviated noun: from the preposition before it, or the ending of the word after it.
 * skręć w ul. Długą (accusative), idź ul. Długą (instrumental), przy ul. Długiej (locative), z prof. Nowakiem
 * (instrumental), do dyr. Kowalskiego (genitive). A comma or a bracket right before it ends the phrase of the word
 * before: Camping Plus sp. z o. o., ul. Kuropatwia
 */
inline Case abbreviation_case(const Context& context) {
  std::u32string_view head = context.head();
  size_t last = head.find_last_not_of(U"  ");
  bool clause = last != std::u32string_view::npos && ::verbalis::text::strings::contains(std::u32string_view(U",;:()[]\""), head[last]);
  std::u32string before = clause ? U"" : context.before();
  std::u32string next = ::verbalis::text::strings::to_lower(context.after());
  if (next.ends_with(U"ą")) {
    return before == U"w" || before == U"we" || before == U"na" ? Case::ACCUSATIVE : Case::INSTRUMENTAL;
  }
  if (kLocativeWords.contains(before)) {
    return Case::LOCATIVE;
  }
  bool along = before == U"z" && context.before(2) == U"zgodnie"; // zgodnie z art. 5
  if (along || kInstrumentalWords.contains(before) || next.ends_with(U"em") || next.ends_with(U"im") ||
      next.ends_with(U"ym")) {
    return Case::INSTRUMENTAL;
  }
  if (next.ends_with(U"owi") || next.ends_with(U"emu")) {
    return Case::DATIVE;
  }
  if (kGenitiveWords.contains(before) || next.ends_with(U"ego") || next.ends_with(U"ej")) {
    return Case::GENITIVE;
  }
  return Case::NOMINATIVE;
}

// The readings of an abbreviated noun from its six forms (in the order of Case), separated by "|"; each one only when
// a condition holds, if any
inline std::vector<Reading> declined(std::u32string_view forms, Condition when = nullptr) {
  std::vector<Reading> readings;
  size_t start = forms.find(U'|') + 1;
  for (int c = 1; c < 6; ++c) {
    size_t end = std::min(forms.find(U'|', start), forms.size());
    readings.emplace_back(std::u32string(forms.substr(start, end - start)), [c, when](const Context& context) {
      return (!when || when(context)) && abbreviation_case(context) == static_cast<Case>(c);
    });
    start = end + 1;
  }
  readings.emplace_back(std::u32string(forms.substr(0, forms.find(U'|'))), when);
  return readings;
}

// The readings of an abbreviation which mostly ends a sentence, with its dot when a capitalized word follows: itd. Potem
inline std::vector<Reading> closing(std::u32string words) {
  return {{words + U".", before_name}, {words}};
}

// Between numbers: the "m." of mieszkanie in ul. Mickiewicza 3 m. 4
inline bool after_number_before_number(const Context& context) {
  std::u32string before = context.before();
  return before_number(context) && !before.empty() && ::verbalis::text::unicode::isdigit(before.back());
}

// The word after it ends with one of given endings (lowercase): św. Anny
inline Condition before_ending(std::vector<std::u32string> endings) {
  return [endings = std::move(endings)](const Context& context) {
    std::u32string next = ::verbalis::text::strings::to_lower(context.after());
    return std::ranges::any_of(endings, [&next](const auto& ending) { return next.ends_with(ending); });
  };
}

// The word before it is one of given words (lowercase): Msza św.
inline Condition after_word(std::unordered_set<std::u32string> words) {
  return [words = std::move(words)](const Context& context) { return words.contains(context.before()); };
}

// Saints whose names end with -a in the nominative (św. Anna), unlike the genitive of the others (św. Jana)
inline const std::unordered_set<std::u32string> kFemaleSaints = {
  U"anna", U"maria", U"barbara", U"teresa", U"katarzyna", U"jadwiga", U"faustyna", U"małgorzata", U"elżbieta",
  U"zofia", U"agnieszka", U"kinga", U"łucja", U"monika", U"cecylia", U"helena", U"weronika", U"rozalia", U"klara",
  U"joanna", U"magdalena", U"urszula", U"marta", U"brygida", U"apolonia", U"genowefa"
};

// św. after a noun it describes (Msza św.), then before the name of a saint, by its ending
inline const std::vector<Reading> kSaint = {
  {U"święta", after_word({U"msza"})},
  {U"świętej", after_word({U"mszy"})},
  {U"świętą", after_word({U"mszę", U"mszą"})},
  {U"święte", after_word({U"pismo"})},
  {U"świętego", after_word({U"pisma"})},
  {U"świętej", before_ending({U"y", U"i"})},
  {U"święta", [](const Context& context) { return kFemaleSaints.contains(::verbalis::text::strings::to_lower(context.after())); }},
  {U"świętego", before_ending({U"a", U"ego"})},
  {U"świętą", before_ending({U"ą", U"ę"})},
  {U"świętym", before_ending({U"em", U"im", U"ym"})},
  {U"świętemu", before_ending({U"owi", U"emu"})},
  {U"święty"}
};

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: nr 5
    {U"nr", {{U"numer", before_number}}},
    {U"Nr", {{U"numer", before_number}}},
    {U"nr.", {{U"numer", before_number}}},
    {U"ust.", {{U"ustęp", before_number}}},
    {U"pkt", {{U"punkt", before_number}}},
    {U"pkt.", {{U"punkt", before_number}}},
    {U"art.", declined(U"artykuł|artykułu|artykułowi|artykuł|artykułem|artykule", before_number)},
    {U"str.", declined(U"strona|strony|stronie|stronę|stroną|stronie", before_number)},
    {U"poz.", {{U"pozycja", before_number}}},
    {U"ok.", {{U"około", before_number}}},
    {U"Ok.", {{U"Około", before_number}}},
    {U"lok.", {{U"lokal", before_number}}},
    {U"m.", {{U"mieszkanie", after_number_before_number}}}, // ul. Mickiewicza 3 m. 4
    {U"#", {{U"numer", before_number}}},
    {U"§", declined(U"paragraf|paragrafu|paragrafowi|paragraf|paragrafem|paragrafie", before_number)},
    // Inflected nouns: przy ul. Długiej, z prof. Nowakiem
    {U"ul.", declined(U"ulica|ulicy|ulicy|ulicę|ulicą|ulicy")},
    {U"Ul.", declined(U"Ulica|Ulicy|Ulicy|Ulicę|Ulicą|Ulicy")},
    {U"al.", declined(U"aleja|alei|alei|aleję|aleją|alei", before_name)},
    {U"Al.", declined(U"Aleje|Alej|Alejom|Aleje|Alejami|Alejach", before_name)},
    {U"pl.", declined(U"plac|placu|placowi|plac|placem|placu", before_name)},
    {U"os.", declined(U"osiedle|osiedla|osiedlu|osiedle|osiedlem|osiedlu", before_name)},
    {U"godz.", declined(U"godzina|godziny|godzinie|godzinę|godziną|godzinie")},
    {U"Godz.", declined(U"Godzina|Godziny|Godzinie|Godzinę|Godziną|Godzinie")},
    {U"w godz.", {U"w godzinach"}}, // w godz. 8-16
    {U"W godz.", {U"W godzinach"}},
    {U"tel.", declined(U"telefon|telefonu|telefonowi|telefon|telefonem|telefonie")},
    {U"Tel.", declined(U"Telefon|Telefonu|Telefonowi|Telefon|Telefonem|Telefonie")},
    {U"dr", declined(U"doktor|doktora|doktorowi|doktora|doktorem|doktorze")},
    {U"dr.", declined(U"doktor|doktora|doktorowi|doktora|doktorem|doktorze")},
    {U"prof.", declined(U"profesor|profesora|profesorowi|profesora|profesorem|profesorze")},
    {U"Prof.", declined(U"Profesor|Profesora|Profesorowi|Profesora|Profesorem|Profesorze")},
    {U"Dr", declined(U"Doktor|Doktora|Doktorowi|Doktora|Doktorem|Doktorze")},
    {U"Ks.", declined(U"Ksiądz|Księdza|Księdzu|Księdza|Księdzem|Księdzu")},
    {U"Gen.", declined(U"Generał|Generała|Generałowi|Generała|Generałem|Generale")},
    {U"mgr", declined(U"magister|magistra|magistrowi|magistra|magistrem|magistrze")},
    {U"inż.", declined(U"inżynier|inżyniera|inżynierowi|inżyniera|inżynierem|inżynierze")},
    {U"dyr.", declined(U"dyrektor|dyrektora|dyrektorowi|dyrektora|dyrektorem|dyrektorze")},
    {U"gen.", declined(U"generał|generała|generałowi|generała|generałem|generale")},
    {U"płk", declined(U"pułkownik|pułkownika|pułkownikowi|pułkownika|pułkownikiem|pułkowniku")},
    {U"ks.", declined(U"ksiądz|księdza|księdzu|księdza|księdzem|księdzu")},
    {U"św.", kSaint},
    {U"hab.", {U"habilitowany"}},
    // tzw. by the ending of the noun after it; -ie is mostly the dative or locative of a feminine noun (strefie)
    {U"tzw.", {{U"tak zwana", before_ending({U"a"})}, {U"tak zwaną", before_ending({U"ą"})},
               {U"tak zwanego", before_ending({U"ego", U"u"})}, {U"tak zwanej", before_ending({U"ej", U"ie"})},
               {U"tak zwanych", before_ending({U"ych", U"ów"})}, {U"tak zwanym", before_ending({U"ym"})},
               {U"tak zwane", before_ending({U"e", U"o"})}, {U"tak zwany"}}},
    // Anywhere
    {U"np.", {U"na przykład"}},
    {U"Np.", {U"Na przykład"}},
    {U"itd.", closing(U"i tak dalej")},
    {U"itp.", closing(U"i tym podobne")},
    {U"tzn.", {U"to znaczy"}},
    {U"tj.", {U"to jest"}},
    {U"m.in.", {U"między innymi"}},
    {U"M.in.", {U"Między innymi"}},
    {U"wg", {U"według"}},
    {U"Wg", {U"Według"}},
    {U"wg.", {U"według"}},
    {U"Wg.", {U"Według"}},
    {U"ds.", {U"do spraw"}},
    {U"im.", {{U"imienia", before_name}}},
    {U"pt.", {{U"pod tytułem", before_name}}},
    {U"nt.", {U"na temat"}},
    {U"ws.", {U"w sprawie"}},
    {U"pw.", {U"pod wezwaniem"}},
    {U"zob.", {U"zobacz"}},
    {U"Zob.", {U"Zobacz"}},
    {U"dn.", {U"dnia"}},
    {U"br.", closing(U"bieżącego roku")},
    {U"ub.r.", closing(U"ubiegłego roku")},
    {U"ub. r.", closing(U"ubiegłego roku")},
    {U"p.n.e.", closing(U"przed naszą erą")},
    {U"n.e.", closing(U"naszej ery")},
    {U"nr tel.", {U"numer telefonu"}},
    {U"Nr tel.", {U"Numer telefonu"}},
    {U"tel. kom.", {U"telefon komórkowy"}},
    {U"Tel. kom.", {U"Telefon komórkowy"}},
    {U"sp. z o.o.", closing(U"spółka z ograniczoną odpowiedzialnością")},
    {U"Sp. z o.o.", closing(U"Spółka z ograniczoną odpowiedzialnością")},
    {U"sp. z o. o.", closing(U"spółka z ograniczoną odpowiedzialnością")},
    {U"Sp. z o. o.", closing(U"Spółka z ograniczoną odpowiedzialnością")},
    {U"z o.o.", closing(U"z ograniczoną odpowiedzialnością")},
    {U"S.A.", closing(U"Spółka Akcyjna")},
  },
  .web = {.dot = U"kropka", .at = U"małpa", .slash = U"ukośnik", .hyphen = U"myślnik", .underscore = U"podkreślnik",
          .digits = {U"zero", U"jeden", U"dwa", U"trzy", U"cztery", U"pięć", U"sześć", U"siedem", U"osiem", U"dziewięć"}}
};

} // namespace verbalis::languages::pl
