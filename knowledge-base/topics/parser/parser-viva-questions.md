---
id: 20261119-parser-viva-questions
title: Parser explain-back and viva questions
tags: [parser, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-parser-design-and-complexity, 20261119-parser-validation-and-errors]
summary: A 5-line explain-back of the parser and likely evaluator questions with short answers.
---

# Parser explain-back and viva questions

> Summary: A 5-line explain-back of the parser and likely evaluator questions with short answers.

## Context
Required after every module so the student can defend the code.

## Details
Explain it back (5 lines):
1. The file is read one line at a time; each character is visited once, so time is linear in the input size.
2. `parse_header` reads `MAP r c`, checks 1..500 and allocates `rows*cols` characters.
3. `parse_map` reads exactly `rows` lines, checks each row length and symbol, and records every R and T cell in order.
4. `parse_records` fills the numbers of the N-th ROBOT/TASK line into the N-th R/T and range-checks them.
5. Any failure returns a status code and a message, and frees the grid so the Problem is empty.

Evaluator questions:
- Q: Why check that ROBOT lines equal R cells instead of trusting the file? A: Later modules index `robots[i]` and `tasks[i]`; a mismatch would read uninitialised numbers. Failing early with a line-numbered message is cheaper than a wrong route.
- Q: How does `read_int` avoid overflow? A: It stops multiplying once the value reaches 10^8, so it never exceeds about 10^9 (fits a 32-bit int). Such a value is still above every limit, so the range check rejects it.
- Q: What is the time complexity? A: Best O(1) (first line invalid), average and worst Theta(N). Space O(rows*cols) for the grid.
- Q: Why a fixed-size robot and task array? A: Limits are small (32, 256), so no allocation and no leaks; the limits are enforced by the parser.
- Q: What happens to a '#' inside the map? A: It is an obstacle. Comments are only recognised outside the map block.

## Related
- [Design and complexity](parser-design-and-complexity.md) — prerequisite.
- [Validation and errors](parser-validation-and-errors.md) — see-also: the full check list.
