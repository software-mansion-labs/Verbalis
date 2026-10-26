import type { Document, Edit } from "../text/document.ts";
import { contains, type Text } from "../text/strings.ts";
import { isDigit, isSpace, isWord } from "../text/unicode.ts";

// A class of characters written in one canonical form: all quotes as ".
export interface CharacterClass {
  readonly characters: string; // « » „ “ ” ‟ ˝
  readonly canonical: string; // "; empty to remove them
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

/**
 * Writes equivalent characters in one canonical form, before anything reads the text:
 * quotes as ", apostrophes as ', an ellipsis as ..., dashes as - or " - ", and no invisible characters.
 */
export class Normalizer {
  readonly #canonical = new Map<string, string>(); // Of each character of the classes: ’ -> '

  constructor(characters: Characters = DEFAULT_CHARACTERS) {
    for (const { characters: members, canonical } of characters) {
      for (const c of members) {
        this.#canonical.set(c, canonical);
      }
    }
  }

  apply(document: Document): void {
    const text = document.text();
    for (let i = 0; i < text.length;) {
      const dash = readDash(text, i);
      if (dash) {
        document.replace(dash.span, dash.text);
        i = dash.span.end;
        continue;
      }
      const canonical = this.#canonical.get(text[i]);
      if (canonical !== undefined) {
        document.replace({ begin: i, end: i + 1 }, canonical);
      }
      i++;
    }
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
