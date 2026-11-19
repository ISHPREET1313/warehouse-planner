/*
 * knapsack.h - which tasks should one robot load? (0/1 knapsack, item 9)
 *
 * Items are the candidate tasks: weight[i] (>= 1), value[i] (>= 0).
 * The robot can carry at most `capacity` in total weight. Each task is taken
 * whole or not at all (0/1). Goal: the largest total value.
 *
 * Two strategies, same inputs and outputs, so they can be compared:
 *
 *   knapsack_dp      exact answer (dynamic programming).
 *       best[w] = largest value reachable with total weight <= w.
 *       For each item i, for w from capacity DOWN to weight[i]:
 *           if best[w - weight[i]] + value[i] > best[w]  take item i at w.
 *       Going downwards means item i is used at most once per row.
 *       A flag table take[i][w] remembers where item i improved best[w], and
 *       the chosen items are read back from i = n-1 down to 0.
 *       Time Theta(n * C), space Theta(n * C) bytes for the flags + O(C).
 *       C = min(capacity, sum of all weights). Best, average and worst are
 *       the same: the loops never stop early.
 *
 *   knapsack_greedy  fast but not always optimal.
 *       Sort items by value/weight (best ratio first), then walk the list and
 *       take every item that still fits. Insertion sort is used to keep the
 *       code short: time O(n^2) worst, Theta(n) best (already sorted), space
 *       O(n). With a merge sort it would be O(n log n).
 *       It can be wrong: capacity 50, items (w10,v60) (w20,v100) (w30,v120)
 *       gives greedy 160 but the best is 220 (items 2 and 3).
 *
 * Counters (metrics.h): DP counts one comparison per cell "is the new value
 * better than best[w]?"; greedy counts one comparison per pair of ratios
 * compared while sorting. aux_bytes: DP adds the best[] and flag tables,
 * greedy adds its index array.
 *
 * Both return 0 on success and -1 (nothing written) for bad input: NULL
 * arrays, n < 0 or n > KNAP_MAX_ITEMS, capacity < 0, a weight outside
 * 1..1000000, a value outside 0..1000000, or an effective capacity above
 * KNAP_MAX_CAPACITY (keeps the table small).
 * chosen[i] is set to 1 or 0 for every i < n. best_value may be NULL.
 */
#ifndef KNAPSACK_H
#define KNAPSACK_H

#define KNAP_MAX_ITEMS     256
#define KNAP_MAX_CAPACITY  20000

int knapsack_dp(const int *weight, const int *value, int n, int capacity,
                int *chosen, int *best_value);

int knapsack_greedy(const int *weight, const int *value, int n, int capacity,
                    int *chosen, int *best_value);

#endif /* KNAPSACK_H */
