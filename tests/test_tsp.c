/*
 * test_tsp.c - tests for tsp.c (item 10).
 *
 * Expected numbers worked out by hand:
 *  - the 3-stop line instance (see group_classic): positions 0 | 1 | -2 | 5,
 *    nearest neighbour gives order 1,2,3 cost 11, the best is 2,1,3 cost 9.
 *  - counters: nearest = n(n+1)/2 comparisons; Held-Karp = n(n-1)2^(n-2)
 *    transitions + n for the final minimum (n=3: 12 + 3 = 15).
 *  - stops on a line all to the right of the start: the best path simply
 *    walks right, so the optimum equals the farthest stop's position.
 *  - random instances are checked against brute force over all orders.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "metrics.h"
#include "tests.h"
#include "tsp.h"

static unsigned long g_seed = 31337UL;

static int next_rand(int range)
{
    g_seed = (g_seed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (int)((g_seed >> 8) % (unsigned long)range);
}

/* Cost of start -> order[0] -> ... -> order[n-1], worked out from the matrix. */
static int cost_of_order(const int *dist, int n, const int *order)
{
    int cur = 0, total = 0, i;

    for (i = 0; i < n; i++) {
        total += dist[cur * (n + 1) + order[i]];
        cur = order[i];
    }
    return total;
}

/* 1 if order holds each of 1..n exactly once. */
static int is_permutation(const int *order, int n)
{
    unsigned char seen[TSP_NN_MAX_STOPS + 1];
    int i;

    memset(seen, 0, sizeof seen);
    for (i = 0; i < n; i++) {
        if (order[i] < 1 || order[i] > n || seen[order[i]]) {
            return 0;
        }
        seen[order[i]] = 1;
    }
    return 1;
}

/* Brute force oracle: try every order, keep the cheapest total. */
static void brute_rec(const int *dist, int n, int cur, int used, int count,
                      int acc, int *best)
{
    int j;

    if (count == n) {
        if (acc < *best) {
            *best = acc;
        }
        return;
    }
    for (j = 1; j <= n; j++) {
        if (used & (1 << (j - 1))) {
            continue;
        }
        brute_rec(dist, n, j, used | (1 << (j - 1)), count + 1,
                  acc + dist[cur * (n + 1) + j], best);
    }
}

static int brute_force(const int *dist, int n)
{
    int best = 2000000000;

    brute_rec(dist, n, 0, 0, 0, 0, &best);
    return best;
}

/* Distances between points on a line: dist[a][b] = |x[a] - x[b]|,
 * x[0] = 0 is the robot's start. */
static void line_matrix(int *dist, const int *x, int n)
{
    int a, b;

    for (a = 0; a <= n; a++) {
        for (b = 0; b <= n; b++) {
            dist[a * (n + 1) + b] = abs(x[a] - x[b]);
        }
    }
}

/* ---------------------------------------------------------------- */

/*
 * Line instance: start at 0, stop 1 at +1, stop 2 at -2, stop 3 at +5.
 *   dist rows:  0: 0 1 2 5   1: 1 0 3 4   2: 2 3 0 7   3: 5 4 7 0
 * Nearest neighbour: 0 -> 1 (cost 1), then 2 (3) beats 3 (4), then 3 (7):
 *   order 1,2,3, cost 11, comparisons 3 + 2 + 1 = 6.
 * All six orders: 123=11, 132=12, 213=9, 231=13, 312=12, 321=15, so the
 * best is 2,1,3 with cost 9 (unique).
 * Held-Karp: 6 transitions for masks of one stop + 6 for two stops = 12,
 * plus 3 for the final minimum = 15 comparisons. Tables: 2^3 * 3 = 24 states,
 * 24 ints + 24 bytes = 96 + 24 = 120 bytes.
 */
static void group_classic(void)
{
    int dist[16] = { 0, 1, 2, 5,
                     1, 0, 3, 4,
                     2, 3, 0, 7,
                     5, 4, 7, 0 };
    int order[3] = { 0, 0, 0 };
    int cost = -1, nn_cost;

    harness_suite("tsp: 3 stops on a line (nearest neighbour is not optimal)");

    metrics_reset();
    check_long("nearest: run", 0, tsp_nearest(dist, 3, order, &cost));
    check_long("nearest: first stop", 1, order[0]);
    check_long("nearest: second stop", 2, order[1]);
    check_long("nearest: third stop", 3, order[2]);
    check_long("nearest: cost 1 + 3 + 7", 11, cost);
    check_long("nearest: comparisons 3 + 2 + 1", 6, (long)g_metrics.comparisons);
    check_long("nearest: aux_bytes (3 flags)", 3, (long)g_metrics.aux_bytes);
    nn_cost = cost;

    metrics_reset();
    check_long("held-karp: run", 0, tsp_held_karp(dist, 3, order, &cost));
    check_long("held-karp: first stop", 2, order[0]);
    check_long("held-karp: second stop", 1, order[1]);
    check_long("held-karp: third stop", 3, order[2]);
    check_long("held-karp: cost 2 + 3 + 4", 9, cost);
    check_long("held-karp: comparisons 12 + 3", 15, (long)g_metrics.comparisons);
    check_long("held-karp: aux_bytes 24 * 4 + 24", 120, (long)g_metrics.aux_bytes);
    check_long("nearest neighbour is worse than the exact answer", 1, nn_cost > cost);
}

