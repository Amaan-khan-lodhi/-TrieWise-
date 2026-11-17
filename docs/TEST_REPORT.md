# Verification report

## Environment

- Verification date: 18 October 2026 for release 1.2; earlier engine benchmarks dated 16 October 2026.
- Runtime: Linux x86-64, GCC 13.3.0, C++17.
- Release flags: `-O2 -Wall -Wextra -Wpedantic`.
- Additional application compilation: `-Wconversion -Wshadow -Werror`.
- No third-party C++ library dependencies.

## Results

| Check | Result |
| --- | --- |
| Optimized application and test compilation | PASS, no warnings |
| Strict application warning build | PASS |
| C++ engine suite | PASS: 27,771 checks |
| Actual-process CLI integration suite | PASS: 44 checks |
| Engine AddressSanitizer + UndefinedBehaviorSanitizer | PASS: 27,771 checks on unchanged engine, release 1.1 |
| CLI AddressSanitizer + UndefinedBehaviorSanitizer | PASS: 44 checks |
| Starter dictionary | 536 unique records, 0 rejected |
| 10,000-entry synthetic benchmark | PASS: all 11 query result sets and candidate counts match |
| 100,000-entry synthetic benchmark | PASS: all 11 query result sets and candidate counts match |
| Save/load frequencies | PASS |
| Paths with spaces and literal backslashes | PASS on Linux |
| Missing files, invalid commands, EOF exit | PASS |
| Native Windows / MSVC execution | Not available in verification environment |
| Optional CMake build | Configuration supplied; CMake unavailable in verification environment |

The C++ count includes each assertion, not 27,771 unrelated scenarios. The Python
count is separate. Release 1.2 adds menu search/correction/selection, duplicate-word
protection, malformed input, save-on-exit, cancellation, failed-save recovery, EOF
at nested prompts, dictionary import, advanced-mode return, automatic resume, and
explicit starter override tests. `tests/test_engine.cpp` verifies 120 nonempty strings over the
alphabet {a,b,c}, lengths 1-4. Every pair is checked against an independent full
matrix (14,400 distance assertions). Exhaustive query/distance/K combinations are
checked against a dictionary scan and an independent full-collection/full-sort
oracle. A seeded, 2,000-insertion random dictionary and 250 random queries exercise
longer paths and broader alphabets.

The tests cover case normalization, prefix versus full-word membership, empty
prefixes, empty dictionaries, maximum word length, invalid characters, numeric
overflow, K=0, edit limit 0-3, duplicate replacement, deterministic heap ordering,
distance-first fuzzy ranking, selection saturation, history counts, malformed file
rows, merge semantics, persistence and read/write failures.

## Sanitizer limitations

Address and undefined-behavior instrumentation ran with `ASAN_OPTIONS=detect_leaks=0`.
LeakSanitizer's process-scanning phase cannot run in this container (restricted
`/proc` access). Leak detection is therefore not claimed. The engine uses standard
container ownership without raw owning pointers. Successful tests do not constitute
a guarantee of zero defects on every compiler, dataset or operating system.

To repeat on a compatible Linux environment:

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Iinclude tests/test_engine.cpp -o build/triewise_tests_sanitized
ASAN_OPTIONS=detect_leaks=0 ./build/triewise_tests_sanitized
```

On a machine supporting LeakSanitizer, set `detect_leaks=1` for additional checking.

## Final performance and audit evidence

The current raw reports are `benchmark_starter.txt`, `benchmark_synthetic10000.txt`
and `benchmark_synthetic100000.txt`. They check all 11 result sets before timing.
The improved scan baseline reuses memory, filters impossible lengths and stops
comparisons early. Four of five fuzzy queries on the 10,000-entry dataset favored
that scan; the Trie won the listed final 100,000-entry queries. Neither result is
a general speed guarantee or evidence of real-language relevance.

`review_comparison100000.txt` compares the original and final search engines on the
same data with 31 alternating repetitions per query. `REVIEW.md` explains the
observed benefits and costs. The original raw benchmark is separately labeled
`review_original_benchmark100000.txt` and used a weaker scan baseline.

Regression additions cover ancestor-bound refresh after frequency decreases,
randomized mutations, snapshot replacement and cleanup, failed save preservation,
unsaved-change notices and stricter argument validation. Tests pass in the final
layout with prefix ranking summaries stored separately from character nodes.

## Release 1.2 packaging check

The source archive is extracted into a folder containing spaces and built using
its own build script. SHA-256 entries and ZIP integrity are checked before delivery.
The Windows batch files are reviewed only; no Windows or MSVC test result is claimed.
Performance reports are retained from release 1.1 because the engine is unchanged.
