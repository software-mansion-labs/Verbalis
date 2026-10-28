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

namespace verbalis::languages::pt {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::RomanCues;
using ::verbalis::numbers::SuffixFormat;

// Brazilian forms: dezesseis, bilhão
inline constexpr std::array<std::u32string_view, 20> kOnes = {
  U"zero",  U"um",   U"dois",  U"três",  U"quatro",  U"cinco",  U"seis",      U"sete",      U"oito",    U"nove",
  U"dez",   U"onze", U"doze",  U"treze", U"catorze", U"quinze", U"dezesseis", U"dezessete", U"dezoito", U"dezenove"
};

inline constexpr std::array<std::u32string_view, 10> kTens = {
  U"", U"", U"vinte", U"trinta", U"quarenta", U"cinquenta", U"sessenta", U"setenta", U"oitenta", U"noventa"
};

inline constexpr std::array<std::u32string_view, 10> kHundreds = {
  U"",           U"cento",      U"duzentos",   U"trezentos",  U"quatrocentos",
  U"quinhentos", U"seiscentos", U"setecentos", U"oitocentos", U"novecentos"
};

// Ordinal stems, completed by -o or -a: vigésima primeira
inline constexpr std::array<std::u32string_view, 10> kOrdinalOnes = {
  U"", U"primeir", U"segund", U"terceir", U"quart", U"quint", U"sext", U"sétim", U"oitav", U"non"
};
inline constexpr std::array<std::u32string_view, 10> kOrdinalTens = {
  U"",             U"décim",     U"vigésim",     U"trigésim",  U"quadragésim",
  U"quinquagésim", U"sexagésim", U"septuagésim", U"octogésim", U"nonagésim"
};
inline constexpr std::array<std::u32string_view, 10> kOrdinalHundreds = {
  U"",             U"centésim",    U"ducentésim",     U"trecentésim",   U"quadringentésim",
  U"quingentésim", U"sexcentésim", U"septingentésim", U"octingentésim", U"noningentésim"
};

inline const std::vector<std::u32string> kMonths = {
  U"janeiro", U"fevereiro", U"março",    U"abril",   U"maio",     U"junho",
  U"julho",   U"agosto",    U"setembro", U"outubro", U"novembro", U"dezembro"
};

inline const std::unordered_map<std::u32string, Forms> kCurrencies = {
  {U"R$", {U"real", U"reais"}}, {U"US$", {U"dólar", U"dólares"}}, {U"$", {U"dólar", U"dólares"}},
  {U"€", {U"euro", U"euros"}},  {U"£", {U"libra", U"libras"}}
};

// Full words are kept as written; abbreviations take the singular below 2
inline const std::unordered_map<std::u32string, Forms> kScales = {
  {U"mil", {U"mil", U"mil"}},                 {U"k", {U"mil", U"mil"}},                 {U"K", {U"mil", U"mil"}},
  {U"milhão", {U"milhão", U"milhão"}},        {U"milhões", {U"milhões", U"milhões"}},
  {U"bilhão", {U"bilhão", U"bilhão"}},        {U"bilhões", {U"bilhões", U"bilhões"}},
  {U"trilhão", {U"trilhão", U"trilhão"}},     {U"trilhões", {U"trilhões", U"trilhões"}},
  {U"mi", {U"milhão", U"milhões"}},           {U"bi", {U"bilhão", U"bilhões"}},
  {U"tri", {U"trilhão", U"trilhões"}}
};

inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"km", {U"quilômetro", U"quilômetros"}},        {U"m", {U"metro", U"metros"}},
  {U"cm", {U"centímetro", U"centímetros"}},        {U"mm", {U"milímetro", U"milímetros"}},
  {U"nm", {U"nanômetro", U"nanômetros"}},          {U"m²", {U"metro quadrado", U"metros quadrados"}},
  {U"km²", {U"quilômetro quadrado", U"quilômetros quadrados"}}, {U"m³", {U"metro cúbico", U"metros cúbicos"}},
  {U"ha", {U"hectare", U"hectares"}},              {U"kg", {U"quilo", U"quilos"}},
  {U"g", {U"grama", U"gramas"}},                   {U"mg", {U"miligrama", U"miligramas"}},
  {U"t", {U"tonelada", U"toneladas"}},             {U"l", {U"litro", U"litros"}},
  {U"L", {U"litro", U"litros"}},                   {U"ml", {U"mililitro", U"mililitros"}},
  {U"mL", {U"mililitro", U"mililitros"}},          {U"TB", {U"terabyte", U"terabytes"}},
  {U"GB", {U"gigabyte", U"gigabytes"}},            {U"MB", {U"megabyte", U"megabytes"}},
  {U"KB", {U"kilobyte", U"kilobytes"}},            {U"Hz", {U"hertz", U"hertz"}},
  {U"kHz", {U"quilohertz", U"quilohertz"}},        {U"MHz", {U"megahertz", U"megahertz"}},
  {U"GHz", {U"gigahertz", U"gigahertz"}},          {U"W", {U"watt", U"watts"}},
  {U"kW", {U"quilowatt", U"quilowatts"}},          {U"kWh", {U"quilowatt-hora", U"quilowatts-hora"}},
  {U"V", {U"volt", U"volts"}},                     {U"km/h", {U"quilômetro por hora", U"quilômetros por hora"}},
  {U"mph", {U"milha por hora", U"milhas por hora"}}, {U"°C", {U"grau Celsius", U"graus Celsius"}}, {U"°F", {U"grau Fahrenheit", U"graus Fahrenheit"}},
  {U"°", {U"grau", U"graus"}},                    {U"min", {U"minuto", U"minutos"}},
  {U"s", {U"segundo", U"segundos"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"meio"}, {{1, 3}, U"um terço"}, {{2, 3}, U"dois terços"}, {{1, 4}, U"um quarto"},
  {{3, 4}, U"três quartos"}
};

