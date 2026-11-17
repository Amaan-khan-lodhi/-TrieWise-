# TrieWise

**Autocomplete and Fuzzy Search Engine | C++17 | DSA Mini-Project | Reviewed release 1.2**

A complete offline console application implementing a Trie, bounded priority queue,
Levenshtein dynamic programming with branch pruning, and hash-based query history.
No AI API, internet connection, database, or third-party C++ library is used at runtime.

## Start here on Windows

1. Extract the ZIP into a **new folder** and open its `TrieWise` folder.
2. Double-click **`run.bat`**. On the first launch it builds the program and runs
   the C++ engine tests. Later launches open the program immediately.
3. Choose **1**, type **comp**, and press Enter to see suggestions. Press Enter
   again to return to the menu, or choose a result number to increase its weight.

A C++17 `g++` compiler must be installed for the first build. The launcher detects
MSYS2's default `C:\msys64\ucrt64\bin` installation automatically; alternatively,
use a suitable `g++` already on PATH. If the compiler is missing:

1. Install the x86-64 version of [MSYS2](https://www.msys2.org/) using its default
   folder (`C:\msys64`) on a supported 64-bit Windows PC.
2. Open **MSYS2 UCRT64** and run:

   ```text
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```

3. Accept the installation prompt, then return to the project folder and
   double-click `run.bat`. Linux or WSL is not required for this route.

Compiler setup follows the official MSYS2 getting-started guide, checked 18 October
2026. For a nondefault installation folder, add its `ucrt64\bin` directory to your
Windows user PATH, then open a new Command Prompt or sign out and back in.

The build creates **`build\triewise.exe`**. The archive contains source code, not a
prebuilt Windows executable. Keep the compiler runtime installed and launch with
`run.bat` so its DLLs and the dictionary can be found. Native Windows execution was
not available for verification; the actual build and tests ran on Linux.

To rebuild after editing source, open Command Prompt in the project folder:

```bat
build.bat
run.bat
```

In PowerShell, use `./build.bat` and `./run.bat`. A failed launch keeps its error
message visible rather than closing the window immediately.

## Ubuntu or WSL2 alternative

Extract the ZIP. Open an Ubuntu terminal **inside the extracted `TrieWise` folder**.
A C++17 compiler is required (GCC 9 or newer recommended).

```bash
# Only if g++ is not installed:
sudo apt update
sudo apt install g++

# Compile the application and run its tests:
bash build.sh

# Start the application:
bash run.sh
```

The `sudo apt` commands install your local compiler, not application dependencies.
Python 3 is optional for the additional CLI tests and synthetic dataset generator.
The C++ application and engine tests do not require Python.

For a folder on Windows, first change to its WSL path, for example:

```bash
cd /mnt/d/Projects/TrieWise
bash build.sh
bash run.sh
```

Replace the example path with your actual extracted folder. Do not run from inside the ZIP.

## Using the simple menu

| Choice | Action |
| --- | --- |
| 1 | Search a word or prefix; tries spelling corrections if there are no prefix matches |
| 2 | Ask for spelling suggestions directly, allowing up to two letter edits |
| 3 | Add a word with weight 1; existing words keep their current weight |
| 4 | Save all words and weights; Enter accepts the displayed filename |
| 5 | Exact lookup, dictionary import, history, statistics and advanced commands |
| 0 | Exit; offers Save, Discard or Cancel when changes are pending |

Search results show up to five words. Choose a result number to increase its weight
by one, or press Enter to return without changing anything. Invalid input shows an
explanation and lets you try again. Searches run after pressing Enter.

The default save is **`data/my_dictionary.txt`**. Normal launches reopen this file
if present; otherwise they use `data/dictionary.txt`. The active file is printed at
startup. The supplied starter dictionary stays unchanged when you use the default
save. Saving to a filename that already exists replaces that file.

If you save to a different filename, pass it when you next start, for example:

```bat
run.bat "data/my_other_dictionary.txt"
```

To deliberately start again with the untouched starter:

```bat
run.bat data/dictionary.txt
```

A failed save keeps the application open with changes still in memory. End-of-input
also exits and reports unsaved changes; it cannot prompt if input has already ended.
History is session-only and is not saved.

## Advanced commands for the DSA demonstration

Choose **5**, then **5** in the menus, or start command mode directly:

```bat
run.bat data/dictionary.txt --commands
```

On WSL/Linux use `bash run.sh data/dictionary.txt --commands`.
This mode shows frequencies, edit counts, traversal counters and timings.
Type `back` to return if you entered through the menu. Direct `--commands` mode is
suitable for scripts: `quit` exits without an interactive save question, printing a
notice if changes are unsaved.

```text
prefix comp
fuzzy computr 1
search netwrok
exact COMPUTER
add customword 100
select customword
history
stats
benchmark 20
save "data/my_dictionary.txt"
quit
```

## Features

- Case-insensitive exact lookup and prefix completions.
- Top-K prefix suggestions via a best-first priority queue, default K=5.
- Per-branch best-word/count summaries avoid exhaustive prefix subtree scans.
- A bounded heap ranks fuzzy candidates; distance and frequency ties are deterministic.
- Whole-word fuzzy matching at Levenshtein edit limits 0-3, default 2.
- Combined search: prefix suggestions first, typo correction only when no prefix matches exist.
- Deterministic ranking: fewer edits, higher frequency, alphabetical tie break.
- Word insertion, frequency replacement, and frequency increments on selection.
- Dictionary merging and saving; invalid dictionary records are counted and skipped.
- Hash-table query counts, printed in frequency/alphabetical order.
- Measured search timings, candidate counts and traversal/DP counters.
- Independent linear-scan baselines and parity-checked benchmarks.
- Automated C++ correctness tests and Python CLI integration tests.

## Advanced command reference

| Command | Meaning |
| --- | --- |
| `prefix TEXT [K]` | Top-K words starting with TEXT |
| `prefix * [K]` | Top-K words across the whole dictionary |
| `fuzzy WORD [D] [K]` | Whole-word corrections within D edits |
| `search WORD [K]` | Prefix search, then fuzzy fallback at D=2 |
| `exact WORD` | Check terminal-word membership |
| `add WORD [F]` | Add a word or replace its frequency; F defaults to 1 |
| `select WORD` | Increment an existing word's frequency |
| `load "PATH"` | Merge records; last valid duplicate replaces frequency |
| `save "PATH"` | Write all dictionary words and frequencies alphabetically |
| `history` | Show search-command usage counts for this session |
| `stats` | Show unique words, Trie nodes, and estimated dictionary storage |
| `benchmark [RUNS]` | Compare prefix and fuzzy search against linear scans |
| `help` | Print command help |
| `back` | Return from advanced commands to the menu, when entered through More options |
| `quit` / `exit` | Exit; EOF also exits cleanly |

Words contain only English letters a-z (uppercase is normalized), with a maximum
length of 64. Punctuation, digits, spaces within words, and non-ASCII letters are
rejected. K can be 0-100; zero requests no work/results. Frequencies are integers
from 1 to 1,000,000,000,000 and selection saturates at the maximum. Benchmark
repetitions range from 1 to 1000. Quoted paths support spaces.

This is an Enter-to-search terminal interface. It does not capture keystrokes or
refresh a graphical dropdown on every typed character. Prefix search and typo
correction are distinct operations; fuzzy matching is not fuzzy-prefix completion.

## Dictionary format and provenance

`data/dictionary.txt` contains **536 curated English and computing terms**.
Every supplied frequency is an invented demonstration weight, not a measured corpus
frequency. No external dictionary was downloaded or included.

```text
# Comments begin with #; blank lines are ignored
computer 9800
compile 8600
network 9500
banana
```

A missing frequency defaults to 1. Malformed records are skipped and reported;
valid rows are merged into the current dictionary. Queries with no matches are
valid searches and appear in history. History is in memory only; saving writes
words/frequencies, not search history.

## Project files

| Path | Purpose |
| --- | --- |
| `include/triewise.hpp` | Search engine, bounded heap, DP baseline, persistence, benchmark |
| `src/main.cpp` | Command parsing, application startup and display |
| `tests/test_engine.cpp` | Examples, exhaustive and random parity tests, persistence |
| `tests/test_cli.py` | Tests of the compiled program through stdin/stdout |
| `data/dictionary.txt` | Starter dictionary and clearly labeled sample weights |
| `data/synthetic10000.txt` | Reproducible synthetic strings for scaling tests |
| `tools/generate_dataset.py` | Generate 1-1,000,000 synthetic entries |
| `docs/ALGORITHMS.md` | Architecture, recurrence, ranking, accurate complexity |
| `docs/DEMO_AND_VIVA.md` | Demo sequence and oral-exam preparation |
| `docs/REVIEW.md` | Review findings, implemented fixes, and remaining limitations |
| `docs/TrieWise_Project_Overview.docx` | Concise illustrated-by-example project report |
| `docs/TrieWise_Project_Overview.pdf` | Report suitable for reading and sharing |
| `docs/TEST_REPORT.md` | Actual validation results and verification limits |
| `docs/benchmark_*.txt` | Actual measurements from this build environment |
| `build.sh`, `run.sh` | WSL/Linux build, verification and launch |
| `build.bat`, `run.bat` | Native Windows build and launch |
| `CMakeLists.txt` | Optional GCC/Clang/MSVC project configuration |

## Manual build

From the project root:

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp -o build/triewise
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude tests/test_engine.cpp -o build/triewise_tests
./build/triewise_tests
python3 tests/test_cli.py build/triewise data/dictionary.txt
./build/triewise
```

Optional CMake build (requires CMake 3.20+):

```bash
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --config Release
ctest --test-dir build/cmake -C Release --output-on-failure
```

Run CMake-built executables from the project root or pass an explicit dictionary
path. Multi-configuration Windows generators place executables under `Release`.

## Reproduce benchmarks

```bash
./build/triewise data/dictionary.txt --benchmark 30
./build/triewise data/synthetic10000.txt --benchmark 10
python3 tools/generate_dataset.py --count 100000 --output data/synthetic100000.txt
./build/triewise data/synthetic100000.txt --benchmark 5
```

The generator refuses to overwrite existing files. Synthetic entries are random
letter strings plus a few fixed computing terms. They test scaling, not English
search relevance. The 100,000-entry file is reproducible and intentionally omitted
from the ZIP to keep the download small; its measured benchmark is included.

Benchmark queries, K, threshold and ranking rules are identical for both algorithms.
Each query is warmed up, full top-K outputs and candidate counts are checked, and
measurement order alternates. Loading and printing are outside timed regions.
Median and p95 are lower-index empirical percentiles; tiny samples are only a rough
indicator. Ratios below 1 mean the scan was faster. This is a single-process
microbenchmark, not a production workload or a guaranteed speedup.

Storage statistics are a capacity-based approximation including a conservative
string-buffer allowance, potentially double-counting inline string storage.
They exclude allocator overhead and query history and are not measured process RAM.

## Learning and submission

Read `docs/ALGORITHMS.md`, rehearse `docs/DEMO_AND_VIVA.md`, and rerun `bash build.sh`
on your own laptop. The project is suitable for an academic DSA demo, with clear
limitations and measured evidence. Adapt attribution and submission to your course's
AI-assistance policy. Do not claim generated benchmark numbers as your own hardware's
measurements without rerunning them.
