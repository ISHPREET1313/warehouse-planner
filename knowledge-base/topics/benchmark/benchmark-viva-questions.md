---
id: 20261119-benchmark-viva-questions
title: Benchmark explain-back and viva questions
tags: [benchmark, viva]
updated: 2026-11-19
---

# Explain it back (5 lines)
1. One table compares every algorithm pair on the same input.
2. Counters (comparisons, relaxations, heap operations, bytes) are exact and repeatable; time is an average and only indicative.
3. Heap Dijkstra beats the scan by orders of magnitude on big maps; Bellman-Ford is slowest but handles negative weights.
4. Greedy knapsack is fast but can be wrong; DP is exact at n times C cost.
5. Held-Karp doubles-plus per added stop, so it stops at 15; nearest neighbour scales but can be about 40 percent off.

# Likely questions
- **Why count operations, not just time?** Time depends on the machine and clock() is about 1 ms; counters are the same everywhere and match the formulas.
- **Why do scan and heap return the same distances?** Same algorithm, different way to find the minimum; the table's sumdist column checks this on every file.
- **Why is the heap's advantage bigger on bigger maps?** Scan is V squared, heap is E log V; the gap grows with V.
- **Can the heuristic beat the exact method?** Never on cost for one robot's route; at n = 15 they tie. Across the whole plan, tie-breaking in the knapsack can make the exact side cost more (30x40 case).
- **How would you improve the knapsack tie-break?** Prefer the lighter route among equal-value loads, or add a small distance penalty.
