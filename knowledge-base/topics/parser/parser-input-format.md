---
id: 20261119-parser-input-format
title: Parser input file format
tags: [parser]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-parser-validation-and-errors, 20261119-parser-design-and-complexity]
summary: The exact text format of the input file (MAP, map rows, ROBOT, TASK, comments) with a worked example.
---

# Parser input file format

> Summary: The exact text format of the input file (MAP, map rows, ROBOT, TASK, comments) with a worked example.

## Context
Plain text, no JSON, so the parser stays small and explainable. Documented at the top of `src/io.h`. Example file: `data/small/sample1.txt`.

## Details
```
# comment
MAP <rows> <cols>
<rows lines of exactly <cols> chars from  . # S C D R T>
ROBOT <capacity> <battery>          one per R
TASK  <weight> <value> <priority>   one per T
```
- The N-th ROBOT line belongs to the N-th R read top-to-bottom, left-to-right. Same for TASK and T.
- Blank lines and `#` comment lines are allowed before MAP and after the map block. Inside the map block `#` is an obstacle.
- No blank line between the MAP header and its rows, none inside the map.
- ROBOT and TASK lines may come in any mix of order after the map; comments must be on their own line.
- `\n` and `\r\n` both work. Keywords are upper case. Spaces and tabs separate numbers.
- Symbols: `.` free, `#` obstacle, `S` shelf, `C` charger, `D` dock, `R` robot start, `T` task pickup.
- Limits (in `src/problem.h`): rows, cols 1..500; robots at most 32; tasks at most 256; priority 1..5; numbers up to 1,000,000.

## Related
- [Validation and errors](parser-validation-and-errors.md) — see-also: what is rejected and how it is reported.
- [Design and complexity](parser-design-and-complexity.md) — see-also: how it is read.
