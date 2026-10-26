import { toLower as lowerOf } from "./unicode.ts";

/**
 * A text as its code points, one per element: positions and lengths count code points, as in the C++ core.
 * Words, keys and readings stay strings.
 */
export type Text = readonly string[];

export function textOf(s: string): string[] {
  return Array.from(s);
}

// The code points [begin, end) of a text, as a string.
export function str(text: Text, begin = 0, end: number = text.length): string {
  return text.slice(begin, end).join("");
}

// The number of code points of a string.
export function size(s: string): number {
  let n = 0;
  for (const _ of s) {
    n++;
  }
  return n;
}

// Whether a string contains a character; never for an out-of-range read.
export function contains(s: string, c: string | undefined): boolean {
  return c !== undefined && c !== "" && s.includes(c);
}

// Whether a text holds a string at a position.
export function startsWith(text: Text, position: number, s: string): boolean {
  let i = position;
  for (const c of s) {
    if (text[i++] !== c) {
      return false;
    }
  }
  return true;
}

// A string in lowercase.
export function toLower(s: string): string {
  let lower = "";
  for (const c of s) {
    lower += lowerOf(c);
  }
  return lower;
}

// The parts of a string between occurrences of a character, empty parts included, but none after a final separator
// (unlike String.split): "a  b " -> ["a", "", "b"].
export function split(s: string, separator: string): string[] {
  const parts = s.split(separator);
  return parts.at(-1) === "" ? parts.slice(0, -1) : parts;
}
