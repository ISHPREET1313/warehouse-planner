---
id: 20261119-test-harness
title: Test harness
tags: [testing]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-operation-counters, 20261119-parser-validation-and-errors]
summary: How tests are written and run (expected vs actual table, check_long, check_str) and the test categories.
---

# Test harness

> Summary: How tests are written and run (expected vs actual table, check_long, check_str) and the test categories.

## Context
Tests must show an expected vs actual table in the demo. Code: `tests/harness.[ch]`, one `tests/test_<module>.c` per module, all called from `tests/run_tests.c`.

## Details
- `harness_suite(name)` prints a title and the table header.
- `check_long(name, expected, actual)` compares integers. `check_str(name, expected, actual)` compares strings (used for parser status names such as `PARSE_ERR_MAP`).
- `harness_summary()` prints totals and returns 0 only if everything passed; `make test` therefore fails on any failed check.
- Test names stay within 44 characters so the table lines up.
- Every module gets: normal, boundary, invalid and adversarial cases.
- Run from the project root: `make test` (file tests use paths like `data/small/sample1.txt`).
- Adding a module: write `tests/test_<module>.c`, declare it in `tests/tests.h`, call it from `run_tests.c`, add the `.o` to `TEST_OBJ` in the Makefile.

## Related
- [Operation counters](../metrics/operation-counters.md) — see-also: the first module tested.
- [Parser validation and errors](../parser/parser-validation-and-errors.md) — see-also: the largest test suite so far.
