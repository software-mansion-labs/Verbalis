import { Kind, numeral, type Match, type Numeral } from "../numbers/match.ts";
import type { Notation } from "../numbers/formats.ts";
import { genericPatterns, type Pattern } from "../numbers/patterns.ts";
import { codeParts, fractionOf } from "../numbers/scanning.ts";
import type { Characters } from "../steps/normalize.ts";
import type { Terms } from "../steps/terms.ts";
import { Context } from "../text/context.ts";
import { Document } from "../text/document.ts";
import type { Readings } from "../text/readings.ts";
import { digitsEnd, join, skipSpace } from "../text/scan.ts";
import { str } from "../text/strings.ts";
import { isDigit } from "../text/unicode.ts";
import type { Joiners } from "./grammar.ts";

// The tables of a language's steps.
export interface Tables {
  readonly characters?: Characters; // Its character classes, if not the default ones
  readonly terms?: Terms; // Its abbreviations and web words, if any
  readonly symbols?: Readings; // Its symbol words, if any
}

declare const opaque: unique symbol;

/** A language for the Verbalizer of "verbalis/core", from an entry point's `language()`. Its contents are internal. */
export interface LanguageHandle {
  readonly [opaque]: true;
}

// What the pipeline needs from a language: the tables of its steps, and how it reads numbers.
export abstract class Language implements LanguageHandle {
  declare readonly [opaque]: true;
  readonly tables: Tables;

  protected constructor(tables: Tables) {
    this.tables = tables;
  }

  // Queues the edits that read the numbers of a document as words.
  abstract readNumbers(document: Document): void;
}

type Amount = (amount: Numeral, match: Match) => string | undefined;

/**
 * A language whose numbers are found by patterns and said by its readings.
 *
 * The patterns run in priority order (see Kind), each one over the whole text as the patterns before it left it.
 * Every match is said by the reading of its kind. A reading returns undefined to leave its match as written, so it also
 * decides whether a match really is what its pattern found: a time can turn out to be a decimal (1.13). The words
 * around a match are in match.context.
 *
 * A language must say its numbers (readNumber) and the digits of codes and phone numbers (readCodeDigits); every other
 * reading has a default built on those two.
 */
export abstract class PatternLanguage extends Language {
  readonly #patterns: { pattern: Pattern; generic: boolean }[] = []; // By kind, then the language's own first
  readonly #joiners: Joiners;

  protected constructor(tables: Tables, notation: Notation, joiners: Joiners) {
    super(tables);
    this.#joiners = joiners;
    for (const pattern of genericPatterns(notation)) {
      this.#patterns.push({ pattern, generic: true });
    }
  }

