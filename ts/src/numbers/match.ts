import { Context } from "../text/context.ts";
import type { Span } from "../text/document.ts";

// Types of numeric expressions, in priority order: at a given position of a text, patterns of earlier kinds are tried
// first. Languages add patterns, but never kinds.
export const Kind = {
  // Whole expressions whose parts are numbers
  DATE: 0, // 12.05.2024, 2024-05-12, 1. 5.
  INTERNATIONAL_PHONE: 1, // +48 600 123 456, +1 (555) 123-4567: before a language's own formats of its groups
  PHONE: 2, // 555-123-4567, 06 12 34 56 78
  VERSION: 3, // 3.8.10, 10.0.0.51
  // Symbols between numbers (only the symbol is matched)
  DIFFERENCE: 4, // the " - " of 5 - 3
  SUM: 5, // the "+" of 2 + 2 and 90+6
  RANGE: 6, // the "-" of 10-15 and 2020-2022
  DIMENSION: 7, // the "x" of 1920x1080 and 3 m x 4 m
  SIGN: 8, // the minus of -5: before FRACTION, which reads its digits (-1/3)
  FRACTION: 9, // 3/4
  TIME: 10, // 18:30, 7pm, 18 Uhr, 21h30
  SCORE: 11, // the ":" of 2:1
  // Numbers with what they count
  MONEY: 12, // $5, 12,99 zł, 10-15 €, 3 mln zł
  SCALE: 13, // 3 mln, 500k, 1.5 million
  PERCENT: 14, // 20%, 10-15%, 8%ige
  UNIT: 15, // 5 kg, 10-15 km
  // Numbers marked by a suffix or joined to letters
  ORDINAL: 16, // 21st, 3., 1er, 1º
  DECADE: 17, // 1990s, 90er, '80
  ROMAN: 18, // Henry VIII, XXI wieku, Kapitel III: read only where the words around it mark it as a number
  COMPOUND: 19, // 21-year-old, 45-metrowe
  CODE: 20, // MP3, A380, COVID-19
  // Any other number
  NUMBER: 21, // 42, 1 000,5, .5, 2020 r.
} as const;
export type Kind = (typeof Kind)[keyof typeof Kind];

// A number as written: "1 000,5" -> integer "1000", fraction "5".
export interface Numeral {
  span: Span; // Where it is written
  integer: string; // Digits without group separators; empty in ".5"
  fraction: string; // Digits after the decimal mark; empty if none
  decimalMark: string; // The decimal mark as written; empty if none
  grouped: boolean; // Written with group separators: 1 000, 1,000
}

// A numeric expression found in a text: its structure only, with no decision on how to read it.
export interface Match {
  kind: Kind;
  span: Span;
  text: string; // As written
  numbers: Numeral[]; // In written order: one for most kinds, two for ranges, three for dates
  unit: string; // A unit or currency key ("km", "zł"), or a law word of a DATE ("Ley")
  scale: string; // A scale key ("mln", "million")
  suffix: string; // Marks after the number: "st", "er", "." (ordinals), "ige" (8%ige), "pm", "r."
  separator: string; // The symbol between numbers: "-", ":", ".", "/", "h", "x"; empty if none
  context: Context; // The words around it, set when it is read
}

// A number with the fields not given empty.
export function numeral(fields: Partial<Numeral> & { span: Span }): Numeral {
  return { integer: "", fraction: "", decimalMark: "", grouped: false, ...fields };
}

// A match with the fields not given empty.
export function match(fields: Partial<Match> & { kind: Kind }): Match {
  return {
    span: { begin: 0, end: 0 },
    text: "",
    numbers: [],
    unit: "",
    scale: "",
    suffix: "",
    separator: "",
    context: new Context(),
    ...fields,
  };
}
