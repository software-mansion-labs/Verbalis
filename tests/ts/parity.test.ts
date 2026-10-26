// The parity contract: tests/parity/baseline/*.jsonl (outputs of process) and tests/parity/api.jsonl (options and the
// changes of processWithChanges), for every language the TypeScript port supports so far.
import { readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, test } from "vitest";

import { Verbalizer, type Options } from "../../ts/src/index.ts";

const PARITY = join(import.meta.dirname, "..", "parity");
const SUPPORTED = new Set(Verbalizer.languages());

interface BaselineRecord {
  lang: string;
  category: string;
  input: string;
  output: string;
}

interface ApiCase {
  lang: string;
  options: {
    [key: string]: unknown;
    symbol_words?: Readonly<Record<string, string>>;
  };
  input: string;
  output: string;
  changes: [number, number, string][];
}

function lines<T>(path: string): T[] {
  return readFileSync(path, "utf-8")
    .split("\n")
    .filter((line) => line !== "")
    .map((line) => JSON.parse(line) as T);
}

describe("baseline", () => {
  const files = readdirSync(join(PARITY, "baseline")).filter((file) =>
    file.endsWith(".jsonl"),
  );
  for (const file of files) {
    const records = lines<BaselineRecord>(join(PARITY, "baseline", file));
    test.skipIf(!SUPPORTED.has(records[0].lang))(file, () => {
      const verbalizer = new Verbalizer(records[0].lang);
      const mismatches = records.filter(
        (record) => verbalizer.process(record.input) !== record.output,
      );
      const first = mismatches[0];
      expect(
        mismatches.length,
        first &&
          `first: ${first.input}\n  expected: ${first.output}\n  got:      ${verbalizer.process(first.input)}`,
      ).toBe(0);
    });
  }
});

describe("api", () => {
  for (const [i, c] of lines<ApiCase>(join(PARITY, "api.jsonl")).entries()) {
    test.skipIf(!SUPPORTED.has(c.lang))(
      `${i + 1} ${c.lang}: ${c.input.slice(0, 30)}`,
      () => {
        const { symbol_words: symbolWords, ...rest } = c.options;
        const options: Options = {
          ...rest,
          ...(symbolWords && { symbolWords }),
        };
        const result = new Verbalizer(c.lang, options).processWithChanges(
          c.input,
        );
        expect(result.text).toBe(c.output);
        expect(
          result.changes.map((change) => [
            change.start,
            change.end,
            change.replacement,
          ]),
        ).toEqual(c.changes);
      },
    );
  }
});
