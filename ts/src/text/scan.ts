/**
 * Scanning: primitives shared by the steps and the number engine, lookarounds and table keys at a position of a text.
 */
import { contains, size, startsWith, type Text } from "./strings.ts";
import { isDigit, isSpace, isUpper, isWord } from "./unicode.ts";

// Keys of a table matched in text (units, currencies, abbreviations); the longest matching one wins.
export type Keys = readonly string[];

/**
 * Checks if a position starts a word: no word character stands right before it.
 * @param also other characters which may not stand there, e.g. "." to reject the "g" of "e.g".
 */
export function isClearBefore(text: Text, position: number, also = ""): boolean {
  return position === 0 || (!isWord(text[position - 1]) && !contains(also, text[position - 1]));
}

/**
 * Checks if a position ends a word: no word character stands at it, or the text ends there.
 * @param also other characters which may not stand there.
 */
export function isClearAfter(text: Text, position: number, also = ""): boolean {
  return position >= text.length || (!isWord(text[position]) && !contains(also, text[position]));
}

/**
 * Checks if a digit stands at a position, possibly after one of given marks: "5", ".5".
 * @param marks characters which may stand between the position and the digit.
 */
export function isDigitAfter(text: Text, position: number, marks = ""): boolean {
  if (position < text.length && contains(marks, text[position])) {
    position++;
  }
  return position < text.length && isDigit(text[position]);
}

/**
 * Checks if a sentence ends at a position: only whitespaces follow it.
 * @param capital whether a capital letter after the whitespaces also starts a new sentence: "2020 r. Potem".
 */
export function isSentenceEnd(text: Text, position: number, capital = false): boolean {
  while (position < text.length && isSpace(text[position])) {
    position++;
  }
  return position === text.length || (capital && isUpper(text[position]));
}

// End of the run of digits starting at a position.
export function digitsEnd(text: Text, position: number): number {
  while (position < text.length && isDigit(text[position])) {
    position++;
  }
  return position;
}

// Position after an optional whitespace.
export function skipSpace(text: Text, position: number): number {
  return position < text.length && isSpace(text[position]) ? position + 1 : position;
}

// The keys starting at a position, longest first.
export function keysAt(text: Text, position: number, keys: Keys): string[] {
  return keys.filter((key) => startsWith(text, position, key)).sort((a, b) => size(b) - size(a));
}

// The non-empty words, joined with a separator.
export function join(words: readonly string[], separator = " "): string {
  return words.filter((word) => word !== "").join(separator);
}
