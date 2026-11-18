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
Course CSEG3053 (Design and Analysis of Algorithms), individual project. Assessed by an 8-minute demo and a 4-minute viva. The source of truth for the rules is `PROMPT.md`.

## Details
- Pure software simulation in C. No hardware. Runs fully offline.
- User: a warehouse supervisor who needs efficient robot routes and pickup order.
- Input: text file with an ASCII map (`. # S C D R T`) plus ROBOT and TASK lines.
- Output: robot routes and costs, task order, comparison tables (operation counts, runtime, memory).
- Architecture: generator or teacher file -> validator -> planner -> plan file + results -> viewer (replay only). Algorithms do no I/O; the planner has no graphics.
- Judged on: problem understanding, algorithm choice and reasons, complexity (best/average/worst), correctness reasoning, key code, comparison of alternatives, testing and limitations.

## Related
- [Build order](build-order.md) — see-also: the tiers of work and current progress.
- [Working rules](working-rules.md) — see-also: how Claude and the student work together.
