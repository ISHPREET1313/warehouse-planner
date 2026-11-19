---
id: 20261119-build-order
title: Build order and progress
tags: [project, process]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-project-overview, 20261119-working-rules]
summary: The demo core (items 1 to 14) is built and verified; only the short item 23 remains; items 8 and 15 to 22 are dropped.
---

# Build order and progress

> Summary: The demo core (items 1 to 14) is built and verified; only the short item 23 remains; items 8 and 15 to 22 are dropped.

## Context
Evaluation is Saturday 21 November 2026 (solo, 8-minute demo + 4-minute viva). The scope is the demo core: the admin asked the teachers and was told to implement only what is required (docs/QUESTIONS.md Q25). `PROMPT.md` section 4 holds the stable plan; `docs/TODO.md` and `docs/STATE.md` hold the live status.

## Details
DEMO CORE, all verified by running (1045 tests passing)
1. Skeleton, Makefile, harness, metrics - DONE
2. Parser and validator - DONE
3. Grid to graph - DONE
4. Min-heap - DONE
5. Dijkstra with heap - DONE
6. Dijkstra array scan and Bellman-Ford - DONE
7. Test suite and data files - DONE
9. 0/1 knapsack, DP vs greedy - DONE
10. Pickup order, nearest neighbour vs Held-Karp - DONE
11. Planner pipeline (assignment, loads, routes, battery) - DONE
12. Three scenarios (normal, constraint shock, large stress) - DONE (data files + both strategy sets printed)
13. CLI menu and CSV export - DONE
14. Benchmark table (`planner --bench`) - DONE
23 (short). Demo script, viva notes, README - NEXT, then tag v1-core.

DROPPED (teachers: implement only what is required): item 8 (sorting and search), items 15 to 22 (the remaining syllabus algorithms from the brief, library packaging, full report). Reopen only if a teacher names a missing algorithm.

## Related
- [Project overview](project-overview.md) — prerequisite: what is being built.
- [Working rules](working-rules.md) — see-also: how Claude and the admin work.
