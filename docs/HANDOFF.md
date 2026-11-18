# HANDOFF

Overwritten at the end of every session.

## Session 4 (2026-11-19)
- Read the docs. Items 4 (heap) and 5 (Dijkstra) are WRITTEN but NOTHING of either has been compiled or run by anyone yet.
- Item 5 files: src/dijkstra.[ch], tests/test_dijkstra.c. This session wired them in: Makefile (CORE_SRC, TEST_OBJ), tests/tests.h, tests/run_tests.c, and src/main.c (new report_distances: per robot, cost to every task plus time and counters).
- Reviewed dijkstra.c and test_dijkstra.c by hand: re-checked the hand counts (6-node graph: 7 pushes, 7 pops, 6 relaxations), sample1 distances, 500x500 counts (250000 pushes, 998000 relaxations) and the snake sizes (125248 / 125249). No mistakes found, but a review is not a compile.
- Knowledge base: topics/dijkstra/ (_topic, design-and-complexity, viva-questions), tag `dijkstra`, INDEX updated.

## Next step
1. User runs: make clean && make && make test, and pastes the real output. Fix any warning or FAIL first. Item 4 added 98 checks (329 expected before item 5); item 5 adds a number I did not count, so just report what SUMMARY says.
2. User runs: ./planner.exe data/small/sample1.txt  (expect robot 1: T1=13 T2=5 T3=10; robot 2: T1=5 T2=3 T3=2; robot 1 counters pushes=38 pops=38 relaxations=94).
3. User commits and pushes.
4. Then item 6: Dijkstra with array scan and Bellman-Ford (same Graph, same ShortestPaths result, same counters) so the comparison table is fair.

## Suggested commit (after tests pass)
git add -A
git commit -m "Item 4 and 5: min-heap, Dijkstra with heap, tests, knowledge base"
git push
