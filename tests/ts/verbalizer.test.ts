// What is specific to the TypeScript API. The shared cases (outputs, options, changes) are in tests/parity/api.jsonl.
import { describe, expect, test } from "vitest";

import { Verbalizer } from "../../ts/src/index.ts";

describe("Verbalizer", () => {
  test("changes count code points", () => {
    // The emoji before "5" is one code point but two UTF-16 units: offsets index [...text], not text
    const text = "🍎 5 apples & pears";
    const result = new Verbalizer("en-us").processWithChanges(text);

    expect(
      result.changes.map((c) => [...text].slice(c.start, c.end).join("")),
    ).toEqual(["5", " & "]);
  });

  test("languages", () => {
    for (const code of [
      "de",
      "en-us",
      "en-gb",
      "es",
      "fr",
      "hi",
      "it",
      "pl",
      "pt",
      "zh",
    ]) {
      expect(Verbalizer.languages()).toContain(code);
    }
    expect(() => new Verbalizer("xx")).toThrow(
      new RangeError("Unsupported language: xx"),
    );
  });
});
