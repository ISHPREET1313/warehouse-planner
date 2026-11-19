/* knapsack.c - 0/1 knapsack, DP and greedy. See knapsack.h for the design. */
#include "knapsack.h"

#include <stdlib.h>

#include "metrics.h"

/*
 * Check the input. On success returns 0 and sets *cap to the capacity that
 * really matters: a robot can never use more than the sum of all weights, so
 * the table only needs min(capacity, total weight) columns.
 */
static int knap_prepare(const int *weight, const int *value, int n,
                        int capacity, int *chosen, int *cap)
{
    long long total = 0;
    int i;

    if (weight == NULL || value == NULL || chosen == NULL
        || n < 0 || n > KNAP_MAX_ITEMS || capacity < 0) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        if (weight[i] < 1 || weight[i] > 1000000
            || value[i] < 0 || value[i] > 1000000) {
            return -1;
        }
        total += weight[i];
    }
    *cap = (total < capacity) ? (int)total : capacity;
    if (*cap > KNAP_MAX_CAPACITY) {
        return -1;
    }
    return 0;
}

int knapsack_dp(const int *weight, const int *value, int n, int capacity,
                int *chosen, int *best_value)
{
    int *best;                 /* best[w]: best value with weight <= w      */
    unsigned char *take;       /* take[i*(cap+1)+w] = 1: item i improved it */
    int cap, i, w, candidate;
    size_t width;

    if (knap_prepare(weight, value, n, capacity, chosen, &cap) != 0) {
        return -1;
    }
    width = (size_t)cap + 1;
    best = (int *)calloc(width, sizeof(int));
    take = (unsigned char *)calloc((size_t)n * width + 1, 1);
    if (best == NULL || take == NULL) {
        free(best);
        free(take);
        return -1;
    }
    ADD_AUX_BYTES(width * sizeof(int) + (size_t)n * width);

    for (i = 0; i < n; i++) {
        /* Downwards, so best[w - weight[i]] still belongs to the previous
         * row: item i cannot be counted twice. */
        for (w = cap; w >= weight[i]; w--) {
            COUNT_COMPARISON();
            candidate = best[w - weight[i]] + value[i];
            if (candidate > best[w]) {
                best[w] = candidate;
                take[(size_t)i * width + (size_t)w] = 1;
            }
        }
    }

    /* Read the chosen items back, last item first. */
    w = cap;
    for (i = n - 1; i >= 0; i--) {
        if (take[(size_t)i * width + (size_t)w]) {
            chosen[i] = 1;
            w -= weight[i];
        } else {
            chosen[i] = 0;
        }
    }
    if (best_value != NULL) {
        *best_value = best[cap];
    }
    free(best);
    free(take);
    return 0;
}

/* 1 if item a has a strictly better value/weight ratio than item b.
 * value[a]/weight[a] > value[b]/weight[b]  <=>  value[a]*weight[b] > value[b]*weight[a]
 * (cross-multiplied so no division and no rounding; long long cannot overflow). */
static int better_ratio(const int *weight, const int *value, int a, int b)
{
    return (long long)value[a] * weight[b] > (long long)value[b] * weight[a];
}

int knapsack_greedy(const int *weight, const int *value, int n, int capacity,
                    int *chosen, int *best_value)
{
    int idx[KNAP_MAX_ITEMS];   /* item numbers, sorted by ratio, best first */
    int cap, i, j, t, remaining, total = 0;

    if (knap_prepare(weight, value, n, capacity, chosen, &cap) != 0) {
        return -1;
    }
    ADD_AUX_BYTES((size_t)n * sizeof(int));
    for (i = 0; i < n; i++) {
        idx[i] = i;
        chosen[i] = 0;
    }

    /* Insertion sort (stable: equal ratios keep their original order). */
    for (i = 1; i < n; i++) {
        for (j = i; j > 0; j--) {
            COUNT_COMPARISON();
            if (better_ratio(weight, value, idx[j], idx[j - 1])) {
                t = idx[j];
                idx[j] = idx[j - 1];
                idx[j - 1] = t;
            } else {
                break;
            }
        }
    }

    /* Take every item that still fits, best ratio first. */
    remaining = cap;
    for (i = 0; i < n; i++) {
        if (weight[idx[i]] <= remaining) {
            chosen[idx[i]] = 1;
            remaining -= weight[idx[i]];
            total += value[idx[i]];
        }
    }
    if (best_value != NULL) {
        *best_value = total;
    }
    return 0;
}
