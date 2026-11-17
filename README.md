TrieWise
Autocomplete and Fuzzy Search Engine | C++17 | DSA Mini-Project

TrieWise is a small offline search application built in C++. It helps users find words, complete partially typed words, and get spelling suggestions when a word is mistyped.

The project was built to demonstrate data structures and algorithms in a practical way. Its main components include a Trie, priority queues, a bounded heap, Levenshtein edit distance using dynamic programming, and a hash table for search history.

TrieWise works locally. It does not need an AI API, internet connection, database, or third-party C++ library while running.
What you can do with TrieWise
- Autocomplete: enter a beginning such as comp and get matching words.
- Spelling suggestions: enter a typo such as computr and find close matches.
- Exact lookup: check whether a complete word is in the active dictionary.
- Manage words: add words, import another wordlist, and save your dictionary.
- Rank suggestions: selecting a suggestion increases its stored frequency, which can affect future rankings.
- View search information: inspect dictionary statistics, search history, and performance counters.
- Run benchmarks: compare Trie-based searching with a linear scan.
Getting started on Windows
You need a C++17 g++ compiler for the first build.
1. Download or clone this repository and open the project folder.
2. Double-click run.bat.
3. On the first launch, TrieWise builds the application and runs the C++ engine tests. Later launches open the program directly.
4. Choose 1, enter comp, and press Enter to see autocomplete suggestions.
If g++ is not installed, the Windows launcher supports the default MSYS2 installation:
1. Install the x86-64 version of MSYS2 in its default folder, C:\msys64.
2. Open MSYS2 UCRT64 and run:
   pacman -S mingw-w64-ucrt-x86_64-gcc
3. Accept the installation prompt. Then return to the project folder and run run.bat again.
The launcher detects the compiler at C:\msys64\ucrt64\bin or uses a suitable g++ already on your PATH. Linux or WSL is not required for the Windows route.

To rebuild after changing the source code, run these commands from the project folder in Command Prompt:
build.bat
run.bat
In PowerShell, use ./build.bat and ./run.bat.

The first build creates build\triewise.exe. A prebuilt Windows executable is not included in the repository. Keep the compiler runtime installed and start the application with run.bat so the required DLLs and dictionary files can be found.

Verification note: the published test report records successful builds and tests on Linux with GCC 13.3.0. Native Windows execution was not available in that verification environment, so the Windows launcher is provided but is not claimed as independently verified there.
Running on Ubuntu or WSL2
Open a terminal inside the project folder. You need a C++17 compiler; GCC 9 or newer is recommended.

If g++ is not installed, run:
sudo apt update
sudo apt install g++
Then build and start TrieWise:
bash build.sh
bash run.sh
For a project folder stored on your Windows drive, the WSL path may look like this:
cd /mnt/d/Projects/TrieWise
bash build.sh
bash run.sh
Replace the example path with the folder where you saved the repository. Do not run the scripts from inside a ZIP file.

Python 3 is optional. It is used for the additional CLI integration tests and the synthetic dataset generator; the C++ application and engine tests do not require Python.
Using the menu
TrieWise opens with a simple numbered menu:
Choice	What it does
1	Search for a word or prefix. If there are no prefix matches, TrieWise tries spelling correction.
2	Get spelling suggestions directly, allowing up to two letter edits by default.
3	Add a new word with a starting weight of 1. If the word already exists, its current weight is kept.
4	Save the active dictionary and its word frequencies. Press Enter to use the displayed save path.
5	Open more options: exact lookup, dictionary import, search history, statistics, and advanced commands.
0	Exit. TrieWise asks whether to save, discard, or cancel when there are unsaved changes.


Search results show up to five suggestions. Choose a result number to increase that word's frequency by one, or press Enter to return to the menu. Searches begin after you press Enter; the terminal does not update suggestions after every keystroke.
Using the expanded English dictionary
The repository includes two wordlists:
- data/dictionary.txt is the original starter dictionary with 536 curated English and computing terms.
- data/english_dictionary.txt is an additional, larger American English wordlist.
The expanded list is included as a separate file. On a fresh clone, TrieWise still starts with the starter dictionary unless you import the expanded list yourself.

To import it from the menu:
1. Start TrieWise with run.bat (or bash run.sh on Linux/WSL).
2. Choose 5 - More options.
3. Choose 2 - Load another dictionary.
4. Enter this path:
   data/english_dictionary.txt
5. To keep the original starter words' demonstration weights, load data/dictionary.txt again after the expanded list. Duplicate entries use the frequency from the most recently loaded file.
6. Return to the main menu and choose 4 - Save dictionary. Press Enter to save to the suggested path, normally data/my_dictionary.txt.
After that, TrieWise automatically uses data/my_dictionary.txt on the next launch if it exists. This saved file is local and is intentionally ignored by Git, so your personal word frequencies do not overwrite the starter files in the repository.

The expanded wordlist comes from the en_US-large English Speller Database (ESDB/SCOWL) release dated 2026.02.25. The upstream project describes its large list as a broader vocabulary that can include uncommon variants and entries that have not been checked as closely as its standard list. As a result, a few unusual suggestions may appear. TrieWise also applies its own input rules, so records outside the supported format are skipped and reported during import. See the upstream wordlist release and attribution for its source and separate terms.

