import type { Characters } from "../../steps/normalize.ts";
import type { Context } from "../../text/context.ts";
import type { Readings } from "../../text/readings.ts";
import { isDigit } from "../../text/unicode.ts";

// Full-width punctuation in the forms Mandarin speech models expect; it replaces the default classes
export const CHARACTERS: Characters = [
  { characters: "、，", canonical: ", " }, { characters: "。．", canonical: ". " }, { characters: "！", canonical: "! " },
  { characters: "：", canonical: ": " }, { characters: "；", canonical: "; " }, { characters: "？", canonical: "? " },
  { characters: "«《「【", canonical: " “" }, { characters: "»》」】", canonical: "” " },
  { characters: "（", canonical: " (" }, { characters: "）", canonical: ") " },
];

export const DIGITS = ["零", "一", "二", "三", "四", "五", "六", "七", "八", "九"] as const;

// Unit of each digit of a group of four, and of each group: 1234 5678 -> 一千二百三十四万五千六百七十八
export const DIGIT_UNITS = ["", "十", "百", "千"] as const;
export const GROUP_UNITS = ["", "万", "亿", "万亿"] as const;

// Decimal digits read after the point; longer fractions are truncated.
export const MAX_DECIMALS = 16;

export const NEGATIVE = "负";
export const POINT = "点";
export const TWO = "两";
export const RANGE_TO = "到";
export const RATIO = "比";
export const FRACTION = "分之";
export const PERCENT = "百分之";

// Times: "13:10:36" -> "十三点十分三十六秒"
export const HOUR = "点";
export const MINUTE = "分";
export const SECOND = "秒";
export const MORNING = "上午";
export const NOON = "中午";
export const AFTERNOON = "下午";

// Dates: "2008-08-08" -> "二零零八年八月八日"; "2934bc" -> "公元前二九三四年"
export const YEAR = "年";
export const MONTH = "月";
export const DAY = "日";
export const ANNO_DOMINI = "公元";
export const BEFORE_CHRIST = "公元前";
// Characters that end a date: a dash after one is no minus ("189年-232年")
export const DATE_MARKERS = "年月日号號";

// After a year of three digits, a count of years rather than a year: "700年前" -> "七百年前"
export const YEAR_COUNTS = [
  "前", "后", "後", "来", "來", "间", "間", "多", "内", "內", "以来", "以來",
] as const;

// Words after a year written without 年: "2020款" -> "二零二零款"
export const YEAR_WORDS = [
  "款", "届", "屆", "财年", "財年", "年度", "赛季", "賽季", "球季",
] as const;

// Words a lone 2 counts as 两 before: "2个" -> "两个", "2万" -> "两万"
export const TWO_BEFORE = [
  "个", "個", "位", "名", "人", "次", "年", "天", "周", "週", "岁", "歲", "倍", "种", "種",
  "件", "条", "條", "只", "隻", "张", "張", "本", "辆", "輛", "家", "所", "架", "部",
  "台", "支", "项", "項", "场", "場", "份", "批", "层", "層", "间", "間", "套", "双", "雙",
  "对", "對", "万", "萬", "亿", "億", "千", "百", "点", "點", "小时", "小時", "分钟", "分鐘",
  "秒钟", "秒鐘", "公里", "公斤", "米", "句", "口", "头", "頭", "匹", "颗", "顆", "块", "塊",
  "片", "枚", "首", "篇", "段", "季", "届", "屆", "轮", "輪", "局", "盘", "盤", "杯", "瓶",
  "箱", "款", "宗", "船",
] as const;

// Words before a number that make it a code read digit by digit: "尾号1702" -> "尾号一七零二"
export const DIGIT_CUES = [
  "尾号", "尾號", "号码", "號碼", "电话", "電話", "手机", "手機", "编号", "編號", "房号", "房號",
  "车次", "車次", "航班", "代码", "代碼", "邮编", "郵編", "密码", "密碼", "验证码", "驗證碼",
  "账号", "帳號", "卡号", "卡號", "热线", "熱線", "专线", "專線", "拨打", "撥打", "致电", "致電",
] as const;
// Words before two numbers that make them a score: "比分为1-2" -> "比分为一比二"
export const SCORE_CUES = ["比分", "比數", "比数", "战绩", "戰績"] as const;
// Words after two numbers that make them a score: "1-3落后" -> "一比三落后"
export const SCORE_RESULTS = [
  "落后", "落後", "领先", "領先", "击败", "擊敗", "战胜", "戰勝", "获胜", "獲勝", "取胜", "取勝",
  "告负", "告負", "输给", "輸給", "力克", "大胜", "大勝", "不敌", "不敵", "落败", "落敗", "逆转", "逆轉",
  "反胜", "反勝", "险胜", "險勝", "完胜", "完勝", "击退", "擊退",
] as const;
// Words between a cue and its number: "尾号是3385"
export const CUE_LINKS = ["是", "为", "為", ": ", ":", " "] as const;

