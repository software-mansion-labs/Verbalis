import { Verbalizer } from "/Users/bartlomiejobrochta/SWM_AI/ECHO/Verbalis/ts/dist/index.js";
import rows from "./rows2.json";
const cache = {};
let bad = 0;
for (const [lang, options, input, output] of rows) {
  const { symbol_words: symbolWords, ...rest } = options;
  const key = lang + JSON.stringify(options);
  const v = (cache[key] ??= new Verbalizer(lang, { ...rest, ...(symbolWords && { symbolWords }) }));
  const got = v.process(input);
  if (got !== output) { bad++; if (bad <= 10) print(JSON.stringify([lang, input.slice(0, 120), output.slice(0, 160), got.slice(0, 160)])); }
}
print("checked " + rows.length + ", mismatches " + bad);
