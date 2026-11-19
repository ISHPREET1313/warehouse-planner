/*
 * test_knapsack.c - tests for knapsack.c (item 9).
 *
 * Expected numbers worked out by hand:
 *  - classic case: capacity 50, items (w10,v60) (w20,v100) (w30,v120).
 *      DP: best 220 (items 2 and 3). Comparisons: one per cell
 *      (capacity - weight + 1 cells per item) = 41 + 31 + 21 = 93.
 *      aux_bytes = best[51] ints + 3*51 flags = 204 + 153 = 357.
 *      Greedy: ratios 6, 5, 4 take items 1 and 2 (weight 30), item 3 does
 *      not fit: value 160 (not optimal). Sorting compares 2 pairs.
 *  - random cases are checked against two independent methods written in
 *    this file: brute force over all subsets, and a plain 2D table.
 *
 * Run from the project root (make test does this).
 */
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "knapsack.h"
#include "metrics.h"
#include "tests.h"

static unsigned long g_seed = 4242UL;

static int next_rand(int range)
{
    g_seed = (g_seed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (int)((g_seed >> 8) % (unsigned long)range);
}

static int sum_chosen(const int *x, const int *chosen, int n)
{
    int i, s = 0;

    for (i = 0; i < n; i++) {
        if (chosen[i]) {
            s += x[i];
        }
    }
    return s;
}

/* Best value over all 2^n subsets (n <= 12 here). Independent oracle 1. */
static int brute_force(const int *w, const int *v, int n, int cap)
{
    int mask, i, sw, sv, best = 0;

    for (mask = 0; mask < (1 << n); mask++) {
        sw = 0;
        sv = 0;
        for (i = 0; i < n; i++) {
            if (mask & (1 << i)) {
                sw += w[i];
                sv += v[i];
            }
        }
        if (sw <= cap && sv > best) {
            best = sv;
        }
    }
    return best;
}

/* Textbook 2D table: t[i][c] = best value using the first i items and
 * capacity c. Independent oracle 2. Returns -1 if memory runs out. */
static int reference_2d(const int *w, const int *v, int n, int cap)
{
    int *t = (int *)calloc((size_t)(n + 1) * (size_t)(cap + 1), sizeof(int));
    int i, c, skip, take, answer;

    if (t == NULL) {
        return -1;
    }
    for (i = 1; i <= n; i++) {
        for (c = 0; c <= cap; c++) {
            skip = t[(i - 1) * (cap + 1) + c];
            take = skip;
            if (c >= w[i - 1]) {
                take = t[(i - 1) * (cap + 1) + (c - w[i - 1])] + v[i - 1];
            }
            t[i * (cap + 1) + c] = (take > skip) ? take : skip;
        }
    }
    answer = t[n * (cap + 1) + cap];
    free(t);
    return answer;
}

/* ---------------------------------------------------------------- */

static void group_classic(void)
{
    int w[3] = { 10, 20, 30 };
    int v[3] = { 60, 100, 120 };
    int ch[3];
    int best = -1;

    harness_suite("knapsack: the classic case (greedy fails)");

    metrics_reset();
    check_long("dp: run", 0, knapsack_dp(w, v, 3, 50, ch, &best));
    check_long("dp: best value", 220, best);
    check_long("dp: item 1 not taken", 0, ch[0]);
    check_long("dp: item 2 taken", 1, ch[1]);
    check_long("dp: item 3 taken", 1, ch[2]);
    check_long("dp: chosen weight fits (50)", 50, sum_chosen(w, ch, 3));
    check_long("dp: comparisons 41 + 31 + 21", 93, (long)g_metrics.comparisons);
    check_long("dp: aux_bytes 204 + 153", 357, (long)g_metrics.aux_bytes);

    metrics_reset();
    check_long("greedy: run", 0, knapsack_greedy(w, v, 3, 50, ch, &best));
    check_long("greedy: best value is only 160", 160, best);
    check_long("greedy: item 1 taken", 1, ch[0]);
    check_long("greedy: item 2 taken", 1, ch[1]);
    check_long("greedy: item 3 left behind", 0, ch[2]);
    check_long("greedy: comparisons (sorted input, 2 pairs)", 2,
               (long)g_metrics.comparisons);
    check_long("greedy: aux_bytes 3 ints", (long)(3 * sizeof(int)),
               (long)g_metrics.aux_bytes);
}

static void group_second_failure(void)
{
    int w[2] = { 1, 10 };
    int v[2] = { 2, 19 };
    int ch[2];
    int best = -1;

    harness_suite("knapsack: a second case where greedy fails");

    /* Ratios 2.0 and 1.9: greedy takes the light item first and then the
     * heavy one no longer fits. The best is the heavy item alone. */
    metrics_reset();
    check_long("dp: run", 0, knapsack_dp(w, v, 2, 10, ch, &best));
    check_long("dp: best is 19", 19, best);
    check_long("dp: light item not taken", 0, ch[0]);
    check_long("dp: heavy item taken", 1, ch[1]);
    check_long("dp: comparisons 10 + 1", 11, (long)g_metrics.comparisons);

    metrics_reset();
    check_long("greedy: run", 0, knapsack_greedy(w, v, 2, 10, ch, &best));
    check_long("greedy: best is only 2", 2, best);
    check_long("greedy: light item taken", 1, ch[0]);
    check_long("greedy: heavy item not taken", 0, ch[1]);
    check_long("greedy: comparisons (1 pair)", 1, (long)g_metrics.comparisons);
}

static void group_small(void)
{
    int w1[2] = { 2, 3 }, v1[2] = { 5, 6 };
    int w2[1] = { 5 },    v2[1] = { 10 };
    int w3[2] = { 3, 4 }, v3[2] = { 0, 5 };
    int w4[2] = { 5, 5 }, v4[2] = { 3, 4 };
    int dummy[1] = { 1 };
    int ch[4];
    int best = -1;

    harness_suite("knapsack: small and boundary cases");

    metrics_reset();
    check_long("everything fits: dp run", 0, knapsack_dp(w1, v1, 2, 100, ch, &best));
    check_long("everything fits: dp best", 11, best);
    check_long("everything fits: both chosen", 2, ch[0] + ch[1]);
    check_long("everything fits: comparisons 4 + 3", 7, (long)g_metrics.comparisons);
    check_long("everything fits: aux_bytes 6*4 + 2*6", 36, (long)g_metrics.aux_bytes);
    check_long("everything fits: greedy run", 0, knapsack_greedy(w1, v1, 2, 100, ch, &best));
    check_long("everything fits: greedy best", 11, best);

    check_long("capacity 0: dp run", 0, knapsack_dp(w2, v2, 1, 0, ch, &best));
    check_long("capacity 0: dp best", 0, best);
    check_long("capacity 0: nothing chosen", 0, ch[0]);
    check_long("capacity 0: greedy best", 0, (knapsack_greedy(w2, v2, 1, 0, ch, &best), best));

    metrics_reset();
    check_long("item too heavy: dp run", 0, knapsack_dp(w2, v2, 1, 3, ch, &best));
    check_long("item too heavy: dp best", 0, best);
    check_long("item too heavy: not chosen", 0, ch[0]);
    check_long("item too heavy: no comparisons", 0, (long)g_metrics.comparisons);
    check_long("item too heavy: greedy best", 0, (knapsack_greedy(w2, v2, 1, 3, ch, &best), best));

    v2[0] = 9;
    check_long("exact fit, one item: dp run", 0, knapsack_dp(w2, v2, 1, 5, ch, &best));
    check_long("exact fit, one item: dp best", 9, best);
    check_long("exact fit, one item: chosen", 1, ch[0]);

    check_long("n = 0: dp run", 0, knapsack_dp(dummy, dummy, 0, 10, ch, &best));
    check_long("n = 0: dp best", 0, best);
    check_long("n = 0: greedy run", 0, knapsack_greedy(dummy, dummy, 0, 10, ch, &best));

    check_long("zero-value item: dp run", 0, knapsack_dp(w3, v3, 2, 7, ch, &best));
    check_long("zero-value item: dp best", 5, best);
    check_long("zero-value item: greedy best", 5,
               (knapsack_greedy(w3, v3, 2, 7, ch, &best), best));

    check_long("two exact-fit items: dp best", 7,
               (knapsack_dp(w4, v4, 2, 10, ch, &best), best));
    check_long("best_value may be NULL (dp)", 0, knapsack_dp(w4, v4, 2, 10, ch, NULL));
    check_long("best_value may be NULL (greedy)", 0,
               knapsack_greedy(w4, v4, 2, 10, ch, NULL));
}

static void group_invalid(void)
{
    static int big[300];
    int w[1] = { 5 }, v[1] = { 7 };
    int ch[1] = { 99 };
    int best = -1;
    int w_zero[1] = { 0 }, v_neg[1] = { -1 }, w_huge[1] = { 1000001 };
    int w_30k[1] = { 30000 }, w_15k[1] = { 15000 };

    harness_suite("knapsack: invalid input");

    check_long("dp: NULL weights", -1, knapsack_dp(NULL, v, 1, 5, ch, &best));
    check_long("dp: NULL values", -1, knapsack_dp(w, NULL, 1, 5, ch, &best));
    check_long("dp: NULL chosen", -1, knapsack_dp(w, v, 1, 5, NULL, &best));
    check_long("greedy: NULL weights", -1, knapsack_greedy(NULL, v, 1, 5, ch, &best));
    check_long("greedy: NULL chosen", -1, knapsack_greedy(w, v, 1, 5, NULL, &best));
    check_long("dp: n = -1", -1, knapsack_dp(w, v, -1, 5, ch, &best));
    check_long("dp: n = 257 (above the limit)", -1, knapsack_dp(big, big, 257, 5, ch, &best));
    check_long("greedy: n = 257", -1, knapsack_greedy(big, big, 257, 5, ch, &best));
    check_long("dp: capacity -1", -1, knapsack_dp(w, v, 1, -1, ch, &best));
    check_long("dp: weight 0", -1, knapsack_dp(w_zero, v, 1, 5, ch, &best));
    check_long("dp: value -1", -1, knapsack_dp(w, v_neg, 1, 5, ch, &best));
    check_long("dp: weight 1000001", -1, knapsack_dp(w_huge, v, 1, 5, ch, &best));
    check_long("greedy: weight 0", -1, knapsack_greedy(w_zero, v, 1, 5, ch, &best));
    check_long("dp: capacity 25000, weight 30000 (too big)", -1,
               knapsack_dp(w_30k, v, 1, 25000, ch, &best));
    check_long("failed call leaves chosen alone", 99, ch[0]);
    check_long("dp: capacity 25000, weight 15000 is fine", 0,
               knapsack_dp(w_15k, v, 1, 25000, ch, &best));
    check_long("  its best value", 7, best);
    check_long("  and it is chosen", 1, ch[0]);
}

/* 200 random instances against brute force and the 2D table. */
static void group_random(void)
{
    int w[12], v[12], chd[12], chg[12];
    int trial, n, i, sum_w, cap, bf, ref, dp, gr;
    int bad_dp = 0, bad_ref = 0, bad_chosen = 0, bad_greedy = 0;
    int greedy_worse = 0, bad_rc = 0;

    harness_suite("knapsack: 200 random instances");

    g_seed = 4242UL;
    for (trial = 0; trial < 200; trial++) {
        n = 1 + next_rand(12);
        sum_w = 0;
        for (i = 0; i < n; i++) {
            w[i] = 1 + next_rand(20);
            v[i] = next_rand(51);
            sum_w += w[i];
        }
        cap = next_rand(sum_w + 1);

        bf  = brute_force(w, v, n, cap);
        ref = reference_2d(w, v, n, cap);
        if (knapsack_dp(w, v, n, cap, chd, &dp) != 0
            || knapsack_greedy(w, v, n, cap, chg, &gr) != 0) {
            bad_rc++;
            continue;
        }
        if (ref != bf) {
            bad_ref++;
        }
        if (dp != bf) {
            bad_dp++;
        }
        if (sum_chosen(w, chd, n) > cap || sum_chosen(v, chd, n) != dp
            || sum_chosen(w, chg, n) > cap || sum_chosen(v, chg, n) != gr) {
            bad_chosen++;
        }
        if (gr > bf) {
            bad_greedy++;
        }
        if (gr < bf) {
            greedy_worse++;
        }
    }
    check_long("every call succeeded", 0, bad_rc);
    check_long("2D reference differs from brute force", 0, bad_ref);
    check_long("dp differs from brute force", 0, bad_dp);
    check_long("chosen sets too heavy or wrong value", 0, bad_chosen);
    check_long("greedy beat the optimum (impossible)", 0, bad_greedy);
    check_long("greedy was strictly worse at least once", 1, greedy_worse > 0);
}

static void group_stress(void)
{
    int w[KNAP_MAX_ITEMS], v[KNAP_MAX_ITEMS], chd[KNAP_MAX_ITEMS], chg[KNAP_MAX_ITEMS];
    int i, cap, dp, gr, ref;
    long expected;

    harness_suite("knapsack: 256 items");

    g_seed = 99UL;
    for (i = 0; i < KNAP_MAX_ITEMS; i++) {
        w[i] = 1 + next_rand(50);
        v[i] = next_rand(101);
    }

    /* capacity 3000 is below the total weight (about 6400), so it is used as is */
    cap = 3000;
    metrics_reset();
    check_long("capacity 3000: dp run", 0, knapsack_dp(w, v, KNAP_MAX_ITEMS, cap, chd, &dp));
    expected = 0;
    for (i = 0; i < KNAP_MAX_ITEMS; i++) {
        expected += cap - w[i] + 1;
    }
    check_long("capacity 3000: comparisons = sum(cap - w + 1)", expected,
               (long)g_metrics.comparisons);
    ref = reference_2d(w, v, KNAP_MAX_ITEMS, cap);
    check_long("capacity 3000: dp equals the 2D table", ref, dp);
    check_long("capacity 3000: chosen weight fits", 1, sum_chosen(w, chd, KNAP_MAX_ITEMS) <= cap);
    check_long("capacity 3000: chosen value matches", dp, sum_chosen(v, chd, KNAP_MAX_ITEMS));
    check_long("capacity 3000: greedy run", 0, knapsack_greedy(w, v, KNAP_MAX_ITEMS, cap, chg, &gr));
    check_long("capacity 3000: greedy is not better", 1, gr <= dp);
    check_long("capacity 3000: greedy chosen value matches", gr, sum_chosen(v, chg, KNAP_MAX_ITEMS));

    /* the largest table: weights up to 200 so the total passes 20000 */
    for (i = 0; i < KNAP_MAX_ITEMS; i++) {
        w[i] = 1 + next_rand(200);
    }
    cap = KNAP_MAX_CAPACITY;
    check_long("capacity 20000: dp run", 0, knapsack_dp(w, v, KNAP_MAX_ITEMS, cap, chd, &dp));
    check_long("capacity 20000: greedy run", 0, knapsack_greedy(w, v, KNAP_MAX_ITEMS, cap, chg, &gr));
    check_long("capacity 20000: chosen weight fits", 1,
               sum_chosen(w, chd, KNAP_MAX_ITEMS) <= cap);
    check_long("capacity 20000: chosen value matches", dp, sum_chosen(v, chd, KNAP_MAX_ITEMS));
    check_long("capacity 20000: greedy is not better", 1, gr <= dp);
}

void test_knapsack(void)
{
    group_classic();
    group_second_failure();
    group_small();
    group_invalid();
    group_random();
    group_stress();
}