// Currencies written before a number, in the order they are tried: "$5" -> "五美元"
export interface Currency {
  symbol: string;
  name: string;
}
export const CURRENCIES: readonly Currency[] = [
  { symbol: "US$", name: "美元" }, { symbol: "HK$", name: "港元" }, { symbol: "NT$", name: "新台币" },
  { symbol: "C$", name: "加元" }, { symbol: "A$", name: "澳元" }, { symbol: "S$", name: "新加坡元" },
  { symbol: "$", name: "美元" }, { symbol: "￥", name: "元" }, { symbol: "¥", name: "元" },
  { symbol: "€", name: "欧元" }, { symbol: "£", name: "英镑" }, { symbol: "₩", name: "韩元" },
  { symbol: "₹", name: "卢比" },
  { symbol: "CNY", name: "人民币" }, { symbol: "RMB", name: "人民币" }, { symbol: "USD", name: "美元" },
  { symbol: "HKD", name: "港元" }, { symbol: "EUR", name: "欧元" }, { symbol: "GBP", name: "英镑" },
  { symbol: "JPY", name: "日元" },
];

// Scale words written after the number of an amount, longest first: "$1.5万" -> "一点五万美元"
export const SCALES = [
  "万亿", "千万", "百万", "十万", "万", "萬", "亿", "億", "千", "百",
] as const;

// Units written after a number: "25kg" -> "二十五千克"; "10km/h" -> "每小时十公里". A lone 2 counts as 两 before
// every unit but degrees.
export interface Unit {
  symbol: string;
  word: string;
  per: string; // Read before the number
  counted: boolean; // 2 is 两
}
function unit(symbol: string, word: string, per = "", counted = true): Unit {
  return { symbol, word, per, counted };
}
export const UNITS: readonly Unit[] = [
  unit("km/h", "公里", "每小时"), unit("公里/小时", "公里每小时"), unit("km/s", "公里", "每秒"), unit("m/s", "米", "每秒"),
  unit("km²", "平方公里"), unit("m²", "平方米"), unit("m2", "平方米"), unit("cm²", "平方厘米"), unit("m³", "立方米"),
  unit("kg", "千克"), unit("KG", "千克"), unit("mg", "毫克"), unit("g", "克"),
  unit("km", "公里"), unit("cm", "厘米"), unit("CM", "厘米"), unit("mm", "毫米"), unit("nm", "纳米"), unit("μm", "微米"), unit("m", "米"),
  unit("ml", "毫升"), unit("mL", "毫升"), unit("L", "升"),
  unit("ms", "毫秒"), unit("min", "分钟"),
  unit("℃", "摄氏度", "", false), unit("°C", "摄氏度", "", false), unit("℉", "华氏度", "", false),
  unit("°F", "华氏度", "", false), unit("°", "度", "", false),
  unit("kWh", "千瓦时"), unit("kW", "千瓦"), unit("W", "瓦"), unit("mAh", "毫安时"), unit("V", "伏"),
  unit("kHz", "千赫"), unit("MHz", "兆赫"), unit("GHz", "吉赫"), unit("Hz", "赫兹"),
  unit("%", "", "百分之", false), unit("％", "", "百分之", false),
];

// Operators between numbers, longest first: "1+2" -> "一加二". A minus counts only beside an equals sign.
export interface Operator {
  symbol: string;
  word: string;
}
export const OPERATORS: readonly Operator[] = [
  { symbol: "<=", word: "小于等于" }, { symbol: ">=", word: "大于等于" }, { symbol: "≤", word: "小于等于" },
  { symbol: "≥", word: "大于等于" }, { symbol: "≠", word: "不等于" }, { symbol: "+", word: "加" },
  { symbol: "=", word: "等于" }, { symbol: "×", word: "乘" }, { symbol: "÷", word: "除以" },
  { symbol: "<", word: "小于" }, { symbol: ">", word: "大于" }, { symbol: "-", word: "减" },
];

// A sign before a number, but not between two (an operator): "(+39)" -> "(加三十九)", but "C++", "DVD+R", "1+2"
function beforeNumeral(context: Context): boolean {
  const head = context.head();
  const tail = context.tail();
  const numeral = tail.length > 0 && (isDigit(tail[0]) || "零一二两三四五六七八九十百千".includes(tail[0]));
  return numeral && (head.length === 0 || !isDigit(head[head.length - 1]));
}

// Symbols left after the numbers: "±1" -> "正负一", "y = x" -> "y 等于 x"
export const SYMBOLS: Readings = {
  "+": [{ words: "加", when: beforeNumeral }], "±": ["正负"], "=": ["等于"],
};
