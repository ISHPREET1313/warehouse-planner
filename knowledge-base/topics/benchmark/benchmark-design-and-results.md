---
id: 20261119-benchmark-design-and-results
title: Benchmark design and results
tags: [benchmark, complexity, comparison]
updated: 2026-11-19
---

# What is measured (src/bench.c)
Same inputs, every pair of algorithms: Dijkstra heap vs scan vs Bellman-Ford (first robot of five data files), knapsack DP vs greedy (seeded random instances n = 10 to 256, plus the classic trap), pickup order nearest neighbour vs Held-Karp (seeded random stops, Manhattan distances, n = 6 to 200), planner A vs B on every data file. Columns: result, comparisons, relaxations, heap operations, auxiliary bytes, ms per run.
Counters are exact and repeatable; time is an average of repeated runs (about 30 ms, at most 2000 runs) because clock() has about 1 ms resolution, so times are indicative only. Saved run: docs/outputs/benchmark_table.txt.

# What the numbers show (from the saved run)
- **Shortest paths agree** (equal sumdist on all five files). On 62x80 (4960 cells): heap 24,352 comparisons, scan 9,613,240 (about 395 times more), Bellman-Ford 185,820 relaxations vs 7,824. Heap wins by growing like E log V against V squared; Bellman-Ford is the slowest in relaxations but is the only one that accepts negative weights.
- **Knapsack:** greedy is wrong on the classic case (160 vs 220) and slightly low on n = 50 (1984 vs 1991), but needs about 40 times fewer comparisons and about 680 times less memory at n = 256 (1,024 vs 700,180 bytes). DP is exact: comparisons about n times C.
- **Pickup order:** Held-Karp comparisons grow by about 2.3 to 2.4 times per extra stop (23,050 at n = 10, 745,486 at n = 14, 1,720,335 at n = 15), memory 2.4 MB at 15 stops. Nearest neighbour is 20,100 comparisons at n = 200 but costs up to 40 percent more route (n = 10: 462 vs 330). At n = 15 both found the same cost (367), so the heuristic can be optimal.
- **Planner:** A and B give equal value on small files; on 62x80 A serves 14 tasks (value 450) vs B 13 (426).
- **Caveat worth knowing:** on warehouse_30x40 both serve 6 tasks with value 243, yet A's route cost is 95 and B's 57. The exact knapsack maximises value and breaks ties arbitrarily, not by distance, so exact per robot is not globally best (see QUESTIONS Q30).
