// Every parity output and API case, read by the TypeScript package on Hermes: prints the mismatches and their count.
import { Verbalizer } from "../../ts/dist/index.js";
import rows from "./rows.json";

const verbalizers = {};
let mismatches = 0;
for (const [lang, options, input, output] of rows) {
  const { symbol_words: symbolWords, ...rest } = options;
  const key = lang + JSON.stringify(options);
  verbalizers[key] ??= new Verbalizer(lang, { ...rest, ...(symbolWords && { symbolWords }) });
  const got = verbalizers[key].process(input);
  if (got !== output) {
    mismatches++;
    if (mismatches <= 10) {
      print(JSON.stringify({ lang, input, expected: output, got }));
    }
  }
}
print(`${rows.length} checked, ${mismatches} mismatches`);
