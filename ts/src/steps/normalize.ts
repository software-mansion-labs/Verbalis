import type { Document, Edit } from "../text/document.ts";
import { contains, str, type Text } from "../text/strings.ts";
import { isAlpha, isDigit, isSpace, isWord } from "../text/unicode.ts";

// A class of characters written in one canonical form: all quotes as ".
export interface CharacterClass {
  readonly characters: string; // « » „ “ ” ‟ ˝
  readonly canonical: string; // "; empty to remove them
  readonly letter?: boolean; // A letter read in the wrong encoding (zupe³nie): replaced only next to a letter
}

export type Characters = readonly CharacterClass[];

// The character classes of Echo (echo/preprocessing/characters.py), the default of every language
export const DEFAULT_CHARACTERS: Characters = [
  { characters: "…", canonical: "..." }, // Ellipsis
  { characters: "«»„“”‟˝", canonical: '"' }, // Quotes
  { characters: "‘’‚‛´`ʼ", canonical: "'" }, // Apostrophes
  { characters: "​‌‍﻿", canonical: "" }, // Invisible: zero-width space, non-joiner, joiner, BOM
];

// Dashes besides "-": em, en, figure dash, horizontal bar
export const DASHES = "—–‒―";

// Whether a character is a dash or a hyphen
function isDash(c: string | undefined): boolean {
  return c === "-" || contains(DASHES, c);
}

// The end of a dash starting at a position: a dash character or two hyphens and more (--); the position if none
function dashEnd(text: Text, position: number): number {
  if (position < text.length && contains(DASHES, text[position])) {
    return position + 1;
  }
  let end = position;
  while (end < text.length && text[end] === "-") {
    end++;
  }
  return end - position >= 2 ? end : position;
}

// Whether an ordinal sign at a position is a degree: 18ºC, 12 ºF
function isDegree(text: Text, position: number): boolean {
  let before = position;
  while (before > 0 && isSpace(text[before - 1])) {
    before--;
  }
  const unit = position + 1;
  return (
    text[position] === "º" &&
    before > 0 &&
    isDigit(text[before - 1]) &&
    unit < text.length &&
    (text[unit] === "C" || text[unit] === "F") &&
    (unit + 1 === text.length || !isWord(text[unit + 1]))
  );
}

// The characters of the Windows-1252 bytes 0x80 to 0x9F; an undefined byte reads as the control character of its value
const WINDOWS_1252 = "€\u0081‚ƒ„…†‡ˆ‰Š‹Œ\u008dŽ\u008f\u0090‘’“”•–—˜™š›œ\u009džŸ";

// The Windows-1252 byte a character was read from, or -1 for none
function byteOf(c: string | undefined): number {
  if (c === undefined) {
    return -1;
  }
  const code = c.codePointAt(0) as number;
  if (code >= 0x80 && code <= 0xff) {
    return code;
  }
  const i = WINDOWS_1252.indexOf(c);
  return i === -1 ? -1 : 0x80 + i;
}

// A character of UTF-8 read as Windows-1252 at a position (Ä™ -> ę), or undefined
function readUtf8(text: Text, position: number): Edit | undefined {
  const lead = byteOf(text[position]);
  // prettier-ignore
  const length = lead >= 0xc2 && lead <= 0xdf ? 2 : lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
  if (length === 0 || position + length > text.length) {
    return undefined;
  }
  let c = lead & (0x7f >> length);
  for (let k = 1; k < length; ++k) {
    const byte = byteOf(text[position + k]);
    if (byte < 0x80 || byte > 0xbf) {
      return undefined;
    }
    c = (c << 6) | (byte & 0x3f);
  }
  if ((length === 3 && c < 0x800) || (length === 4 && (c < 0x10000 || c > 0x10ffff))) {
    return undefined;
  }
  return { span: { begin: position, end: position + length }, text: String.fromCodePoint(c) };
}

/**
 * Writes equivalent characters in one canonical form, before anything reads the text:
 * quotes as ", apostrophes as ', an ellipsis as ..., dashes as - or " - ", a degree written as an ordinal sign
 * (18ºC) as °, and no invisible characters.
 *
 * A language whose classes have letters read in the wrong encoding also gets UTF-8 read as Windows-1252 repaired:
 * siÄ™ -> się.
 */
export class Normalizer {
  readonly #canonical = new Map<string, string>(); // Of each character of the classes: ’ -> '
  readonly #letters = new Set<string>(); // The characters of letter classes
  #utf8 = false; // Whether UTF-8 read as Windows-1252 is repaired

  constructor(characters: Characters = DEFAULT_CHARACTERS) {
    for (const { characters: members, canonical, letter } of characters) {
      for (const c of members) {
        this.#canonical.set(c, canonical);
        if (letter) {
          this.#letters.add(c);
          this.#utf8 = true;
        }
      }
    }
  }

  apply(document: Document): void {
    const text = document.text();
    for (let i = 0; i < text.length;) {
      const edit = (this.#utf8 ? readUtf8(text, i) : undefined) ?? readDash(text, i);
      if (edit) {
        document.replace(edit.span, edit.text);
        i = edit.span.end;
        continue;
      }
      const canonical = this.#canonical.get(text[i]);
      if (isDegree(text, i)) {
        document.replace({ begin: i, end: i + 1 }, "°");
      } else if (canonical !== undefined && (!this.#letters.has(text[i]) || this.#isLetter(text, i))) {
        document.replace({ begin: i, end: i + 1 }, canonical);
      }
      i++;
    }
  }

  // A letter class character at a position stands for a letter: next to a letter, but not after a metric unit (m³)
  #isLetter(text: Text, position: number): boolean {
    const letter = (i: number) => isAlpha(text[i]) || this.#letters.has(text[i]);
    if (position + 1 < text.length && letter(position + 1)) {
      return true;
    }
    let start = position;
    while (start > 0 && letter(start - 1)) {
      start--;
    }
    const word = str(text, start, position);
    return word !== "" && word !== "m" && word !== "cm" && word !== "dm" && word !== "km" && word !== "mm";
  }
}

/**
 * A dash at a position: "-" inside a word or a number (1990–2000), and " - " for a run of dashes with the spaces
 * around it elsewhere (a – b), except between numbers, which stay a range (2020 – 2022 -> 2020-2022).
 * @returns its edit, or undefined if no dash starts there ("-" alone is a hyphen).
 */
function readDash(text: Text, position: number): Edit | undefined {
  let end = dashEnd(text, position);
  if (end === position) {
    return undefined;
  }

  // 1. Inside a word or a number: Szczurek–Żelazko, 1990–2000, a--b
  if (position > 0 && isWord(text[position - 1]) && end < text.length && isWord(text[end])) {
    return { span: { begin: position, end }, text: "-" };
  }

  // 2. Elsewhere, the whole run of dashes with the spaces around it: "a – b", "a – — b", "a ---- b"
  let begin = position;
  while (begin > 0 && isSpace(text[begin - 1])) {
    begin--;
  }
  for (let next = end; next < text.length;) {
    while (next < text.length && isSpace(text[next])) {
      next++;
    }
    if (next === text.length || !isDash(text[next])) {
      break;
    }
    while (next < text.length && isDash(text[next])) {
      next++;
    }
    end = next;
  }
  while (end < text.length && isSpace(text[end])) {
    end++;
  }

  // Between numbers it stays a range: 2020 – 2022 -> 2020-2022
  const range = begin > 0 && isDigit(text[begin - 1]) && end < text.length && isDigit(text[end]);
  return { span: { begin, end }, text: range ? "-" : " - " };
}
