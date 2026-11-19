---
id: 20261119-comparison-viva-questions
title: Scan and Bellman-Ford explain-back and viva questions
tags: [comparison, bellman-ford, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-comparison-design-and-complexity]
summary: A 5-line explain-back of the array-scan Dijkstra and Bellman-Ford and likely evaluator questions with short answers.
---

# Scan and Bellman-Ford explain-back and viva questions

> Summary: A 5-line explain-back of the array-scan Dijkstra and Bellman-Ford and likely evaluator questions with short answers.

## Context
Required after every module so the admin can defend the code. The evaluator will probably ask why three algorithms exist and when each is right.

## Details
Explain it back (5 lines):
1. The scan version is Dijkstra with the heap replaced by a loop that finds the smallest unfinished distance, O(V) per round, so O(V^2) overall.
2. Bellman-Ford ignores order: it relaxes every arc again and again, at most n-1 passes, because a shortest path has at most n-1 arcs.
3. If pass n still improves a distance, some negative cycle is reachable, so no shortest path exists; the code reports it.
4. All three return the same `ShortestPaths`, so tests compare their distances and the counters show the cost difference.
5. Heap Dijkstra wins on sparse grids, scan on dense graphs, Bellman-Ford when weights can be negative.

Evaluator questions:
- Q: Why is the scan version O(V^2) and the heap version O((V+E) log V)? A: The scan costs V per round for V rounds. The heap costs log V per push or pop, with at most E + 1 pushes. On a grid E is about 4V, so V log V beats V^2; on a dense graph E is about V^2, so V^2 log V is worse than V^2.
- Q: Why do n-1 passes suffice in Bellman-Ford? A: A shortest path with no repeated node has at most n-1 arcs, and pass k fixes every path of at most k arcs.
- Q: How does it detect a negative cycle? A: Without one, everything is final after n-1 passes, so pass n changes nothing. If pass n still improves a distance, some path keeps getting cheaper by going around a cycle with negative total weight.
- Q: Why does the scan count a comparison for unreachable nodes too? A: Every round it really examines every unfinished node, reachable or not. Counting only candidates would hide the quadratic cost.
- Q: Can you show Dijkstra failing? A: Arcs A->B 2, A->C 3, C->B -2: Dijkstra finalises B at 2, but A->C->B costs 1. Our Dijkstra rejects negative weights; Bellman-Ford returns the 1.
- Q: Does the early exit change the worst case? A: No. A graph that needs n-1 improving passes still costs O(V*E); the exit only helps easy inputs.

## Related
- [Dijkstra array scan and Bellman-Ford design and complexity](comparison-design-and-complexity.md) - prerequisite.
