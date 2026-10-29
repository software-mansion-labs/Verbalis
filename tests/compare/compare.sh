#!/usr/bin/env bash
# Checks that the working tree verbalizes exactly like a reference commit: builds both, runs both on the same inputs,
# and lists every input whose output differs.
#
#   tests/compare/compare.sh [reference-commit]     (default: 3143074, the extraction from Phonemis)
#
# Inputs: the inputs of tests/parity/outputs/*.jsonl (the corpus and the gold set).
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

cat "$ROOT"/tests/parity/outputs/*.jsonl \
  | python3 -c 'import json,sys; [print(json.dumps({"lang": r["lang"], "input": r["input"]}, ensure_ascii=False)) for r in map(json.loads, sys.stdin)]' \
  > "$WORK/inputs.jsonl"

"$WORK/reference-$REFERENCE-build/run" < "$WORK/inputs.jsonl" > "$WORK/reference.jsonl"
"$WORK/current-build/run" < "$WORK/inputs.jsonl" > "$WORK/current.jsonl"

python3 "$ROOT/tests/compare/diff.py" "$WORK/reference.jsonl" "$WORK/current.jsonl" reference current
