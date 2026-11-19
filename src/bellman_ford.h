/*
 * bellman_ford.h - Bellman-Ford shortest paths (item 6, comparison).
 *
 * Same input and same result type as Dijkstra (ShortestPaths, dijkstra.h),
 * but arc weights may be NEGATIVE (-BF_MAX_WEIGHT .. BF_MAX_WEIGHT), and it
 * detects a negative cycle that can be reached from the source.
 *
 * How it works: repeat "pass over every arc u -> v and relax it" (skipping
 * arcs whose start u is still unreached). After pass k, every node whose
 * shortest path has at most k arcs is correct. A shortest path has at most
 * n - 1 arcs, so n - 1 passes are enough. If pass n still changes
 * something, a negative cycle is reachable. A pass with no change stops
 * the loop early.
 *
 * Counters: every arc tested = one relaxation (arcs that start at an
 * unreached node are skipped and not counted). No comparisons, no heap.
 * aux_bytes = dist + prev.
 *
 * Overflow guard: a negative cycle would make distances fall without end,
 * so as soon as a distance would drop below -DIST_INF the function stops
 * and reports the cycle. A genuine shortest distance is never that low
 * (at least -1000 * (n - 1), n <= 250000).
 *
 * Complexity (V nodes, E arcs):
 *   time  worst and average O(V * E); best O(E) (one pass changes things,
 *         the next confirms nothing changed, or the source is boxed in);
 *   space O(V).
 * It is slower than Dijkstra on this project's non-negative weights; it is
 * here to show the trade-off and to handle negative weights.
 */
#ifndef BELLMAN_FORD_H
#define BELLMAN_FORD_H

#include "dijkstra.h"

#define BF_MAX_WEIGHT 1000   /* largest |weight| accepted */
#define BF_NEG_CYCLE  1      /* return value: negative cycle reachable */

/*
 * Returns 0 on success, BF_NEG_CYCLE (1) if a negative cycle is reachable
 * from the source (sp is filled but its distances are not trustworthy),
 * and -1 (sp left empty) for bad input: NULL pointers, empty graph, source
 * out of range, a weight outside -BF_MAX_WEIGHT..BF_MAX_WEIGHT, or memory
 * running out. Free sp with sp_free() (from dijkstra.h) in both 0 and 1.
 */
int bellman_ford(const Graph *g, int source, ShortestPaths *sp);

#endif /* BELLMAN_FORD_H */
