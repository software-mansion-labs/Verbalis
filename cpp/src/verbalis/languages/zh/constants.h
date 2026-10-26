#pragma once

#include <verbalis/steps/normalize.h>

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

// Unit of the digit at each power of ten, up to 10^15.
inline const std::array<std::u32string, 16> kUnits = {
  U"", U"十", U"百", U"千", U"万", U"十", U"百", U"千",
  U"亿", U"十", U"百", U"千", U"万", U"十", U"百", U"千"
};

// Decimal digits read after the point; longer fractions are truncated.
inline constexpr size_t kMaxDecimals = 16;

inline constexpr char32_t kNegative = U'负';
inline constexpr char32_t kPoint = U'点';
inline const std::u32string kRangeTo = U"到";
inline const std::u32string kFraction = U"分之";
inline const std::u32string kPercent = U"百分之";
inline const std::u32string kCelsius = U"摄氏度";

// A range like "3-5" is only read as one when a measure word follows it.
inline const std::array<std::u32string, 34> kMeasureWords = {
  U"斤", U"克", U"千克", U"公斤", U"吨", U"米", U"厘米", U"毫米", U"公里", U"升", U"毫升",
  U"元", U"角", U"分", U"个", U"只", U"条", U"张", U"块", U"瓶", U"杯", U"份", U"本",
  U"辆", U"台", U"匹", U"头", U"位", U"亩", U"小时", U"分钟", U"秒", U"天", U"半"
};

} // namespace verbalis::languages::zh
