---
id: 20261119-project-overview
title: Project overview
tags: [project]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-build-order, 20261119-working-rules]
summary: What the Warehouse Robot Path and Task Planner is, who it is for, and how it is assessed.
---

# Project overview

> Summary: What the Warehouse Robot Path and Task Planner is, who it is for, and how it is assessed.

## Context
Course CSEG3053 (Design and Analysis of Algorithms), individual project. Assessed by an 8-minute demo and a 4-minute viva. The source of truth for the rules is `PROMPT.md`; the source of truth for scope is the official brief in `docs/BRIEF.md` (35 mandatory algorithms, 3 demo scenarios, library, CLI, benchmarks, tests, report, guides).

## Details
- Scope decision (2026-11-19): the teachers said to implement only what is required, so the project is the demo core (shortest paths three ways, knapsack two ways, pickup order two ways, planner, three scenarios, CLI, benchmark, tests). The 35-algorithm list in the brief is NOT being built.
- Pure software simulation in C. No hardware. Runs fully offline.
- User: a warehouse supervisor who needs efficient robot routes and pickup order.
- Input: text file with an ASCII map (`. # S C D R T`) plus ROBOT and TASK lines.
- Output: robot routes and costs, task order, comparison tables (operation counts, runtime, memory).
- Architecture: generator or teacher file -> validator -> planner -> plan file + results -> viewer (replay only). Algorithms do no I/O; the planner has no graphics.
- Judged on: problem understanding, algorithm choice and reasons, complexity (best/average/worst), correctness reasoning, key code, comparison of alternatives, testing and limitations.
- Demo must show 3 scenarios (normal, capacity or constraint shock, large-data stress) and compare at least 2 strategies on the same input.

## Related
- [Build order](build-order.md) — see-also: the tiers of work and current progress.
- [Working rules](working-rules.md) — see-also: how Claude and the student work together.
