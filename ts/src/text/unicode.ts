import { ALPHA_RANGES, LOWER, TABLE_SIZE, UPPER } from "./unicode-table.ts";

/**
 * Character classes of code points, each given as a one-code-point string. Out-of-range reads (undefined) have no
 * class. Every code point from TABLE_SIZE on, and so every UTF-16 surrogate, is neither a letter, a digit, nor a space.
 */

const ALPHA = new Uint8Array(TABLE_SIZE);
for (const [first, last] of ALPHA_RANGES) {
  ALPHA.fill(1, first, last + 1);
}

function code(c: string | undefined): number {
  return c === undefined || c === "" ? -1 : (c.codePointAt(0) as number);
}

// A Latin letter, accented ones included
export function isLatin(c: string | undefined): boolean {
  const n = code(c);
  return (
    (n >= 0x41 && n <= 0x5a) ||
    (n >= 0x61 && n <= 0x7a) ||
    (n >= 0xc0 && n <= 0xd6) ||
    (n >= 0xd8 && n <= 0xf6) ||
    (n >= 0xf8 && n <= 0x24f)
  );
}

export function isAlpha(c: string | undefined): boolean {
  const n = code(c);
  return n >= 0 && n < TABLE_SIZE && ALPHA[n] === 1;
}

export function isDigit(c: string | undefined): boolean {
  const n = code(c);
  return n >= 0x30 && n <= 0x39;
}

export function isAlnum(c: string | undefined): boolean {
  return isAlpha(c) || isDigit(c);
}

// A letter, a digit or "_": a word character of a regex (\w)
export function isWord(c: string | undefined): boolean {
  return isAlnum(c) || c === "_";
}

// Also the Unicode spaces: no-break, thin (14 h 20), ideographic, ...
export function isSpace(c: string | undefined): boolean {
  const n = code(c);
  return (
    (n >= 0x09 && n <= 0x0d) ||
    n === 0x20 ||
    n === 0x85 ||
    n === 0xa0 ||
    n === 0x1680 ||
    (n >= 0x2000 && n <= 0x200a) ||
    n === 0x2028 ||
    n === 0x2029 ||
    n === 0x202f ||
    n === 0x205f ||
    n === 0x3000
  );
}

export function isLower(c: string | undefined): boolean {
  return isAlpha(c) && UPPER.has(code(c));
}

export function isUpper(c: string | undefined): boolean {
  return isAlpha(c) && LOWER.has(code(c));
}

export function toLower(c: string): string {
  const lower = LOWER.get(code(c));
  return lower === undefined ? c : String.fromCodePoint(lower);
}

export function toUpper(c: string): string {
  const upper = UPPER.get(code(c));
  return upper === undefined ? c : String.fromCodePoint(upper);
}
