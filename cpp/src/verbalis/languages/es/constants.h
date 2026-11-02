#pragma once

#include <verbalis/languages/grammar.h>
#include <verbalis/numbers/formats.h>
#include <verbalis/steps/terms.h>
#include <verbalis/text/readings.h>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace verbalis::languages::es {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::RomanCues;
using ::verbalis::numbers::SuffixFormat;

// Up to 29, each a single word
inline constexpr std::array<std::u32string_view, 30> kOnes = {
  U"cero",       U"uno",        U"dos",         U"tres",        U"cuatro",       U"cinco",
  U"seis",       U"siete",      U"ocho",        U"nueve",       U"diez",         U"once",
  U"doce",       U"trece",      U"catorce",     U"quince",      U"dieciséis",    U"diecisiete",
  U"dieciocho",  U"diecinueve", U"veinte",      U"veintiuno",   U"veintidós",    U"veintitrés",
  U"veinticuatro", U"veinticinco", U"veintiséis", U"veintisiete", U"veintiocho", U"veintinueve"
};

inline constexpr std::array<std::u32string_view, 10> kTens = {
  U"", U"", U"", U"treinta", U"cuarenta", U"cincuenta", U"sesenta", U"setenta", U"ochenta", U"noventa"
};

inline constexpr std::array<std::u32string_view, 10> kHundreds = {
  U"",           U"ciento",      U"doscientos",  U"trescientos", U"cuatrocientos",
  U"quinientos", U"seiscientos", U"setecientos", U"ochocientos", U"novecientos"
};

// Ordinal stems, completed by -o or -a: centésima trigésima séptima
inline constexpr std::array<std::u32string_view, 10> kOrdinalOnes = {
  U"", U"primer", U"segund", U"tercer", U"cuart", U"quint", U"sext", U"séptim", U"octav", U"noven"
};
inline constexpr std::array<std::u32string_view, 10> kOrdinalTens = {
  U"",           U"décim",      U"vigésim",     U"trigésim",   U"cuadragésim",
  U"quincuagésim", U"sexagésim", U"septuagésim", U"octogésim", U"nonagésim"
};
inline constexpr std::array<std::u32string_view, 10> kOrdinalHundreds = {
  U"",             U"centésim",      U"ducentésim",     U"tricentésim",   U"cuadringentésim",
  U"quingentésim", U"sexcentésim",   U"septingentésim", U"octingentésim", U"noningentésim"
};

inline const std::vector<std::u32string> kMonths = {
  U"enero", U"febrero", U"marzo",      U"abril",   U"mayo",      U"junio",
  U"julio", U"agosto",  U"septiembre", U"octubre", U"noviembre", U"diciembre"
};

// Currencies by symbol and by code, as written: upper case, or lower case after a number (5 usd)
// Months by their names and abbreviations, lowercase: 5 febrero, abr. 22
inline const std::unordered_map<std::u32string, uint64_t> kMonthNumbers = {
  {U"enero", 1},     {U"febrero", 2},  {U"marzo", 3},      {U"abril", 4},      {U"mayo", 5},   {U"junio", 6},
  {U"julio", 7},     {U"agosto", 8},   {U"septiembre", 9}, {U"setiembre", 9},  {U"octubre", 10},
  {U"noviembre", 11}, {U"diciembre", 12}, {U"ene.", 1},    {U"feb.", 2},       {U"mar.", 3},   {U"abr.", 4},
  {U"may.", 5},      {U"jun.", 6},     {U"jul.", 7},       {U"ago.", 8},       {U"sep.", 9},   {U"sept.", 9},
  {U"set.", 9},      {U"oct.", 10},    {U"nov.", 11},      {U"dic.", 12}
};

