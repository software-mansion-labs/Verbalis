import type { Document } from "../text/document.ts";
import { contains, startsWith, str, type Text } from "../text/strings.ts";
import { isDigit, isSpace } from "../text/unicode.ts";

// Whether a position starts punctuation which takes no space before it: "a ," -> "a,", but "a ..." and the point of
// "about .5 kg" stay
function isClosingPunctuation(text: Text, position: number): boolean {
  if (position >= text.length) {
    return false;
  }
  const ellipsis = startsWith(text, position, "...");
  const decimal = isDigit(text[position + 1]);
  return contains(",!?;:", text[position]) || (text[position] === "." && !ellipsis && !decimal);
}

// Single spaces between words, none at the edges or before punctuation; a run with a newline becomes one newline.
export function trim(document: Document): void {
  const text = document.text();
  for (let i = 0; i < text.length;) {
    if (!isSpace(text[i])) {
      i++;
      continue;
    }

    // Find the whole run of whitespaces
    let end = i;
    let newline = false;
    while (end < text.length && isSpace(text[end])) {
      newline ||= text[end] === "\n";
      end++;
    }

    // Edges and spaces before punctuation lose the run entirely, the middle keeps a single character
    const closing = !newline && isClosingPunctuation(text, end);
    const replacement = i === 0 || end === text.length || closing ? "" : newline ? "\n" : " ";
    if (str(text, i, end) !== replacement) {
      document.replace({ begin: i, end }, replacement);
    }
    i = end;
  }
}
