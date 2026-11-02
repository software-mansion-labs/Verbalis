#pragma once

#include <verbalis/steps/normalize.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/unicode.h>

#include <array>
#include <string>
#include <unordered_map>

namespace verbalis::languages::zh {

using ::verbalis::steps::Characters;

// Full-width punctuation in the forms Mandarin speech models expect; it replaces the default classes
inline const Characters kCharacters = {
  {U"、，",   U", "}, {U"。．", U". "}, {U"！", U"! "}, {U"：", U": "}, {U"；", U"; "}, {U"？", U"? "},
  {U"«《「【", U" “"}, {U"»》」】", U"” "}, {U"（", U" ("}, {U"）", U") "}
};

inline constexpr std::array<char32_t, 10> kDigits = {
  U'零', U'一', U'二', U'三', U'四', U'五', U'六', U'七', U'八', U'九'
};

// Unit of each digit of a group of four, and of each group: 1234 5678 -> 一千二百三十四万五千六百七十八
inline const std::array<std::u32string, 4> kDigitUnits = {U"", U"十", U"百", U"千"};
inline const std::array<std::u32string, 4> kGroupUnits = {U"", U"万", U"亿", U"万亿"};

// Decimal digits read after the point; longer fractions are truncated.
inline constexpr size_t kMaxDecimals = 16;

inline constexpr char32_t kNegative = U'负';
inline constexpr char32_t kPoint = U'点';
inline const std::u32string kTwo = U"两";
inline const std::u32string kRangeTo = U"到";
inline const std::u32string kRatio = U"比";
inline const std::u32string kFraction = U"分之";
inline const std::u32string kPercent = U"百分之";

// Times: "13:10:36" -> "十三点十分三十六秒"
inline const std::u32string kHour = U"点";
inline const std::u32string kMinute = U"分";
inline const std::u32string kSecond = U"秒";
inline const std::u32string kMorning = U"上午";
inline const std::u32string kNoon = U"中午";
inline const std::u32string kAfternoon = U"下午";

// Dates: "2008-08-08" -> "二零零八年八月八日"; "2934bc" -> "公元前二九三四年"
inline const std::u32string kYear = U"年";
inline const std::u32string kMonth = U"月";
inline const std::u32string kDay = U"日";
inline const std::u32string kAnnoDomini = U"公元";
inline const std::u32string kBeforeChrist = U"公元前";
// Characters that end a date: a dash after one is no minus ("189年-232年")
inline constexpr std::u32string_view kDateMarkers = U"年月日号號";

// After a year of three digits, a count of years rather than a year: "700年前" -> "七百年前"
inline const auto kYearCounts = std::to_array<std::u32string>({
  U"前", U"后", U"後", U"来", U"來", U"间", U"間", U"多", U"内", U"內", U"以来", U"以來"
});

// Words after a year written without 年: "2020款" -> "二零二零款"
inline const auto kYearWords = std::to_array<std::u32string>({
  U"款", U"届", U"屆", U"财年", U"財年", U"年度", U"赛季", U"賽季", U"球季"
});

// Words a lone 2 counts as 两 before: "2个" -> "两个", "2万" -> "两万"
inline const auto kTwoBefore = std::to_array<std::u32string>({
  U"个", U"個", U"位", U"名", U"人", U"次", U"年", U"天", U"周", U"週", U"岁", U"歲", U"倍", U"种", U"種",
  U"件", U"条", U"條", U"只", U"隻", U"张", U"張", U"本", U"辆", U"輛", U"家", U"所", U"架", U"部",
  U"台", U"支", U"项", U"項", U"场", U"場", U"份", U"批", U"层", U"層", U"间", U"間", U"套", U"双", U"雙",
  U"对", U"對", U"万", U"萬", U"亿", U"億", U"千", U"百", U"点", U"點", U"小时", U"小時", U"分钟", U"分鐘",
  U"秒钟", U"秒鐘", U"公里", U"公斤", U"米", U"句", U"口", U"头", U"頭", U"匹", U"颗", U"顆", U"块", U"塊",
  U"片", U"枚", U"首", U"篇", U"段", U"季", U"届", U"屆", U"轮", U"輪", U"局", U"盘", U"盤", U"杯", U"瓶",
  U"箱", U"款", U"宗", U"船"
});

// Words before a number that make it a code read digit by digit: "尾号1702" -> "尾号一七零二"
inline const auto kDigitCues = std::to_array<std::u32string>({
  U"尾号", U"尾號", U"号码", U"號碼", U"电话", U"電話", U"手机", U"手機", U"编号", U"編號", U"房号", U"房號",
  U"车次", U"車次", U"航班", U"代码", U"代碼", U"邮编", U"郵編", U"密码", U"密碼", U"验证码", U"驗證碼",
  U"账号", U"帳號", U"卡号", U"卡號", U"热线", U"熱線", U"专线", U"專線", U"拨打", U"撥打", U"致电", U"致電"
});
// Words before two numbers that make them a score: "比分为1-2" -> "比分为一比二"
inline const auto kScoreCues = std::to_array<std::u32string>({U"比分", U"比數", U"比数", U"战绩", U"戰績"});
// Words after two numbers that make them a score: "1-3落后" -> "一比三落后"
inline const auto kScoreResults = std::to_array<std::u32string>({
  U"落后", U"落後", U"领先", U"領先", U"击败", U"擊敗", U"战胜", U"戰勝", U"获胜", U"獲勝", U"取胜", U"取勝",
  U"告负", U"告負", U"输给", U"輸給", U"力克", U"大胜", U"大勝", U"不敌", U"不敵", U"落败", U"落敗", U"逆转", U"逆轉",
  U"反胜", U"反勝", U"险胜", U"險勝", U"完胜", U"完勝", U"击退", U"擊退"
});
// Words between a cue and its number: "尾号是3385"
inline const auto kCueLinks = std::to_array<std::u32string>({U"是", U"为", U"為", U": ", U":", U" "});

// Currencies written before a number, in the order they are tried: "$5" -> "五美元"
struct Currency {
  std::u32string symbol;
  std::u32string name;
};
inline const auto kCurrencies = std::to_array<Currency>({
  {U"US$", U"美元"}, {U"HK$", U"港元"}, {U"NT$", U"新台币"}, {U"C$", U"加元"}, {U"A$", U"澳元"}, {U"S$", U"新加坡元"},
  {U"$", U"美元"}, {U"￥", U"元"}, {U"¥", U"元"}, {U"€", U"欧元"}, {U"£", U"英镑"}, {U"₩", U"韩元"}, {U"₹", U"卢比"},
  {U"CNY", U"人民币"}, {U"RMB", U"人民币"}, {U"USD", U"美元"}, {U"HKD", U"港元"}, {U"EUR", U"欧元"},
  {U"GBP", U"英镑"}, {U"JPY", U"日元"}
});

// Currencies written as words after a number, before a unit they price: "5元/kg" -> "五元每千克"
inline const auto kPriceCurrencies = std::to_array<std::u32string>({
  U"元", U"圆", U"圓", U"块", U"塊", U"美元", U"欧元", U"港元", U"日元", U"英镑"
});

// Scale words written after the number of an amount, longest first: "$1.5万" -> "一点五万美元"
inline const auto kScales = std::to_array<std::u32string>({
  U"万亿", U"千万", U"百万", U"十万", U"万", U"萬", U"亿", U"億", U"千", U"百"
});

// Units written after a number: "25kg" -> "二十五千克"; "10km/h" -> "每小时十公里". A lone 2 counts as 两 before
// every unit but degrees.
struct Unit {
  std::u32string symbol;
  std::u32string word;
  std::u32string per = U""; // Read before the number
  bool counted = true;      // 2 is 两
};
inline const auto kUnits = std::to_array<Unit>({
  {U"km/h", U"公里", U"每小时"}, {U"公里/小时", U"公里每小时"}, {U"km/s", U"公里", U"每秒"}, {U"m/s", U"米", U"每秒"},
  {U"km²", U"平方公里"}, {U"m²", U"平方米"}, {U"m2", U"平方米"}, {U"cm²", U"平方厘米"}, {U"m³", U"立方米"},
  {U"kg", U"千克"}, {U"KG", U"千克"}, {U"mg", U"毫克"}, {U"g", U"克"},
  {U"km", U"公里"}, {U"cm", U"厘米"}, {U"CM", U"厘米"}, {U"mm", U"毫米"}, {U"nm", U"纳米"}, {U"μm", U"微米"}, {U"m", U"米"},
  {U"ml", U"毫升"}, {U"mL", U"毫升"}, {U"L", U"升"},
  {U"ms", U"毫秒"}, {U"min", U"分钟"},
  {U"℃", U"摄氏度", U"", false}, {U"°C", U"摄氏度", U"", false}, {U"℉", U"华氏度", U"", false},
  {U"°F", U"华氏度", U"", false}, {U"°", U"度", U"", false},
  {U"kWh", U"千瓦时"}, {U"kW", U"千瓦"}, {U"W", U"瓦"}, {U"mAh", U"毫安时"}, {U"V", U"伏"},
  {U"kHz", U"千赫"}, {U"MHz", U"兆赫"}, {U"GHz", U"吉赫"}, {U"Hz", U"赫兹"},
  {U"%", U"", U"百分之", false}, {U"％", U"", U"百分之", false}
});

// Operators between numbers, longest first: "1+2" -> "一加二". A minus counts only beside an equals sign.
struct Operator {
  std::u32string symbol;
  std::u32string word;
};
inline const auto kOperators = std::to_array<Operator>({
  {U"<=", U"小于等于"}, {U">=", U"大于等于"}, {U"≤", U"小于等于"}, {U"≥", U"大于等于"}, {U"≠", U"不等于"}, {U"≈", U"约等于"},
  {U"+", U"加"}, {U"=", U"等于"}, {U"×", U"乘"}, {U"÷", U"除以"}, {U"<", U"小于"}, {U">", U"大于"}, {U"-", U"减"}
});

// A sign before a number, but not between two (an operator): "(+39)" -> "(加三十九)", but "C++", "DVD+R", "1+2"
inline bool before_numeral(const ::verbalis::text::Context& context) {
  std::u32string_view head = context.head();
  std::u32string_view tail = context.tail();
  bool numeral = !tail.empty() && (::verbalis::text::unicode::isdigit(tail[0]) ||
                                   std::u32string_view(U"零一二两三四五六七八九十百千").find(tail[0]) != std::u32string_view::npos);
  return numeral && (head.empty() || !::verbalis::text::unicode::isdigit(head.back()));
}

// Symbols left after the numbers: "±1" -> "正负一", "y = x" -> "y 等于 x"
inline const ::verbalis::text::Readings kSymbols = {
  {U"+", {{U"加", before_numeral}}}, {U"≈", {{U"约", before_numeral}}}, {U"±", {U"正负"}}, {U"=", {U"等于"}}
};

} // namespace verbalis::languages::zh
