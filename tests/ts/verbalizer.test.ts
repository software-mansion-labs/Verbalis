// The TypeScript API: the same cases as tests/cpp/verbalizer_test.cpp and tests/python/test_verbalizer.py.
import { describe, expect, test } from "vitest";

import { Verbalizer } from "../../ts/src/index.ts";

describe("Verbalizer", () => {
  test("process", () => {
    expect(new Verbalizer("en-us").process("I bought 3 tickets for $49.")).toBe(
      "I bought three tickets for forty-nine dollars.",
    );
    expect(new Verbalizer("pl").process("Mam 2 koty.")).toBe("Mam dwa koty.");
  });

  test("changes count code points", () => {
    // The emoji before "5" is one code point but two UTF-16 units: offsets index [...text], not text
    const text = "🍎 5 apples & pears";
    const result = new Verbalizer("en-us").processWithChanges(text);

    expect(result.text).toBe("🍎 five apples and pears");
    expect(result.changes).toEqual([
      { start: 2, end: 3, replacement: "five" },
      { start: 10, end: 13, replacement: " and " },
    ]);
    expect(
      result.changes.map((c) => [...text].slice(c.start, c.end).join("")),
    ).toEqual(["5", " & "]);
  });

  test("options", () => {
    expect(
      new Verbalizer("en-us", { numbers: false }).process("3 cats & 2 dogs"),
    ).toBe("3 cats and 2 dogs");

    const custom = new Verbalizer("en-us", {
      abbreviations: { "approx.": "approximately" },
      symbolWords: { "™": "trademark" },
    });
    expect(custom.process("approx. 10 Acme™")).toBe(
      "approximately ten Acme trademark",
    );
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
