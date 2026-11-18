---
id: 20261119-toolchain
title: Toolchain on the student's machine
tags: [environment]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-graphify-setup]
summary: Windows with MinGW GCC 6.3 and Make 3.81, so C11, no sanitizers, and a few library functions to avoid.
---

# Toolchain on the student's machine

> Summary: Windows with MinGW GCC 6.3 and Make 3.81, so C11, no sanitizers, and a few library functions to avoid.

## Context
The earlier sandbox (GCC 13, Linux) is not the student's machine. Code must build with the student's tools.

## Details
- Compiler: MinGW GCC 6.3.0 -> use `-std=c11` (C17 needs GCC 8).
- Make: GNU Make 3.81 -> keep the Makefile simple, no `mkdir` tricks, outputs next to sources.
- Sanitizers: not available in MinGW GCC 6. Use `make clean && make test SAN=1` under WSL or Linux.
- Avoid `snprintf` and `sscanf` in project code: `snprintf` can be undeclared under `-std=c11` on old MinGW, and hand-written number reading keeps every rule visible. `sprintf` is fine only where the buffer size is provably enough (tests).
- On MinGW `long` is 32 bits. Use `int` for the small numbers here and cap huge parsed values so nothing overflows.
- `clock()` resolution is about 1 ms, so benchmarks need big inputs or repeats.
- Build: `make`. Run: `./planner.exe <file>`. Test: `make test` (run from the project root so `data/` paths work).

## Related
- [Graphify setup](graphify-setup.md) — see-also: the code index needs a GitHub push.