  // Adds one of the language's own patterns; it goes before the generic patterns of its kind.
  protected addPattern(pattern: Pattern): void {
    // After every pattern of an earlier kind, and every pattern of its own kind which is not generic
    const kind = pattern.kind;
    let at = this.#patterns.findIndex(
      (entry) => entry.pattern.kind > kind || (entry.pattern.kind === kind && entry.generic),
    );
    at = at === -1 ? this.#patterns.length : at;
    this.#patterns.splice(at, 0, { pattern, generic: false });
  }

  protected joiners(): Joiners {
    return this.#joiners;
  }

  readNumbers(document: Document): void {
    // A draft rewritten by each pattern in turn: later patterns see the words of earlier ones
    const draft = new Document(document.text());

    for (const { pattern } of this.#patterns) {
      const text = draft.text();

      for (let position = 0; position < text.length;) {
        const match = pattern.match(text, position);
        if (!match) {
          position++;
          continue;
        }
        match.text = str(text, match.span.begin, match.span.end);
        match.context = new Context(text, match.span);

        const words = this.#readMatch(match);
        if (words !== undefined && words !== match.text) {
          draft.replace(match.span, words);
        }
        position = match.span.end;
      }
      draft.commit();
    }

    // Each change of the draft is an edit of the document
    for (const { span, text } of draft.changes()) {
      document.replace(span, text);
    }
  }

  // A number, in the context of its match: 42, 1 000,5, .5
  protected abstract readNumber(number: Numeral, match: Match): string;

  // A run of digits in a code or a phone number: the 380 of A380
  protected abstract readCodeDigits(digits: string): string;

  // The letters of a code (the MP of MP3), as written by default
  protected readCodeLetters(letters: string): string {
    return letters;
  }

  // The country code of an international phone number (the 48 of +48 600 123 456), as code digits by default
  protected readCountryCode(digits: string): string {
    return this.readCodeDigits(digits);
  }

  // Readings of whole matches; by default their numbers one by one
  protected readDate(match: Match): string | undefined {
    return this.#readEach(match);
  }

  // A phone number's groups. It also reads the groups of an international number, as a match of that kind.
  protected readPhone(match: Match): string | undefined {
    return join(match.numbers.map((group) => this.readCodeDigits(group.integer)));
  }

  // A fraction: its last two numbers (see fractionOf); the whole number of 3 1/2 is read before it
  protected readFraction(match: Match): string | undefined {
    return this.#readEach({ ...match, numbers: match.numbers.slice(-2) });
  }

  protected readTime(match: Match): string | undefined {
    return this.#readEach(match);
  }

  protected readOrdinal(match: Match): string | undefined {
    return this.readNumber(match.numbers[0], match);
  }

  protected readDecade(match: Match): string | undefined {
    return this.readNumber(match.numbers[0], match);
  }

  // A Roman numeral; left as written by default, since most need a language's cues (see romanRole)
  protected readRoman(_match: Match): string | undefined {
    return undefined;
  }

  protected readCompound(match: Match): string | undefined {
    return `${this.readNumber(match.numbers[0], match)}-${match.suffix}`;
  }

  // Any other number
  protected readPlain(match: Match): string | undefined {
    return this.readNumber(match.numbers[0], match);
  }

  // Amounts: each says the last number of its match; a range (10-15 km) gets its start and the range joiner before it
  protected readMoney(amount: Numeral, match: Match): string | undefined {
    return this.#withWord(amount, match, match.unit);
  }

  protected readScale(amount: Numeral, match: Match): string | undefined {
    return this.#withWord(amount, match, match.scale);
  }

  protected readPercent(amount: Numeral, match: Match): string | undefined {
    return this.#withWord(amount, match, "%");
  }

  protected readUnit(amount: Numeral, match: Match): string | undefined {
    return this.#withWord(amount, match, match.unit);
  }

  // Symbols between numbers (DIFFERENCE, SUM, RANGE, DIMENSION, SIGN, RATING, SCORE), from the joiners by default
  protected readJoiner(match: Match): string | undefined {
    const joiners = this.#joiners;
    switch (match.kind) {
      case Kind.DIFFERENCE: {
        // 5 - 3 = 2 is a minus, but 1993 - 1996 a range, read by the range pattern
        const tail = match.context.tail();
        const equation = tail[skipSpace(tail, digitsEnd(tail, 0))] === "=";
        return match.text === " - " && !equation ? "-" : ` ${joiners.minus} `;
      }
      case Kind.SUM:
        return ` ${joiners.plus} `;
      case Kind.RANGE:
        return this.readRange(match.numbers[0], match.numbers[1], match);
      case Kind.DIMENSION:
        return ` ${joiners.by} `;
      case Kind.SIGN:
        return `${joiners.minus} `;
      case Kind.SCORE:
        return joiners.score;
      case Kind.RATING:
        return joiners.rating ? ` ${joiners.rating} ` : undefined;
      default:
        return undefined; // Not a joiner
    }
  }

  protected readRange(_from: Numeral, _to: Numeral, _match: Match): string {
    return this.#joiners.range;
  }

  // The plural of a reading, for codes like F-16s; none by default.
  protected plural(_words: string): string | undefined {
    return undefined;
  }

  #readMatch(match: Match): string | undefined {
    switch (match.kind) {
      case Kind.DATE:
        return this.readDate(match);
      case Kind.INTERNATIONAL_PHONE:
        return this.#readInternationalPhone(match);
      case Kind.PHONE:
        return this.readPhone(match);
      case Kind.VERSION:
        return this.#readVersion(match);
      case Kind.DIFFERENCE:
      case Kind.SUM:
      case Kind.RANGE:
      case Kind.DIMENSION:
      case Kind.SIGN:
      case Kind.RATING:
      case Kind.SCORE:
        return this.readJoiner(match);
      case Kind.FRACTION:
        return this.readMixedFraction(match);
      case Kind.TIME:
        return this.readTime(match);
      case Kind.MONEY:
        return this.#readAmount(match, (amount, m) => this.readMoney(amount, m));
      case Kind.SCALE:
        return this.#readAmount(match, (amount, m) => this.readScale(amount, m));
      case Kind.PERCENT:
        return this.#readAmount(match, (amount, m) => this.readPercent(amount, m));
      case Kind.UNIT:
        return this.#readAmount(match, (amount, m) => this.readUnit(amount, m));
      case Kind.ORDINAL:
        return this.readOrdinal(match);
      case Kind.DECADE:
        return this.readDecade(match);
      case Kind.ROMAN:
        return this.readRoman(match);
      case Kind.COMPOUND:
        return this.readCompound(match);
      case Kind.CODE:
        return this.readCode(match);
      case Kind.NUMBER:
        return this.readPlain(match);
    }
  }

  // Its numbers one by one
  #readEach(match: Match, separator = " "): string {
    return join(
      match.numbers.map((number) => this.readNumber(number, match)),
      separator,
    );
  }

  #readVersion(match: Match): string {
    // 1.2.3.4 is an address, read with dots; 1.2.3 a version, read with points
    const joiners = this.#joiners;
    return this.#readEach(match, ` ${match.numbers.length === 4 ? joiners.dot : joiners.point} `);
  }

  // An amount read as a number followed by a word as written: 5 km, 10 %
  #withWord(amount: Numeral, match: Match, word: string): string {
    return `${this.readNumber(amount, match)} ${word}`;
  }

  // A fraction, after its whole number if it has one (3 1/2), by the joiners
  protected readMixedFraction(match: Match): string | undefined {
    if (match.numbers.length === 2) {
      return this.readFraction(match);
    }
    // 3 1/2: three and a half; 3 3/4: three and three quarters; -4 1/3: minus four and one third
    let whole = this.readNumber(match.numbers[0], match);
    if (match.text.startsWith("-")) {
      whole = `${this.joiners().minus} ${whole}`;
    }
    const [a, b] = fractionOf(match);
    const { fraction: joiner = "", half = "" } = this.joiners();
    if (a === 1n && b === 2n && half !== "") {
      return whole + half;
    }
    const fraction = this.readFraction(match);
    if (fraction === undefined) {
      return undefined;
    }
    return whole + (joiner === "" ? " " : joiner) + fraction;
  }

  #readInternationalPhone(match: Match): string | undefined {
    // plus forty-eight, then the groups as the language reads a phone number
    const groups: Match = { ...match, numbers: match.numbers.slice(1) };
    groups.text = [...match.text].slice(groups.numbers[0].span.begin - match.span.begin).join("");
    const words = this.readPhone(groups);
    if (words === undefined) {
      return undefined;
    }
    return `${this.#joiners.plus} ${this.readCountryCode(match.numbers[0].integer)} ${words}`;
  }

  protected readCode(match: Match): string {
    // Letters stay, digit runs read as numbers: MP3, COVID-19, L'A380, F-16s
    const out: string[] = [];
    let glue = false;
    let previous = "";

    for (const part of codeParts(match.text)) {
      if (part === "'" && out.length > 0) {
        out[out.length - 1] += part; // L'A380
        glue = true;
        continue;
      }

      let word: string | undefined;
      const afterDigits = previous !== "" && isDigit(previous.at(-1));
      let plural: string | undefined;
      if (part === "-" || part === ".") {
        word = undefined;
      } else if (isDigit(part[0]) && !part.includes(".")) {
        word = this.readCodeDigits(part);
      } else if (part[0] === "." && out.length > 0) {
        word = this.readCodeDigits(part.slice(1)); // H.264
      } else if (isDigit(part.at(-1))) {
        // 2.5 inside a code, or .5 starting it
        const dot = part.indexOf(".");
        const number = { span: { begin: 0, end: 0 }, integer: part.slice(0, dot), fraction: part.slice(dot + 1) };
        word = this.readNumber(numeral({ ...number, decimalMark: "." }), match);
      } else if (part === "s" && afterDigits && (plural = this.plural(out[out.length - 1])) !== undefined) {
        out[out.length - 1] = plural; // F-16s
      } else {
        word = this.readCodeLetters(part);
      }

      if (word !== undefined) {
        if (glue && out.length > 0) {
          out[out.length - 1] += word;
        } else {
          out.push(word);
        }
      }
      glue = false;
      previous = part;
    }
    return join(out);
  }

  #readAmount(match: Match, reading: Amount): string | undefined {
    const last = { ...match.numbers[match.numbers.length - 1] };

    // A unit reads its number without leading zeros: 05 km
    while (match.kind === Kind.UNIT && last.integer.length > 1 && last.integer[0] === "0") {
      last.integer = last.integer.slice(1);
    }

    const words = reading(last, match);
    if (words === undefined || match.numbers.length === 1) {
      return words;
    }
    return this.readNumber(match.numbers[0], match) + this.readRange(match.numbers[0], last, match) + words;
  }
}
