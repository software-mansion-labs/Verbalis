import { Context, PUNCTUATION } from "../text/context.ts";
import type { Document, Span } from "../text/document.ts";
import { beforeName, read, type Readings } from "../text/readings.ts";
import { isClearAfter, isClearBefore, isSentenceEnd, join, keysAt, type Keys } from "../text/scan.ts";
import { contains, size, startsWith, str, toLower, type Text } from "../text/strings.ts";
import { isAlnum, isAlpha, isDigit, isLatin, isSpace, isWord } from "../text/unicode.ts";

// The words between the parts of a web address.
interface WebWords {
  readonly dot: string; // example dot com
  readonly at: string; // jan at example dot com
  readonly slash: string; // example dot com slash news
  readonly hyphen?: string; // my hyphen site dot com; the hyphen stays as written if empty
  readonly underscore?: string; // jan underscore nowak at example dot com; stays as written if empty
  // C colon backslash Users: paths (/home/desktop, C:\Users) and file names (report.pdf) stay as written if empty
  readonly backslash?: string;
  readonly colon?: string;
  readonly digits?: readonly string[]; // The names of the digits 0-9, read one by one: covid one nine dot gov
}

// The terms of a language: written forms which are not said as written.
export interface Terms {
  // nr -> numer before a number; St. -> Saint before a name, Street otherwise
  readonly abbreviations: Readings;
  readonly web: WebWords;
  // Abbreviations whose dot may also close a sentence before a capital: etc. The, 1st St. About
  readonly closing?: ReadonlySet<string>;
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
  "me", "mx", "us", "ru", "nl",
]);

// India's only under its second-level domains: rbi.org.in, but not the Polish m.in.
const INDIAN_DOMAINS = new Set(["gov", "nic", "org", "co", "ac", "edu", "net"]);

// Whether the labels of a host end with a top-level domain, which makes a bare host an address: example.com, gov.in
function isDomain(labels: readonly string[]): boolean {
  const n = labels.length;
  const last = toLower(labels[n - 1]);
  return n >= 2 && (DOMAINS.has(last) || (n >= 3 && last === "in" && INDIAN_DOMAINS.has(toLower(labels[n - 2]))));
}

// prettier-ignore
const EXTENSIONS = new Set([
  "pdf", "jpg", "jpeg", "png", "gif", "csv", "txt", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
  "zip", "mp3", "mp4", "html", "htm", "php", "js", "py", "exe", "json", "xml",
]);

// Whether a part of a path is a file name, by its extension: report.pdf
function isFile(part: string): boolean {
  const dot = part.lastIndexOf(".");
  return dot > 0 && EXTENSIONS.has(toLower(part.slice(dot + 1)));
}

// A character of a part of a path: Users, report.pdf, my_file-2
function isPath(c: string | undefined): boolean {
  return isLatin(c) || isDigit(c) || c === "." || c === "-" || c === "_";
}

// A character of the user name of an email: jan.kowalski+news
function isUser(c: string | undefined): boolean {
  return isWord(c) || contains(".-+%", c);
}

// A character of a host label: my-site
function isLabel(c: string | undefined): boolean {
  return isWord(c) || c === "-";
}

// A part of an address with its dots (and hyphens, if the language reads them) read: jan.kowalski -> jan dot kowalski
const SCHEMES: ReadonlySet<string> = new Set(["http", "https", "ftp", "sftp", "file", "ws", "wss"]);

