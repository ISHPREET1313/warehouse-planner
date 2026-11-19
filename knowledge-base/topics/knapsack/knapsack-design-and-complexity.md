---
id: 20261119-knapsack-design-and-complexity
title: 0/1 knapsack design and complexity
tags: [knapsack, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-knapsack-viva-questions, 20261119-tsp-design-and-complexity]
summary: How a robot chooses which tasks to load: exact dynamic programming vs greedy by value/weight, with a case where greedy fails.
---

# 0/1 knapsack design and complexity

> Summary: How a robot chooses which tasks to load: exact dynamic programming vs greedy by value/weight, with a case where greedy fails.

## Context
Item 9. A robot has a capacity; each candidate task has a weight and a value. Each task is taken whole or not at all (0/1). Goal: the largest total value that fits. Code: `src/knapsack.[ch]`, tests: `tests/test_knapsack.c`.

## Details
Dynamic programming (`knapsack_dp`, exact, paradigm: DP with optimal substructure)
- `best[w]` = best value using weight at most `w`. For each item `i`, for `w` from capacity DOWN to `weight[i]`: if `best[w - weight[i]] + value[i] > best[w]`, update. Going downwards means item `i` is used at most once per row.
- A flag table `take[i][w]` records where item `i` improved `best[w]`; the chosen items are read back from the last item to the first.
- Effective capacity is min(capacity, total weight). Limits: 256 items, effective capacity at most 20000.
- Time Theta(n * C) in every case (the loops never stop early). Space Theta(n * C) bytes of flags plus O(C) ints. This is pseudo-polynomial: it depends on the number C, not on its digit count; the problem itself is NP-hard.

Greedy (`knapsack_greedy`, heuristic, paradigm: greedy)
- Sort items by value/weight, best first (insertion sort, ratios compared by cross-multiplication so there is no division), then take every item that still fits.
- Time O(n^2) worst (reverse-sorted input), Theta(n) best (already sorted), space O(n). With merge sort it would be O(n log n).
- Not always optimal. Capacity 50, items (w10,v60) (w20,v100) (w30,v120): ratios 6, 5, 4. Greedy takes items 1 and 2 (weight 30, value 160) and item 3 does not fit. The best is items 2 and 3 (weight 50, value 220).
- Second case: capacity 10, items (w1,v2) and (w10,v19). Greedy takes the light item (ratio 2.0 beats 1.9) and then the heavy one no longer fits: 2 instead of 19.

Counters
- DP: one comparison per table cell, so sum over items of (C - weight + 1). Classic case: 41 + 31 + 21 = 93. aux_bytes = best row + flag table = 204 + 153 = 357.
- Greedy: one comparison per ratio comparison in the sort (2 on the sorted classic case), aux_bytes = the index array.

Why it matters here: it decides which tasks one robot loads; the DP is exact, the greedy is the cheap alternative the planner can compare against.

## Related
- [Knapsack viva questions](knapsack-viva-questions.md) — see-also: defence prep.
- [TSP design and complexity](../tsp/tsp-design-and-complexity.md) — see-also: the next decision, in what order to visit the chosen tasks.