inline const std::unordered_map<std::u32string, Forms> kCurrencies = {
  {U"$", {U"dólar", U"dólares"}},           {U"€", {U"euro", U"euros"}},          {U"£", {U"libra", U"libras"}},
  {U"¥", {U"yen", U"yenes"}},               {U"₩", {U"won", U"wones"}},           {U"₡", {U"colón", U"colones"}},
  {U"₧", {U"peseta", U"pesetas"}},          {U"R$", {U"real", U"reales"}},        {U"r$", {U"real", U"reales"}},
  {U"US$", {U"dólar", U"dólares"}},         {U"us$", {U"dólar", U"dólares"}},
  {U"USD", {U"dólar estadounidense", U"dólares estadounidenses"}},
  {U"usd", {U"dólar estadounidense", U"dólares estadounidenses"}},
  {U"MEX$", {U"peso mexicano", U"pesos mexicanos"}}, {U"mex$", {U"peso mexicano", U"pesos mexicanos"}},
  {U"MXN", {U"peso mexicano", U"pesos mexicanos"}},  {U"mxn", {U"peso mexicano", U"pesos mexicanos"}},
  {U"EUR", {U"euro", U"euros"}},            {U"eur", {U"euro", U"euros"}},
  {U"PLN", {U"esloti", U"eslotis"}},       {U"GBP", {U"libra", U"libras"}},          {U"JPY", {U"yen", U"yenes"}},         {U"jpy", {U"yen", U"yenes"}},
  {U"CRC", {U"colón costarricense", U"colones costarricenses"}},
  {U"crc", {U"colón costarricense", U"colones costarricenses"}},
  {U"NIO", {U"córdoba", U"córdobas"}},      {U"nio", {U"córdoba", U"córdobas"}},
  {U"PAB", {U"balboa", U"balboas"}},        {U"pab", {U"balboa", U"balboas"}},
  {U"HNL", {U"lempira", U"lempiras"}},      {U"hnl", {U"lempira", U"lempiras"}},
  {U"GTQ", {U"quetzal", U"quetzales"}},     {U"gtq", {U"quetzal", U"quetzales"}},
  {U"Q", {U"quetzal", U"quetzales"}},       {U"q", {U"quetzal", U"quetzales"}},
  {U"BRL", {U"real brasileño", U"reales brasileños"}}, {U"brl", {U"real brasileño", U"reales brasileños"}},
  {U"BOP", {U"peso boliviano", U"pesos bolivianos"}},  {U"bop", {U"peso boliviano", U"pesos bolivianos"}},
  {U"CHF", {U"franco suizo", U"francos suizos"}},      {U"chf", {U"franco suizo", U"francos suizos"}},
  {U"HKD", {U"dólar de Hong Kong", U"dólares de Hong Kong"}},
  {U"hkd", {U"dólar de Hong Kong", U"dólares de Hong Kong"}},
  {U"TTD", {U"dólar de Trinidad y Tobago", U"dólares de Trinidad y Tobago"}},
  {U"ttd", {U"dólar de Trinidad y Tobago", U"dólares de Trinidad y Tobago"}},
  {U"AWG", {U"florín arubeño", U"florines arubeños"}}, {U"awg", {U"florín arubeño", U"florines arubeños"}},
  {U"Rs", {U"rupia", U"rupias"}},           {U"rs", {U"rupia", U"rupias"}},
  {U"kr", {U"corona", U"coronas"}},         {U"mk", {U"marco", U"marcos"}},
  {U"Fr", {U"franco", U"francos"}},         {U"fr", {U"franco", U"francos"}},
  {U"ptas", {U"peseta", U"pesetas"}},       {U"pta", {U"peseta", U"pesetas"}},       {U"Ptas", {U"peseta", U"pesetas"}},
  {U"RD$", {U"peso dominicano", U"pesos dominicanos"}}, {U"U$S", {U"dólar", U"dólares"}},
  {U"¢", {U"colón", U"colones"}},           {U"N$", {U"nuevo peso", U"nuevos pesos"}},
  {U"Bs", {U"bolívar", U"bolívares"}},
  // Millions of pesos and of dollars, read as a currency: 2 mdp
  {U"mdp", {U"millón de pesos", U"millones de pesos"}}, {U"mdd", {U"millón de dólares", U"millones de dólares"}}
};

// Currency codes which also precede an amount: USD 5, q50,10 (Q1 to Q9 are quarters)
inline const Keys kCurrencyWords = {
  U"USD", U"usd", U"EUR", U"eur", U"MXN", U"mxn", U"CRC", U"crc", U"NIO", U"nio", U"PAB", U"pab", U"HNL", U"hnl",
  U"GTQ", U"gtq", U"Q",   U"q",   U"BRL", U"brl", U"BOP", U"bop", U"CHF", U"chf", U"HKD", U"hkd", U"TTD", U"ttd",
  U"AWG", U"awg", U"Rs",  U"rs",  U"Fr",  U"fr",  U"JPY", U"jpy", U"GBP"
};

