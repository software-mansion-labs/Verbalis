#!/bin/sh
# Runs the parity outputs and API cases on Hermes, the engine of React Native, as release apps do (bytecode).
#
#   HERMES=<path to a hermes-engine pod's destroot> tools/hermes/run.sh
#
# The destroot holds bin/hermesc and Library/Frameworks/macosx/hermesvm.framework, as in an app's
# ios/Pods/hermes-engine/destroot after `pod install` (React Native 0.88 was checked). Builds the TypeScript package
# first. Hermes shares one binding of a loop's let among closures made in the loop: this run catches such bugs.
set -e
: "${HERMES:?Set HERMES to a hermes-engine destroot}"
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

c++ -std=c++20 -O2 -I"$HERMES/include" -F"$HERMES/Library/Frameworks/macosx" -framework hermesvm \
  -Wl,-rpath,"$HERMES/Library/Frameworks/macosx" "$here/host.cpp" -o "$work/host"
(cd "$root/ts" && npm run -s build >/dev/null)
python3 - "$root" "$work/rows.json" <<'PY'
import json, sys
from pathlib import Path
root, out = Path(sys.argv[1]), sys.argv[2]
rows = []
for path in sorted((root / "tests/parity/outputs").glob("*.jsonl")):
    rows += [[r["lang"], {}, r["input"], r["output"]] for r in map(json.loads, path.read_text().splitlines()) if r]
rows += [[r["lang"], r.get("options", {}), r["input"], r["output"]]
         for r in map(json.loads, (root / "tests/parity/api.jsonl").read_text().splitlines())]
Path(out).write_text(json.dumps(rows, ensure_ascii=False))
PY
cp "$here/parity.js" "$work/parity.js"
sed -i '' "s#\"../../ts/dist/index.js\"#\"$root/ts/dist/index.js\"#" "$work/parity.js"
(cd "$work" && npx -y esbuild@0.25 parity.js --bundle --format=iife --target=es2022 --outfile=bundle.js --log-level=error)
"$HERMES/bin/hermesc" -O -emit-binary -out "$work/bundle.hbc" "$work/bundle.js" 2>/dev/null
"$work/host" "$work/bundle.hbc"
