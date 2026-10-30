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

namespace verbalis::languages::it {

using ::verbalis::numbers::DateFormat;
using ::verbalis::text::Keys;
using ::verbalis::numbers::keys_of;
using ::verbalis::numbers::Notation;
using ::verbalis::numbers::PhoneFormat;
using ::verbalis::numbers::RomanCues;
using ::verbalis::numbers::SuffixFormat;

inline constexpr std::array<std::u32string_view, 20> kOnes = {
  U"zero",   U"uno",    U"due",         U"tre",      U"quattro",    U"cinque",      U"sei",
  U"sette",  U"otto",   U"nove",        U"dieci",    U"undici",     U"dodici",      U"tredici",
  U"quattordici", U"quindici", U"sedici", U"diciassette", U"diciotto", U"diciannove"
};

inline constexpr std::array<std::u32string_view, 10> kTens = {
  U"", U"", U"venti", U"trenta", U"quaranta", U"cinquanta", U"sessanta", U"settanta", U"ottanta", U"novanta"
};

// Ordinals up to ten; later ones are the cardinal with -esimo
inline constexpr std::array<std::u32string_view, 11> kOrdinals = {
  U"", U"primo", U"secondo", U"terzo", U"quarto", U"quinto", U"sesto", U"settimo", U"ottavo", U"nono", U"decimo"
};

inline const std::vector<std::u32string> kMonths = {
  U"gennaio", U"febbraio", U"marzo",     U"aprile",  U"maggio",   U"giugno",
  U"luglio",  U"agosto",   U"settembre", U"ottobre", U"novembre", U"dicembre"
};

inline const std::unordered_map<std::u32string, Forms> kCurrencies = {
  {U"€", {U"euro", U"euro"}},          {U"EUR", {U"euro", U"euro"}},       {U"eur", {U"euro", U"euro"}},
  {U"euro", {U"euro", U"euro"}},       {U"Euro", {U"euro", U"euro"}},
  {U"$", {U"dollaro", U"dollari"}},    {U"US$", {U"dollaro", U"dollari"}}, {U"USD", {U"dollaro", U"dollari"}},
  {U"£", {U"sterlina", U"sterline"}},  {U"GBP", {U"sterlina", U"sterline"}},
  {U"¥", {U"yen", U"yen"}},            {U"₽", {U"rublo", U"rubli"}},       {U"RUB", {U"rublo", U"rubli"}},
  {U"CHF", {U"franco svizzero", U"franchi svizzeri"}}, {U"₹", {U"rupia", U"rupie"}}
};

// The hundredth of a currency: 10,50 € dieci euro e cinquanta centesimi; 0,01 € un centesimo
inline const std::unordered_map<std::u32string, Forms> kCents = {
  {U"€", {U"centesimo", U"centesimi"}}, {U"EUR", {U"centesimo", U"centesimi"}}, {U"eur", {U"centesimo", U"centesimi"}},
  {U"euro", {U"centesimo", U"centesimi"}}, {U"Euro", {U"centesimo", U"centesimi"}},
  {U"$", {U"centesimo", U"centesimi"}}, {U"US$", {U"centesimo", U"centesimi"}}, {U"USD", {U"centesimo", U"centesimi"}},
  {U"£", {U"penny", U"penny"}},         {U"GBP", {U"penny", U"penny"}},
  {U"₽", {U"copeco", U"copechi"}},      {U"RUB", {U"copeco", U"copechi"}},
  {U"CHF", {U"centesimo", U"centesimi"}}
};

// Most scales are written as the word they are read with: 3 milioni
inline const std::unordered_map<std::u32string, Forms> kScales = {
  {U"mila", {U"mila", U"mila"}},             {U"k", {U"mila", U"mila"}},           {U"K", {U"mila", U"mila"}},
  {U"milione", {U"milione", U"milione"}},    {U"milioni", {U"milioni", U"milioni"}},
  {U"miliardo", {U"miliardo", U"miliardo"}}, {U"miliardi", {U"miliardi", U"miliardi"}},
  {U"mln", {U"milione", U"milioni"}},        {U"mld", {U"miliardo", U"miliardi"}}
};

// A singular ending in -a is feminine: una tonnellata, un'ora
inline const std::unordered_map<std::u32string, Forms> kUnits = {
  {U"km", {U"chilometro", U"chilometri"}},          {U"m", {U"metro", U"metri"}},
  {U"cm", {U"centimetro", U"centimetri"}},          {U"mm", {U"millimetro", U"millimetri"}},
  {U"m²", {U"metro quadro", U"metri quadri"}},      {U"m2", {U"metro quadro", U"metri quadri"}},
  {U"mq", {U"metro quadro", U"metri quadri"}},      {U"cm²", {U"centimetro quadrato", U"centimetri quadrati"}},
  {U"km²", {U"chilometro quadrato", U"chilometri quadrati"}}, {U"m³", {U"metro cubo", U"metri cubi"}},
  {U"mc", {U"metro cubo", U"metri cubi"}},          {U"kg", {U"chilo", U"chili"}},
  {U"g", {U"grammo", U"grammi"}},                   {U"gr", {U"grammo", U"grammi"}},
  {U"mg", {U"milligrammo", U"milligrammi"}},        {U"t", {U"tonnellata", U"tonnellate"}},
  {U"l", {U"litro", U"litri"}},                     {U"lt", {U"litro", U"litri"}},
  {U"ml", {U"millilitro", U"millilitri"}},          {U"cl", {U"centilitro", U"centilitri"}},
  {U"dl", {U"decilitro", U"decilitri"}},            {U"TB", {U"terabyte", U"terabyte"}},
  {U"GB", {U"giga", U"giga"}},                      {U"MB", {U"mega", U"mega"}},
  {U"kB", {U"kilobyte", U"kilobyte"}},              {U"Mbps", {U"megabit al secondo", U"megabit al secondo"}},
  {U"Hz", {U"hertz", U"hertz"}},                    {U"kHz", {U"kilohertz", U"kilohertz"}},
  {U"MHz", {U"megahertz", U"megahertz"}},           {U"GHz", {U"gigahertz", U"gigahertz"}},
  {U"W", {U"watt", U"watt"}},                       {U"kW", {U"kilowatt", U"kilowatt"}},
  {U"MW", {U"megawatt", U"megawatt"}},              {U"kWh", {U"kilowattora", U"kilowattora"}},
  {U"V", {U"volt", U"volt"}},                       {U"kV", {U"kilovolt", U"kilovolt"}},
  {U"A", {U"ampere", U"ampere"}},                   {U"mA", {U"milliampere", U"milliampere"}},
  {U"mAh", {U"milliampereora", U"milliampereora"}}, {U"CV", {U"cavallo", U"cavalli"}},
  {U"cv", {U"cavallo", U"cavalli"}},                {U"Nm", {U"newton metro", U"newton metro"}},
  {U"kcal", {U"chilocaloria", U"chilocalorie"}},    {U"dB", {U"decibel", U"decibel"}},
  {U"km/h", {U"chilometro orario", U"chilometri orari"}},
  {U"km/s", {U"chilometro al secondo", U"chilometri al secondo"}},
  {U"m/s", {U"metro al secondo", U"metri al secondo"}},
  {U"km/l", {U"chilometro al litro", U"chilometri al litro"}},
  {U"°C", {U"grado", U"gradi"}},                    {U"°F", {U"grado Fahrenheit", U"gradi Fahrenheit"}},
  {U"°", {U"grado", U"gradi"}},                     {U"h", {U"ora", U"ore"}},
  {U"min", {U"minuto", U"minuti"}},                 {U"sec", {U"secondo", U"secondi"}},
  {U"ms", {U"millisecondo", U"millisecondi"}},      {U"gg", {U"giorno", U"giorni"}},
  {U"d", {U"giorno", U"giorni"}},                   {U"mL", {U"millilitro", U"millilitri"}},
  {U"KWh", {U"kilowattora", U"kilowattora"}},       {U"KW", {U"kilowatt", U"kilowatt"}},
  {U"Kg", {U"chilo", U"chili"}},                    {U"cc", {U"centimetro cubo", U"centimetri cubi"}},
  {U"uF", {U"microfarad", U"microfarad"}}
};

inline const std::map<std::pair<uint64_t, uint64_t>, std::u32string> kFractions = {
  {{1, 2}, U"un mezzo"}, {{1, 3}, U"un terzo"}, {{2, 3}, U"due terzi"}, {{1, 4}, U"un quarto"},
  {{3, 4}, U"tre quarti"}
};

// Words after a fraction which mark it as one: 1/3 del
inline const std::unordered_set<std::u32string> kFractionWords = {U"di", U"del", U"della", U"dello", U"dei", U"degli", U"delle", U"dell'"};

// Words before a time with a dot, which make it a time: alle 21.30
inline const std::unordered_set<std::u32string> kTimeWords = {
  U"alle", U"ore", U"dalle", U"le", U"verso", U"entro", U"per"
};

// Giovanni XXIII (ventitreesimo), nel XX secolo (ventesimo), capitolo IV (quarto), la II guerra (seconda)
inline const RomanCues kRomanCues = {
  .names = {
    U"giovanni", U"paolo", U"benedetto", U"pio", U"leone", U"gregorio", U"clemente", U"innocenzo", U"bonifacio",
    U"emanuele", U"umberto", U"carlo", U"luigi", U"enrico", U"filippo", U"federico", U"francesco", U"ferdinando",
    U"napoleone", U"elisabetta", U"giorgio", U"guglielmo", U"alessandro", U"edoardo", U"riccardo", U"giacomo",
    U"pietro", U"amedeo", U"ramses", U"maria", U"vittoria", U"caterina", U"giovanna"
  },
  .before = {
    U"capitolo", U"libro", U"tomo", U"atto", U"scena", U"parte", U"titolo", U"articolo", U"secolo", U"volume",
    U"stagione", U"fase", U"legislatura", U"canto"
  },
  .after = {
    U"secolo", U"secoli", U"millennio", U"guerra", U"congresso", U"edizione", U"festival", U"legislatura",
    U"giornata", U"convegno", U"conferenza", U"assemblea", U"concorso", U"forum", U"simposio", U"seminario",
    U"campionato", U"olimpiade", U"repubblica", U"anniversario", U"centenario", U"premio", U"torneo", U"salone",
    U"settimana", U"municipio", U"circoscrizione", U"rapporto", U"piano", U"stagione"
  }
};

// Umberto I°: an ordinal marker on a Roman numeral
inline const Keys kRomanMarkers = {U"°", U"º", U"ª"};

// Names with a regnal number, and nouns around a numeral, which are feminine: Elisabetta II (seconda), parte II
inline const std::unordered_set<std::u32string> kFeminineNames = {
  U"elisabetta", U"maria", U"vittoria", U"caterina", U"giovanna"
};
inline const std::unordered_set<std::u32string> kFeminineRomanNouns = {
  U"scena", U"parte", U"stagione", U"fase", U"legislatura", U"classe", U"guerra", U"edizione", U"giornata",
  U"conferenza", U"assemblea", U"olimpiade", U"repubblica", U"settimana", U"circoscrizione"
};

// --- Notation: how numbers are written ---
inline const Notation kNotation = {
  .group_separators = U".", .decimal_separators = U",.", .time_separators = U":.",
  .currencies = keys_of(kCurrencies), .scales = keys_of(kScales), .units = keys_of(kUnits), .fractions = true
};

// Words between numbers: 3.8.10, 10.0.0.51, 10-15, 1920x1080, 2:1, -5, 2 + 2
inline const Joiners kJoiners = {
  .point = U"punto", .dot = U"punto", .range = U"-", .by = U"per", .score = U" a ", .minus = U"meno", .plus = U"più",
  .fraction = U" e ", .half = U" e mezzo", .rating = U"su"
};

// 24/03/2022, 24.03.2022, 24-03-2022
inline const DateFormat kDateFormat = {.separators = U"/.-", .boundaries = U".,/-"};

// 333 1234567, 06 1234 5678, 06 12345678
inline const std::vector<PhoneFormat> kPhoneFormats = {
  {.groups = {{3, 3}, {6, 7}}, .separators = U" "},
  {.groups = {{2, 4}, {3, 4}, {4, 4}}, .separators = U" ", .leading_zero = true},
  {.groups = {{2, 4}, {6, 8}}, .separators = U" ", .leading_zero = true},
  {.groups = {{2, 4}, {5, 8}}, .separators = U"-/.", .leading_zero = true}
};

// 1º, 2 ª, 3°, 2.°: a sign, possibly after a space
inline const Keys kOrdinalSigns = {U"º", U"ª", U"°", U".°", U".º", U".ª"};

// Articles and articulated prepositions before 3° which make it an ordinal (il 3° posto), not degrees (a 30°)
inline const std::unordered_set<std::u32string> kOrdinalArticles = {
  U"il",    U"lo",    U"la",    U"al",    U"allo",  U"alla",   U"del",    U"dello", U"della", U"nel",   U"nello",
  U"nella", U"dal",   U"dallo", U"dalla", U"sul",   U"sullo",  U"sulla",  U"un",    U"una",   U"questo", U"questa",
  U"suo",   U"sua",   U"mio",   U"mia",   U"tuo",   U"tua",    U"nostro", U"nostra", U"vostro", U"vostra",
  U"l",     U"all",   U"dell",  U"nell",  U"dall",  U"sull"
};
// The articles above which are feminine; an elided one (l'11° edizione) takes the gender of the noun after it
inline const std::unordered_set<std::u32string> kFeminineArticles = {
  U"la", U"alla", U"della", U"nella", U"dalla", U"sulla", U"una", U"questa", U"sua", U"mia", U"tua", U"nostra",
  U"vostra"
};
// Nouns after 3° which make it an ordinal: 3° lotto, 17° Meeting
inline const std::unordered_set<std::u32string> kOrdinalNouns = {
  U"posto", U"lotto", U"anno", U"semestre", U"trimestre", U"tasto", U"giorno", U"anniversario",
  U"edizione", U"piano", U"classificato", U"classificata", U"livello", U"torneo", U"campionato", U"capitolo",
  U"premio", U"tappa", U"giro", U"sezione", U"distretto", U"battaglione", U"reggimento", U"stormo",
  U"grado", U"meeting", U"festival", U"congresso", U"concorso", U"incontro", U"giornata", U"turno",
  U"tempo", U"round", U"circolo", U"municipio", U"reparto", U"corso", U"convegno", U"raduno",
  U"trofeo", U"memorial", U"salone", U"forum", U"secolo", U"movimento"
};
inline const std::unordered_set<std::u32string> kElidedArticles = {U"l", U"all", U"dell", U"nell", U"dall", U"sull"};

// Words after 1 which are not nouns it counts: 1 su 3 (uno su tre), 1 di loro, 1 a 0
inline const std::unordered_set<std::u32string> kStopWords = {
  U"e",     U"ed",    U"o",     U"od",    U"a",     U"ad",    U"di",     U"da",    U"in",    U"su",    U"per",
  U"con",   U"tra",   U"fra",   U"del",   U"della", U"dei",   U"degli",  U"delle", U"dello", U"dell",  U"al",
  U"alla",  U"allo",  U"ai",    U"agli",  U"alle",  U"all",   U"nel",    U"nella", U"nello", U"nei",   U"negli",
  U"nelle", U"nell",  U"dal",   U"dalla", U"dallo", U"dai",   U"dagli",  U"dalle", U"dall",  U"sul",   U"sulla",
  U"sullo", U"sui",   U"sugli", U"sulle", U"sull",  U"col",   U"coi",    U"è",     U"era",   U"sono",  U"ha",
  U"hanno", U"che",   U"contro", U"più",  U"meno",  U"x",     U"come",   U"ma",    U"se",    U"non",   U"il",
  U"lo",    U"la",    U"le",    U"gli",   U"i",     U"un",    U"una",    U"uno",   U"rispetto", U"circa",
  U"dopo",  U"prima", U"fa",    U"oppure", U"anche", U"poi",  U"così",   U"invece"
};

// Words before a number with a dot which make it a label read with punto: versione 2.1, paragrafo 4.2
inline const std::unordered_set<std::u32string> kVersionWords = {
  U"versione", U"release", U"v", U"ver", U"paragrafo", U"par", U"art", U"articolo", U"comma", U"punto",
  U"sezione", U"capitolo", U"cap", U"regola", U"nota", U"tabella", U"figura", U"fig", U"allegato", U"modello",
  U"aggiornamento", U"standard", U"protocollo"
};

// Words before 1 which make it a count of the noun after it (con 1 figlio: un figlio), unlike a label (articolo 1)
inline const std::unordered_set<std::u32string> kCountWords = {
  U"con",    U"per",     U"in",     U"di",      U"da",     U"a",       U"ad",     U"su",      U"e",      U"ed",
  U"o",      U"solo",    U"soltanto", U"almeno", U"circa", U"ogni",    U"tra",    U"fra",     U"entro",  U"dopo",
  U"oltre",  U"ha",      U"ho",     U"hai",     U"hanno",  U"abbiamo", U"avete",  U"aveva",   U"avevano", U"c'è",
  U"è",      U"era",     U"costa",  U"pesa",    U"dura",   U"misura",  U"già",    U"ancora",  U"appena", U"quasi",
  U"manca",  U"mancano", U"resta",  U"restano", U"rimane", U"più",     U"meno",   U"che",     U"cioè",   U"sotto",
  U"sopra",  U"verso",   U"contro", U"senza",   U"servono", U"serve",  U"basta",  U"bastano"
};

// Nouns in -a which are masculine (un problema), and nouns in -e which are feminine (una notte)
inline const std::unordered_set<std::u32string> kMasculineInA = {
  U"problema", U"sistema", U"programma", U"tema", U"schema", U"clima", U"dramma", U"diploma", U"poeta",
  U"pilota", U"atleta", U"pianeta", U"papa", U"panorama", U"cinema", U"dilemma", U"enigma", U"telegramma",
  U"diagramma", U"teorema", U"emblema", U"stemma", U"sofà", U"cruciverba", U"pigiama", U"collega", U"giornalista",
  U"artista", U"turista", U"ciclista", U"dentista", U"regista", U"barista", U"autista", U"tassista", U"sindacalista"
};
inline const std::unordered_set<std::u32string> kFeminineInE = {
  U"notte", U"chiave", U"parte", U"classe", U"fase", U"legge", U"gente", U"mente", U"morte", U"sede", U"nave",
  U"neve", U"fame", U"sete", U"voce", U"croce", U"pace", U"luce", U"rete", U"carne", U"madre", U"moglie",
  U"torre", U"valle", U"fonte", U"sorte", U"frase", U"base", U"lente", U"arte", U"febbre", U"specie", U"serie",
  U"superficie", U"volpe", U"ape", U"fede", U"tosse", U"stirpe", U"siepe", U"lode", U"cassaforte",
  U"automobile", U"canzone", U"nube", U"pelle", U"vite", U"dote", U"radice", U"cornice", U"attrice",
  U"lavatrice", U"calcolatrice", U"stampante", U"corrente", U"costante"
};

// Nouns in -ione which are masculine: un milione, un campione
inline const std::unordered_set<std::u32string> kMasculineInIone = {
  U"milione", U"campione", U"padiglione", U"battaglione", U"scorpione", U"rione", U"bastione"
};
inline const SuffixFormat kOrdinalSignFormat = {.space = true};

// 21esimo, 33esima
inline const Keys kOrdinalSuffixes = {U"esimo", U"esima", U"esimi", U"esime"};

using ::verbalis::text::Readings;

// "&" reads as "e", in names too: Gratta&Vinci, H&M
inline const Readings kSymbols = {
  {U"&", {U"e"}},           {U"+", {U"più"}},       {U"=", {U"uguale a"}}, {U"@", {U"chiocciola"}}, {U"%", {U"per cento"}},
  {U"°", {U"gradi"}},       {U"#", {U"hashtag"}},   {U"²", {U"al quadrato"}}, {U"³", {U"al cubo"}}, {U"π", {U"pi greco"}},
  {U"×", {U"per"}},         {U"€", {U"euro"}},      {U"$", {U"dollari"}},  {U"£", {U"sterline"}},   {U"§", {U"paragrafo"}},
  {U"±", {U"più o meno"}},  {U"+/-", {U"più o meno"}}, {U"‰", {U"per mille"}}, {U"∞", {U"infinito"}}
};

using ::verbalis::text::before_name;
using ::verbalis::text::before_number;
using ::verbalis::steps::Terms;

inline const Terms kTerms = {
  .abbreviations = {
    // Before a number only: n. 5
    {U"n.", {{U"numero", before_number}}},
    {U"N.", {{U"numero", before_number}}},
    {U"nr.", {{U"numero", before_number}}},
    {U"n°", {{U"numero", before_number}}},
    {U"N°", {{U"numero", before_number}}},
    {U"n.°", {{U"numero", before_number}}},
    {U"N.°", {{U"numero", before_number}}},
    {U"nº", {{U"numero", before_number}}},
    {U"Nº", {{U"numero", before_number}}},
    {U"n.º", {{U"numero", before_number}}},
    {U"N.º", {{U"numero", before_number}}},
    {U"art.", {{U"articolo", before_number}}},
    {U"Art.", {{U"articolo", before_number}}},
    {U"pag.", {{U"pagina", before_number}}},
    {U"cap.", {{U"capitolo", before_number}}},
    {U"§", {{U"paragrafo", before_number}}},
    {U"pp.", {{U"pagine", before_number}}},
    {U"fig.", {{U"figura", before_number}}},
    {U"Fig.", {{U"figura", before_number}}},
    {U"tab.", {{U"tabella", before_number}}},
    {U"vol.", {{U"volume", before_number}}},
    {U"art", {{U"articolo", before_number}}},
    {U"num", {{U"numero", before_number}}},
    {U"p.", {{U"pagina", before_number}}},
    {U"ff.", {{U"fogli", before_number}}},
    {U"vv.", {{U"versetti", before_number}}},
    {U"c.", {{U"comma", before_number}}},
    {U"t.", {{U"telefono", before_number}}},
    // Titles, which drop their final -e before a name: Sig. Rossi, signor Rossi
    {U"sig.", {{U"signor", before_name}, {U"signore"}}},
    {U"Sig.", {{U"signor", before_name}, {U"signore"}}},
    {U"sig.ra", {{U"signora"}}},
    {U"Sig.ra", {{U"signora"}}},
    {U"sig.na", {{U"signorina"}}},
    {U"Sig.na", {{U"signorina"}}},
    {U"sigg.", {{U"signori"}}},
    {U"Sigg.", {{U"signori"}}},
    {U"dott.", {{U"dottor", before_name}, {U"dottore"}}},
    {U"Dott.", {{U"dottor", before_name}, {U"dottore"}}},
    {U"dr.", {{U"dottor", before_name}}},
    {U"Dr.", {{U"dottor", before_name}}},
    {U"dott.ssa", {{U"dottoressa"}}},
    {U"Dott.ssa", {{U"dottoressa"}}},
    {U"prof.", {{U"professor", before_name}, {U"professore"}}},
    {U"Prof.", {{U"professor", before_name}, {U"professore"}}},
    {U"prof.ssa", {{U"professoressa"}}},
    {U"Prof.ssa", {{U"professoressa"}}},
    {U"ing.", {{U"ingegner", before_name}, {U"ingegnere"}}},
    {U"Ing.", {{U"ingegner", before_name}, {U"ingegnere"}}},
    {U"avv.", {{U"avvocato"}}},
    {U"Avv.", {{U"avvocato"}}},
    {U"arch.", {{U"architetto"}}},
    {U"Arch.", {{U"architetto"}}},
    {U"geom.", {{U"geometra"}}},
    {U"Geom.", {{U"geometra"}}},
    {U"rag.", {{U"ragionier", before_name}}},
    {U"Rag.", {{U"ragionier", before_name}}},
    {U"on.", {{U"onorevole", before_name}}},
    {U"On.", {{U"onorevole", before_name}}},
    {U"sen.", {{U"senatore", before_name}}},
    {U"Sen.", {{U"senatore", before_name}}},
    {U"mons.", {{U"monsignor", before_name}}},
    {U"Mons.", {{U"monsignor", before_name}}},
    {U"gent.mo", {{U"gentilissimo"}}},
    {U"Gent.mo", {{U"gentilissimo"}}},
    {U"gent.ma", {{U"gentilissima"}}},
    {U"Gent.ma", {{U"gentilissima"}}},
    {U"egr.", {{U"egregio"}}},
    {U"Egr.", {{U"egregio"}}},
    {U"spett.le", {{U"spettabile"}}},
    {U"Spett.le", {{U"spettabile"}}},
    // Places in addresses: p.za Grande, v.le Monza, c.so Italia
    {U"p.za", {{U"piazza"}}},
    {U"P.za", {{U"piazza"}}},
    {U"p.zza", {{U"piazza"}}},
    {U"P.zza", {{U"piazza"}}},
    {U"v.le", {{U"viale"}}},
    {U"V.le", {{U"viale"}}},
    {U"c.so", {{U"corso"}}},
    {U"C.so", {{U"corso"}}},
    {U"l.go", {{U"largo"}}},
    {U"L.go", {{U"largo"}}},
    // Anywhere
    {U"dip.", {{U"dipartimento"}}},
    {U"Dip.", {{U"dipartimento"}}},
    {U"ecc.", {{U"eccetera"}}},
    {U"etc.", {{U"eccetera"}}},
    {U"ad es.", {{U"ad esempio"}}},
    {U"Ad es.", {{U"ad esempio"}}},
    {U"per es.", {{U"per esempio"}}},
    {U"Per es.", {{U"per esempio"}}},
    {U"p.es.", {{U"per esempio"}}},
    {U"p. es.", {{U"per esempio"}}},
    {U"es.", {{U"esempio"}}},
    {U"cfr.", {{U"confronta"}}},
    {U"Cfr.", {{U"confronta"}}},
    {U"tel.", {{U"telefono"}}},
    {U"Tel.", {{U"telefono"}}},
    {U"ca.", {{U"circa"}}},
    {U"sec.", {{U"secolo", before_name}}},
    {U"a.C.", {{U"avanti Cristo"}}},
    {U"d.C.", {{U"dopo Cristo"}}},
    {U"d.c.", {{U"dopo Cristo"}}},
    {U"N.B.", {{U"nota bene"}}},
    {U"n.b.", {{U"nota bene"}}},
    {U"P.S.", {{U"post scriptum"}}},
    {U"q.tà", {{U"quantità"}}},
    {U"tot.", {{U"totale"}}},
  },
  .web = {.dot = U"punto", .at = U"chiocciola", .slash = U"slash", .hyphen = U"trattino", .underscore = U"trattino basso",
          .digits = {U"zero", U"uno", U"due", U"tre", U"quattro", U"cinque", U"sei", U"sette", U"otto", U"nove"}},
  .closing = {U"ecc.", U"etc."}
};

} // namespace verbalis::languages::it