// The hundredth of a currency, read alone or after the units: $0,55, cincuenta y cinco centavos
inline const std::unordered_map<std::u32string, Forms> kCents = {
  {U"$", {U"centavo", U"centavos"}},  {U"US$", {U"centavo", U"centavos"}}, {U"us$", {U"centavo", U"centavos"}},
  {U"USD", {U"centavo", U"centavos"}}, {U"usd", {U"centavo", U"centavos"}}, {U"MEX$", {U"centavo", U"centavos"}},
  {U"mex$", {U"centavo", U"centavos"}}, {U"MXN", {U"centavo", U"centavos"}}, {U"mxn", {U"centavo", U"centavos"}},
  {U"€", {U"céntimo", U"céntimos"}},  {U"EUR", {U"céntimo", U"céntimos"}}, {U"eur", {U"céntimo", U"céntimos"}},
  {U"£", {U"penique", U"peniques"}},  {U"GBP", {U"penique", U"peniques"}}, {U"₡", {U"céntimo", U"céntimos"}},
  {U"Q", {U"centavo", U"centavos"}},  {U"q", {U"centavo", U"centavos"}},   {U"GTQ", {U"centavo", U"centavos"}},
  {U"gtq", {U"centavo", U"centavos"}}, {U"R$", {U"centavo", U"centavos"}}, {U"r$", {U"centavo", U"centavos"}}
};

// Each scale by its singular: millones -> millón
inline const std::unordered_map<std::u32string, std::u32string> kScales = {
  {U"millones", U"millón"}, {U"millón", U"millón"}, {U"billones", U"billón"}, {U"billón", U"billón"},
  {U"mil", U"mil"},         {U"k", U"mil"},         {U"K", U"mil"},         {U"mn", U"millón"}
};

inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"″", {U"pulgada", U"pulgadas"}},
  {U"km", {U"kilómetro", U"kilómetros"}},         {U"m", {U"metro", U"metros"}},
  {U"cm", {U"centímetro", U"centímetros"}},       {U"mm", {U"milímetro", U"milímetros"}},
  {U"nm", {U"nanómetro", U"nanómetros"}},         {U"m²", {U"metro cuadrado", U"metros cuadrados"}},
  {U"km²", {U"kilómetro cuadrado", U"kilómetros cuadrados"}}, {U"m³", {U"metro cúbico", U"metros cúbicos"}},
  {U"ha", {U"hectárea", U"hectáreas"}},           {U"kg", {U"kilogramo", U"kilogramos"}},
  {U"g", {U"gramo", U"gramos"}},                  {U"mg", {U"miligramo", U"miligramos"}},
  {U"t", {U"tonelada", U"toneladas"}},            {U"l", {U"litro", U"litros"}},
  {U"L", {U"litro", U"litros"}},                  {U"ml", {U"mililitro", U"mililitros"}},
  {U"TB", {U"terabyte", U"terabytes"}},           {U"GB", {U"gigabyte", U"gigabytes"}},
  {U"MB", {U"megabyte", U"megabytes"}},           {U"KB", {U"kilobyte", U"kilobytes"}},
  {U"Hz", {U"hercio", U"hercios"}},               {U"kHz", {U"kilohercio", U"kilohercios"}},
  {U"MHz", {U"megahercio", U"megahercios"}},      {U"GHz", {U"gigahercio", U"gigahercios"}},
  {U"W", {U"vatio", U"vatios"}},                  {U"kW", {U"kilovatio", U"kilovatios"}},
  {U"V", {U"voltio", U"voltios"}},                {U"km/h", {U"kilómetro por hora", U"kilómetros por hora"}},
  {U"MPH", {U"milla por hora", U"millas por hora"}}, {U"mph", {U"milla por hora", U"millas por hora"}},
  {U"°C", {U"grado Celsius", U"grados Celsius"}}, {U"°F", {U"grado Fahrenheit", U"grados Fahrenheit"}}, {U"°", {U"grado", U"grados"}},
  {U"° C", {U"grado Celsius", U"grados Celsius"}}, {U"° F", {U"grado Fahrenheit", U"grados Fahrenheit"}},
  {U"°c", {U"grado centígrado", U"grados centígrados"}}, {U"° c", {U"grado centígrado", U"grados centígrados"}},
  {U"K", {U"kelvin", U"kelvin"}},                 {U"°K", {U"kelvin", U"kelvin"}},  {U"° K", {U"kelvin", U"kelvin"}},
  {U"°k", {U"grado kelvin", U"grados kelvin"}},   {U"° k", {U"grado kelvin", U"grados kelvin"}},
  {U"s", {U"segundo", U"segundos"}},              {U"seg", {U"segundo", U"segundos"}},
  {U"min", {U"minuto", U"minutos"}},              {U"/s", {U"por segundo", U"por segundo"}},
  {U"kph", {U"kilómetro por hora", U"kilómetros por hora"}}, {U"psi", {U"psi", U"psi"}},
  {U"rpm", {U"revolución por minuto", U"revoluciones por minuto"}},
  {U"Km", {U"kilómetro", U"kilómetros"}},         {U"mts", {U"metro", U"metros"}},
  {U"cms", {U"centímetro", U"centímetros"}},      {U"m2", {U"metro cuadrado", U"metros cuadrados"}},
  {U"oz", {U"onza", U"onzas"}},                 {U"kWh", {U"kilovatio hora", U"kilovatios hora"}},
  {U"hp", {U"caballo de fuerza", U"caballos de fuerza"}}, {U"℃", {U"grado centígrado", U"grados centígrados"}},
  {U"gb", {U"gigabyte", U"gigabytes"}},           {U"px", {U"píxel", U"píxeles"}},
  {U"MP", {U"megapíxel", U"megapíxeles"}},        {U"Mpx", {U"megapíxel", U"megapíxeles"}},
  {U"pts", {U"punto", U"puntos"}}
};

