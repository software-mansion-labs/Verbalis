import { str, textOf, type Text } from "./strings.ts";

// Appends source[begin, end) to target; a loop, since spreading a long array into push() overflows the stack.
function append<T>(target: T[], source: readonly T[], begin = 0, end: number = source.length): void {
  for (let i = begin; i < end; i++) {
    target.push(source[i]);
  }
}

// A half-open range [begin, end) of code points in a document's text.
export interface Span {
  begin: number;
  end: number;
}

// A span of a text, with the text in its place.
export interface Edit {
  span: Span;
  text: string;
}

// A piece of the current text: a copy of a span of the original text, or the text which replaced it.
// A replaced piece maps to its original span only as a whole.
interface Piece {
  original: Span;
  size: number; // In the current text
  replaced: boolean;
}

/**
 * The text being preprocessed, with lazy edits: every step queues its edits against the text as the step found it,
 * and commit() applies them at once. changes() maps the current text back to the original one.
 */
export class Document {
  #text: string[];
  #edits: { span: Span; text: string[] }[] = []; // Sorted by position
  #pieces: Piece[] = [];
  readonly #originalSize: number;

  constructor(text: Text | string) {
    this.#text = typeof text === "string" ? textOf(text) : [...text];
    this.#originalSize = this.#text.length;
    // At first, the whole text is a single copied piece
    if (this.#text.length > 0) {
      this.#pieces.push({ original: { begin: 0, end: this.#text.length }, size: this.#text.length, replaced: false });
    }
  }

  text(): Text {
    return this.#text;
  }

  /**
   * Queues a replacement of a span of the current text.
   * @param span a range of the text as it was when the current step started.
   * @throws Error if the span is out of range or overlaps an already queued edit.
   */
  replace(span: Span, replacement: string): void {
    if (span.begin > span.end || span.end > this.#text.length) {
      throw new Error("Document: edit span out of range");
    }

    // Keep the edits sorted, so that commit() applies them in a single pass
    let at = this.#edits.findIndex((edit) => edit.span.begin > span.begin);
    at = at === -1 ? this.#edits.length : at;
    const overlapsPrevious = at > 0 && this.#edits[at - 1].span.end > span.begin;
    const overlapsNext = at < this.#edits.length && this.#edits[at].span.begin < span.end;
    if (overlapsPrevious || overlapsNext) {
      throw new Error("Document: overlapping edits");
    }

    this.#edits.splice(at, 0, { span: { ...span }, text: textOf(replacement) });
  }

