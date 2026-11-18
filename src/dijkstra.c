/* dijkstra.c - Dijkstra with a binary min-heap. See dijkstra.h for the plan. */
#include "dijkstra.h"

#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "metrics.h"

void sp_free(ShortestPaths *sp)
{
    if (sp == NULL) {
        return;
    }
    free(sp->dist);
    free(sp->prev);
    memset(sp, 0, sizeof *sp);
}

int dijkstra(const Graph *g, int source, ShortestPaths *sp)
{
    MinHeap heap;
    int a, i, u, v, d, nd;

    if (sp == NULL) {
        return -1;
    }
    memset(sp, 0, sizeof *sp);
    if (g == NULL || g->first == NULL || g->to == NULL || g->weight == NULL
        || g->n < 1 || source < 0 || source >= g->n) {
        return -1;
    }

    /* Reject weights Dijkstra cannot handle before doing any work. */
    for (a = 0; a < g->n_arcs; a++) {
        if (g->weight[a] < 0 || g->weight[a] > DIJKSTRA_MAX_WEIGHT) {
            return -1;
        }
    }

    sp->dist = (int *)malloc((size_t)g->n * sizeof(int));
    sp->prev = (int *)malloc((size_t)g->n * sizeof(int));
    if (sp->dist == NULL || sp->prev == NULL) {
        sp_free(sp);
        return -1;
    }
    ADD_AUX_BYTES(2 * (size_t)g->n * sizeof(int));

    /* Each node is expanded once, so there are at most n_arcs successful
     * relaxations, plus the first push of the source: n_arcs + 1 pairs. */
    if (heap_init(&heap, g->n_arcs + 1) != 0) {
        sp_free(sp);
        return -1;
    }

    sp->n = g->n;
    sp->source = source;
    for (i = 0; i < g->n; i++) {
        sp->dist[i] = DIST_INF;
        sp->prev[i] = -1;
    }
    sp->dist[source] = 0;
    heap_push(&heap, 0, source);

    while (heap_pop(&heap, &d, &u) == 0) {
        COUNT_COMPARISON();
        if (d > sp->dist[u]) {
            continue;                       /* stale pair: u was improved later */
        }
        for (a = g->first[u]; a < g->first[u + 1]; a++) {
            v = g->to[a];
            nd = sp->dist[u] + g->weight[a];
            COUNT_RELAXATION();             /* one arc tested */
            if (nd < sp->dist[v]) {
                sp->dist[v] = nd;
                sp->prev[v] = u;
                heap_push(&heap, nd, v);
            }
        }
    }

    heap_free(&heap);
    return 0;
}

int sp_path(const ShortestPaths *sp, int target, int *out, int max_len)
{
    int len = 0, v, i;

    if (sp == NULL || sp->dist == NULL || sp->prev == NULL
        || target < 0 || target >= sp->n) {
        return -1;
    }
    if (sp->dist[target] >= DIST_INF) {
        return 0;                           /* no path */
    }

    /* Walk back from the target to the source to find the length. The
     * check against n stops an endless loop if prev were ever corrupt. */
    for (v = target; v != -1; v = sp->prev[v]) {
        len++;
        if (len > sp->n) {
            return -1;
        }
    }
    if (out == NULL || max_len < len) {
        return -1;
    }

    /* Walk back again, filling the array from the end. */
    i = len - 1;
    for (v = target; v != -1; v = sp->prev[v]) {
        out[i--] = v;
    }
    return len;
}