// An inch after a fraction: 3/4"
inline const Keys kInchMarks = {U"\"", U"''", U"″", U"”"};
inline const Forms& kInch = kUnits.at(U"″");

// Words of a text about time, which make its 7'11" minutes and seconds, and a lone 25″ seconds
inline const std::unordered_set<std::u32string> kPrimeCues = {
  U"tiempo", U"carrera", U"crono", U"cronómetro", U"récord", U"ganador", U"general", U"etapa", U"ventaja", U"vuelta",
  U"minuto", U"minutos", U"segundos", U"maratón", U"clasificación", U"podio", U"corrió", U"correr", U"marca"
};

// Words around a lone 10' which make it minutes: durante más de 10', (36' PT), (49' ST)
inline const std::unordered_set<std::u32string> kMinuteCues = {U"durante", U"pt", U"st"};

// Words around a 5'5" which make it feet and inches: mide 5'5"
inline const std::unordered_set<std::u32string> kHeightCues = {U"mide", U"altura", U"estatura", U"alto", U"alta"};

// A time written with prime marks: hours, minutes, and seconds; and a height, in feet and inches
inline const std::unordered_map<Prime, Forms> kPrimeUnits = {
  {Prime::HOUR, {U"hora", U"horas"}}, {Prime::MINUTE, {U"minuto", U"minutos"}}, {Prime::SECOND, {U"segundo", U"segundos"}},
  {Prime::FOOT, {U"pie", U"pies"}},   {Prime::INCH, kInch},
};

// Words after a fraction which mark it as one: el 1/3 de
inline const std::unordered_set<std::u32string> kFractionWords = {U"de", U"del"};

// Words before a day and month which mark them as a date: el 3/5, del 3/5 al 7/5 (a bare 3/5 is a fraction)
inline const std::unordered_set<std::u32string> kDateWords = {
  U"el", U"del", U"al", U"desde", U"hasta", U"día", U"fecha", U"lunes", U"martes", U"miércoles", U"jueves",
  U"viernes", U"sábado", U"domingo"
};

// Words before the number of a law with its year: Ley 2/2018
inline const std::unordered_set<std::u32string> kLaws = {
  U"ley", U"decreto", U"legislativo", U"sentencia", U"resolución", U"declaración", U"orden", U"acuerdo",
  U"circular", U"reglamento", U"directiva", U"constitucionalidad", U"expediente", U"norma", U"instrucción"
};

// Words before a time with a dot, which make it a time: a las 18.30 (and "a" before a quarter hour: de 9.00 a 13.30)
inline const std::unordered_set<std::u32string> kTimeWords = {U"las", U"la"};

// After a time: 19 h, 16:00 hrs. (horas), 5:02 p. m. (de la tarde)
inline const Keys kHourSuffixes = {U"h", U"h.", U"hrs", U"hrs.", U"Hrs", U"Hrs.", U"hs", U"hs."};
inline const Keys kMorningSuffixes = {U"a. m.", U"a.m.", U"a.m", U"am"};
inline const Keys kEveningSuffixes = {U"p. m.", U"p.m.", U"p.m", U"pm"};

// Nouns in -a, -ión, -dad, ... are feminine unless listed below; other feminine nouns are listed
inline const std::unordered_set<std::u32string> kMasculineInA = {
  U"día", U"dia", U"mapa", U"problema", U"sistema", U"programa", U"tema", U"idioma", U"clima", U"planeta", U"poema",
  U"dilema", U"drama", U"esquema", U"sofá", U"papá", U"cometa", U"fantasma", U"panorama", U"diagrama", U"teorema",
  U"córdoba", U"lempira", U"balboa"
};
inline const std::unordered_set<std::u32string> kFeminineNouns = {
  U"vez", U"veces", U"mano", U"foto", U"moto", U"radio", U"mujer", U"flor", U"noche", U"clase", U"parte",
  U"gente", U"leche", U"muerte", U"suerte", U"calle", U"carne", U"red", U"voz", U"luz", U"paz", U"ley", U"piel",
  U"fase", U"base", U"frase", U"nube", U"llave", U"fuente", U"mente", U"torre", U"tarde", U"serie", U"especie",
  U"superficie", U"imagen", U"orden", U"edición"
};
inline const std::vector<std::u32string> kFeminineEndings = {U"a", U"ión", U"dad", U"tad", U"tud", U"umbre", U"sis", U"itis"};