/*
 * Asymmetric: going 2 -> 1 is very expensive, 1 -> 2 is cheap.
 *   dist rows:  0: 0 10 1   1: 5 0 1   2: 5 100 0
 * Orders: 1,2 costs 10 + 1 = 11;  2,1 costs 1 + 100 = 101.
 * Nearest neighbour grabs the close stop 2 first and pays 101.
 * Held-Karp finds 1,2 with cost 11.
 */
static void group_asymmetric(void)
{
    int dist[9] = { 0, 10, 1,
                    5, 0, 1,
                    5, 100, 0 };
    int order[2] = { 0, 0 };
    int cost = -1;

    harness_suite("tsp: asymmetric costs (direction matters)");

    check_long("nearest: run", 0, tsp_nearest(dist, 2, order, &cost));
    check_long("nearest: first stop is the close one", 2, order[0]);
    check_long("nearest: second stop", 1, order[1]);
    check_long("nearest: cost 1 + 100", 101, cost);
    check_long("held-karp: run", 0, tsp_held_karp(dist, 2, order, &cost));
    check_long("held-karp: first stop", 1, order[0]);
    check_long("held-karp: second stop", 2, order[1]);
    check_long("held-karp: cost 10 + 1", 11, cost);
}

static void group_small(void)
{
    int one[4]  = { 0, 7,
                    3, 0 };
    int two[9]  = { 0, 4, 9,
                    4, 0, 2,
                    9, 2, 0 };
    int same[25], zero[16], big[9];
    int order[4] = { 0, 0, 0, 0 };
    int cost = -1, i, bad;

    harness_suite("tsp: small and boundary cases");

    /* one stop: only one possible order */
    metrics_reset();
    check_long("1 stop: nearest run", 0, tsp_nearest(one, 1, order, &cost));
    check_long("1 stop: nearest order", 1, order[0]);
    check_long("1 stop: nearest cost", 7, cost);
    check_long("1 stop: nearest comparisons", 1, (long)g_metrics.comparisons);
    metrics_reset();
    check_long("1 stop: held-karp run", 0, tsp_held_karp(one, 1, order, &cost));
    check_long("1 stop: held-karp order", 1, order[0]);
    check_long("1 stop: held-karp cost", 7, cost);
    check_long("1 stop: held-karp comparisons (0 + 1)", 1, (long)g_metrics.comparisons);
    check_long("1 stop: held-karp aux_bytes 2 * 4 + 2", 10, (long)g_metrics.aux_bytes);

    /* two stops: 1,2 costs 4 + 2 = 6, 2,1 costs 9 + 2 = 11 */
    metrics_reset();
    check_long("2 stops: nearest run", 0, tsp_nearest(two, 2, order, &cost));
    check_long("2 stops: nearest cost", 6, cost);
    check_long("2 stops: nearest comparisons 2 + 1", 3, (long)g_metrics.comparisons);
    metrics_reset();
    check_long("2 stops: held-karp run", 0, tsp_held_karp(two, 2, order, &cost));
    check_long("2 stops: held-karp cost", 6, cost);
    check_long("2 stops: held-karp comparisons 2 + 2", 4, (long)g_metrics.comparisons);
    check_long("2 stops: held-karp first stop", 1, order[0]);
    check_long("2 stops: held-karp second stop", 2, order[1]);

    /* every trip costs 5: any order costs 20; nearest breaks ties by number */
    for (i = 0; i < 25; i++) {
        same[i] = (i % 5 == i / 5) ? 0 : 5;
    }
    check_long("equal costs: nearest run", 0, tsp_nearest(same, 4, order, &cost));
    bad = (order[0] != 1) + (order[1] != 2) + (order[2] != 3) + (order[3] != 4);
    check_long("equal costs: nearest keeps stops in number order", 0, bad);
    check_long("equal costs: nearest cost", 20, cost);
    check_long("equal costs: held-karp run", 0, tsp_held_karp(same, 4, order, &cost));
    check_long("equal costs: held-karp cost", 20, cost);
    check_long("equal costs: held-karp order is a permutation", 1, is_permutation(order, 4));

    /* everything free */
    memset(zero, 0, sizeof zero);
    check_long("all zero: nearest run", 0, tsp_nearest(zero, 3, order, &cost));
    check_long("all zero: nearest cost", 0, cost);
    check_long("all zero: held-karp run", 0, tsp_held_karp(zero, 3, order, &cost));
    check_long("all zero: held-karp cost", 0, cost);

    /* largest allowed entry: totals stay far below the int limit */
    for (i = 0; i < 9; i++) {
        big[i] = (i % 3 == i / 3) ? 0 : TSP_MAX_DIST;
    }
    check_long("entries of 1000000: nearest run", 0, tsp_nearest(big, 2, order, &cost));
    check_long("entries of 1000000: nearest cost", 2000000, cost);
    check_long("entries of 1000000: held-karp run", 0, tsp_held_karp(big, 2, order, &cost));
    check_long("entries of 1000000: held-karp cost", 2000000, cost);
}

