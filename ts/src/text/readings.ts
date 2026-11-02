import type { Context } from "./context.ts";
import { isDigitAfter, skipSpace } from "./scan.ts";
import { contains, str } from "./strings.ts";
import { isAlpha, isDigit, isSpace, isUpper } from "./unicode.ts";

// A condition on the context of a written phrase, which decides between its readings.
export type Condition = (context: Context) => boolean;

// A way of saying a written phrase (a symbol, an abbreviation): its words, always or when a condition holds.
// Empty words drop the phrase.
export type Reading = string | { readonly words: string; readonly when: Condition };

// Written phrases with their readings: the first one whose condition holds is used.
export type Readings = Readonly<Record<string, readonly Reading[]>>;

// The words of the first reading whose condition holds, or undefined if there is none.
export function read(readings: readonly Reading[], context: Context): string | undefined {
  for (const reading of readings) {
    if (typeof reading === "string") {
      return reading;
    }
    if (reading.when(context)) {
      return reading.words;
    }
  }
  return undefined;
}

// Two letters right before, maybe then digits: Google+, LAB4+
export function afterName(context: Context): boolean {
  // Digits right after the letters count with them: LAB4+, but not 50%+ or HDR 10+
  const head = context.head();
  let end = head.length;
  while (end > 0 && isDigit(head[end - 1])) {
    end--;
  }
  return end >= 2 && isAlpha(head[end - 1]) && isAlpha(head[end - 2]);
}

// A digit right after, possibly after a space: nr 5, nr5
export function beforeNumber(context: Context): boolean {
  const tail = context.tail();
  return isDigitAfter(tail, skipSpace(tail, 0));
}

// A number after a comparison or approximation sign, maybe after a currency sign: p<0.05, ~$145, (< 60%).
// Not between numbers (2018~2019), in an arrow (->), a tag (<27>, <sup>2), a chain (100<P<250), or a heart (<3), nor
// spaced as a separator (a > 9h).
export function beforeValue(context: Context): boolean {
  const head = context.head();
  const tail = context.tail();
  // Not an arrow (->, =>), between numbers (2018~2019, a range), or in a tag or a chain (<sup>2, </i> 10, 100<P<250)
  const last = head[head.length - 1];
  if (head.length > 0 && (isDigit(last) || last === "-" || last === "=")) {
    return false;
  }
  for (let i = head.length; i > 0 && !isSpace(head[i - 1]); --i) {
    if (head[i - 1] === "<") {
      return false;
    }
  }
  // A space after only without one before: THI< 84, (< 60%), but not "octobre > 9h", "ME ~ 8pm"; a spaced <, ≤, ≥,
  // or ≈ only compares: creatinina < 30
  let i = 0;
  if (i < tail.length && isSpace(tail[i])) {
    if (head.length > 0 && isSpace(last) && (str(context.text()) === ">" || str(context.text()) === "~")) {
      return false;
    }
    i++;
  }
  if (i < tail.length && contains("$€£", tail[i])) {
    i++;
  }
  const number = i;
  while (
    i < tail.length &&
    (isDigit(tail[i]) || (i > number && contains(".,", tail[i]) && i + 1 < tail.length && isDigit(tail[i + 1])))
  ) {
    i++;
  }
  // Not a tag or a citation (<27>), nor a heart (<3, but x < 3 compares)
  const closed = i < tail.length && tail[i] === ">";
  const heart = str(context.text()) === "<" && number === 0 && str(tail, number, i) === "3";
  return i > number && !closed && !heart;
}

// A capitalized word after: St. Louis
export function beforeName(context: Context): boolean {
  const word = context.after();
  return word !== "" && isUpper(String.fromCodePoint(word.codePointAt(0) as number));
}

// The word before is one of given words: im § 5
export function afterAny(words: readonly string[]): Condition {
  const set = new Set(words);
  return (context) => set.has(context.before());
}