// Words after a fraction which mark it as one: 1/3 da
inline const std::unordered_set<std::u32string> kFractionWords = {U"de", U"do", U"da", U"dos", U"das"};

// Words before a day and a month without a year which make them a date: dia 15/9, quinta-feira (16/3)
inline const std::unordered_set<std::u32string> kDateWords = {
  U"dia", U"dias", U"em", U"até", U"desde", U"hoje", U"ontem", U"amanhã", U"data", U"sábado", U"domingo"
};

// Words before a number and a year after a slash, which is not a fraction: Lei 15/98, nº 913/2010
inline const std::unordered_set<std::u32string> kLawWords = {
  U"lei", U"decreto", U"decreto-lei", U"portaria", U"processo", U"proc", U"resolução", U"despacho", U"emenda",
  U"parecer", U"acórdão", U"número", U"nº", U"n.º"
};

// Scale words which take "de" before a currency: um milhão de reais, but dez mil reais
inline const std::vector<std::u32string> kScaleWords = {
  U"milhão", U"milhões", U"bilhão", U"bilhões", U"trilhão", U"trilhões"
};

// Words before an hour, which make it feminine: às duas, das dez
inline const std::unordered_set<std::u32string> kHourWords = {U"às", U"à", U"as", U"das", U"pelas"};

// Parts of the day after "da": duas da tarde
inline const std::unordered_set<std::u32string> kDayParts = {U"manhã", U"tarde", U"noite", U"madrugada"};

// Legal headings, after which an ordinal from 10 on reads as a cardinal: artigo 80º, artigo oitenta
inline const std::unordered_set<std::u32string> kHeadings = {U"artigo", U"art", U"parágrafo", U"capítulo", U"inciso"};

// Nouns in -a are feminine unless listed below; other feminine nouns are listed or end like one
inline const std::unordered_set<std::u32string> kMasculineInA = {
  U"dia", U"mapa", U"problema", U"sistema", U"programa", U"tema", U"idioma", U"clima", U"planeta", U"poema",
  U"cometa", U"drama", U"esquema", U"sofá", U"diagrama", U"teorema", U"telefonema", U"cinema", U"prisma",
  U"pijama", U"aroma", U"dilema", U"fantasma", U"panorama", U"enigma", U"dogma", U"carisma", U"atleta", U"guia",
  U"pila", U"grama", U"quilograma", U"miligrama", U"micrograma"
};
inline const std::unordered_set<std::u32string> kFeminineNouns = {
  U"vez", U"mão", U"foto", U"moto", U"mulher", U"flor", U"noite", U"tarde", U"parte", U"gente", U"lei", U"luz",
  U"voz", U"paz", U"rede", U"sede", U"fase", U"base", U"frase", U"classe", U"chave", U"fonte", U"mente", U"morte",
  U"sorte", U"ponte", U"árvore", U"nuvem", U"ordem", U"série", U"espécie", U"superfície", U"cor", U"dor", U"mãe",
  U"pele", U"neve", U"febre", U"carne", U"torre", U"hora", U"semana", U"polegada", U"lição", U"colher"
};
inline const std::vector<std::u32string> kFeminineEndings = {
  U"a", U"ã", U"ção", U"são", U"dade", U"tude", U"gem", U"ise", U"ite", U"ez"
};

