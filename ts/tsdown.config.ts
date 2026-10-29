import { defineConfig } from "tsdown";

// One entry per language, so "verbalis/core" plus "verbalis/<code>" bundles only that language
const LANGUAGES = {
  de: "german",
  en: "english",
  es: "spanish",
  fr: "french",
  hi: "hindi",
  it: "italian",
  pl: "polish",
  pt: "portuguese",
  zh: "mandarin",
};

export default defineConfig({
  entry: {
    index: "src/index.ts",
    core: "src/core.ts",
    ...Object.fromEntries(Object.entries(LANGUAGES).map(([code, name]) => [code, `src/languages/${code}/${name}.ts`])),
  },
  format: "esm",
  dts: true,
  platform: "neutral",
});
