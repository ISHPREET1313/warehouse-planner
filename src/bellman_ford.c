/* bellman_ford.c - Bellman-Ford. See bellman_ford.h for the plan. */
#include "bellman_ford.h"

#include <stdlib.h>
#include <string.h>

#include "metrics.h"

int bellman_ford(const Graph *g, int source, ShortestPaths *sp)
{
    int a, i, u, v, nd, pass, changed;

    if (sp == NULL) {
        return -1;
    }
    memset(sp, 0, sizeof *sp);
    if (g == NULL || g->first == NULL || g->to == NULL || g->weight == NULL
        || g->n < 1 || source < 0 || source >= g->n) {
        return -1;
    }
    for (a = 0; a < g->n_arcs; a++) {
        if (g->weight[a] < -BF_MAX_WEIGHT || g->weight[a] > BF_MAX_WEIGHT) {
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

    sp->n = g->n;
    sp->source = source;
    for (i = 0; i < g->n; i++) {
        sp->dist[i] = DIST_INF;
        sp->prev[i] = -1;
    }
    sp->dist[source] = 0;

    for (pass = 1; pass <= g->n; pass++) {
        changed = 0;
        for (u = 0; u < g->n; u++) {
            if (sp->dist[u] >= DIST_INF) {
                continue;                   /* u not reached yet: nothing to relax */
            }
            for (a = g->first[u]; a < g->first[u + 1]; a++) {
                v = g->to[a];
                nd = sp->dist[u] + g->weight[a];
                COUNT_RELAXATION();         /* one arc tested */
                if (nd < sp->dist[v]) {
                    if (nd < -DIST_INF) {
                        return BF_NEG_CYCLE;   /* falling without end */
                    }
                    sp->dist[v] = nd;
                    sp->prev[v] = u;
                    changed = 1;
                }
            }
        }
        if (!changed) {
            return 0;                       /* nothing improved: all final */
        }
    }
    /* Pass n still improved something, which cannot happen without a
     * negative cycle reachable from the source. */
    return BF_NEG_CYCLE;
}
