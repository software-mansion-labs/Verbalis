import type { Span } from "./document.ts";
import { contains, str, toLower, type Text } from "./strings.ts";
import { isSpace } from "./unicode.ts";

// Punctuation stripped from the words around a match
export const PUNCTUATION = ".,;:!?\"'()[]«»„“”…";

/**
 * The surroundings of a span of a text: the words before and after it.
 * A word the span starts or ends inside of counts with its part outside the span.
 */
export class Context {
  readonly #text: Text;
  readonly #span: Span;

  constructor(text: Text = [], span: Span = { begin: 0, end: 0 }) {
    this.#text = text;
    this.#span = span;
  }

  /**
   * The k-th word before the span, lowercased and without surrounding punctuation.
   * Empty if there is no such word.
   */
  before(k = 1): string {
    // Walk back over k words of the head
    const head = this.#text;
    let end = this.#span.begin;
    for (; k > 0; --k) {
      while (end > 0 && isSpace(head[end - 1])) {
        end--;
      }
      if (end === 0) {
        return "";
      }
      let begin = end;
      while (begin > 0 && !isSpace(head[begin - 1])) {
        begin--;
      }
      if (k === 1) {
        return toLower(str(stripped(head.slice(begin, end))));
      }
      end = begin;
    }
    return "";
  }

  /**
   * The k-th word after the span, case kept, without surrounding punctuation.
   * Empty if there is no such word, or if a punctuation mark directly follows the span.
   */
  after(k = 1): string {
    const text = this.#text;
    if (this.#span.end < text.length && contains(PUNCTUATION, text[this.#span.end])) {
      return "";
    }

    // Walk forward over k words of the tail
    let begin = this.#span.end;
    for (; k > 0; --k) {
      while (begin < text.length && isSpace(text[begin])) {
        begin++;
      }
      if (begin === text.length) {
        return "";
      }
      let end = begin;
      while (end < text.length && !isSpace(text[end])) {
        end++;
      }
      if (k === 1) {
        return str(stripped(text.slice(begin, end)));
      }
      begin = end;
    }
    return "";
  }

  head(): Text {
    return this.#text.slice(0, this.#span.begin); // The text before the span
  }

  tail(): Text {
    return this.#text.slice(this.#span.end); // The text after the span
  }

  text(): Text {
    return this.#text.slice(this.#span.begin, this.#span.end); // The span itself
  }

  span(): Span {
    return this.#span;
  }
}

// Whitespace-separated words of a text, like Python's str.split().
export function wordsOf(text: Text): Text[] {
  const words: Text[] = [];
  for (let i = 0; i < text.length;) {
    if (isSpace(text[i])) {
      i++;
      continue;
    }
    let end = i;
    while (end < text.length && !isSpace(text[end])) {
      end++;
    }
    words.push(text.slice(i, end));
    i = end;
  }
  return words;
}

// A word without the punctuation around it.
export function stripped(word: Text): Text {
  let begin = 0;
  while (begin < word.length && contains(PUNCTUATION, word[begin])) {
    begin++;
  }
  if (begin === word.length) {
    return [];
  }
  let end = word.length;
  while (contains(PUNCTUATION, word[end - 1])) {
    end--;
  }
  return word.slice(begin, end);
}