// A part of an address with its separators read, and its digits apart, to be read one by one: a1 -> a 1
function dotted(part: string, web: WebWords): string {
  const chars = [...part];
  let words = "";
  for (let i = 0; i < chars.length; i++) {
    const c = chars[i];
    const word = c === "." ? web.dot : c === "-" ? (web.hyphen ?? "") : c === "_" ? (web.underscore ?? "") : "";
    if (word !== "") {
      words += ` ${word} `;
    } else if (isDigit(c)) {
      const apart = words !== "" && !words.endsWith(" ");
      const next = i + 1 < chars.length && chars[i + 1] !== "." && chars[i + 1] !== "-" && chars[i + 1] !== "_";
      const digit = web.digits?.length === 10 ? web.digits[Number(c)] : c;
      words += (apart ? " " : "") + digit + (next ? " " : "");
    } else {
      words += c;
    }
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
      // Terms start at word boundaries, but not after a dot: e.g. is not g.; a URL also right after a word, from
      // which a space parts it, read if complete (Councilhttps://www2.example.com) or else as written
      let term: Term | undefined;
      const scheme = startsWith(text, position, "http://") || startsWith(text, position, "https://");
      if (scheme && position > 0 && isAlnum(text[position - 1])) {
        term = this.#readAddress(text, position);
        if (!term) {
          let end = position;
          while (end < text.length && !isSpace(text[end])) {
            end++;
          }
          term = { span: { begin: position, end }, words: str(text, position, end) };
        }
        term = { ...term, words: ` ${term.words}` };
      } else if (isClearBefore(text, position, ".")) {
        term =
          this.#readUuid(text, position) ??
          this.#readAddress(text, position) ??
          this.#readPath(text, position) ??
          this.#readHandle(text, position) ??
          this.#readAbbreviation(text, position);
      }

      if (!term) {
        position++;
        continue;
      }
      document.replace(term.span, term.words);
      position = term.span.end;
    }
  }

  // 3f2504e0-4f89-11d3-9a0c-0305e82c3301: a UUID, character by character with the names of the digits
  #readUuid(text: Text, position: number): Term | undefined {
    const digits = this.#terms.web.digits;
    if (digits?.length !== 10) {
      return undefined;
    }
    const hex = (c: string | undefined) => c !== undefined && /^[0-9a-fA-F]$/.test(c);
    let end = position;
    for (const length of [8, 4, 4, 4, 12]) {
      if (end > position && (end >= text.length || text[end++] !== "-")) {
        return undefined;
      }
      for (let i = 0; i < length; ++i, ++end) {
        if (end >= text.length || !hex(text[end])) {
          return undefined;
        }
      }
    }
    if (end < text.length && isWord(text[end])) {
      return undefined;
    }

    const words: string[] = [];
    for (const c of text.slice(position, end)) {
      if (c !== "-") {
        words.push(isDigit(c) ? digits[Number(c)] : c.toLowerCase());
      }
    }
    return { span: { begin: position, end }, words: join(words) };
  }

  // https://www.example.com/news, jan.kowalski@example.pl: its parts joined with the words of the separators
  #readAddress(text: Text, position: number): Term | undefined {
    const web = this.#terms.web;

    // 1. An optional scheme, which is not read: https://
    let host = position;
    while (host < text.length && isLatin(text[host])) {
      host++;
    }
    // A known scheme only: in Councilhttps://, the word before isn't part of it
    const scheme = startsWith(text, host, "://") && SCHEMES.has(toLower(str(text, position, host)));
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
    const domain = labels.length > 0 && isDomain(labels);
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
    let words = user === "" ? "" : `${dotted(user, web)} ${web.at} `;
    labels.forEach((label, i) => {
      const spelled = i === 0 && www;
      words += (i > 0 ? ` ${web.dot} ` : "") + (spelled ? "w w w" : dotted(label, web));
    });
    let path = text.slice(end, pathEnd);
    while (path.length > 0) {
      const slash = path.indexOf("/", 1);
      const segment = str(path, 1, slash === -1 ? path.length : slash);
      words += ` ${web.slash}` + (segment === "" ? "" : ` ${dotted(segment, web)}`);
      path = slash === -1 ? [] : path.slice(slash);
    }
    return { span: { begin: position, end: pathEnd }, words };
  }

  // @wezyr1986: a handle with digits, which read one by one
  #readHandle(text: Text, position: number): Term | undefined {
    const web = this.#terms.web;
    if (text[position] !== "@" || !web.digits?.length || (position > 0 && isWord(text[position - 1]))) {
      return undefined;
    }
    let end = position + 1;
    while (end < text.length && isWord(text[end])) {
      end++;
    }
    const handle = text.slice(position + 1, end);
    if (
      !handle.some((c) => isAlpha(c)) ||
      !handle.some((c) => isDigit(c)) ||
      (end + 1 < text.length && (text[end] === "@" || text[end] === ".") && isWord(text[end + 1]))
    ) {
      return undefined;
    }
    return { span: { begin: position, end }, words: `${web.at} ${dotted(handle.join(""), web)}` };
  }

  // /home/desktop, C:\Users\HP, home/desktop, report.pdf: a path or a file name, with its separators read
  #readPath(text: Text, position: number): Term | undefined {
    const web = this.#terms.web;
    if (!web.backslash) {
      return undefined;
    }

    // upgrade / update: two lowercase words with a spaced slash between them
    const wordEnd = (i: number): number => {
      while (i < text.length && text[i] >= "a" && text[i] <= "z") {
        i++;
      }
      return i;
    };
    const first = wordEnd(position);
    if (
      first - position >= 3 &&
      str(text, first, first + 3) === " / " &&
      (position === 0 || isSpace(text[position - 1]))
    ) {
      const second = wordEnd(first + 3);
      if (second - first - 3 >= 3 && isClearAfter(text, second)) {
        return {
          span: { begin: position, end: second },
          words: `${str(text, position, first)} ${web.slash} ${str(text, first + 3, second)}`,
        };
      }
    }

    // 1. The token, from its start, without the punctuation closing it: (see /etc/apache).
    if (position > 0 && !isSpace(text[position - 1]) && !contains("(\"'“‘[", text[position - 1])) {
      return undefined;
    }
    let end = position;
    while (end < text.length && !isSpace(text[end])) {
      end++;
    }
    while (end > position && (contains(PUNCTUATION, text[end - 1]) || text[end - 1] === "।")) {
      end--;
    }
    let token = str(text, position, end);

    // 2. An optional drive, then the parts between the separators: C:\Users\HP
    const words: string[] = [];
    let separator = "/";
    if (token.startsWith("file://")) {
      words.push("file", web.colon ?? "", web.slash, web.slash); // file:///photos
      token = token.slice(7);
    } else if (token.length > 3 && isLatin(token[0]) && token[1] === ":" && token[2] === "\\") {
      words.push(token[0], web.colon ?? "");
      token = token.slice(2);
      separator = "\\";
    }
    const parts = token.split(separator);

    // 3. A path from the root (/etc/apache), a drive, or from a folder of lowercase words (home/desktop, but not
    // PCB/PCM or km/h); or a file name
    const rooted = parts[0] === "" || words.length > 0;
    const valid = parts.every((part) => [...part].every(isPath));
    const named = (part: string) => part.length >= 3 && [...part].every((c) => c >= "a" && c <= "z");
    const namedParts = parts.filter((part) => part !== "").length;
    const path = rooted ? namedParts >= 2 : parts.length >= 2 && parts.every(named);
    const file = parts.length === 1 && isFile(parts[0]) && isLatin(parts[0][0]);
    if (!valid || !(path || file)) {
      return undefined;
    }

    // 4. The words: फॉरवर्ड स्लैश etc फॉरवर्ड स्लैश apache, report डॉट pdf
    const slash = separator === "/" ? web.slash : web.backslash;
    parts.forEach((part, i) => {
      if (i > 0) {
        words.push(slash);
      }
      words.push(dotted(part, web));
    });
    return { span: { begin: position, end }, words: join(words) };
  }

  // The longest abbreviation at a position with a reading in its context
  #readAbbreviation(text: Text, position: number): Term | undefined {
    for (const key of keysAt(text, position, this.#keys)) {
      // An abbreviation ending with a letter ends a word, or touches a number: nr5
      const end = position + size(key);
      if (isWord(key.at(-1)) && !isClearAfter(text, end) && !isDigit(text[end])) {
        continue;
      }

      const readings = this.#terms.abbreviations[key];
      const context = new Context(text, { begin: position, end });
      let words = read(readings, context);
      if (words === undefined) {
        continue;
      }

      // A dot which also ends the text stays with it, and so does the dot of a closing abbreviation before a
      // capital (1st St. About, etc. The), but not the dot of a title (St. Louis). A word right after gets a space:
      // nr5 -> numer 5
      const chosen = readings.find((r) => typeof r === "string" || r.when(context));
      const title = chosen !== undefined && typeof chosen !== "string" && chosen.when === beforeName;
      const closing = (this.#terms.closing?.has(key) ?? false) && !title;
      if (key.endsWith(".") && (isSentenceEnd(text, end) || (closing && isSentenceEnd(text, end, true)))) {
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
