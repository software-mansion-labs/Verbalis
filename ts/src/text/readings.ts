import type { Context } from "./context.ts";
import { isDigitAfter, skipSpace } from "./scan.ts";
import { isAlpha, isUpper } from "./unicode.ts";

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

// Two letters right before: Google+
export function afterName(context: Context): boolean {
  const head = context.head();
  return head.length >= 2 && isAlpha(head[head.length - 1]) && isAlpha(head[head.length - 2]);
}

// A digit right after, possibly after a space: nr 5, nr5
export function beforeNumber(context: Context): boolean {
  const tail = context.tail();
  return isDigitAfter(tail, skipSpace(tail, 0));
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