static void group_invalid(void)
{
    int ok[4]       = { 0, 1, 1, 0 };
    int negative[4] = { 0, -1, 1, 0 };
    int too_big[4]  = { 0, TSP_MAX_DIST + 1, 1, 0 };
    int order[2]    = { 99, 99 };
    int cost        = 77;

    harness_suite("tsp: invalid input");

    check_long("nearest: NULL matrix", -1, tsp_nearest(NULL, 1, order, &cost));
    check_long("nearest: NULL order", -1, tsp_nearest(ok, 1, NULL, &cost));
    check_long("nearest: NULL cost", -1, tsp_nearest(ok, 1, order, NULL));
    check_long("nearest: n = 0", -1, tsp_nearest(ok, 0, order, &cost));
    check_long("nearest: n = -1", -1, tsp_nearest(ok, -1, order, &cost));
    check_long("nearest: n = 257 (above the limit)", -1, tsp_nearest(ok, 257, order, &cost));
    check_long("nearest: negative entry", -1, tsp_nearest(negative, 1, order, &cost));
    check_long("nearest: entry above 1000000", -1, tsp_nearest(too_big, 1, order, &cost));

    check_long("held-karp: NULL matrix", -1, tsp_held_karp(NULL, 1, order, &cost));
    check_long("held-karp: NULL order", -1, tsp_held_karp(ok, 1, NULL, &cost));
    check_long("held-karp: NULL cost", -1, tsp_held_karp(ok, 1, order, NULL));
    check_long("held-karp: n = 0", -1, tsp_held_karp(ok, 0, order, &cost));
    check_long("held-karp: n = 16 (above the limit)", -1, tsp_held_karp(ok, 16, order, &cost));
    check_long("held-karp: negative entry", -1, tsp_held_karp(negative, 1, order, &cost));
    check_long("held-karp: entry above 1000000", -1, tsp_held_karp(too_big, 1, order, &cost));

    check_long("failed calls leave order alone", 99, order[0]);
    check_long("failed calls leave cost alone", 77, cost);
}

/* 100 random asymmetric instances (1..8 stops) against brute force. */
static void group_random(void)
{
    int dist[81];
    int order[8];
    int trial, n, i, entries, best, hk_cost, nn_cost;
    int bad_rc = 0, bad_hk = 0, bad_hk_order = 0, bad_nn_order = 0;
    int nn_beats_best = 0, bad_counts = 0, nn_worse = 0;
    long want_hk;

    harness_suite("tsp: 100 random instances vs brute force");

    g_seed = 31337UL;
    for (trial = 0; trial < 100; trial++) {
        n = 1 + next_rand(8);
        entries = (n + 1) * (n + 1);
        for (i = 0; i < entries; i++) {
            dist[i] = next_rand(51);
        }
        best = brute_force(dist, n);

        metrics_reset();
        if (tsp_held_karp(dist, n, order, &hk_cost) != 0) {
            bad_rc++;
            continue;
        }
        want_hk = (n > 1 ? (long)n * (n - 1) * (1L << (n - 2)) : 0L) + n;
        if ((long)g_metrics.comparisons != want_hk) {
            bad_counts++;
        }
        if (hk_cost != best) {
            bad_hk++;
        }
        if (!is_permutation(order, n) || cost_of_order(dist, n, order) != hk_cost) {
            bad_hk_order++;
        }

        metrics_reset();
        if (tsp_nearest(dist, n, order, &nn_cost) != 0) {
            bad_rc++;
            continue;
        }
        if ((long)g_metrics.comparisons != (long)n * (n + 1) / 2) {
            bad_counts++;
        }
        if (!is_permutation(order, n) || cost_of_order(dist, n, order) != nn_cost) {
            bad_nn_order++;
        }
        if (nn_cost < best) {
            nn_beats_best++;
        }
        if (nn_cost > best) {
            nn_worse++;
        }
    }
    check_long("every call succeeded", 0, bad_rc);
    check_long("held-karp cost differs from brute force", 0, bad_hk);
    check_long("held-karp order wrong or cost not its own", 0, bad_hk_order);
    check_long("nearest order wrong or cost not its own", 0, bad_nn_order);
    check_long("nearest beat the optimum (impossible)", 0, nn_beats_best);
    check_long("comparison counts differ from the formulas", 0, bad_counts);
    check_long("nearest was strictly worse at least once", 1, nn_worse > 0);
}