// Words before a year: en 1971 publicó
inline const std::unordered_set<std::u32string> kYearWords = {U"en", U"de", U"del", U"desde", U"hasta", U"año"};

// Words after a number which are not the noun it counts
inline const std::unordered_set<std::u32string> kStopWords = {
  U"de", U"del", U"en", U"con", U"por", U"para", U"sin", U"que", U"y", U"e", U"o", U"u", U"a", U"al", U"los",
  U"las", U"el", U"la", U"lo", U"un", U"una", U"unos", U"unas", U"es", U"son", U"era", U"sea", U"pero", U"como",
  U"más", U"menos", U"hasta", U"desde", U"entre", U"sobre", U"contra", U"hacia", U"tras", U"según", U"durante",
  U"mediante", U"cada", U"toda", U"todas", U"todo", U"todos", U"esta", U"estas", U"este", U"estos", U"esa",
  U"esas", U"ese", U"esos", U"otra", U"otras", U"otro", U"otros", U"nada", U"ya", U"ahora", U"cuando", U"donde",
  U"mientras", U"muy", U"casi", U"también", U"así", U"se", U"le", U"les", U"me", U"te", U"nos", U"esquina"
};

// Felipe VI (sexto), Alfonso XIII (trece), siglo XXI (veintiuno), capítulo IV (cuatro), la II Guerra (Segunda)
inline const RomanCues kRomanCues = {
  .names = {
    U"felipe", U"juan", U"carlos", U"alfonso", U"fernando", U"isabel", U"enrique", U"pedro", U"jaime", U"sancho",
    U"luis", U"benedicto", U"pío", U"pablo", U"león", U"gregorio", U"clemente", U"inocencio", U"eduardo", U"jorge",
    U"guillermo", U"napoleón", U"ramsés", U"juana", U"maría", U"catalina", U"leonor", U"victoria"
  },
  .before = {
    U"capítulo", U"tomo", U"libro", U"acto", U"título", U"parte", U"volumen", U"temporada", U"fase", U"siglo", U"año",
    U"siglos", U"vaticano", U"escena", U"episodio", U"nivel", U"número", U"distrito", U"región", U"tipo", U"clase"
  },
  .after = {
    U"guerra", U"congreso", U"encuentro", U"festival", U"edición", U"jornada", U"jornadas", U"bienal", U"feria",
    U"cumbre", U"conferencia", U"asamblea", U"concurso", U"foro", U"simposio", U"seminario", U"campeonato",
    U"olimpiada", U"república", U"legislatura", U"aniversario", U"centenario", U"reunión", U"convención", U"gira",
    U"coloquio", U"censo", U"plan", U"informe", U"premio", U"torneo", U"salón", U"semana", U"carrera", U"marcha"
  }
};

// Names with a regnal number, and nouns after a numeral, which are feminine: Isabel II (segunda), la II Guerra
inline const std::unordered_set<std::u32string> kFeminineNames = {
  U"isabel", U"juana", U"maría", U"catalina", U"leonor", U"victoria"
};
inline const std::unordered_set<std::u32string> kFeminineRomanNouns = {
  U"guerra", U"edición", U"jornada", U"jornadas", U"bienal", U"feria", U"cumbre", U"conferencia", U"asamblea",
  U"olimpiada", U"república", U"legislatura", U"reunión", U"convención", U"gira", U"semana", U"carrera", U"marcha"
};

// --- Notation: how numbers are written ---
// What an amount is counted by after a slash (por): 12 €/kg, doce euros por kilogramo
inline const std::unordered_map<std::u32string, std::u32string> kPer = {
  {U"h", U"por hora"}, {U"hora", U"por hora"}, {U"min", U"por minuto"}, {U"s", U"por segundo"},
  {U"seg", U"por segundo"}, {U"día", U"por día"}, {U"semana", U"por semana"}, {U"mes", U"por mes"},
  {U"año", U"por año"}, {U"a", U"por año"}, {U"kg", U"por kilogramo"}, {U"g", U"por gramo"}, {U"t", U"por tonelada"},
  {U"l", U"por litro"}, {U"L", U"por litro"}, {U"ml", U"por mililitro"}, {U"km", U"por kilómetro"},
  {U"m", U"por metro"}, {U"m²", U"por metro cuadrado"}, {U"m2", U"por metro cuadrado"}, {U"m³", U"por metro cúbico"},
  {U"m3", U"por metro cúbico"}, {U"ha", U"por hectárea"}, {U"kWh", U"por kilovatio hora"},
  {U"MWh", U"por megavatio hora"}, {U"oz", U"por onza"}, {U"persona", U"por persona"}
};

