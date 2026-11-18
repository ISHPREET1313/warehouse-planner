# TODO

Follow the build order. Do not start a lower tier until the one above is finished.

## PRIMARY
1. [done, explain-back pending] Skeleton, Makefile, test harness, metrics
2. [done, verified 139/139, explain-back pending] Parser and validator
3. [done, verified 231/231, explain-back pending] Grid to graph conversion
4. [code written and wired, NOT yet run] Min-heap (Claude writes it since the authorship change) - waiting for user's `make clean && make test` output
5. Dijkstra with heap (Claude writes it; admin reviews) - NEXT after item 4 is verified
6. Dijkstra with array scan, and Bellman-Ford, as comparisons
7. Test suite: normal, boundary, invalid, stress, adversarial, expected vs actual table
8. CLI menu and ASCII path replay
9. Benchmarks and CSV output

## IMPORTANT (after Primary)
10. 0/1 knapsack DP vs greedy ratio (include a case where greedy fails)
11. Pickup order: nearest neighbour vs Held-Karp (~12 stops)
12. Task sorting: merge sort vs quick sort (include quick sort worst case)
13. Seeded generator, profiles: normal, dense, sparse, maze, knapsack-hard, tsp-small

## SECONDARY (only if all above is done, tested, documented)
14. Bipartite matching (teach with small example first)
15. KMP vs naive string matching (teach first)
16. Ford-Fulkerson max-flow (teach first)
17. Extra generator profiles
18. raylib replay viewer

## Open housekeeping
- User: run the item 4 build/test, then commit and push (Graphify needs the push).
- User: answer the review questions in docs/QUESTIONS.md Q15 (items 1 to 4).
- Try `make clean` on the user's shell (not yet confirmed).
- Add a knowledge-base note per finished item and update INDEX.md.
- Review docs/QUESTIONS.md together after the project core is done.
- Tag v1-core when Primary is complete.
