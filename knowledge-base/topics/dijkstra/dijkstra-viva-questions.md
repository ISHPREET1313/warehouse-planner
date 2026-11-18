---
id: 20261119-dijkstra-viva-questions
title: Dijkstra explain-back and viva questions
tags: [dijkstra, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-dijkstra-design-and-complexity]
summary: A 5-line explain-back of Dijkstra with the heap and likely evaluator questions with short answers.
---

# Dijkstra explain-back and viva questions

> Summary: A 5-line explain-back of Dijkstra with the heap and likely evaluator questions with short answers.

## Context
Required after every module so the admin can defend the code. Dijkstra is the core algorithm, so expect the most questions here.

## Details
Explain it back (5 lines):
1. `dist[]` holds the best known cost to each node; the source is 0, all others infinity.
2. The heap always gives the unfinished node with the smallest known distance; that node's distance is now final.
3. For each arc out of it, relax: if `dist[u] + w < dist[v]`, store the shorter distance, remember `prev[v] = u`, push `(dist[v], v)`.
4. A node may be pushed more than once; a popped pair with `d > dist[u]` is stale and skipped (lazy deletion, no decrease-key).
5. `sp_path` walks `prev` from the target back to the source; cost is O((V + E) log V) time and O(V + E) space.

Evaluator questions:
- Q: Why does Dijkstra fail with negative weights? A: It treats a popped node as final. A later negative arc could make a path through another node cheaper, but that node is never revisited. Example: A->B 2, A->C 3, C->B -2: B is finalised at 2, but A->C->B costs 1. Use Bellman-Ford (item 6) for that.
- Q: Why is a stale pair harmless? A: A stale pair has `d > dist[u]`. The shorter pair for u was popped earlier (smaller key), u was expanded then, so skipping it loses nothing.
- Q: Why heap capacity arcs + 1? A: Pushes happen once for the source and once per successful relaxation. Each node is expanded once, so successful relaxations are at most the number of arcs.
- Q: Why not plain BFS, the grid has cost 1 everywhere? A: BFS is correct for unit weights and would be faster. Dijkstra is the general algorithm, it already handles other non-negative costs (zero weights are tested), and it is the base that item 6 compares against.
- Q: Compare with the array-scan version. A: Scan finds the smallest unfinished node in O(V), so O(V^2) total, no heap; better when the graph is dense, worse on sparse grids where E is about 4V.
- Q: How do you count the work? A: Relaxations (arcs tested), heap pushes and pops, comparisons; they are printed per robot by `planner`.

## Related
- [Dijkstra design and complexity](dijkstra-design-and-complexity.md) - prerequisite.
