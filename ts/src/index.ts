/**
 * Verbalis rewrites text the way it is read aloud: numbers, dates, symbols, abbreviations, and web addresses as words.
 * This entry point includes every language; "verbalis/core" with a language's entry point includes only that one.
 */
import { Verbalizer as Core, type Options } from "./core.ts";
import { codes, find } from "./languages/registry.ts";

export type { Change, Options, Result } from "./core.ts";

export class Verbalizer extends Core {
  /**
   * @param language a code from Verbalizer.languages(), for example "en-us" or "pl".
   * @throws RangeError if the language isn't supported.
   */
  constructor(language: string, options: Options = {}) {
    const found = find(language);
    if (!found) {
      throw new RangeError(`Unsupported language: ${language}`);
    }
    super(found, options);
  }

  /** The supported language codes. */
  static languages(): string[] {
    return codes();
  }
}
