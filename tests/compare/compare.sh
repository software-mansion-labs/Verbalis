#!/usr/bin/env bash
# Checks that the working tree verbalizes exactly like a reference commit: builds both, runs both on the same inputs,
# and lists every input whose output differs.
#
#   tests/compare/compare.sh [reference-commit]     (default: 3143074, the extraction from Phonemis)
#
# Inputs: the inputs of tests/parity/baseline/*.jsonl, plus any tests/parity/local/*.jsonl (gitignored, uncommitted).
set -euo pipefail

ROOT=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
REFERENCE=${1:-3143074}
WORK=$ROOT/build/compare
mkdir -p "$WORK"

build() { # <source tree> <build dir> -> runner binary
  cmake -S "$1/cpp" -B "$2" -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build "$2" -j >/dev/null
  c++ -std=c++20 -O2 -I"$1/cpp/include" -I"$ROOT/tests/cpp/third_party" "$ROOT/tests/compare/run.cpp" \
    "$2/libverbalis.a" -o "$2/run" 2>/dev/null
}

# The reference, checked out once into its own worktree
if [ ! -d "$WORK/reference-$REFERENCE" ]; then
  git -C "$ROOT" worktree add --detach "$WORK/reference-$REFERENCE" "$REFERENCE" >/dev/null 2>&1
fi
build "$WORK/reference-$REFERENCE" "$WORK/reference-$REFERENCE-build"
build "$ROOT" "$WORK/current-build"

cat "$ROOT"/tests/parity/baseline/*.jsonl "$ROOT"/tests/parity/local/*.jsonl 2>/dev/null \
  | python3 -c 'import json,sys; [print(json.dumps({"lang": r["lang"], "input": r["input"]}, ensure_ascii=False)) for r in map(json.loads, sys.stdin)]' \
  > "$WORK/inputs.jsonl"

"$WORK/reference-$REFERENCE-build/run" < "$WORK/inputs.jsonl" > "$WORK/reference.jsonl"
"$WORK/current-build/run" < "$WORK/inputs.jsonl" > "$WORK/current.jsonl"

python3 - "$WORK/reference.jsonl" "$WORK/current.jsonl" <<'PY'
import json, sys
from collections import Counter
reference = [json.loads(l) for l in open(sys.argv[1], encoding="utf-8")]
current = [json.loads(l) for l in open(sys.argv[2], encoding="utf-8")]
total, differ = Counter(), Counter()
shown = 0
for r, c in zip(reference, current):
    total[r["lang"]] += 1
    if r["output"] != c["output"]:
        differ[r["lang"]] += 1
        if shown < 10:
            shown += 1
            print(f"[{r['lang']}] {r['input']}\n  reference: {r['output']}\n  current:   {c['output']}")
for lang in sorted(total):
    print(f"{lang}: {total[lang] - differ[lang]}/{total[lang]} identical")
print(f"compare: {sum(total.values()) - sum(differ.values())}/{sum(total.values())} identical")
sys.exit(1 if differ else 0)
PY
