/* tsp.c - pickup order: nearest neighbour and Held-Karp. See tsp.h. */
#include "tsp.h"

#include <stdlib.h>
#include <string.h>

#include "metrics.h"

#define HK_INF 1073741824   /* 2^30: larger than any real total (<= 15 * 10^6) */

/* Cost from stop a to stop b; the matrix has n + 1 rows and columns. */
#define DIST(a, b) dist[(a) * (n + 1) + (b)]

/* Check pointers, n and every matrix entry. 0 = fine, -1 = bad input. */
static int tsp_check(const int *dist, int n, const int *order, const int *cost,
                     int max_stops)
{
    int i, entries;

    if (dist == NULL || order == NULL || cost == NULL
        || n < 1 || n > max_stops) {
        return -1;
    }
    entries = (n + 1) * (n + 1);
    for (i = 0; i < entries; i++) {
        if (dist[i] < 0 || dist[i] > TSP_MAX_DIST) {
            return -1;
        }
    }
    return 0;
}

int tsp_nearest(const int *dist, int n, int *order, int *cost)
{
    unsigned char visited[TSP_NN_MAX_STOPS];   /* visited[j - 1]: stop j done */
    int step, j, cur, best, best_d, d, total;

    if (tsp_check(dist, n, order, cost, TSP_NN_MAX_STOPS) != 0) {
        return -1;
    }
    memset(visited, 0, sizeof visited);
    ADD_AUX_BYTES((size_t)n);

    cur = 0;                    /* start at the robot's position */
    total = 0;
    for (step = 0; step < n; step++) {
        best = -1;
        best_d = 0;
        for (j = 1; j <= n; j++) {
            if (visited[j - 1]) {
                continue;
            }
            COUNT_COMPARISON();
            d = DIST(cur, j);
            if (best == -1 || d < best_d) {   /* strict: ties keep the lower stop */
                best = j;
                best_d = d;
            }
        }
        visited[best - 1] = 1;
        order[step] = best;
        total += best_d;
        cur = best;
    }
    *cost = total;
    return 0;
}

int tsp_held_karp(const int *dist, int n, int *order, int *cost)
{
    int *dp;                    /* dp[mask * n + j]                          */
    signed char *parent;        /* parent[mask * n + j]: stop before j, or -1 */
    size_t states, s;
    int full, mask, new_mask, j, k, cur, cand, best, best_j, i, prev;

    if (tsp_check(dist, n, order, cost, TSP_HK_MAX_STOPS) != 0) {
        return -1;
    }
    full = (1 << n) - 1;
    states = ((size_t)1 << n) * (size_t)n;
    dp = (int *)malloc(states * sizeof(int));
    parent = (signed char *)malloc(states);
    if (dp == NULL || parent == NULL) {
        free(dp);
        free(parent);
        return -1;
    }
    ADD_AUX_BYTES(states * sizeof(int) + states);

    for (s = 0; s < states; s++) {
        dp[s] = HK_INF;
        parent[s] = -1;
    }
    /* A visited set of just stop j + 1: straight from the start. */
    for (j = 0; j < n; j++) {
        dp[((size_t)1 << j) * (size_t)n + (size_t)j] = DIST(0, j + 1);
    }

    /* Smaller masks come before larger ones, so dp[mask][*] is final when we
     * use it (new_mask is always bigger than mask). */
    for (mask = 1; mask <= full; mask++) {
        for (j = 0; j < n; j++) {
            if (!(mask & (1 << j))) {
                continue;
            }
            cur = dp[(size_t)mask * (size_t)n + (size_t)j];
            for (k = 0; k < n; k++) {
                if (mask & (1 << k)) {
                    continue;
                }
                cand = cur + DIST(j + 1, k + 1);
                new_mask = mask | (1 << k);
                COUNT_COMPARISON();
                if (cand < dp[(size_t)new_mask * (size_t)n + (size_t)k]) {
                    dp[(size_t)new_mask * (size_t)n + (size_t)k] = cand;
                    parent[(size_t)new_mask * (size_t)n + (size_t)k] = (signed char)j;
                }
            }
        }
    }

    /* Cheapest way to have visited everything, over the last stop. */
    best = HK_INF;
    best_j = -1;
    for (j = 0; j < n; j++) {
        COUNT_COMPARISON();
        if (dp[(size_t)full * (size_t)n + (size_t)j] < best) {
            best = dp[(size_t)full * (size_t)n + (size_t)j];
            best_j = j;
        }
    }

    /* Walk the parents back from the last stop to the first. */
    mask = full;
    j = best_j;
    for (i = n - 1; i >= 0; i--) {
        order[i] = j + 1;
        prev = parent[(size_t)mask * (size_t)n + (size_t)j];
        mask &= ~(1 << j);
        j = prev;
    }
    *cost = best;

    free(dp);
    free(parent);
    return 0;
}
