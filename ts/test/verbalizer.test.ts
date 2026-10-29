// What is specific to the TypeScript API. The shared cases (outputs, options, changes) are in tests/parity/api.jsonl.
import { describe, expect, test } from "vitest";

import { Verbalizer as Core } from "../src/core.ts";
import { Verbalizer } from "../src/index.ts";
import { language as polish } from "../src/languages/pl/polish.ts";

describe("Verbalizer", () => {
  test("changes count UTF-16 code units", () => {
    // The emoji before "5" is two UTF-16 units, so the offsets index the string itself
    const text = "🍎 5 apples & pears";
    const result = new Verbalizer("en-us").processWithChanges(text);

    expect(result.changes.map((c) => text.slice(c.start, c.end))).toEqual(["5", " & "]);
  });

  test("core with one language", () => {
    expect(new Core(polish()).process("Mam 2 koty.")).toBe("Mam dwa koty.");
  });

  test("languages", () => {
    for (const code of ["de", "en-us", "en-gb", "es", "fr", "hi", "it", "pl", "pt", "zh"]) {
      expect(Verbalizer.languages()).toContain(code);
    }
    expect(() => new Verbalizer("xx")).toThrow(new RangeError("Unsupported language: xx"));
  });
});
