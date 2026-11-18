---
id: 20261119-graph-design-and-complexity
title: Grid to graph design and complexity
tags: [graph, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-parser-design-and-complexity, 20261119-graph-viva-questions]
summary: How the grid becomes a compact adjacency-list graph, the BFS reachability check, and their time and space.
---

# Grid to graph design and complexity

> Summary: How the grid becomes a compact adjacency-list graph, the BFS reachability check, and their time and space.

## Context
Item 3. Every later algorithm (heap Dijkstra, array Dijkstra, Bellman-Ford) searches this graph, so it is built once, read-only, and has no I/O. Code: `src/graph.[ch]`; tests: `tests/test_graph.c`.

## Details
Nodes and walkable cells
- Node id = `row * cols + col`, so every cell is a node and the id converts back to a position with `/` and `%`.
- Walkable symbols: `. C D R T`. Not walkable: `#` obstacle and `S` shelf. Blocked cells stay as nodes with no arcs (isolated), so ids never need renumbering.
- Tasks (`T`) are walkable: the robot stands on the pickup cell.

Arcs
- 4-neighbour moves (up, down, left, right). Each edge is stored as two arcs, one per direction, cost 1. The weight is stored per arc so Bellman-Ford can use other weights later.

Storage (CSR, compact adjacency lists)
- `first[n+1]`, `to[n_arcs]`, `weight[n_arcs]`. The arcs leaving node `u` are `to[first[u]] .. to[first[u+1]-1]`; degree is `first[u+1]-first[u]`.
- Built in two passes: pass 1 counts the arcs leaving each node, a prefix sum turns the counts into start positions, pass 2 writes each arc into its slot using a temporary `fill` array.

Reachability
- Multi-source BFS: every robot start goes into the queue first, then the queue is expanded once. A task is unreachable if its node was never seen. This is the check the parser deferred (item 2).

Complexity (V = rows*cols, E = arcs, E <= 4V)
- `graph_build`: time Theta(V) in every case (best = worst, it always scans every cell twice); space O(V + E) for the graph plus O(V) temporary.
- `graph_unreachable_tasks`: time O(V + E) (each node enters the queue at most once, each arc is looked at once); auxiliary space O(V) for `seen` and the queue.
- Largest legal map, 500 x 500 all open: V = 250000, E = 998000 arcs.

Weak spots
- The weights are all 1, so plain BFS would also give shortest paths; Dijkstra is still the project's tool because later weights can differ.
- Memory for the largest map: about 4 MB for `to` plus 4 MB for `weight` plus 1 MB for `first`.

## Related
- [Parser design and complexity](../parser/parser-design-and-complexity.md) — prerequisite: produces the Problem.
- [Graph viva questions](graph-viva-questions.md) — see-also: defence prep.
