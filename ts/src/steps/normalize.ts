import type { Document, Edit } from "../text/document.ts";
import { contains, str, type Text } from "../text/strings.ts";
import { isAlnum, isAlpha, isDigit, isSpace, isWord } from "../text/unicode.ts";

// A class of characters written in one canonical form: all quotes as ".
interface CharacterClass {
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
const DASHES = "—–‒―";

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

// A character lost in decoding, which says nothing: the replacement character, also as UTF-8 read as Latin-1
// (dï¿½clarant -> dclarant)
function readLost(text: Text, position: number): Edit | undefined {
  for (const lost of ["ï¿½", "\ufffd"]) {
    const length = [...lost].length;
    if (str(text, position, position + length) === lost) {
      return { span: { begin: position, end: position + length }, text: "" };
    }
  }
  return undefined;
}

// An ellipsis right before a digit, spaced from it so the number reads on its own: 114...48 -> 114... 48
function readEllipsis(text: Text, position: number): Edit | undefined {
  const end = str(text, position, position + 3) === "..." ? position + 3 : position + (text[position] === "…" ? 1 : 0);
  if (end === position || end >= text.length || !isDigit(text[end])) {
    return undefined;
  }
  return { span: { begin: position, end }, text: "... " };
}

// A hyphen spaced from the number before it only, which is a range, not a minus: 1989 -1990 -> 1989-1990
function readSpacedRange(text: Text, position: number): Edit | undefined {
  if (
    text[position] !== " " ||
    text[position + 1] !== "-" ||
    !isDigit(text[position - 1]) ||
    !isDigit(text[position + 2])
  ) {
    return undefined;
  }
  return { span: { begin: position, end: position + 2 }, text: "-" };
}

// A fraction character standing apart, written with digits for the fraction readings: ¾ szklanki -> 3/4, 2½ -> 2 1/2;
// undefined next to a letter, where it is likely a mis-encoded letter (dï¿½clarant)
const VULGAR_FRACTIONS: ReadonlyMap<string, string> = new Map([
  ["½", "1/2"],
  ["¼", "1/4"],
  ["¾", "3/4"],
  ["⅓", "1/3"],
  ["⅔", "2/3"],
]);
function vulgarFraction(text: Text, position: number): string | undefined {
  const fraction = VULGAR_FRACTIONS.get(text[position]);
  if (fraction === undefined || isAlpha(text[position - 1]) || isAlpha(text[position + 1])) {
    return undefined;
  }
  return (isDigit(text[position - 1]) ? " " : "") + fraction;
}

// A power written as a digit after a unit after a number, as its superscript: 345m2 -> 345m², 900 cm3 -> 900 cm³
function readPower(text: Text, position: number): Edit | undefined {
  if ((text[position] !== "2" && text[position] !== "3") || isAlnum(text[position + 1])) {
    return undefined;
  }
  for (const unit of ["km", "cm", "mm", "dm", "ft", "m"]) {
    const start = position - Math.min(position, unit.length);
    if (str(text, start, position) !== unit) {
      continue;
    }
    const before = start > 0 && text[start - 1] === " " ? start - 1 : start;
    if (before > 0 && isDigit(text[before - 1])) {
      return { span: { begin: position, end: position + 1 }, text: text[position] === "2" ? "²" : "³" };
    }
    return undefined;
  }
  return undefined;
}

// Whether an ordinal sign at a position is a degree: 18ºC, 12 ºF; a ring after a number is one too: 180˚C
function isDegree(text: Text, position: number): boolean {
  let before = position;
  while (before > 0 && isSpace(text[before - 1])) {
    before--;
  }
  const unit = position + 1;
  if (text[position] === "˚") {
    return before > 0 && isDigit(text[before - 1]);
  }
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
  // Only what mis-encoded text holds: Latin letters, Devanagari, and punctuation (â€™ -> ’); not a pair which is
  // itself text (Spaß», ÉTÉ»)
  const latin = c >= 0xa0 && c <= 0x24f;
  const devanagari = c >= 0x900 && c <= 0x97f;
  const punctuation = (c >= 0x2010 && c <= 0x206f) || c === 0x20ac || c === 0x2122;
  if (!latin && !devanagari && !punctuation && length < 4) {
    // and emoji (ðŸ™‚ -> 🙂)
    return undefined;
  }
  return { span: { begin: position, end: position + length }, text: String.fromCodePoint(c) };
}

// The characters of named HTML entities, as the canonical forms of this step: &nbsp; is a space, &rsquo; is '
// prettier-ignore
const ENTITIES: ReadonlyMap<string, string> = new Map([
  ["amp", "&"], ["lt", "<"], ["gt", ">"], ["quot", '"'], ["apos", "'"], ["nbsp", " "],
  ["ndash", "-"], ["mdash", "-"], ["hellip", "..."], ["laquo", '"'], ["raquo", '"'], ["ldquo", '"'],
  ["rdquo", '"'], ["bdquo", '"'], ["lsquo", "'"], ["rsquo", "'"], ["euro", "€"], ["pound", "£"],
  ["yen", "¥"], ["cent", "¢"], ["deg", "°"], ["sup1", "¹"], ["sup2", "²"], ["sup3", "³"],
  ["middot", "·"], ["times", "×"], ["divide", "÷"], ["plusmn", "±"], ["frac12", "1/2"], ["frac14", "1/4"],
  ["frac34", "3/4"], ["copy", "©"], ["reg", "®"], ["trade", "™"], ["sect", "§"], ["para", "¶"],
  ["ordm", "º"], ["ordf", "ª"], ["iexcl", "¡"], ["iquest", "¿"], ["szlig", "ß"], ["ccedil", "ç"],
  ["Ccedil", "Ç"], ["aelig", "æ"], ["AElig", "Æ"], ["oslash", "ø"], ["Oslash", "Ø"], ["aring", "å"],
  ["Aring", "Å"], ["oelig", "œ"], ["OElig", "Œ"], ["eth", "ð"], ["thorn", "þ"],
  // Accented letters: &eacute; -> é
  ["Aacute", "Á"], ["Agrave", "À"], ["Acirc", "Â"], ["Auml", "Ä"], ["Atilde", "Ã"], ["Eacute", "É"],
  ["Egrave", "È"], ["Ecirc", "Ê"], ["Euml", "Ë"], ["Iacute", "Í"], ["Igrave", "Ì"], ["Icirc", "Î"],
  ["Iuml", "Ï"], ["Oacute", "Ó"], ["Ograve", "Ò"], ["Ocirc", "Ô"], ["Ouml", "Ö"], ["Otilde", "Õ"],
  ["Uacute", "Ú"], ["Ugrave", "Ù"], ["Ucirc", "Û"], ["Uuml", "Ü"], ["aacute", "á"], ["agrave", "à"],
  ["acirc", "â"], ["auml", "ä"], ["atilde", "ã"], ["eacute", "é"], ["egrave", "è"], ["ecirc", "ê"],
  ["euml", "ë"], ["iacute", "í"], ["igrave", "ì"], ["icirc", "î"], ["iuml", "ï"], ["oacute", "ó"],
  ["ograve", "ò"], ["ocirc", "ô"], ["ouml", "ö"], ["otilde", "õ"], ["uacute", "ú"], ["ugrave", "ù"],
  ["ucirc", "û"], ["uuml", "ü"], ["Ntilde", "Ñ"], ["ntilde", "ñ"], ["Yacute", "Ý"], ["Yuml", "Ÿ"],
  ["yacute", "ý"], ["yuml", "ÿ"],
]);

// An HTML entity at a position, as its character: &iacute; -> í, &#8217; -> ', &nbsp -> " ". Corpora drop the
// semicolon (Computaci&oacuten), so a name of four letters and more reads without it.
function readEntity(text: Text, position: number): Edit | undefined {
  if (text[position] !== "&" || position + 2 >= text.length) {
    return undefined;
  }
  let end = position + 1;
  let decoded = "";
  if (text[end] === "#") {
    const hex = text[end + 1] === "x" || text[end + 1] === "X";
    const digits = (end = end + 1 + (hex ? 1 : 0));
    let c = 0;
    while (
      end < text.length &&
      end - digits < 7 &&
      (/^[0-9]$/.test(text[end]) || (hex && /^[a-fA-F]$/.test(text[end])))
    ) {
      c = c * (hex ? 16 : 10) + parseInt(text[end], 16);
      end++;
    }
    if (
      end === digits ||
      end === text.length ||
      text[end] !== ";" ||
      c === 0 ||
      c > 0x10ffff ||
      (c >= 0xd800 && c <= 0xdfff)
    ) {
      return undefined;
    }
    end++;
    decoded = String.fromCodePoint(c);
  } else {
    let nameEnd = end;
    while (nameEnd < text.length && nameEnd - end < 8 && /^[A-Za-z0-9]$/.test(text[nameEnd])) {
      nameEnd++;
    }
    for (; nameEnd > end + 1; --nameEnd) {
      // The longest name: &oacuten -> ó + n
      const entity = ENTITIES.get(str(text, end, nameEnd));
      const semicolon = text[nameEnd] === ";";
      if (entity !== undefined && (semicolon || nameEnd - end >= 4)) {
        decoded = entity;
        end = nameEnd + (semicolon ? 1 : 0);
        break;
      }
    }
    if (decoded === "") {
      return undefined;
    }
  }
  return { span: { begin: position, end }, text: decoded };
}

/**
 * Writes equivalent characters in one canonical form, before anything reads the text:
 * quotes as ", apostrophes as ', an ellipsis as ..., dashes as - or " - ", a degree written as an ordinal sign
 * (18ºC) as °, a fraction character apart as digits (¾ -> 3/4), and no invisible or lost characters (ï¿½).
 *
 * UTF-8 read as Windows-1252 is repaired first: siÄ™ -> się, â€™ -> '; then HTML entities: &oacute; -> ó.
 */
export class Normalizer {
  readonly #canonical = new Map<string, string>(); // Of each character of the classes: ’ -> '
  readonly #letters = new Set<string>(); // The characters of letter classes

