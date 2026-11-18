# DECISIONS

One line per decision, with the reason.

- 2026-11-18 C standard is C11 (-std=c11), not C17: the installed MinGW GCC 6.3 does not support -std=c17 (added in GCC 8). No language feature we need is lost.
- 2026-11-18 Sanitizers are optional via `make SAN=1`: MinGW GCC 6 has none; use WSL/Linux for ASan/UBSan runs.
- 2026-11-18 Makefile writes outputs and .o files next to sources, no build/ folder: avoids mkdir portability problems between cmd.exe and sh with Make 3.81.
- 2026-11-18 Timer uses clock() from time.h: standard library only, simple. Resolution is about 1 ms on MinGW, so benchmarks must use inputs large enough, or repeat runs, to be meaningful.
- 2026-11-18 Counters are global (g_metrics) updated through macros: keeps algorithm signatures clean so algorithms stay swappable in benchmarks.
- 2026-11-18 Counter definitions are fixed: comparison = one key comparison (heap/scan/sort); relaxation = one test of dist[u]+w < dist[v], success or not; no double counting between the two.
- 2026-11-18 Test harness compares integers only (check_long) for now; add string/double checks when a module needs them.
- 2026-11-19 Input format v1 is plain text: MAP header, exact-width map rows, ROBOT and TASK lines matched to R and T cells in row-major order. Reason: no JSON parser to write, and the N-th-line rule needs no ids.
- 2026-11-19 Parser has no sscanf/snprintf: hand-written read_int and bounded put_text. Reason: old MinGW may not declare snprintf under -std=c11, and every rule stays visible for the viva.
- 2026-11-19 Fixed arrays for robots (32) and tasks (256) inside Problem; only the grid is malloc'd. Reason: fewer allocations and leaks to explain.
- 2026-11-19 Parse failures always leave the Problem empty and return a status code plus a line-numbered message. Reason: callers never see half-built data.
- 2026-11-19 Harness gets check_str and 18-wide columns. Reason: status names must be readable in the expected-vs-actual table.
- 2026-11-19 Questions go to docs/QUESTIONS.md, not chat (admin change). Reason: user wants to finish the project first and discuss later.
- 2026-11-19 Knowledge base lives in knowledge-base/ (index-first layout). Graphify indexes the GitHub repo on push, so commit and push after each item. Reason: Graphify cannot see local files.
- 2026-11-19 .gitignore rewritten as ASCII; PROMPT.md not ignored (see QUESTIONS Q3). Reason: the old line was UTF-16 and did nothing.
