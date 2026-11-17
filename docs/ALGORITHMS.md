# TrieWise architecture and algorithms

## 1. Data model

A Trie node has 26 child indices and a terminal-entry index. `absent` denotes a
missing child/terminal. Nodes are stored in a flat `std::vector`; indices stay
valid if the vector reallocates. Words and frequencies live in a separate entry
vector. Only complete words have terminal entries, so a prefix is not automatically
a dictionary member. No raw owning pointers or recursive destruction are needed.

A parallel ranking array stores each subtree's word count and best terminal word.
It is separate from character nodes so fuzzy traversal does not increase its node stride.
Insertions and frequency changes recompute summaries from the changed word to the root,
including frequency decreases and alphabetical ties.

Query history uses `std::unordered_map<string,uint64_t>`. It records usage, not cached
results. This avoids stale cached rankings when frequencies change.

Example dictionary: `car`, `cat`, `cart`. The first two characters share the path
root -> c -> a, then branches r/t. The `r` node marks `car`; its child `t` marks `cart`.

## 2. Prefix autocomplete

1. Normalize the prefix and follow its child indices from the root.
2. Read the subtree's word count and best-word summary.
3. Put that subtree into a best-first priority queue. Its best word is an exact
   bound on the highest-ranked word that can still come from that subtree.
4. Pop the most promising item. Expand subtree items into their terminal word and
   child subtrees; return terminal-word items as suggestions.
5. Stop as soon as K complete words have been returned.

Queue subtrees are disjoint; each terminal is produced once. Equal keys may pop
in either order, but the final word ordering is deterministic. A subtree's own
terminal may also be its bound, so it must be considered separately from descendants.
The rank summaries let short prefixes skip many branches. The queue is NOT bounded
by K, unlike the fuzzy candidate heap. Its worst-case memory can grow with the
matching subtree. This trades extra memory and update work for faster prefix queries.
An empty prefix (`prefix *`) uses the same algorithm from the root.

## 3. Ranked Top-K with a heap

A better candidate has:

1. Smaller edit distance.
2. Higher frequency when distance ties.
3. Alphabetically earlier word when both tie.

The fuzzy candidate heap uses the ordering above. Prefix candidates have distance
zero and use the separate best-first frontier described above. The bounded fuzzy queue's comparator places
the **worst retained candidate at the root**. Add candidates until size K, then
replace the root only when the new candidate is better. Heap size never exceeds K.
C++ `priority_queue` is max-oriented by default; the custom `Better` comparator
makes the root the worst according to our ranking. This is a bounded worst-first
heap, equivalent to a min-heap of relevance scores.

## 4. Levenshtein distance

Allowed edits: insertion, deletion, substitution. Each costs one. Adjacent
transposition is not a special operation: `ab -> ba` costs two, not one.

For dictionary path s and query q:

```text
D[i][0] = i
D[0][j] = j
D[i][j] = min(
    D[i-1][j] + 1,
    D[i][j-1] + 1,
    D[i-1][j-1] + (s[i-1] != q[j-1])
)
```

`computr -> computer` costs 1 (insert e).
`computr -> computing` costs 3 (substitute r -> i, insert n, insert g).

`edit_distance()` is a rolling-row reference helper. The final linear scan reuses
its DP buffers across words, filters impossible lengths and stops over-budget rows.
Tests implement a separate full matrix and compare answers exhaustively.

## 5. Trie-integrated DP and pruning

The root row is `[0, 1, ..., query_length]`. Each outgoing character computes one
new row using its parent's row. Shared dictionary prefixes therefore share DP work.
A terminal word is accepted when the row's final cell is at most the edit limit.

If the minimum value in a row exceeds the limit, descendants cannot produce an
accepted full word. The minimum is a lower bound on all descendant full-word edit
costs: any alignment must pass through the current path prefix, and further edits
cannot remove costs already incurred. Thus the branch may be pruned safely.

The recursive depth is bounded by 64, the enforced word-length maximum.
DP scratch arrays live on the stack, avoiding a heap allocation per visited node. Each active
level owns a fixed array of 65 cells (only the active query-length portion is read); the code does not share mutable rows between sibling branches.
`branches_pruned` counts cutoff roots, not the total number of nodes avoided.
`nodes_visited` counts inspected nodes/lookup steps, not distinct nodes across all
operations. Fuzzy search excludes the root because it has no character DP row.

## 6. Complexity

Let P = prefix length, Q = query length, H = maximum dictionary word length,
T = number of Trie nodes, V = nodes visited below the prefix or during fuzzy search,
C = valid candidate count, E = prefix subtrees expanded, F = maximum prefix frontier size,
N = dictionary word count, K = requested result count,
and S = total characters across all dictionary words. Alphabet size is fixed at 26.

| Operation | Time (ignoring bounded string-copy/comparison costs) | Auxiliary space |
| --- | --- | --- |
| Insert / replace a word of length L | O(26L), amortized growth and bounded comparisons | O(L) path and new nodes |
| Exact membership | O(L) | O(1), excluding normalized string |
| Prefix node lookup | O(P) | O(1), excluding normalized string |
| Full autocomplete | O(P + (26E + K) log(max(2,F))) | O(F + K), plus strings |
| Rolling-row distance for one word length L | O(LQ) | O(Q) |
| Trie fuzzy search | O(VQ + C log K + K log K) | O(65H + K), plus strings |
| Linear fuzzy baseline | O(SQ + C log K + K log K) | O(Q + K), plus strings |
| Linear prefix baseline | O(NP + C log K + K log K), upper bound | O(K), plus strings |
| Hash history access | Expected O(1) table probes; O(N) worst collision case | O(distinct recorded queries) |

For K=0, searches return immediately after validation. Use log(max(2,K)) when
interpreting the displayed formulas for K=1. Hashing a string also costs its length.
String copying and alphabetical tie comparisons can add factors of up to H.
Fixed 26-way nodes use O(26T) child storage; branch summaries add O(T); entry strings require O(S). Since
T <= 1+S, the Trie is linear in total dictionary characters for a fixed alphabet.
It can consume more memory than a plain string array because of child slots.

## 7. Semantics and persistence

`search` is a convenience command: complete a prefix if any prefix matches exist,
otherwise correct the whole word within two edits. It does not combine both result
sets and it does not support typo-tolerant prefix matching.

`add` replaces a duplicate's frequency. `select` increments an existing word and
saturates at the maximum. `load` merges valid records and reports invalid rows;
last valid duplicate wins. `save` writes and closes a sorted snapshot in a private sibling directory, then
replaces the destination using POSIX rename or Windows MoveFileExW. A failed write
does not truncate the destination. Temporary files are cleaned up on ordinary
failures. This is not a power-loss-proof database transaction; no fsync-based
durability or concurrent-writer locking is implemented. History is session-only.

## 8. Why this is a DSA project

The Trie, traversal, DP recurrence, pruning, ranking comparator and bounded-heap
logic are implemented explicitly. Standard containers provide memory management
and priority-queue primitives; there is no external fuzzy-search implementation.
If your instructor requires a manually implemented heap, confirm that requirement
before submission; this version uses `std::priority_queue`.