inline const Notation kNotation = {
  .group_separators = U"., \u00a0\u202f", .decimal_separators = U".,", .time_separators = U":.",
  .time_suffixes = {U"h", U"h.", U"hrs", U"hrs.", U"Hrs", U"Hrs.", U"hs", U"hs.", U"a. m.", U"a.m.", U"a.m", U"am",
                    U"p. m.", U"p.m.", U"p.m", U"pm"},
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true,
  .per = kPer
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"punto", .dot = U"punto", .range = U"-", .by = U"por", .score = U" a ", .minus = U"menos", .plus = U"más",
  .fraction = U" y ", .half = U" y medio", .rating = U"sobre"
};

// Starts of words of sport, which make a falling or tied range a score: ganó 2-0, empató 1-1 (dos a cero, uno a uno)
inline const Keys kSportWords = {
  U"partido", U"gol", U"marcador", U"ganó", U"ganar", U"ganaron", U"venci", U"vencer", U"empat", U"derrot",
  U"victoria", U"triunf", U"impus", U"impone", U"cayó", U"superó", U"encuentro", U"jornada", U"torneo", U"liga",
  U"selección", U"canasta", U"anotación", U"clasificatoria", U"duelo", U"fútbol"
};

// 14/07/2023, 31/12/99, 15/06
inline const DateFormat kDateFormat = {.separators = U"/-.", .boundaries = U".,/-", .year_required = false,
                                         .months = kMonths};

// 600-123-456, 600 123 456 (after a word of kPhoneWords)
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 3}, {3, 3}, {3, 3}}, .separators = U"-", .boundaries = U".,-", .closing = U"-"},
  {.groups = {{3, 3}, {3, 3}, {3, 3}}, .separators = U" ", .boundaries = U".,-", .closing = U"-"},
  // 123-123-5678, 1234-5678: Latin American numbers
  {.groups = {{3, 3}, {3, 3}, {4, 4}}, .separators = U"-", .boundaries = U".,-", .closing = U"-"},
  {.groups = {{4, 4}, {4, 4}}, .separators = U"-", .boundaries = U".,-/", .closing = U"-/"}
};

// Words before a phone number written with spaces, which else reads as numbers: llame al 600 123 456
inline const std::unordered_set<std::u32string> kPhoneWords = {
  U"al", U"teléfono", U"tel", U"móvil", U"número", U"llame", U"llamar", U"fax", U"whatsapp", U"contacto"
};

// 1º, 2 ª, 4.º, 1.ª, 1.ᵉʳ, 1.°: a sign, possibly after a space or a dot; 2° is an ordinal only by its words
inline const Keys kOrdinalSigns = {U"º", U"ª", U".º", U".ª", U"ᵉʳ", U".ᵉʳ", U".°", U"°"};

// Articles before 2° which make it an ordinal (el 2° lugar), not degrees (a 30°), and the feminine ones (la 6°)
inline const std::unordered_set<std::u32string> kOrdinalArticles = {
  U"el", U"la", U"del", U"al", U"su", U"este", U"esta", U"nuestro",
  U"nuestra"
};
inline const std::unordered_set<std::u32string> kFeminineOrdinalArticles = {
  U"la", U"esta", U"nuestra"
};
// Nouns after 2° which make it an ordinal: 2° lugar, 1° piso
inline const std::unordered_set<std::u32string> kOrdinalNouns = {
  U"lugar", U"puesto", U"piso", U"fecha", U"aniversario", U"edición", U"festival", U"congreso",
  U"concurso", U"encuentro", U"jornada", U"bono", U"año", U"semestre", U"trimestre", U"cuatrimestre",
  U"bimestre", U"nivel", U"torneo", U"campeonato", U"capítulo", U"lote", U"premio", U"etapa",
  U"vuelta", U"ronda", U"sección", U"distrito", U"juzgado", U"batallón", U"regimiento", U"grado",
  U"día", U"semana", U"período", U"periodo", U"ciclo", U"curso", U"tiempo", U"round",
  U"foro", U"seminario", U"simposio", U"salón", U"certamen", U"convención", U"cumbre", U"sesión",
  U"división", U"informe"
};

inline const SuffixFormat kOrdinalSignFormat = {.space = true, .word_end = false};

