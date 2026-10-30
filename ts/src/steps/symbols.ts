import { Context, PUNCTUATION } from "../text/context.ts";
import type { Document } from "../text/document.ts";
import type { Reading, Readings } from "../text/readings.ts";
import { join, keysAt, type Keys } from "../text/scan.ts";
import { contains, size } from "../text/strings.ts";
import { isSpace } from "../text/unicode.ts";

// The words of a symbol before numbers are read: those of its first reading whose condition holds, if that one has a
// condition. After: those of its first reading without one.
function read(readings: readonly Reading[], context: Context, conditional: boolean): string | undefined {
  const reading = readings.find((r) => typeof r === "string" || (conditional && r.when(context)));
  if (reading === undefined || (conditional && typeof reading === "string")) {
    return undefined;
  }
  return typeof reading === "string" ? reading : reading.words;
}

/**
 * Reads the symbols left in a text as words, in two passes.
 * A reading with a condition looks at the text as written, so it is read before numbers are: p<0.05, Disney+, 50%+1.
 * A plain reading is read after them. A symbol with neither stays as written: <i>, Año~
 */
export class SymbolReader {
  readonly #symbols: Readings;
  readonly #keys: Keys;

  constructor(symbols: Readings) {
    this.#symbols = symbols;
    this.#keys = Object.keys(symbols);
  }

  // Reads the symbols with a reading whose condition holds (before numbers), or with a plain reading (after)
  apply(document: Document, conditional: boolean): void {
    const text = document.text();

    for (let start = 0; start < text.length;) {
      // 1. A run of symbols, with the whitespaces around it: " & ", "++"
      let symbol = start;
      while (symbol < text.length && isSpace(text[symbol])) {
        symbol++;
      }
      // Each symbol in its own context: "Google+", but "tiene +1". A symbol it can't read ends the run, and a run
      // without a symbol read stays as written: "<i>", "Año~"
      const words: string[] = [];
      let end = symbol;
      for (let keys = keysAt(text, end, this.#keys); keys.length > 0; keys = keysAt(text, end, this.#keys)) {
        const context = new Context(text, { begin: end, end: end + size(keys[0]) });
        const reading = read(this.#symbols[keys[0]], context, conditional);
        if (reading === undefined) {
          break;
        }
        if (reading !== "") {
          words.push(reading);
        }
        end += size(keys[0]);
      }
      if (end === symbol) {
        start = symbol > start ? symbol : start + 1;
        continue;
      }
      while (end < text.length && isSpace(text[end])) {
        end++;
      }

      // 2. The words, separated from the text around by single spaces (none before punctuation).
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
