# Demo and viva guide

## A five-minute demonstration

For an everyday demo on Windows, double-click `run.bat`, choose 1, enter `comp`,
and press Enter again to return. Choose 2 and enter `computr` to show corrections.

For the algorithm demonstration below, open Command Prompt in the extracted root:

```bat
run.bat data/dictionary.txt --commands
```

On WSL/Linux, use `bash run.sh data/dictionary.txt --commands`. The explicit starter
path keeps the demonstration independent of any saved personal dictionary.

1. `stats`: explain 536 words, shared prefix paths and approximate storage.
2. `prefix comp 5`: show five ranked completions. The starter's leading suggestions
   are computer (9800), compile (8600), compiler (8100), computing (7400), comparison (4685).
3. `exact comp`: explain that a prefix path need not terminate a complete word.
4. `fuzzy computr 1`: computer and compute are both one-edit candidates; frequency
   puts computer first. Explain the insertion of e to form computer.
5. `search netwrok`: no prefix exists, so fuzzy fallback can find network at distance 2.
6. `add comp 9900`, then `prefix comp 3`: show a new terminal word becoming first.
7. `select computer`, then `prefix comp 3`: show computer's frequency becoming 9801.
8. `history`: explain hash-based search usage counts.
9. `save "data/my_dictionary.txt"`: demonstrate persistence without overwriting the starter.
10. `benchmark 20`: show actual timings, same results, and explain hardware dependence.
11. `quit`: exit. Relaunch with `run.bat data/my_dictionary.txt --commands` to verify persistence.

Changing the dictionary/frequencies changes these expected results. Use an untouched
starter file for a repeatable demonstration. For large-data timings, use the supplied
10,000 synthetic entries and clearly label them as synthetic.

## A short project explanation

“TrieWise stores words in a Trie so common prefixes share paths. Prefix search finds
the prefix node and explores the most promising branches using a best-first queue. Each branch
remembers its best word. Fuzzy candidates use a bounded heap to retain five results. For misspellings, it carries Levenshtein DP rows down the
Trie and prunes branches whose minimum row value exceeds the edit limit. I compare
its output and timings with an independent dictionary scan.”

## Likely questions

**Why a Trie instead of scanning a vector?**
It can narrow prefix search to one subtree, and shared prefixes reuse fuzzy DP work.
Small dictionaries and some fuzzy queries can still favor a vector scan.

**Is autocomplete O(P)?**
Only reaching the prefix node. Producing and ranking suggestions also expands promising descendant branches and manages the priority queue.

**Why not sort every candidate?**
A bounded heap processes C candidates in O(C log K), rather than sorting C entries
in O(C log C). The final K entries are sorted for display.

**What is stored at the heap root?**
The worst of the currently retained candidates. A better incoming candidate can
replace it without comparing against all K entries.

**Why is the DP row minimum a pruning criterion?**
It is a lower bound on descendant completion cost. If every possible partial
alignment is already over the allowed edit budget, descendants cannot succeed.

**Does it correct swapped letters in one edit?**
No. Standard Levenshtein has insert, delete and substitute. A transposition such as
netwrok -> network costs two. Damerau-Levenshtein would add a transposition operation.

**Why rank distance before frequency in fuzzy search?**
A more accurate correction should outrank a popular but less similar word. Equal
distances use frequency, then alphabetical order for deterministic results.

**Why store nodes in a vector with child indices?**
It gives automatic ownership and avoids leaks and invalidated child pointers when
the vector grows. Fixed child arrays provide direct alphabet indexing.

**What does the hash table do?**
It counts successful search commands, including valid searches with no matches.
It is not essential for frequency storage and is not a result cache.

**Are the frequencies real usage statistics?**
No. The bundled frequencies are clearly labeled demonstration weights. Selecting a
word increments its local weight. Corpus-derived counts could be loaded later.

**How was correctness checked?**
Known examples, malformed inputs, a full-matrix distance oracle, exhaustive small
strings, randomized dictionary parity, file round trips and CLI integration tests.
The benchmark also checks both search implementations return identical results.

**What are the limitations?**
English letters only, 64-character words, no graphical live dropdown, no semantic
search, no fuzzy-prefix search, session-only history and no crash-durability or multi-writer guarantees.
The performance reports describe this environment and datasets, not guaranteed speed.

**What was improved during review?**
Prefix search now uses best-word subtree summaries instead of scanning every
matching descendant. DP uses reusable scratch storage, the scan baseline is stronger,
saves replace a completed temporary file, and the simple menu offers to save before
exit. Technical commands remain available for a DSA demonstration.

**What could be extended next?**
Unicode-aware text handling, corpus-derived frequencies, compressed Trie edges,
richer per-node Top-K caches, or a thin UI that calls the existing engine. Each changes
complexity or memory tradeoffs and should follow a demonstrated requirement.

## Submission checklist

- Compile and run the tests on your laptop.
- Rehearse the Trie insertion, heap comparator and DP row calculation.
- Keep the starter dataset unchanged; save your demo changes to another file.
- Use your own measured timings if discussing your laptop's performance.
- Include source, README, tests and the dataset; confirm the course's STL rules.
- Acknowledge AI assistance as required by your institution.