// 1er, 1.er, 3ero, 2da, 5ta
inline const Keys kOrdinalSuffixes = {
  U"er", U".er", U"ero", U"era", U"ro", U"ra", U"do", U"da", U"to", U"ta", U"vo", U"va", U"mo", U"ma", U"no", U"na"
};

// 80s, 90's
inline const Keys kDecadeSuffixes = {U"s"};
inline const SuffixFormat kDecadeFormat = {.apostrophes = true, .boundaries = U".,-"};

using ::verbalis::text::Readings;
using ::verbalis::text::after_name;

// "+" is "plus" after a name (Google+), "más" otherwise (C++)
using ::verbalis::text::before_value;

inline const Readings kSymbols = {
  {U"&", {U"y"}},       {U"=", {U"igual a"}},     {U"@", {U"arroba"}},  {U"%", {U"por ciento"}}, {U"°", {U"grados"}},
  {U"#", {U"numeral"}}, {U"²", {U"al cuadrado"}}, {U"³", {U"al cubo"}}, {U"π", {U"pi"}},         {U"×", {U"por"}},
  {U"€", {U"euros"}},   {U"$", {U"dólares"}},     {U"£", {U"libras"}},  {U"+", {{U"plus", after_name}, U"más"}},
  {U"⅓", {U"un tercio"}},   {U"⅔", {U"dos tercios"}},   {U"¼", {U"un cuarto"}},
  {U"¾", {U"tres cuartos"}}, {U"⅕", {U"un quinto"}}, {U"⅖", {U"dos quintos"}}, {U"⅗", {U"tres quintos"}},
  {U"⅘", {U"cuatro quintos"}}, {U"⅙", {U"un sexto"}}, {U"⅚", {U"cinco sextos"}}, {U"⅛", {U"un octavo"}},
  {U"⅜", {U"tres octavos"}}, {U"⅝", {U"cinco octavos"}}, {U"⅞", {U"siete octavos"}},
  {U"±", {U"más menos"}},    {U"+/-", {U"más menos"}},   {U"‰", {U"por mil"}},
  // Only before a number: p<0.05, ~$145, but <i>, ->, Año~
  {U"<", {{U"menor que", before_value}}},
  {U">", {{U"mayor que", before_value}}},
  {U"≤", {{U"menor o igual que", before_value}}},
  {U"≥", {{U"mayor o igual que", before_value}}},
  {U"~", {{U"aproximadamente", before_value}}},
  {U"≈", {{U"aproximadamente", before_value}}}
};

using ::verbalis::text::before_number;
using ::verbalis::text::before_name;
using ::verbalis::text::Context;
using ::verbalis::steps::Terms;

// After a year or a century, as digits or a Roman numeral: 10.000 a. C., siglo x ec
inline bool after_year(const Context& context) {
  std::u32string word = context.before();
  bool digits = !word.empty() && word.find_first_not_of(U"0123456789.") == std::u32string::npos;
  bool roman = !word.empty() && word.find_first_not_of(U"ivxlcdm") == std::u32string::npos;
  return digits || roman;
}

// After a clock time: 1.00 est, 2:00 pm gmt-4:30, 10 h utc+1:30
inline bool after_time(const Context& context) {
  static const std::unordered_set<std::u32string> suffixes = {U"am", U"pm", U"a.m", U"p.m", U"a.m.", U"p.m.", U"h",
                                                              U"hrs", U"horas"};
  std::u32string word = context.before();
  return (!word.empty() && word.find_first_not_of(U"0123456789.:") == std::u32string::npos) || suffixes.contains(word);
}

// Time zones, in capitals after a clock time, with the sign of an offset: gmt-3 GMT menos tres
inline const std::vector<std::u32string> kTimeZones = {U"gmt", U"utc", U"est", U"pst", U"cst", U"mst",
                                                       U"edt", U"pdt", U"cet", U"cest"};

// At the start of a sentence: da. ana pérez (not the verb of "se lo da.")
inline bool sentence_start(const Context& context) {
  std::u32string_view head = context.head();
  size_t end = head.find_last_not_of(U' ');
  return end == std::u32string_view::npos || std::u32string_view(U".!?:;").find(head[end]) != std::u32string_view::npos;
}

// Before a name or at the start of a sentence: Da. Ana
inline bool before_title(const Context& context) {
  return before_name(context) || sentence_start(context);
}

