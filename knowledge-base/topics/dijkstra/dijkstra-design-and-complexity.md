---
id: 20261119-dijkstra-design-and-complexity
title: Dijkstra with heap design and complexity
tags: [dijkstra, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-heap-design-and-complexity, 20261119-graph-design-and-complexity, 20261119-operation-counters]
summary: How src/dijkstra.c works (lazy deletion), what the counters mean for it, best/average/worst time and space, and a hand-worked example.
---

# Dijkstra with heap design and complexity

> Summary: How src/dijkstra.c works (lazy deletion), what the counters mean for it, best/average/worst time and space, and a hand-worked example.

## Context
Item 5, the core algorithm. Input is a `Graph` (CSR arcs, item 3) and a source node. Output is `ShortestPaths`: `dist[v]` (or `DIST_INF`), `prev[v]` (or -1), and `sp_path` to rebuild a route. One run per robot start. No I/O in the algorithm; `main.c` prints.

## Details
Loop (see the header comment in `src/dijkstra.h`):
1. `dist[source] = 0`, all others `DIST_INF`, `prev` all -1; push `(0, source)`.
2. Pop the smallest pair `(d, u)`. If `d > dist[u]` it is stale (u was improved after this pair was pushed): skip.
3. Otherwise relax each arc `u -> v` of weight `w`: if `dist[u] + w < dist[v]`, update `dist[v]`, `prev[v] = u`, push `(dist[v], v)`.
4. Stop when the heap is empty.

Why it is correct: the popped pair has the smallest distance of all unfinished nodes, and every weight is >= 0, so no later path can reach `u` more cheaply. Negative weights break this, so they are rejected (weights must be 0..1000).

Counters: one relaxation per arc tested; one comparison per pop for the stale check, plus the heap's own comparisons; pushes and pops come from the heap; `dist`, `prev` and the heap array go to `aux_bytes`.
Rules that hold for any graph (tested on 200 random graphs): pops == pushes; pushes <= arcs + 1; relaxations == number of arcs that start at a reachable node (each reachable node is expanded once).

Complexity (V nodes, E arcs; on the grid E <= 4V):
| | Time | Space |
|---|---|---|
| Worst | O((V + E) log V) | O(V + E) |
| Average | O((V + E) log V) | O(V + E) |
| Best | Theta(V + E) (source boxed in: arrays are still filled, weights checked) | O(V + E) |

Space: `dist` and `prev` are V ints each; the heap holds at most E + 1 pairs because there is one push per successful relaxation plus the source. With the array-scan version (item 6) time would be O(V^2) but space only O(V).

Hand example (6-node graph in `tests/test_dijkstra.c`): from node 0, the direct arc 0->1 costs 4 but 0->2->1 costs 3. Pairs pushed: (0,0), (4,1), (1,2), (3,1), (6,3), (4,3), (7,4) = 7 pushes, 7 pops (two are stale), 6 relaxations, result dist = 0,3,1,4,7,INF.

On `data/small/sample1.txt` with unit weights no node is pushed twice, so a run from one robot gives 38 pushes, 38 pops and 94 relaxations.

## Related
- [Min-heap design and complexity](../heap/heap-design-and-complexity.md) - the priority queue used here.
- [Grid to graph design and complexity](../graph/graph-design-and-complexity.md) - where the Graph comes from.
- [Operation counters](../metrics/operation-counters.md) - meaning of each counter.