// Words before a year, which agrees with no noun after it: em 2020 havia
inline const std::unordered_set<std::u32string> kYearWords = {U"em", U"de", U"desde", U"até", U"ano"};

// Words after a number which are not the noun it counts
inline const std::unordered_set<std::u32string> kStopWords = {
  U"de", U"do", U"da", U"dos", U"das", U"em", U"no", U"na", U"nos", U"nas", U"com", U"por", U"para", U"sem", U"que",
  U"e", U"ou", U"a", U"o", U"os", U"as", U"um", U"uma", U"uns", U"umas", U"é", U"são", U"era", U"foi", U"mais",
  U"menos", U"até", U"desde", U"entre", U"sobre", U"contra", U"após", U"cada", U"todo", U"toda", U"todos", U"todas",
  U"este", U"esta", U"estes", U"estas", U"esse", U"essa", U"esses", U"essas", U"isso", U"isto", U"já", U"agora",
  U"quando", U"onde", U"muito", U"também", U"se", U"lhe", U"me", U"te", U"ao", U"à", U"aos", U"às", U"pelo",
  U"pela", U"pelos", U"pelas", U"não", U"sim", U"meu", U"minha", U"seu", U"sua",
  U"nada", U"ainda", U"apenas", U"embora", U"fora"
};

// D. Pedro II (segundo), Bento XVI (dezesseis), século XXI (vinte e um), capítulo IV (quatro), a II Guerra (Segunda)
inline const RomanCues kRomanCues = {
  .names = {
    U"pedro", U"joão", U"paulo", U"bento", U"leão", U"pio", U"manuel", U"afonso", U"sebastião", U"josé", U"luís",
    U"carlos", U"filipe", U"henrique", U"dinis", U"maria", U"isabel", U"jorge", U"guilherme", U"gregório",
    U"clemente", U"napoleão", U"fernando", U"duarte", U"sancho", U"eduardo", U"elizabeth", U"beatriz", U"leonor",
    U"teresa"
  },
  .before = {
    U"capítulo", U"tomo", U"livro", U"ato", U"título", U"parte", U"volume", U"temporada", U"fase", U"século",
    U"séculos", U"vaticano", U"cena", U"episódio", U"nível", U"tipo", U"classe"
  },
  .after = {
    U"guerra", U"congresso", U"encontro", U"festival", U"edição", U"bienal", U"feira", U"cimeira", U"conferência",
    U"jornada", U"jornadas", U"seminário", U"simpósio", U"fórum", U"campeonato", U"república", U"olimpíada",
    U"assembleia", U"aniversário", U"centenário", U"reunião", U"plano", U"prémio", U"prêmio", U"torneio", U"semana",
    U"mostra", U"corrida", U"marcha", U"salão", U"colóquio"
  }
};

// D. João Iº: an ordinal marker on a Roman numeral
inline const Keys kRomanMarkers = {U"º", U"ª", U"°"};

// Names with a regnal number, and nouns after a numeral, which are feminine: D. Maria II (segunda), a II Guerra
inline const std::unordered_set<std::u32string> kFeminineNames = {
  U"maria", U"isabel", U"elizabeth", U"beatriz", U"leonor", U"teresa"
};
inline const std::unordered_set<std::u32string> kFeminineRomanNouns = {
  U"guerra", U"edição", U"bienal", U"feira", U"cimeira", U"conferência", U"jornada", U"jornadas", U"república",
  U"olimpíada", U"assembleia", U"reunião", U"semana", U"mostra", U"corrida", U"marcha"
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U". \u00a0\u202f", .decimal_separators = U",.", .time_separators = U":hH.", .time_suffixes = {U"h", U"hs"},
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"ponto", .dot = U"ponto", .range = U" a ", .by = U"por", .score = U" a ", .minus = U"menos", .plus = U"mais",
  .fraction = U" e ", .rating = U"de"
};

// 24/03/2022, 07-11-2018, 15/09
inline const DateFormat kDateFormat = {.separators = U"/-", .boundaries = U".,/-", .year_required = false};

// 15.03.2024: with dots, only with a full year
inline const DateFormat kDotDateFormat = {.separators = U".", .boundaries = U".,/-", .short_year = false};

// (11) 98765-4321, 11 98765-4321, 0800-123456, 3456-7890, 213 456 789 (after a word of kPhoneWords), 555-1234: the
// longest first
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{2, 2}, {4, 5}, {4, 4}}, .parenthesized = true, .closing = U"-"},
  {.groups = {{2, 2}, {4, 5}, {4, 4}}, .separators = U" -", .boundaries = U".,-", .closing = U"-"},
  {.groups = {{4, 5}, {4, 7}}, .separators = U"-", .boundaries = U".,-", .closing = U"-"},
  {.groups = {{3, 3}, {3, 3}, {3, 3}}, .separators = U" ", .boundaries = U".,-", .closing = U"-"},
  {.groups = {{3, 3}, {4, 4}}, .separators = U"-", .boundaries = U".,-", .closing = U"-"}
};

