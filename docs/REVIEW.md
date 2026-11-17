# Review of TrieWise release 1.2

## Assessment

The project meets the defined C++ console mini-project scope. There is reproducible
correctness evidence for prefix lookup, ranked completion, whole-word fuzzy search,
frequency updates and persistence. It demonstrates a Trie, best-first search,
bounded-heap ranking, dynamic programming and a hash table. It is not a production
search service, and no grade or universal speed/accuracy score is claimed.

## Usability review on 18 October 2026

The previous CLI was complete but required users to remember commands and exposed
technical counters during ordinary searches. Release 1.2 adds a numbered menu with
plain results. Advanced commands remain available through More options or
`--commands` for demonstrations and scripts. The search algorithms are unchanged.

New usability safeguards include preserving a duplicate word's weight when using
Add, validating result selections, Save/Discard/Cancel on exit, retaining changes
after a failed save, and automatically reopening the default saved dictionary.
The active file is displayed at startup. Alternate save filenames still need an
explicit path on the next launch; this avoids adding a hidden settings file.
The Windows launcher builds on first use, detects the default MSYS2 compiler, and
keeps startup errors visible. Actual Windows execution still needs verification.

The updated build passes 27,771 engine assertions and 44 CLI integration checks.
The CLI also passes address/undefined-behaviour instrumentation and strict warning
compilation. Automated input tests establish behavior, not measured user satisfaction;
no user study or numerical usability score is claimed.

## Improvements implemented

- Prefix search previously enumerated the entire matching subtree. Each node now
  has a parallel summary of its best word and word count. A best-first queue expands
  promising branches and stops after K complete words. The queue is not bounded by K.
- Ancestor summaries refresh after additions, selections and frequency replacement,
  including decreases. Randomized mutation/parity tests cover stale-bound errors.
- Fuzzy search no longer allocates a vector at every visited node. The depth-bounded
  traversal owns fixed stack scratch arrays and reads only initialized active cells.
- The scan baseline was strengthened: reusable row buffers, impossible-length skips
  and over-budget early stopping. It is intentionally a credible competitor.
- Saving writes a completed, closed temporary snapshot before replacing the target.
  POSIX uses rename; Windows uses MoveFileExW. Ordinary failures clean up temporary
  files and avoid direct destination truncation. This is not guaranteed durable
  storage against power loss or coordinated concurrent writers.
- The CLI offers to save before exiting the menu, rejects extra help arguments and looks for
  a default dictionary near the executable when it is absent in the current folder.
- The README and start guide now present Windows setup first.

## Before and after

`review_comparison100000.txt` records the final implementation and original code
loaded together, with 31 alternating timing repetitions, fixed queries, K=5 and
D=2. Result parity was checked for all 11 queries. Key observations:

- Prefix `a`: median 645.071 us -> 6.930 us; node visits 23,021 -> 50.
- Prefix `co`: median 19.809 us -> 2.624 us; visits 1,012 -> 41.
- Fuzzy `computr`: median 4,366.23 us -> 2,822.00 us; same 40,001 node visits.
- Dictionary load in that run: 99.1456 ms -> 146.478 ms (one observation each).
- Capacity estimate: 120,089,088 -> 128,477,696 bytes, about 7% higher.

The load and storage increases are costs of ranking summaries. The memory estimate
includes conservative string buffers and excludes allocator overhead and history;
it is not RSS. The comparison executable was built with GCC 13.3.0, C++17, -O2.
Raw historical benchmark output is retained separately and labeled as the original
release; it used a weaker scan baseline and must not be mistaken for final timings.

## Fairness checks and losses

The release benchmarks compare matching semantics, dictionaries, thresholds, K and
ranking. Output/candidate parity is checked before timing, both methods are warmed,
and timing order alternates. The scan has also been improved rather than left
needlessly slow. Prefix preprocessing costs and memory tradeoffs are disclosed.

For 10,000 synthetic entries, the final fuzzy scan was faster for four of the five
benchmark queries: computr, netwrok, securty and zzzz. For example, netwrok measured
404.67 us for the Trie versus 332.72 us for the scan. At 100,000 entries, the final
Trie was faster for all listed benchmark queries. Results depend on data, caching,
compiler, run noise and query distribution. They do not establish universal wins.

The synthetic datasets contain random letter strings, not natural-language usage.
The 536-term curated dictionary uses invented weights. No claim about real-world
relevance, multilingual correctness or typo-detection accuracy follows from these
benchmarks.

## Limits kept in scope

- English letters only, 64-character words, edit limit 0-3, K at most 100.
- Menu inputs and advanced commands run after Enter; no live graphical dropdown.
- Whole-word fuzzy search, not semantic search or typo-tolerant prefix completion.
- Ordinary Levenshtein distance: a transposition requires two edits.
- Session-only, unbounded query history; no eviction policy or persistent analytics.
- No synchronization for concurrent readers/writers or database-grade durability.
- Fixed 26-child nodes can consume substantially more memory than compact string storage.
- Windows source/build scripts supplied; native Windows execution unavailable here.
- Standard priority_queue used; a custom heap may be required by a particular rubric.

These are disclosed limitations, not features claimed to have been completed.
Useful next work is native Windows validation and a verified executable package,
followed by a real licensed frequency corpus and relevance evaluation. A live UI
can be added if the presentation requires it. Unicode and compressed Tries
are separate changes requiring new tests, not safe cosmetic additions.
