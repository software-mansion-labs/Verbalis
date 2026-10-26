import type { Characters } from "../../steps/normalize.ts";

// Full-width punctuation in the forms Mandarin speech models expect; it replaces the default classes
export const CHARACTERS: Characters = [
  { characters: "、，", canonical: ", " }, { characters: "。．", canonical: ". " }, { characters: "！", canonical: "! " },
  { characters: "：", canonical: ": " }, { characters: "；", canonical: "; " }, { characters: "？", canonical: "? " },
  { characters: "«《「【", canonical: " “" }, { characters: "»》」】", canonical: "” " },
  { characters: "（", canonical: " (" }, { characters: "）", canonical: ") " },
];

export const DIGITS = ["零", "一", "二", "三", "四", "五", "六", "七", "八", "九"] as const;

// Unit of the digit at each power of ten, up to 10^15.
export const UNITS = [
  "", "十", "百", "千", "万", "十", "百", "千",
  "亿", "十", "百", "千", "万", "十", "百", "千",
] as const;

// Decimal digits read after the point; longer fractions are truncated.
export const MAX_DECIMALS = 16;

export const NEGATIVE = "负";
export const POINT = "点";
export const RANGE_TO = "到";
export const FRACTION = "分之";
export const PERCENT = "百分之";
export const CELSIUS = "摄氏度";

// A range like "3-5" is only read as one when a measure word follows it.
export const MEASURE_WORDS = [
  "斤", "克", "千克", "公斤", "吨", "米", "厘米", "毫米", "公里", "升", "毫升",
  "元", "角", "分", "个", "只", "条", "张", "块", "瓶", "杯", "份", "本",
  "辆", "台", "匹", "头", "位", "亩", "小时", "分钟", "秒", "天", "半",
] as const;
