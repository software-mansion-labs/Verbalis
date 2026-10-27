# zh gold notes

## Added readings

Each reading below joins the gold reading as equally correct; the gold reading stays.

- `10000100000`, `100000100000`, and their `第` ordinals: 一百亿零一十万 is the formal reading of 零十万 (一十 after 零).
- `2015年08月08号`, `6号`, `30号`: 号 is read as written (八号); the gold reading turns it into 日, a different word.
- `50年`: without context, fifty years (五十年) is the commoner reading in running text; the gold reads a year ('50).
- `￥5`: 五元 is how a price in yuan is read; 五人民币 is the gold reading.
- `03:04`, `03:04:05`, and the sentence with `下午03:04:05`: minutes below ten with 零 (三点零四分) are as usual as
  without (三点四分).
- `03:04:05am`, `03:04pm`, `10:04am`, `10:04pm`: am and pm read as 上午 and 下午 before the time, as in the
  WeTextProcessing case `1:02:36am`; the NeMo readings keep the Latin letters.
- `13:10pm`: 13 o'clock is already in the afternoon, so pm adds nothing to read.
- `110`: a bare number is 一百一十; 幺幺零 is the reading of the emergency number, which needs context.
- `尾号1702`: 一 is as usual as 幺 for the digit 1 of a code.
- `200`: 二百 is as usual as 两百 (NeMo's own cardinal case `200` reads 二百).

## Unreachable cases

- `wetext/sport 中国1-2` (gold 一比二): a dash between two numbers whose second is larger is a range in running text
  (`第1-3名`, `15-17日`, `1-2年`); only knowing that 中国 is a team makes it a score. Verbalis reads 一到二.
  A score cue (比分, 战绩) or a falling pair (`23-0`) reads 比.
- `wetext/sport 拉齐奥 2/2` (gold 二比二): a slash between numbers is a fraction or a date; nothing in the fragment
  says score.