Dictionary format: a word can be stored by itself or followed by a frequency. If no frequency is provided, TrieWise uses 1. Lines beginning with # are comments, and blank lines are ignored.
# Example entries
computer 9800
compile 8600
network 9500
banana
Malformed or unsupported records are skipped and counted. Valid records are merged into the active dictionary. If a word already exists, the frequency in the newly loaded file replaces its previous frequency.
How the suggestions are ranked
For autocomplete, TrieWise follows the prefix through the Trie and uses a best-first priority queue to find promising words without scanning every word in the matching branch.

For spelling correction, it calculates Levenshtein edit distance using dynamic programming. This measures how many single-character insertions, deletions, or substitutions are needed to turn one word into another. Branch pruning helps avoid work on paths that cannot lead to a result within the allowed edit limit.

Suggestions are ranked consistently:
1. Fewer edits are preferred.
2. Higher stored frequency is preferred when edit distance is equal.
3. Alphabetical order is used to break any remaining tie.
Normal search tries prefix matches first. Spelling correction is used as a fallback when no prefix matches are found. Fuzzy matching checks whole words; it is not fuzzy-prefix completion.
Advanced commands
For a closer look at the algorithms and their performance counters, open 5 - More options, then 5 - Advanced commands and benchmarks.

You can also start command mode directly:
run.bat data/dictionary.txt --commands
On Ubuntu or WSL2:
bash run.sh data/dictionary.txt --commands
Useful commands include:
Command	What it does
prefix comp	Shows words beginning with comp.
prefix comp 10	Shows up to 10 prefix matches.
prefix * 5	Shows the top five words across the dictionary.
fuzzy computr 1	Finds whole-word suggestions within one edit.
search netwrok	Tries prefix search first, then fuzzy correction.
exact COMPUTER	Checks whether the complete word exists.
add customword 100	Adds a word or replaces its stored frequency.
select customword	Increases an existing word's frequency.
load "data/english_dictionary.txt"	Imports and merges another wordlist.
save "data/my_dictionary.txt"	Saves the active dictionary and frequencies.
history	Displays search-command counts for the current session.
stats	Shows word count, Trie node count, and estimated storage.
benchmark 20	Compares Trie search with linear-scan baselines.
help	Displays the available commands.
quit	Exits command mode.


Use back to return to the numbered menu when you entered command mode through More options. Quoted file paths can contain spaces.
Project structure
Path	Purpose
src/main.cpp	Application startup, menus, command handling, and output.
src/triewise.hpp	Main TrieWise engine and search algorithms.
Include/triewise.hpp	Header copy included in the project.
tests/test_engine.cpp	C++ tests for search behavior, ranking, and dictionary operations.
tests/test_cli.py	Integration tests that run the compiled application.
data/dictionary.txt	Original 536-word starter dictionary.
data/english_dictionary.txt	Expanded English wordlist for optional import.
docs/ALGORITHMS.md	Explanation of the data structures, algorithms, and complexity.
docs/DEMO_AND_VIVA.md	Demonstration steps and viva preparation.
docs/REVIEW.md	Project review, improvements, and remaining limitations.
docs/TEST_REPORT.md	Test results and verification environment.
build.bat, run.bat	Windows build and launch scripts.
build.sh, run.sh	Linux and WSL2 build and launch scripts.
CMakeLists.txt	Optional CMake configuration.


Input rules and current limitations
- Words can contain English letters a-z; uppercase input is converted to lowercase.
- A word can be up to 64 characters long.
- Punctuation, digits, spaces inside words, and non-English characters are not accepted as part of a word.
- Fuzzy matching uses an edit-distance limit from 0 to 3, with 2 as the default.
- The number of suggestions (K) can be from 0 to 100.
- Frequencies are stored as integers from 1 to 1,000,000,000,000.
- Search history is kept only for the current session; it is not saved with the dictionary.
- TrieWise is a terminal application, not a live graphical autocomplete box. Users press Enter to run a search.
- The wordlists provide words and ranking weights, not definitions or meanings.
Tests and verification
The published verification report records the following results from the Linux x86-64 environment using GCC 13.3.0 and C++17:
- C++ engine test suite: 27,771 checks passed.
- CLI integration test suite: 44 checks passed.
- Optimized and strict-warning builds: passed.
- Save/load, invalid input, missing files, and paths with spaces: passed in the reported test environment.
These are test results for the checked project version and test environment, not a guarantee that every word or every computer will behave identically. Native Windows/MSVC execution and the optional CMake build were not available in that verification environment. See [`docs/TEST_REPORT.md`](docs/TEST_REPORT.md) for the details and limits of the checks.
Benchmarks
TrieWise includes benchmarks that compare its Trie-based search with a linear-scan baseline. The benchmark checks that both approaches return the same results before comparing timings. It reports measurements such as elapsed time, candidate counts, Trie node visits, and dynamic-programming work.

Synthetic datasets can be used to explore how the algorithms behave with larger inputs. They contain generated letter strings and a few fixed computing terms, so they are useful for scaling tests but do not measure real-world English search quality. Benchmark timings depend on the computer and workload; they are not a promise that the Trie will be faster for every query.

For the algorithm details and benchmark methodology, see [`docs/ALGORITHMS.md`](docs/ALGORITHMS.md) and the benchmark files in docs/.
License and wordlist attribution
The repository includes a license for the project code. The expanded wordlist has its own upstream source and terms: it is based on the en_US-large ESDB/SCOWL wordlist, release 2026.02.25, copyright 2000-2026 Kevin Atkinson. Please retain the upstream copyright and permission notice when redistributing that wordlist. The full source information and notice are available from the upstream release page.
