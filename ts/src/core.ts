/**
 * The Verbalizer without the language registry, so a bundle keeps only the languages it imports:
 *
 *   import { Verbalizer } from "verbalis/core";
 *   import { language } from "verbalis/pl";
 *   new Verbalizer(language()).process("Mam 2 koty.");
 */
import type { Language } from "./languages/language.ts";
import { DEFAULT_CHARACTERS, Normalizer } from "./steps/normalize.ts";
import { SymbolReader } from "./steps/symbols.ts";
import { TermReader } from "./steps/terms.ts";
import { trim } from "./steps/trim.ts";
import { Document } from "./text/document.ts";
import type { Readings } from "./text/readings.ts";

/** Which steps run, and readings added to the language's own. Every step is on by default. */
export interface Options {
  /** One form for equivalent characters: quotes, apostrophes, dashes, ellipses */
  normalize?: boolean;
  /** Single spaces between words, none at the edges or before punctuation */
  trim?: boolean;
  /** Abbreviations and web addresses as words: "e.g." -> "for example" */
  terms?: boolean;
  /** Numbers, dates, times, money, and units as words: "$5" -> "five dollars" */
  numbers?: boolean;
  /** Remaining symbols as words: "&" -> "and" */
  symbols?: boolean;
  /** Added or replaced readings: { "approx.": "approximately" } */
  abbreviations?: Readonly<Record<string, string>>;
  /** Added or replaced readings: { "™": "trademark" } */
  symbolWords?: Readonly<Record<string, string>>;
}

/** One edit of the input: `text.slice(start, end)` (UTF-16 code units, like every JavaScript string index) became `replacement`. */
export interface Change {
  start: number;
  end: number;
  replacement: string;
}

/** A verbalized text, with the edits that turned the input into it. */
export interface Result {
  text: string;
  changes: Change[];
}

// Readings added by the caller, each one unconditional
function readingsOf(words: Readonly<Record<string, string>> = {}): Readings {
  return Object.fromEntries(Object.entries(words).map(([written, spoken]) => [written, [spoken]]));
}

/**
 * Rewrites text the way it is read aloud. Its steps run in a fixed order: normalize, trim, terms, numbers, symbols.
 * A Verbalizer is immutable after construction.
 */
export class Verbalizer {
  readonly #language: Language;
  readonly #options: Required<Pick<Options, "normalize" | "trim" | "numbers">>;
  readonly #normalizer: Normalizer;
  readonly #terms: TermReader | undefined;
  readonly #symbols: SymbolReader | undefined;

  /** @param language a language's `language()`, from its entry point, for example "verbalis/pl". */
  constructor(language: Language, options: Options = {}) {
    this.#language = language;
    this.#options = {
      normalize: options.normalize ?? true,
      trim: options.trim ?? true,
      numbers: options.numbers ?? true,
    };
    const tables = language.tables;
    this.#normalizer = new Normalizer(tables.characters ?? DEFAULT_CHARACTERS);

    // The caller's readings replace the language's own with the same key
    const abbreviations = readingsOf(options.abbreviations);
    if ((options.terms ?? true) && (tables.terms || Object.keys(abbreviations).length > 0)) {
      const terms = tables.terms ?? { abbreviations: {}, web: { dot: "", at: "", slash: "" } };
      this.#terms = new TermReader({ ...terms, abbreviations: { ...terms.abbreviations, ...abbreviations } });
    }
    const symbolWords = readingsOf(options.symbolWords);
    if ((options.symbols ?? true) && (tables.symbols || Object.keys(symbolWords).length > 0)) {
      this.#symbols = new SymbolReader({ ...tables.symbols, ...symbolWords });
    }
  }

  /** The text as it is read aloud. */
  process(text: string): string {
    return this.#run(text).text().join("");
  }

  /** The text as it is read aloud, with the edits of the input that produced it, in order. */
  processWithChanges(text: string): Result {
    const document = this.#run(text);
    // The core counts code points; units[i] is where code point i starts in the input's UTF-16
    const units = [0];
    for (const character of text) {
      units.push(units[units.length - 1] + character.length);
    }
    return {
      text: document.text().join(""),
      changes: document
        .changes()
        .map(({ span, text: replacement }) => ({ start: units[span.begin], end: units[span.end], replacement })),
    };
  }

  #run(text: string): Document {
    const document = new Document(text);
    if (this.#options.normalize) {
      this.#normalizer.apply(document);
      document.commit();
    }
    if (this.#options.trim) {
      trim(document);
      document.commit();
    }
    if (this.#terms) {
      this.#terms.apply(document);
      document.commit();
    }
    if (this.#options.numbers) {
      this.#language.readNumbers(document);
      document.commit();
    }
    if (this.#symbols) {
      this.#symbols.apply(document);
      document.commit();
    }
    return document;
  }
}

export type { Language };
