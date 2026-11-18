---
id: 20261119-graph-viva-questions
title: Graph explain-back and viva questions
tags: [graph, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-graph-design-and-complexity]
summary: A 5-line explain-back of the grid-to-graph step and likely evaluator questions with short answers.
---

# Graph explain-back and viva questions

> Summary: A 5-line explain-back of the grid-to-graph step and likely evaluator questions with short answers.

## Context
Required after every module so the student can defend the code.

## Details
Explain it back (5 lines):
1. Every grid cell is a node with id `row*cols + col`; `#` and `S` cells are nodes with no arcs.
2. Each pair of walkable 4-neighbours gets two arcs (one each way) of cost 1.
3. Arcs are stored compactly: `first[]` marks where each node's arcs start in `to[]` and `weight[]`.
4. Pass 1 counts arcs per node, a prefix sum gives start positions, pass 2 fills the slots.
5. A BFS from all robots at once finds tasks nobody can reach, in time O(V + E).

Evaluator questions:
- Q: Why not an adjacency matrix? A: V can be 250000, so a V x V matrix would need 62.5 billion entries; the compact lists need about E (at most 4V).
- Q: Why keep `#` and `S` cells as nodes instead of removing them? A: Node id = row*cols + col stays valid for every cell, so printing paths and mapping back to the grid needs no lookup table.
- Q: Why two passes? A: We must know each node's degree to place its arcs contiguously without reallocating; pass 1 counts, pass 2 writes.
- Q: Why BFS and not Dijkstra for reachability? A: Reachability ignores cost. BFS is O(V + E) and needs no heap.
- Q: Why start the BFS from all robots together? A: One sweep answers "can any robot reach this task?" instead of one BFS per robot.
- Q: Can a task be reachable but the robot still fail it? A: Yes. Capacity and battery are checked by later modules; this check only says a path exists.

## Related
- [Graph design and complexity](graph-design-and-complexity.md) — prerequisite.
