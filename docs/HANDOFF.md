# HANDOFF

Overwritten at the end of every session.

## Session 3 (2026-11-19)
- Item 3 (grid to graph) was wired in and VERIFIED by the user: 231/231 tests, sample1 prints nodes=48 walkable=38 arcs=94.
- Admin change: Claude now writes the min-heap and Dijkstra too; the admin reviews. PROMPT.md (Authorship + change log), knowledge-base working-rules and build-order notes updated.
- Wrote item 4: src/heap.[ch], tests/test_heap.c (98 checks, counter values worked out by hand). Wired into Makefile, tests/tests.h, tests/run_tests.c.
- Knowledge base: topics/heap/ (_topic, design-and-complexity, viva-questions), tag `heap`, INDEX registry.
- Docs: DECISIONS, QUESTIONS (Q12 to Q15), STATE, TODO, this file.
- Tooling: filesystem connector edit_file/write_file work. The container's str_replace does not reach the user's folder. Desktop Commander did not appear in the tool list, so nothing has been compiled by Claude.
- NOTHING OF ITEM 4 WAS COMPILED OR RUN.

## Next step
1. User runs: make clean && make && make test, and pastes the real output. Fix any warning or FAIL first. Expected: SUMMARY: 329 tests, 329 passed, 0 failed (231 old + 98 new heap checks).
2. User commits and pushes.
3. Then item 5: Dijkstra with heap (src/dijkstra.[ch], tests/test_dijkstra.c). Plan: dist[] and prev[] arrays, lazy deletion with the heap (capacity n_arcs + 1), counters for comparisons, relaxations, pushes and pops, path rebuild from prev[], multi-robot use via one run per robot start. Hand-count expected distances on data/small/sample1.txt before writing tests.

## Suggested commit (after tests pass)
git add -A
git commit -m "Item 4: min-heap, tests, knowledge base, authorship change"
git push
