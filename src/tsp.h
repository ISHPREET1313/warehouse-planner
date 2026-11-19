/*
 * tsp.h - in which order should one robot visit its pickup stops? (item 10)
 *
 * Input: n stops numbered 1..n, plus the robot's start as stop 0, and a
 * (n+1) x (n+1) matrix dist (row-major): dist[a*(n+1)+b] = cost of going
 * from stop a to stop b (entries 0..TSP_MAX_DIST; the matrix need not be
 * symmetric). The planner fills it from Dijkstra distances.
 * The robot starts at stop 0, visits every stop exactly once and does NOT
 * come back (open path). Goal: the smallest total cost.
 *
 * Output: order[0..n-1] = the stops in visiting order (values 1..n) and
 * *cost = total cost of start -> order[0] -> order[1] -> ... -> order[n-1].
 *
 *   tsp_nearest     heuristic. From the current stop always go to the
 *       nearest unvisited stop (ties: lowest number). Time Theta(n^2) in
 *       every case, space O(n). Fast, but can be far from optimal.
 *       n up to TSP_NN_MAX_STOPS.
 *
 *   tsp_held_karp   exact (dynamic programming over subsets).
 *       dp[mask][j] = cheapest way to start at 0, visit exactly the stops in
 *       `mask` (bit j means stop j+1) and end at stop j+1.
 *           start:  dp[{j}][j] = dist[0][j+1]
 *           step:   dp[mask + k][k] = min over j in mask of
 *                                     dp[mask][j] + dist[j+1][k+1]
 *       The answer is the smallest dp[all][j]; a parent table gives the
 *       order. Time Theta(n^2 * 2^n) in every case, space Theta(n * 2^n).
 *       n up to TSP_HK_MAX_STOPS (15: about 2 MB of tables).
 *       Why not just try every order? That is n! orders; for n = 12 it is
 *       479 million, while Held-Karp needs 12*12*4096 = about 590 thousand.
 *
 * Counters (metrics.h): nearest neighbour counts one comparison per
 * candidate stop examined, n(n+1)/2 in total. Held-Karp counts one
 * comparison per transition tried, n(n-1)2^(n-2), plus n for the final
 * minimum. aux_bytes: Held-Karp adds its two tables, nearest its flags.
 *
 * Both return 0 on success, -1 for bad input (NULL pointers, n < 1, n above
 * the limit, an entry outside 0..TSP_MAX_DIST) or when memory runs out.
 */
#ifndef TSP_H
#define TSP_H

#define TSP_MAX_DIST        1000000
#define TSP_NN_MAX_STOPS    256
#define TSP_HK_MAX_STOPS    15

int tsp_nearest(const int *dist, int n, int *order, int *cost);

int tsp_held_karp(const int *dist, int n, int *order, int *cost);

#endif /* TSP_H */
