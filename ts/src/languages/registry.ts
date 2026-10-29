import type { Language, LanguageHandle } from "./language.ts";
import { language as de } from "./de/german.ts";
import { language as en } from "./en/english.ts";
import { language as es } from "./es/spanish.ts";
import { language as fr } from "./fr/french.ts";
import { language as hi } from "./hi/hindi.ts";
import { language as it } from "./it/italian.ts";
import { language as pl } from "./pl/polish.ts";
import { language as pt } from "./pt/portuguese.ts";
import { language as zh } from "./zh/mandarin.ts";

// Each language's folder defines its instance; add a language here.
const LANGUAGES: ReadonlyArray<readonly [string, () => LanguageHandle]> = [
  ["de", de],
  ["en", en],
  ["en-gb", en],
  ["en-us", en],
  ["es", es],
  ["fr", fr],
  ["hi", hi],
  ["it", it],
  ["pl", pl],
  ["pt", pt],
  ["zh", zh],
];

// The language of a code, or undefined if it isn't supported.
export function find(code: string): Language | undefined {
  return LANGUAGES.find(([key]) => key === code)?.[1]() as Language | undefined;
}

// The supported language codes.
export function codes(): string[] {
  return LANGUAGES.map(([code]) => code);
}
