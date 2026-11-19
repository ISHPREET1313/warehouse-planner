---
id: 20261119-tsp-viva-questions
title: Pickup order explain-back and viva questions
tags: [tsp, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-tsp-design-and-complexity]
summary: A 5-line explain-back of the pickup-order module and likely evaluator questions with short answers.
---

# Pickup order explain-back and viva questions

> Summary: A 5-line explain-back of the pickup-order module and likely evaluator questions with short answers.

## Context
Required after every module so the admin can defend the code.

## Details
Explain it back (5 lines):
1. One robot has several stops; the order changes the total travel cost, and the costs between stops come from Dijkstra.
2. Nearest neighbour always goes to the closest unvisited stop: Theta(n^2) and simple, but not always optimal.
3. Held-Karp stores `dp[mask][j]`: the cheapest way to visit the set `mask` and end at `j`.
4. It grows the sets one stop at a time and keeps a parent table so the order can be read back.
5. Time Theta(n^2 * 2^n) and space Theta(n * 2^n), far better than trying all n! orders.

Evaluator questions:
- Q: Why can nearest neighbour be wrong? A: A cheap first step can force an expensive later step (line example: 11 against the best 9).
- Q: What does the bitmask mean? A: Bit j is 1 if stop j+1 has been visited; `mask` is the set of visited stops.
- Q: Why is Held-Karp better than n!? A: It reuses the best cost for each (set, last stop) pair instead of recomputing all orders: 2^n * n states with n transitions each.
- Q: Why limit Held-Karp to 15 stops? A: Memory and time double with every extra stop: 15 stops need about 2.4 MB and 1.7 million transitions; 25 would need gigabytes.
- Q: Is TSP in P? A: No, it is NP-hard. Held-Karp is exponential; no polynomial exact algorithm is known.
- Q: What if the matrix is not symmetric? A: Both algorithms still work because they read dist[a][b] in the direction of travel (test case: 1 then 2 costs 11, 2 then 1 costs 101).
- Q: Why an open path? A: The robot does not need to return; a return trip would add the last stop's distance back to the start.

## Related
- [TSP design and complexity](tsp-design-and-complexity.md) — prerequisite.