  // Applies all queued edits at once.
  commit(): void {
    const edits = this.#edits;
    if (edits.length === 0) {
      return;
    }
    const current = this.#text;

    // Where each piece starts in the current text
    const starts: number[] = [];
    let start = 0;
    for (const piece of this.#pieces) {
      starts.push(start);
      start += piece.size;
    }

    // 1. Group the edits into the spans they really change.
    // An edit inside an earlier replacement replaces all of it ("ten" -> "Ten" changes the whole "ten" of "10"),
    // and edits whose widened spans meet are merged: each group becomes one replaced piece.
    const groups: { span: Span; text: string[] }[] = [];
    for (let i = 0; i < edits.length;) {
      // Extend the group while the next widened edit starts inside it
      const group = { span: this.#widen(edits[i].span, starts), text: [] as string[] };
      let last = i;
      while (last + 1 < edits.length && this.#widen(edits[last + 1].span, starts).begin < group.span.end) {
        group.span.end = Math.max(group.span.end, this.#widen(edits[++last].span, starts).end);
      }

      // Its text: the current text of the span, with each edit's text in place of its range
      let position = group.span.begin;
      for (; i <= last; ++i) {
        append(group.text, current, position, edits[i].span.begin);
        append(group.text, edits[i].text);
        position = edits[i].span.end;
      }
      append(group.text, current, position, group.span.end);
      groups.push(group);
    }

    // 2. Rebuild the text and its pieces, walking the groups and the old pieces side by side.
    // Pieces before a group are kept, pieces it touches are merged into one replaced piece.
    const text: string[] = [];
    const pieces: Piece[] = [];
    const old = this.#pieces.map((piece) => ({ ...piece, original: { ...piece.original } }));
    let next = 0; // The first old piece not handled yet
    let position = 0; // The end of the current text copied so far

    for (const { span, text: replacement } of groups) {
      // An empty piece (a deletion) counts as touched when it lies at the group's edge, so it merges into the group
      const endOf = (i: number) => starts[i] + old[i].size;
      const before = (i: number) => (old[i].size > 0 ? endOf(i) <= span.begin : starts[i] < span.begin);
      const touched = (i: number) =>
        old[i].size > 0
          ? starts[i] < span.end && span.begin < endOf(i)
          : span.begin <= starts[i] && starts[i] <= span.end;

      // 2.1. The pieces before the group stay as they are
      while (next < old.length && before(next)) {
        pieces.push(old[next++]);
      }

      // 2.2. The original span behind the group: from its first touched piece to its last one.
      // An insertion between pieces touches none, and has an empty original span.
      let originalBegin = next < old.length ? old[next].original.begin : this.#originalSize;
      let originalEnd = originalBegin;
      if (next < old.length && touched(next)) {
        // A copied piece starting before the group keeps its head: "I have 2 cats" -> "I have " stays copied
        if (!old[next].replaced && starts[next] < span.begin) {
          const cut = span.begin - starts[next];
          pieces.push({ original: { begin: originalBegin, end: originalBegin + cut }, size: cut, replaced: false });
          originalBegin += cut;
        }

        let last = next;
        while (next < old.length && touched(next)) {
          last = next++;
        }
        originalEnd = old[last].original.end;

        // A copied piece ending after the group keeps its tail as an old piece: the next group may cut it again
        if (!old[last].replaced && endOf(last) > span.end) {
          const cut = span.end - starts[last];
          originalEnd = old[last].original.begin + cut;
          old[last].original.begin += cut;
          old[last].size -= cut;
          starts[last] = span.end;
          next = last;
        }
      }

      // 2.3. The text up to the group, then the group's replacement as one replaced piece
      append(text, current, position, span.begin);
      append(text, replacement);
      pieces.push({ original: { begin: originalBegin, end: originalEnd }, size: replacement.length, replaced: true });
      position = span.end;
    }

    // 3. The rest of the text after the last group, and its pieces
    append(text, current, position);
    append(pieces, old, next);

    this.#text = text;
    this.#pieces = pieces;
    this.#edits = [];
  }

  /**
   * The committed changes of the original text, in order: each replaced span of the original text with the current
   * text in its place.
   */
  changes(): Edit[] {
    const changes: Edit[] = [];

    // Walk the pieces with their position in the current text; each replaced one is a change, even when it touches the
    // previous one: " – " then "ceBooks.de" stay two changes, so a caller can map each back to its own words
    let position = 0;
    for (const piece of this.#pieces) {
      if (piece.replaced) {
        changes.push({ span: { ...piece.original }, text: str(this.#text, position, position + piece.size) });
      }
      position += piece.size;
    }
    return changes;
  }

  // The span of the current text an edit really changes: widened to whole replaced pieces it touches.
  #widen(span: Span, starts: readonly number[]): Span {
    const widened = { ...span };
    // An edge falling strictly inside a replaced piece moves to that piece's edge
    for (let i = 0; i < this.#pieces.length; ++i) {
      const begin = starts[i];
      const end = begin + this.#pieces[i].size;
      if (!this.#pieces[i].replaced) {
        continue;
      }
      if (begin < widened.begin && widened.begin < end) {
        widened.begin = begin;
      }
      if (begin < widened.end && widened.end < end) {
        widened.end = end;
      }
    }
    return widened;
  }
}
