# Verbalis

Verbalis rewrites text the way it is read aloud, for text-to-speech front ends:

```
Dnia 12.03.2025 kupiłem 3 bilety.
→ Dnia dwunastego marca dwa tysiące dwudziestego piątego kupiłem trzy bilety.
```

Numbers, dates, times, phone numbers, money, units, symbols, abbreviations, and web addresses become words.
Languages: German, English (US, GB), Spanish, French, Hindi, Italian, Polish, Portuguese, and Chinese.

## C++

```cpp
#include <verbalis/verbalizer.h>

verbalis::Verbalizer english("en-us");
english.process("I bought 3 tickets for $49.");  // "I bought three tickets for forty-nine dollars."

// Turn steps off, or add readings of your own
verbalis::Verbalizer custom("en-us", {.numbers = false, .abbreviations = {{"approx.", "approximately"}}});

// Map the output back to the input: each change of [start, end), counted in code points
verbalis::Result result = english.process_with_changes("5 apples & pears");
```

Add it to a CMake project:

```cmake
add_subdirectory(verbalis/cpp)   # or FetchContent
target_link_libraries(your_target PRIVATE verbalis::verbalis)
```

Python and TypeScript packages with the same API are in progress.

## Development

```bash
cmake -S cpp -B cpp/build -DVERBALIS_BUILD_TESTS=ON && cmake --build cpp/build -j
ctest --test-dir cpp/build --output-on-failure
```

The parity test checks the C++ output against `tests/parity/baseline/`, which every implementation must match.

## License

MIT. Verbalis is extracted from the text preprocessor of [Phonemis](https://github.com/IgorSwat/Phonemis). Third-party
notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
