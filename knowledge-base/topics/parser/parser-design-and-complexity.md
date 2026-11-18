---
id: 20261119-parser-design-and-complexity
title: Parser design and complexity
tags: [parser, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-parser-input-format, 20261119-parser-viva-questions]
summary: How the parser is structured (read line, three steps, validate) and its best, average and worst time and space.
---

# Parser design and complexity

> Summary: How the parser is structured (read line, three steps, validate) and its best, average and worst time and space.

## Context
Item 2. Files: `src/problem.[ch]` (data), `src/io.[ch]` (parser).

## Details
Structure of `parse_problem_text`:
1. `read_line` copies one line at a time (strips `\r`), so each character is looked at once.
2. `parse_header` finds `MAP r c`, checks 1..500, allocates the grid (`malloc`).
3. `parse_map` reads exactly `rows` lines, checks length and symbols, stores cells, records every R and T position in `robots[]` and `tasks[]`.
4. `parse_records` reads ROBOT/TASK lines, fills capacity/battery and weight/value/priority, checks ranges and counts.
5. `validate_problem` re-checks the finished Problem. Any failure calls `problem_free`.

Small helpers: `read_int` (hand-written whole-number reader; growth stops near 10^9 so an int never overflows, then the range check rejects it), `keyword_is` (word boundary, so `ROBOTS` is not `ROBOT`), `put_text` (bounded copy for messages).

Complexity, N = characters in the input, rows*cols <= 250,000:
- Time: worst and average Theta(N): every character is read once. Best case O(1): the first meaningful line is wrong and parsing stops.
- Auxiliary space: O(rows*cols) for the grid plus a fixed 1 KB line buffer. Robots and tasks are fixed arrays (32 and 256 entries), so they are O(1).
- File version also holds the file text: O(N) extra.

Why this design: one pass, no backtracking, no library parsing tricks, every rule is a visible `if`.

## Related
- [Input format](parser-input-format.md) — prerequisite.
- [Viva questions](parser-viva-questions.md) — see-also: explain-back and evaluator questions.
