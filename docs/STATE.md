# STATE

Last updated: 2026-11-19

## Done
- Item 1 (skeleton, Makefile, harness, metrics): verified by the user, 12/12 passed.
- Item 2 (parser and validator): verified by the user, 139/139 passed (metrics + parser).
- Item 3 (grid to graph, BFS reachability): verified by the user, 231/231 passed; `planner data/small/sample1.txt` prints nodes=48 walkable=38 arcs=94 and "every task can be reached".

## Half-done
- Item 4 (min-heap): code and tests WRITTEN and wired into Makefile, tests.h and run_tests.c, but NOT yet compiled or run by anyone. Files: src/heap.[ch], tests/test_heap.c, knowledge-base/topics/heap/. Expected: 98 new checks, 329 in total, 0 failed.
- Admin review/viva answers for items 1 to 4 are pending (see docs/QUESTIONS.md Q15 and the knowledge-base viva notes).
- Graphify: connected to ISHPREET1313/warehouse-planner but only sees pushed commits.

## Authorship (changed 2026-11-19)
Claude writes ALL code including min-heap and Dijkstra; the admin reviews.

## Build and test (Windows, MinGW GCC 6.3, GNU Make 3.81)
- Build:  make
- Run:    ./planner.exe data/small/sample1.txt   (prints the problem, graph size, unreachable tasks)
- Test:   make clean && make test   (from the project root)
- Sanitizers: make clean && make test SAN=1   (WSL/Linux only, not yet tried)

## Layout
src/ (main, metrics, problem, io, graph, heap), tests/ (harness, run_tests, test_metrics, test_parser, test_graph, test_heap), data/ (small, adversarial), docs/, knowledge-base/.
