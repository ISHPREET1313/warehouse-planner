---
id: 20261119-comparison-design-and-complexity
title: Dijkstra array scan and Bellman-Ford design and complexity
tags: [comparison, bellman-ford, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-dijkstra-design-and-complexity, 20261119-operation-counters]
summary: How dijkstra_scan and bellman_ford work, what their counters mean, best/average/worst time and space, and when each beats Dijkstra with a heap.
---

# Dijkstra array scan and Bellman-Ford design and complexity

> Summary: How dijkstra_scan and bellman_ford work, what their counters mean, best/average/worst time and space, and when each beats Dijkstra with a heap.

## Context
Item 6. Both use the same `Graph` and return the same `ShortestPaths` as the heap version, so the three can be swapped and compared fairly. Used by `planner` (per robot) and later by the benchmarks (item 9).

## Details
**Dijkstra with array scan** (`dijkstra_scan` in `src/dijkstra.c`): same relax step, but no heap. Each round it scans all unfinished nodes for the smallest known distance, marks it done and relaxes its arcs. A `done` flag per node replaces the heap.
- Counters: one comparison per unfinished node examined in a scan; one relaxation per arc tested; no pushes or pops. With R reachable nodes out of n: comparisons = R*n - R*(R-1)/2 + (n - R). Sample1 from one robot: 1131. Open 30x30: 405450.
- Time: Theta(V*R + E); O(V^2 + E) worst and average; best O(V + E) for a boxed-in source (R is 1). Space O(V) (dist, prev, done).

**Bellman-Ford** (`src/bellman_ford.c`): repeat passes over every arc and relax it. After pass k all nodes whose shortest path has at most k arcs are right, so n-1 passes are enough; if pass n still changes something there is a reachable negative cycle. Stops early when a pass changes nothing. Allows negative weights.
- Counters: one relaxation per arc whose start node is reached; no comparisons, no heap.
- Time: worst and average O(V*E); best O(E) (one improving pass and one confirming pass, or nothing reachable). Space O(V).
- On the open 60x60 grid from the top-left corner, node order matches the direction of travel, so it needs only 2 passes (28320 relaxations); from the opposite corner it needs many passes.

Which to use: heap Dijkstra for sparse graphs with non-negative weights (our grids, E about 4V): O((V+E) log V). Scan is competitive only for dense graphs (E close to V^2), where the heap adds overhead for no gain. Bellman-Ford only when negative weights can occur or a negative cycle must be detected.

## Related
- [Dijkstra with heap design and complexity](../dijkstra/dijkstra-design-and-complexity.md) - the baseline.
- [Operation counters](../metrics/operation-counters.md) - meaning of each counter.