  constructor(characters: Characters = DEFAULT_CHARACTERS) {
    for (const { characters: members, canonical, letter } of characters) {
      for (const c of members) {
        this.#canonical.set(c, canonical);
        if (letter) {
          this.#letters.add(c);
        }
      }
    }
  }

  apply(document: Document): void {
    const text = document.text();
    for (let i = 0; i < text.length;) {
      const edit =
        readUtf8(text, i) ??
        this.#entity(text, i) ??
        readLost(text, i) ??
        readEllipsis(text, i) ??
        readDash(text, i) ??
        readSpacedRange(text, i) ??
        readPower(text, i);
      if (edit) {
        document.replace(edit.span, edit.text);
        i = edit.span.end;
        continue;
      }
      const canonical = this.#canonical.get(text[i]);
      const fraction = vulgarFraction(text, i);
      if (isDegree(text, i)) {
        document.replace({ begin: i, end: i + 1 }, "°");
      } else if (fraction !== undefined) {
        document.replace({ begin: i, end: i + 1 }, fraction);
      } else if (canonical !== undefined && (!this.#letters.has(text[i]) || this.#isLetter(text, i))) {
        document.replace({ begin: i, end: i + 1 }, canonical);
      }
      i++;
    }
  }

  // An HTML entity at a position, written as the character it stands for (&eacute; -> é), or undefined
  #entity(text: Text, position: number): Edit | undefined {
    const edit = readEntity(text, position);
    if (edit && [...edit.text].length === 1) {
      // A character by number in its canonical form: &#8217; -> '
      const c = edit.text;
      const canonical = this.#canonical.get(c);
      if (canonical !== undefined && !this.#letters.has(c)) {
        edit.text = canonical;
      } else if (contains(DASHES, c)) {
        edit.text = "-";
      } else if (c === "\u00a0") {
        edit.text = " ";
      }
    }
    return edit;
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

  // 1. Inside a word or a number: Szczurek–Żelazko, 1990–2000; but an em dash or two hyphens before a word is a
  // pause, as with spaces around it (case 2): briefs—styles, I--test. A shorter part after them is a clitic
  // (limitou--se, elegê--los) or a mis-encoded letter (spos—b). An em dash before a number is a pause too.
  let wordEnd = end;
  while (wordEnd < text.length && isAlpha(text[wordEnd])) {
    wordEnd++;
  }
  const number = end < text.length && isDigit(text[end]); // drizzle—0.2 mm
  const pause = text[position] === "—" ? number || wordEnd - end >= 3 : end - position >= 2 && wordEnd - end >= 4;
  const digits = position > 0 && isDigit(text[position - 1]) && end < text.length && isDigit(text[end]);
  if (position > 0 && isWord(text[position - 1]) && end < text.length && isWord(text[end]) && (!pause || digits)) {
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