inline const Terms kTermsBase = {
  .abbreviations = {
    // Before a number only: Nº 5
    {U"Nº", {{U"número", before_number}}},
    {U"ext.", {{U"extensión", before_number}}},
    {U"Ext.", {{U"extensión", before_number}}},
    {U"nº", {{U"número", before_number}}},
    {U"N°", {{U"número", before_number}}},
    {U"n°", {{U"número", before_number}}},
    {U"n.°", {{U"número", before_number}}},
    {U"N.°", {{U"número", before_number}}},
    {U"n.º", {{U"número", before_number}}},
    {U"N.º", {{U"número", before_number}}},
    {U"núm.", {{U"número", before_number}}},
    {U"pág.", {{U"página", before_number}}},
    {U"art.", {{U"artículo", before_number}}},
    {U"Núm.", {{U"número", before_number}}},
    {U"Pág.", {{U"página", before_number}}},
    {U"pp", {{U"páginas", before_number}}},
    {U"Km.", {{U"kilómetro", before_number}}},
    {U"Tel.", {{U"teléfono", before_number}}},
    {U"No.", {{U"número", before_number}}},
    {U"p.", {{U"página", before_number}}},
    {U"Av.", {{U"avenida", before_name}, {U"avenida", before_number}}},
    {U"Col.", {{U"colonia", before_name}}},
    // Before a day: vier. 10 de mayo
    {U"lun.", {{U"lunes", before_number}}},
    {U"mié.", {{U"miércoles", before_number}}},
    {U"miér.", {{U"miércoles", before_number}}},
    {U"jue.", {{U"jueves", before_number}}},
    {U"vie.", {{U"viernes", before_number}}},
    {U"vier.", {{U"viernes", before_number}}},
    {U"sáb.", {{U"sábado", before_number}}},
    {U"dom.", {{U"domingo", before_number}}},
    // After a year: 10.000 a. C., siglo X e. c.
    {U"a. C.", {{U"antes de Cristo", after_year}}},
    {U"a.C.", {{U"antes de Cristo", after_year}}},
    {U"a. c.", {{U"antes de Cristo", after_year}}},
    {U"a.c.", {{U"antes de Cristo", after_year}}},
    {U"d. C.", {{U"después de Cristo", after_year}}},
    {U"d.C.", {{U"después de Cristo", after_year}}},
    {U"d. c.", {{U"después de Cristo", after_year}}},
    {U"a. e. c.", {{U"antes de la era común", after_year}}},
    {U"e. c.", {{U"era común", after_year}}},
    {U"ec", {{U"era común", after_year}}},
    // Titles: Da. Ana
    {U"Da.", {{U"doña", before_title}}},
    {U"da.", {{U"doña", sentence_start}}},
    // Anywhere
    {U"Sr.", {U"señor"}},
    {U"sr.", {U"señor"}},
    {U"Sra.", {U"señora"}},
    {U"sra.", {U"señora"}},
    {U"Srta.", {U"señorita"}},
    {U"Dr.", {U"doctor"}},
    {U"dr.", {U"doctor"}},
    {U"Dra.", {U"doctora"}},
    {U"dra.", {U"doctora"}},
    {U"Ud.", {U"usted"}},
    {U"Uds.", {U"ustedes"}},
    {U"Ing.", {U"ingeniero"}},
    {U"Prof.", {U"profesor"}},
    {U"Avda.", {U"avenida"}},
    {U"calz.", {U"calzada"}},
    {U"pp.", {U"páginas"}},
    {U"esq.", {U"esquina"}},
    {U"pza.", {U"plaza"}},
    {U"Pza.", {U"plaza"}},
    {U"c.º b.", {U"camino bajo"}},
    {U"EE. UU.", {U"Estados Unidos"}},
    {U"EE.UU.", {U"Estados Unidos"}},
    {U"ee. uu.", {U"Estados Unidos"}},
    {U"etc.", {U"etcétera"}},
    {U"aprox.", {U"aproximadamente"}},
  },
  .web = {.dot = U"punto", .at = U"arroba", .slash = U"barra", .hyphen = U"guion", .underscore = U"guion bajo",
          .digits = {U"cero", U"uno", U"dos", U"tres", U"cuatro", U"cinco", U"seis", U"siete", U"ocho", U"nueve"}},
  .closing = {U"etc."}
};

inline const Terms kTerms = [] {
  Terms terms = kTermsBase;
  for (const auto& zone : kTimeZones) {
    std::u32string upper;
    for (char32_t c : zone) {
      upper += c - U'a' + U'A'; // The zones are ASCII
    }
    for (const auto& form : {zone, upper}) {
      terms.abbreviations[form] = {{upper, after_time}};
      terms.abbreviations[form + U"-"] = {{upper + U" menos", after_time}};
      terms.abbreviations[form + U"+"] = {{upper + U" más", after_time}};
    }
  }
  return terms;
}();

} // namespace verbalis::languages::es
