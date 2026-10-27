# Third-party notices

Verbalis is licensed under the MIT license (see [LICENSE](LICENSE)). Parts of it come from other projects and are
distributed under their own terms, listed below. The full license texts are in [`licenses/`](licenses/).

## cn2an

- **Files:** `cpp/src/verbalis/languages/zh/mandarin.cpp`, `cpp/src/verbalis/languages/zh/constants.h`.
- **Source:** a C++ port of the `an2cn` transform of [cn2an](https://github.com/Ailln/cn2an) by Ailln.
- **License:** MIT, see [`licenses/cn2an-MIT.txt`](licenses/cn2an-MIT.txt).

## Unicode Character Database

- **Files:** `cpp/src/verbalis/text/unicode_table.h`.
- **Source:** character properties generated from Python's `unicodedata` (the Unicode Character Database) by
  Phonemis's `scripts/tools/generate_unicode_table.py`.
- **License:** Unicode License v3, see [`licenses/Unicode-3.0.txt`](licenses/Unicode-3.0.txt).

## nlohmann/json

- **Files:** `tests/cpp/third_party/nlohmann/json.hpp`, used only by the parity test, never by the library.
- **Source:** [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann.
- **License:** MIT, see [`licenses/nlohmann-json-MIT.txt`](licenses/nlohmann-json-MIT.txt).

## Gold test cases

- **Files:** `tests/lang/*/gold.jsonl`, test data only, never in a package.
- **Source:** the text normalization test cases of [NeMo text processing](https://github.com/NVIDIA/NeMo-text-processing)
  by NVIDIA and of [WeTextProcessing](https://github.com/wenet-e2e/WeTextProcessing) by WeNet, converted to one format
  by `tests/lang/build.py` (German cases swapped to written → spoken; acceptable readings of an input merged).
- **License:** Apache License 2.0, see [`licenses/NeMo-text-processing-Apache-2.0.txt`](licenses/NeMo-text-processing-Apache-2.0.txt)
  and [`licenses/WeTextProcessing-Apache-2.0.txt`](licenses/WeTextProcessing-Apache-2.0.txt).
