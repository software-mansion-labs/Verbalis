#!/usr/bin/env bash
# Checks that the TypeScript port verbalizes exactly like the C++ core of the working tree: runs both on the same
# inputs and lists every input whose output differs.
#
#   tests/compare/cross.sh [language ...]     (default: every language the TypeScript port supports)
#
# Inputs: the inputs of tests/parity/outputs/*.jsonl (the corpus and the gold set).
set -euo pipefail

ROOT=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
WORK=$ROOT/build/cross
mkdir -p "$WORK"

cmake -S "$ROOT/cpp" -B "$WORK/cpp" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$WORK/cpp" -j >/dev/null
c++ -std=c++20 -O2 -I"$ROOT/cpp/include" -I"$ROOT/tests/cpp/third_party" "$ROOT/tests/compare/run.cpp" \
  "$WORK/cpp/libverbalis.a" -o "$WORK/run" 2>/dev/null

LANGUAGES=${*:-$(node --input-type=module -e "import('$ROOT/ts/src/index.ts').then(({ Verbalizer }) => console.log(Verbalizer.languages().join(' ')))")}
cat "$ROOT"/tests/parity/outputs/*.jsonl \
  | python3 -c 'import json,sys; languages = set(sys.argv[1].split()); [print(json.dumps({"lang": r["lang"], "input": r["input"]}, ensure_ascii=False)) for r in map(json.loads, sys.stdin) if r["lang"] in languages]' "$LANGUAGES" \
  > "$WORK/inputs.jsonl"

"$WORK/run" < "$WORK/inputs.jsonl" > "$WORK/cpp.jsonl"
node "$ROOT/tests/compare/run.ts" < "$WORK/inputs.jsonl" > "$WORK/ts.jsonl"
python3 "$ROOT/tests/compare/diff.py" "$WORK/cpp.jsonl" "$WORK/ts.jsonl" cpp ts
