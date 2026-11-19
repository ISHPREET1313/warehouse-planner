/*
 * bench.h - the benchmark table (item 14).
 *
 * Runs every comparison pair on the same inputs and prints one row per
 * (case, algorithm): a short result, the operation counters, auxiliary
 * memory and the time of one run.
 *
 *   shortest paths  Dijkstra heap, Dijkstra scan, Bellman-Ford, from the
 *                   first robot of each data file (small, medium, large,
 *                   adversarial). The result column is the sum of all finite
 *                   distances: equal numbers show the three agree.
 *   knapsack        DP vs greedy on seeded random instances of growing
 *                   size, plus the classic case where greedy is wrong.
 *   pickup order    nearest neighbour vs Held-Karp on seeded random stops
 *                   (Manhattan distances on a 100 x 100 floor), growing n;
 *                   beyond 15 stops only nearest neighbour is run.
 *   planner         strategy set A (DP + Held-Karp) vs B (greedy + nearest)
 *                   on every data file.
 *
 * Counters are exact and repeatable (same input, same numbers every run).
 * Time is the average of repeated runs (each case repeats until about 30 ms
 * have passed, at most 2000 times) because clock() only has about 1 ms
 * resolution; it varies a little from run to run and machine to machine.
 *
 * bench_run writes to `out` as an aligned text table (csv = 0) or as CSV
 * with a header line (csv = 1). Returns 0, or -1 if out is NULL. A data file
 * that cannot be loaded is reported as a row "skipped" and does not stop
 * the run. Algorithms keep no I/O of their own: only this module prints.
 */
#ifndef BENCH_H
#define BENCH_H

#include <stdio.h>

int bench_run(FILE *out, int csv);

#endif /* BENCH_H */
