"""Compares two JSON Lines outputs of the same inputs, record by record, and lists the differences.

    python3 tests/compare/diff.py <a.jsonl> <b.jsonl> [a name] [b name]

Prints the first 10 differing records, then per language how many outputs are identical. Exits 1 if any differ.
"""

import json
import sys
from collections import Counter

a_path, b_path = sys.argv[1:3]
a_name, b_name = (sys.argv[3:5] + ["a", "b"][len(sys.argv[3:5]):])
a_records = [json.loads(line) for line in open(a_path, encoding="utf-8")]
b_records = [json.loads(line) for line in open(b_path, encoding="utf-8")]
assert len(a_records) == len(b_records), f"{len(a_records)} vs {len(b_records)} records"

total, differ = Counter(), Counter()
shown = 0
width = max(len(a_name), len(b_name))
for a, b in zip(a_records, b_records):
    total[a["lang"]] += 1
    if a["output"] != b["output"]:
        differ[a["lang"]] += 1
        if shown < 10:
            shown += 1
            print(f"[{a['lang']}] {a['input']}\n  {a_name:<{width}}: {a['output']}\n  {b_name:<{width}}: {b['output']}")
for lang in sorted(total):
    print(f"{lang}: {total[lang] - differ[lang]}/{total[lang]} identical")
print(f"compare: {sum(total.values()) - sum(differ.values())}/{sum(total.values())} identical")
sys.exit(1 if differ else 0)
