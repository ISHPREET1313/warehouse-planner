---
id: 20261119-tsp-design-and-complexity
title: Pickup order (TSP) design and complexity
tags: [tsp, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-tsp-viva-questions, 20261119-knapsack-design-and-complexity, 20261119-dijkstra-design-and-complexity]
summary: In what order one robot visits its stops: nearest neighbour heuristic vs exact Held-Karp dynamic programming, with counters and a worked example.
---

# Pickup order (TSP) design and complexity

> Summary: In what order one robot visits its stops: nearest neighbour heuristic vs exact Held-Karp dynamic programming, with counters and a worked example.

## Context
Item 10. After a robot has its tasks, it must decide the visiting order. The distances between the robot's start and each stop come from Dijkstra on the map. The route is an open path: start at the robot, visit every stop once, do not come back. Code: `src/tsp.[ch]`, tests: `tests/test_tsp.c`.

## Details
Input: stops 1..n, the robot start as stop 0, and an (n+1) x (n+1) matrix of costs (0..1,000,000, may be asymmetric). Output: the order and its total cost.

Nearest neighbour (`tsp_nearest`, heuristic, greedy)
- From the current stop always go to the closest unvisited stop (ties: lowest number).
- Time Theta(n^2) in every case, space O(n). Up to 256 stops. Counter: one comparison per candidate, n(n+1)/2 in total.
- Can be far from optimal. Line example: start 0, stops at +1, -2, +5. Nearest neighbour goes 1, 2, 3 for cost 1 + 3 + 7 = 11; the best order is 2, 1, 3 for 2 + 3 + 4 = 9.

Held-Karp (`tsp_held_karp`, exact, dynamic programming over subsets)
- `dp[mask][j]` = cheapest way to start at 0, visit exactly the stops in `mask` and end at stop `j+1`. Start: `dp[{j}][j] = dist[0][j+1]`. Step: `dp[mask + k][k] = min over j in mask of dp[mask][j] + dist[j+1][k+1]`.
- The answer is the smallest `dp[all][j]`; a parent table gives the order.
- Time Theta(n^2 * 2^n) in every case, space Theta(n * 2^n). Up to 15 stops (about 2.4 MB). Counter: one comparison per transition, n(n-1)2^(n-2), plus n for the final minimum. For n = 3: 12 + 3 = 15.
- Why not try all orders? n! orders: 12 stops is 479 million orders, while Held-Karp does about 135 thousand transitions.
- TSP is NP-hard; Held-Karp is exponential but far better than n!.

Hand worked example (n = 3, distances 0:[0 1 2 5] 1:[1 0 3 4] 2:[2 3 0 7] 3:[5 4 7 0]): the table ends with dp[all][stop 3] = 9, read back through the parents to the order 2, 1, 3.

Known optimum test: stops all to the right of the start on a line: the cheapest path walks right once, cost = farthest position (95 for the generated 12 and 15-stop cases).

## Related
- [TSP viva questions](tsp-viva-questions.md) — see-also: defence prep.
- [Knapsack design and complexity](../knapsack/knapsack-design-and-complexity.md) — see-also: which tasks are loaded first.
- [Dijkstra design and complexity](../dijkstra/dijkstra-design-and-complexity.md) — prerequisite: gives the distance matrix.