// Words before a phone number written with spaces, which else reads as numbers: ligue para 213 456 789
inline const std::unordered_set<std::u32string> kPhoneWords = {
  U"para", U"telefone", U"tel", U"ligue", U"contato", U"contacto", U"número", U"whatsapp", U"fax"
};

// 1º, 2 ª, 3°, 24.º: a sign, possibly after a space; 3° is an ordinal only by its words
inline const Keys kOrdinalSigns = {U"º", U"ª", U"°", U".º", U".ª", U".°"};

// Articles before 2° which make it an ordinal (o 2° lugar), not degrees (a 30°), and the feminine ones (na 5°)
inline const std::unordered_set<std::u32string> kOrdinalArticles = {
  U"o", U"do", U"no", U"ao", U"pelo", U"à", U"da", U"na",
  U"pela", U"seu", U"sua", U"este", U"esta", U"nosso", U"nossa"
};
inline const std::unordered_set<std::u32string> kFeminineOrdinalArticles = {
  U"à", U"da", U"na", U"pela", U"sua", U"esta", U"nossa"
};
// Nouns after 2° which make it an ordinal: 2° lugar, 1° andar
inline const std::unordered_set<std::u32string> kOrdinalNouns = {
  U"lugar", U"posto", U"andar", U"aniversário", U"edição", U"festival", U"congresso", U"concurso",
  U"encontro", U"jornada", U"ano", U"semestre", U"trimestre", U"bimestre", U"nível", U"torneio",
  U"campeonato", U"capítulo", U"lote", U"prêmio", U"prémio", U"etapa", U"volta", U"rodada",
  U"secção", U"seção", U"distrito", U"batalhão", U"regimento", U"grau", U"dia", U"semana",
  U"período", U"ciclo", U"turno", U"tempo", U"round", U"colocado", U"colocada", U"classificado",
  U"classificada", U"ofício", U"cartório", U"juizado", U"vara", U"mandato", U"sessão", U"salão",
  U"simpósio", U"seminário", U"fórum", U"certame", U"convenção", U"cúpula", U"sargento", U"tenente"
};

inline const SuffixFormat kOrdinalSignFormat = {.space = true, .word_end = false};

// anos 90s, década de 80s, '80s, 1980s; but 30s alone is thirty seconds
inline const Keys kDecadeSuffixes = {U"s"};
inline const SuffixFormat kDecadeFormat = {.apostrophes = true, .boundaries = U".,-"};

using ::verbalis::text::Readings;

inline const Readings kSymbols = {
  {U"&", {U"e"}},         {U"+", {U"mais"}},    {U"=", {U"igual a"}},     {U"@", {U"arroba"}},   {U"%", {U"por cento"}},
  {U"°", {U"graus"}},     {U"#", {U"hashtag"}}, {U"²", {U"ao quadrado"}}, {U"³", {U"ao cubo"}},  {U"π", {U"pi"}},
  {U"×", {U"vezes"}},     {U"€", {U"euros"}},   {U"$", {U"dólares"}},     {U"£", {U"libras"}},   {U"§", {U"parágrafo"}}
};

using ::verbalis::text::before_number;
using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: nº 5
    {U"nº", {{U"número", before_number}}},
    {U"Nº", {{U"número", before_number}}},
    {U"n.º", {{U"número", before_number}}},
    {U"N.º", {{U"número", before_number}}},
    {U"n°", {{U"número", before_number}}},
    {U"N°", {{U"número", before_number}}},
    {U"n.°", {{U"número", before_number}}},
    {U"N.°", {{U"número", before_number}}},
    {U"núm.", {{U"número", before_number}}},
    {U"art.", {{U"artigo", before_number}}},
    {U"Art.", {{U"artigo", before_number}}},
    {U"pág.", {{U"página", before_number}}},
    {U"§", {{U"parágrafo", before_number}}},
    {U"ext.", {{U"extensão", before_number}}},
  },
  .web = {.dot = U"ponto", .at = U"arroba", .slash = U"barra", .hyphen = U"hífen", .underscore = U"sublinhado",
          .digits = {U"zero", U"um", U"dois", U"três", U"quatro", U"cinco", U"seis", U"sete", U"oito", U"nove"}}
};

} // namespace verbalis::languages::pt