/*
 * Stops on a line, all to the right of the start (position 0). Stop i is at
 * x[i] = ((i * 37) % 101) + 1, e.g. 38, 75, 11, 48, 85, 21, 58, 95, ...
 * Any path must reach the farthest stop, so the cheapest path is to walk
 * right once: cost = largest position = 95 (stop 8 is at 95, and none of
 * stops 1..15 is farther). That route is the ascending order of positions.
 */
static void line_case(int n, long want_hk_comparisons, long want_hk_aux)
{
    int x[16], dist[16 * 16], order[15];
    int i, cost = 0, farthest = 0, bad_order;
    char name[64];

    x[0] = 0;
    for (i = 1; i <= n; i++) {
        x[i] = ((i * 37) % 101) + 1;
        if (x[i] > farthest) {
            farthest = x[i];
        }
    }
    line_matrix(dist, x, n);

    metrics_reset();
    sprintf(name, "%d stops on a line: held-karp run", n);
    check_long(name, 0, tsp_held_karp(dist, n, order, &cost));
    sprintf(name, "%d stops on a line: held-karp cost = farthest", n);
    check_long(name, farthest, cost);
    bad_order = 0;
    for (i = 0; i + 1 < n; i++) {
        if (x[order[i]] >= x[order[i + 1]]) {
            bad_order++;
        }
    }
    sprintf(name, "%d stops on a line: held-karp walks right once", n);
    check_long(name, 0, bad_order);
    sprintf(name, "%d stops on a line: held-karp comparisons", n);
    check_long(name, want_hk_comparisons, (long)g_metrics.comparisons);
    sprintf(name, "%d stops on a line: held-karp aux_bytes", n);
    check_long(name, want_hk_aux, (long)g_metrics.aux_bytes);

    sprintf(name, "%d stops on a line: nearest run", n);
    check_long(name, 0, tsp_nearest(dist, n, order, &cost));
    sprintf(name, "%d stops on a line: nearest cost", n);
    check_long(name, farthest, cost);
}

static void group_line(void)
{
    harness_suite("tsp: 12 and 15 stops on a line (known optimum 95)");

    /* n = 12: 12*11*2^10 = 135168 transitions + 12; tables 4096*12 = 49152
     * states, 49152 * 4 + 49152 = 245760 bytes. */
    line_case(12, 135180L, 245760L);
    /* n = 15: 15*14*2^13 = 1720320 + 15; 32768*15 = 491520 states,
     * 491520 * 4 + 491520 = 2457600 bytes. */
    line_case(15, 1720335L, 2457600L);
}

static void group_max_nearest(void)
{
    static int dist[257 * 257];
    int x[257], order[256];
    int i, cost = -1, bad = 0;

    harness_suite("tsp: nearest neighbour with 256 stops");

    /* stop i at position 3i: walking right is the only sensible route.
     * cost = 3 * 256 = 768, comparisons = 256 * 257 / 2 = 32896. */
    x[0] = 0;
    for (i = 1; i <= 256; i++) {
        x[i] = 3 * i;
    }
    line_matrix(dist, x, 256);
    metrics_reset();
    check_long("256 stops: run", 0, tsp_nearest(dist, 256, order, &cost));
    check_long("256 stops: cost", 768, cost);
    check_long("256 stops: comparisons", 32896, (long)g_metrics.comparisons);
    for (i = 0; i < 256; i++) {
        if (order[i] != i + 1) {
            bad++;
        }
    }
    check_long("256 stops: visited in order 1..256", 0, bad);
}

void test_tsp(void)
{
    group_classic();
    group_asymmetric();
    group_small();
    group_invalid();
    group_random();
    group_line();
    group_max_nearest();
}
