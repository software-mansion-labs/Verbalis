import { Context, PUNCTUATION } from "../text/context.ts";
import type { Document } from "../text/document.ts";
import { read, type Readings } from "../text/readings.ts";
import { join, keysAt, type Keys } from "../text/scan.ts";
import { contains, size } from "../text/strings.ts";
import { isSpace } from "../text/unicode.ts";

/**
 * Reads the symbols left in a text as words, after numbers are read.
 * A symbol whose readings all have unmet conditions stays as written.
 */
export class SymbolReader {
  readonly #symbols: Readings;
  readonly #keys: Keys;

  constructor(symbols: Readings) {
    this.#symbols = symbols;
    this.#keys = Object.keys(symbols);
  }

  apply(document: Document): void {
    const text = document.text();

    for (let start = 0; start < text.length;) {
      // 1. A run of symbols, with the whitespaces around it: " & ", "++"
      let symbol = start;
      while (symbol < text.length && isSpace(text[symbol])) {
        symbol++;
      }
      const symbols: string[] = [];
      let end = symbol;
      for (let keys = keysAt(text, end, this.#keys); keys.length > 0; keys = keysAt(text, end, this.#keys)) {
        symbols.push(keys[0]);
        end += size(keys[0]);
      }
      if (symbols.length === 0) {
        start = symbol > start ? symbol : start + 1;
        continue;
      }
      while (end < text.length && isSpace(text[end])) {
        end++;
      }

      // 2. The words of each symbol in the context of the run
      const context = new Context(text, { begin: start, end });
      const words: string[] = [];
      for (const s of symbols) {
        const word = read(this.#symbols[s], context) ?? s;
        if (word !== "") {
          words.push(word);
        }
      }

      // 3. The words, separated from the text around by single spaces (none before punctuation).
      // A run right after another one already has its space: "™ &" -> "trademark and"
      const left = start > 0 && !isSpace(text[start - 1]);
      let replacement: string;
      if (words.length === 0) {
        replacement = left && end < text.length ? " " : "";
      } else {
        const right = end < text.length && !contains(PUNCTUATION, text[end]);
        replacement = (left ? " " : "") + join(words) + (right ? " " : "");
      }
      document.replace({ begin: start, end }, replacement);
      start = end;
    }
  }
}
