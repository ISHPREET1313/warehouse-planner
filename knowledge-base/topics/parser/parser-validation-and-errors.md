---
id: 20261119-parser-validation-and-errors
title: Parser validation rules and error codes
tags: [parser, testing]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-parser-input-format, 20261119-test-harness]
summary: Every check the parser makes, the five error codes, and the message format with line numbers.
---

# Parser validation rules and error codes

> Summary: Every check the parser makes, the five error codes, and the message format with line numbers.

## Context
Bad input must be reported clearly, never crash, and leave the Problem empty. Code: `src/io.c`; tests: `tests/test_parser.c`.

## Details
Status codes (`ParseStatus`):
- `PARSE_ERR_FILE`: cannot open or read the file, file over 4 MB, out of memory.
- `PARSE_ERR_SYNTAX`: no MAP header first, MAP not exactly two whole numbers, bad ROBOT/TASK number list, unknown keyword, line over 1023 characters, NULL text.
- `PARSE_ERR_MAP`: rows or cols outside 1..500, row length differs from the header, symbol not in `. # S C D R T`, text ends inside the map, blank line in the map.
- `PARSE_ERR_COUNT`: R cells vs ROBOT lines (or T vs TASK) differ, no robot, more than 32 robots or 256 tasks.
- `PARSE_ERR_VALUE`: capacity, battery, weight outside 1..1000000; value outside 0..1000000; priority outside 1..5.

Messages look like `line 3: invalid map symbol (allowed: . # S C D R T)`. Count errors found at the end of the file have no line number. `validate_problem` messages look like `robot 1: capacity must be between 1 and 1000000`.

Guarantees: on any failure the Problem is empty (cells NULL, counts 0); a too-small or NULL error buffer is safe.

Not checked here: whether a task is reachable. That is found when the grid becomes a graph (item 3).

## Related
- [Input format](parser-input-format.md) — prerequisite: the format being checked.
- [Test harness](../testing/test-harness.md) — see-also: how these cases are run.
