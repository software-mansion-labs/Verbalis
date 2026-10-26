import { Context, PUNCTUATION } from "../text/context.ts";
import type { Document, Span } from "../text/document.ts";
import { read, type Readings } from "../text/readings.ts";
import { isClearAfter, isClearBefore, isSentenceEnd, keysAt, type Keys } from "../text/scan.ts";
import { contains, size, startsWith, str, toLower, type Text } from "../text/strings.ts";
import { isDigit, isLatin, isSpace, isWord } from "../text/unicode.ts";

// The words between the parts of a web address.
export interface WebWords {
  readonly dot: string; // example dot com
  readonly at: string; // jan at example dot com
  readonly slash: string; // example dot com slash news
}

// The terms of a language: written forms which are not said as written.
export interface Terms {
  // nr -> numer before a number; St. -> Saint before a name, Street otherwise
  readonly abbreviations: Readings;
  readonly web: WebWords;
}

// A term found in a text, with its words
interface Term {
  span: Span;
  words: string;
}

// prettier-ignore
const DOMAINS = new Set([
  "com", "org", "net", "edu", "gov", "io", "pl", "de", "fr", "es",
  "it", "pt", "uk", "eu", "at", "ch", "info", "tv", "co", "br",
]);

// Whether a host label is a top-level domain, which makes a bare host an address: example.com
function isDomain(label: string): boolean {
  return DOMAINS.has(toLower(label));
}

// A character of the user name of an email: jan.kowalski+news
function isUser(c: string | undefined): boolean {
  return isWord(c) || contains(".-+%", c);
}

// A character of a host label: my-site
function isLabel(c: string | undefined): boolean {
  return isWord(c) || c === "-";
}

// A part of an address with its dots read: jan.kowalski -> jan dot kowalski
function dotted(part: string, dot: string): string {
  let words = "";
  for (const c of part) {
    words += c === "." ? ` ${dot} ` : c;
  }
  return words;
}

/**
 * Handles non-numeric and non-symbolic terms: web addresses (www.example.com/news, jan@example.com) and
 * abbreviations (nr 5, e.g.).
 */
export class TermReader {
  readonly #terms: Terms;
  readonly #keys: Keys; // Of the abbreviations

  constructor(terms: Terms) {
    this.#terms = terms;
    this.#keys = Object.keys(terms.abbreviations);
  }

  apply(document: Document): void {
    const text = document.text();

    for (let position = 0; position < text.length;) {
      // Terms start at word boundaries, but not after a dot: e.g. is not g.
      let term: Term | undefined;
      if (isClearBefore(text, position, ".")) {
        term = this.#readAddress(text, position) ?? this.#readAbbreviation(text, position);
      }

      if (!term) {
        position++;
        continue;
      }
      document.replace(term.span, term.words);
      position = term.span.end;
    }
  }

  // https://www.example.com/news, jan.kowalski@example.pl: its parts joined with the words of the separators
  #readAddress(text: Text, position: number): Term | undefined {
    const web = this.#terms.web;

    // 1. An optional scheme, which is not read: https://
    let host = position;
    while (host < text.length && isLatin(text[host])) {
      host++;
    }
    const scheme = host > position && startsWith(text, host, "://");
    host = scheme ? host + 3 : position;

    // 2. The user of an email: jan.kowalski@
    let user = "";
    if (!scheme) {
      let at = host;
      while (at < text.length && isUser(text[at])) {
        at++;
      }
      if (at > host && at < text.length && text[at] === "@") {
        user = str(text, host, at);
        host = at + 1;
      }
    }

    // 3. The host: labels joined by dots
    const labels: string[] = [];
    let end = host;
    for (;;) {
      let labelEnd = end;
      while (labelEnd < text.length && isLabel(text[labelEnd])) {
        labelEnd++;
      }
      if (labelEnd === end) {
        break;
      }
      labels.push(str(text, end, labelEnd));
      end = labelEnd;
      if (end + 1 >= text.length || text[end] !== "." || !isLabel(text[end + 1])) {
        break;
      }
      end++;
    }

    // A bare host needs a known top-level domain: example.com, but not file.txt or e.g
    const www = labels.length > 0 && toLower(labels[0]) === "www";
    const domain = labels.length >= 2 && isDomain(labels[labels.length - 1]);
    if (labels.length < 2 || !(scheme || www || domain || user !== "")) {
      return undefined;
    }

    // 4. An optional path, without the punctuation closing it: (see example.com/news).
    let pathEnd = end;
    if (end < text.length && text[end] === "/") {
      while (pathEnd < text.length && !isSpace(text[pathEnd])) {
        pathEnd++;
      }
      while (pathEnd > end + 1 && contains(PUNCTUATION, text[pathEnd - 1])) {
        pathEnd--;
      }
    }

    // 5. The words: jan at example dot com, w w w dot example dot com slash news
    let words = user === "" ? "" : `${dotted(user, web.dot)} ${web.at} `;
    labels.forEach((label, i) => {
      const spelled = i === 0 && www;
      words += (i > 0 ? ` ${web.dot} ` : "") + (spelled ? "w w w" : label);
    });
    let path = text.slice(end, pathEnd);
    while (path.length > 0) {
      const slash = path.indexOf("/", 1);
      const segment = str(path, 1, slash === -1 ? path.length : slash);
      words += ` ${web.slash}` + (segment === "" ? "" : ` ${dotted(segment, web.dot)}`);
      path = slash === -1 ? [] : path.slice(slash);
    }
    return { span: { begin: position, end: pathEnd }, words };
  }

  // The longest abbreviation at a position with a reading in its context
  #readAbbreviation(text: Text, position: number): Term | undefined {
    for (const key of keysAt(text, position, this.#keys)) {
      // An abbreviation ending with a letter ends a word, or touches a number: nr5
      const end = position + size(key);
      if (isWord(key.at(-1)) && !isClearAfter(text, end) && !isDigit(text[end])) {
        continue;
      }

      let words = read(this.#terms.abbreviations[key], new Context(text, { begin: position, end }));
      if (words === undefined) {
        continue;
      }

      // A dot which also ends the text stays with it; a word right after gets a space: nr5 -> numer 5
      if (key.endsWith(".") && isSentenceEnd(text, end)) {
        words += ".";
      }
      if (end < text.length && isWord(text[end])) {
        words += " ";
      }
      return { span: { begin: position, end }, words };
    }
    return undefined;
  }
}
