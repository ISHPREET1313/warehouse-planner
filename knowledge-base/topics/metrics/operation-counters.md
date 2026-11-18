---
id: 20261119-operation-counters
title: Operation counters
tags: [metrics, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-test-harness]
summary: The fixed meaning of each counter (comparisons, relaxations, heap pushes and pops, aux bytes) and the timer.
---

# Operation counters

> Summary: The fixed meaning of each counter (comparisons, relaxations, heap pushes and pops, aux bytes) and the timer.

## Context
Algorithms are compared by how much work they do, not only by wall-clock time. Definitions are fixed so benchmark numbers stay comparable (see `docs/DECISIONS.md`). Code: `src/metrics.h`, `src/metrics.c`.

## Details
- `comparisons`: one comparison of two keys (heap sift, array-scan "smaller than best?", sort compare).
- `relaxations`: one test of `dist[u] + w < dist[v]`, whether or not it succeeds.
- `heap_pushes`, `heap_pops`: one insert / one extract-min.
- `aux_bytes`: auxiliary memory an algorithm allocates (dist array, heap array), added by hand.
- No double counting between comparisons and relaxations.
- Counters are one global `g_metrics`, bumped through macros (`COUNT_COMPARISON()` etc.) so algorithm signatures stay clean and algorithms stay swappable.
- Timer: `clock()` based, about 1 ms resolution on MinGW.
- Call `metrics_reset()` before each measured run.

## Related
- [Test harness](../testing/test-harness.md) — see-also: how the counters are tested.
