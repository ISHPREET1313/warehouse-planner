---
id: 20261119-build-order
title: Build order and progress
tags: [project, process]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-project-overview, 20261119-working-rules]
summary: The 18 build items in tier order, with which are done, and the finish-before-next rule.
---

# Build order and progress

> Summary: The 18 build items in tier order, with which are done, and the finish-before-next rule.

## Context
Each item is finished completely (code, tests, complexity note, explain-back) before the next starts. `docs/TODO.md` and `docs/STATE.md` hold the live status; this note is the stable outline.

## Details
PRIMARY
1. Skeleton, Makefile, harness, metrics - DONE (user verified, 12/12 tests).
2. Parser and validator - DONE (user verified, 139/139 tests).
3. Grid to graph conversion - DONE (user verified, 231/231 tests).
4. Min-heap (Claude writes it since the 2026-11-19 authorship change) - CODE WRITTEN, waiting for the user's build/test output.
5. Dijkstra with heap (Claude writes it too) - CODE WRITTEN and wired in, waiting for the same build/test output.
6. Dijkstra with array scan, and Bellman-Ford, as comparisons.
7. Test suite: normal, boundary, invalid, stress, adversarial.
8. CLI menu and ASCII path replay.
9. Benchmarks and CSV output.

IMPORTANT: 10 knapsack DP vs greedy, 11 nearest neighbour vs Held-Karp, 12 merge sort vs quick sort, 13 seeded generator.

SECONDARY: 14 bipartite matching, 15 KMP vs naive, 16 Ford-Fulkerson, 17 extra generator profiles, 18 raylib viewer. Teach the idea first for 14 to 16.

## Related
- [Project overview](project-overview.md) — prerequisite: what is being built.
- [Working rules](working-rules.md) — see-also: who writes which module.
