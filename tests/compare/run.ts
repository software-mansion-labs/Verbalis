// Verbalizes JSON Lines from stdin with the TypeScript port: each {"lang": ..., "input": ...} line gets an "output"
// field, as run.cpp does. Run with Node 22.18 or later, which strips types: node tests/compare/run.ts < in > out
import { createInterface } from "node:readline";

import { Verbalizer } from "../../ts/src/index.ts";

const verbalizers = new Map<string, Verbalizer>();
for await (const line of createInterface({ input: process.stdin, crlfDelay: Infinity })) {
  if (line === "") {
    continue;
  }
  const record = JSON.parse(line) as { lang: string; input: string; output?: string };
  let verbalizer = verbalizers.get(record.lang);
  if (!verbalizer) {
    verbalizer = new Verbalizer(record.lang);
    verbalizers.set(record.lang, verbalizer);
  }
  record.output = verbalizer.process(record.input);
  process.stdout.write(`${JSON.stringify(record)}\n`);
}
