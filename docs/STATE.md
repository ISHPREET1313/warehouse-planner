# STATE

Last updated: 2026-11-19

## Done
- Item 1 (skeleton, Makefile, harness, metrics): built and tested by the user, 12/12 passed. Explain-back answers still pending from the user.

## Half-done
- Item 2 (parser and validator): all code and tests WRITTEN but NOT yet compiled or run by anyone. Files: src/problem.[ch], src/io.[ch], tests/test_parser.c, data/small/sample1.txt, data/adversarial/bad_count.txt. Harness gained check_str; table columns widened to 18. main.c accepts `planner <file>`.
- Knowledge base: knowledge-base/INDEX.md plus 10 notes. Needs a note per finished item.
- Graphify: connected to ISHPREET1313/warehouse-planner but only sees pushed commits (2 nodes now).

## Build and test (Windows, MinGW GCC 6.3, GNU Make 3.81)
- Build:  make
- Run:    ./planner.exe data/small/sample1.txt
- Test:   make clean && make test   (from the project root)
- Sanitizers: make clean && make test SAN=1   (WSL/Linux only, not yet tried)

## Layout
src/ (main, metrics, problem, io), tests/ (harness, run_tests, test_metrics, test_parser), data/ (small, adversarial), docs/, knowledge-base/.
