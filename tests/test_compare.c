/*
 * test_compare.c - tests for item 6: dijkstra_scan and bellman_ford.
 *
 * Expected numbers were worked out by hand:
 *  - the 6-node graph (same as in test_dijkstra.c) and a 4-node star,
 *  - a small graph with a negative arc, a negative cycle, and a ring,
 *  - sample1.txt and open grids from the shape of the map,
 * and random graphs are cross-checked three ways: heap Dijkstra, array-scan
 * Dijkstra and Bellman-Ford must give the same distances. Negative weights
 * are checked with the potential trick (see group_negative_random).
 *
 * Comparison counts of the scan version follow one formula. With n nodes of
 * which R are reachable, round k (k = 0 .. R-1) examines n - k unfinished
 * nodes, and the last round examines the n - R unreachable ones:
 *     comparisons = R*n - R*(R-1)/2 + (n - R)
 * Run from the project root (make test does this).
 */
#include <stdlib.h>
#include <string.h>

#include "bellman_ford.h"
#include "dijkstra.h"
#include "graph.h"
#include "harness.h"
#include "io.h"
#include "metrics.h"
#include "problem.h"
#include "tests.h"

/* ---- small repeatable pseudo-random numbers ---- */
static unsigned long g_seed = 4242UL;

static int next_rand(int range)
{
    g_seed = (g_seed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (int)((g_seed >> 8) % (unsigned long)range);
}

/* ---- helpers ---- */

/* Build a Graph by hand from m arcs (from[i] -> to[i], weight w[i]).
 * Arcs of one node keep the order they have in the input. 0 = ok. */
static int make_graph(Graph *g, int n, int m,
                      const int *from, const int *to, const int *w)
{
    int i, u;
    int *fill;

    memset(g, 0, sizeof *g);
    g->rows = 1;
    g->cols = n;
    g->n = n;
    g->n_walkable = n;
    g->n_arcs = m;
    g->first  = (int *)calloc((size_t)n + 1, sizeof(int));
    g->to     = (int *)malloc(((size_t)m + 1) * sizeof(int));
    g->weight = (int *)malloc(((size_t)m + 1) * sizeof(int));
    fill      = (int *)malloc((size_t)n * sizeof(int));
    if (g->first == NULL || g->to == NULL || g->weight == NULL || fill == NULL) {
        free(fill);
        graph_free(g);
        return -1;
    }
    for (i = 0; i < m; i++) {
        g->first[from[i] + 1]++;
    }
    for (u = 0; u < n; u++) {
        g->first[u + 1] += g->first[u];
    }
    memcpy(fill, g->first, (size_t)n * sizeof(int));
    for (i = 0; i < m; i++) {
        u = from[i];
        g->to[fill[u]]     = to[i];
        g->weight[fill[u]] = w[i];
        fill[u]++;
    }
    free(fill);
    return 0;
}

/* A rows x cols map with every cell '.'. Returns 0 on success. */
static int make_open(Problem *p, int rows, int cols)
{
    problem_init(p);
    p->rows = rows;
    p->cols = cols;
    p->cells = (char *)malloc((size_t)rows * (size_t)cols);
    if (p->cells == NULL) {
        return -1;
    }
    memset(p->cells, '.', (size_t)rows * (size_t)cols);
    return 0;
}

/* How many nodes have a different distance in a and b (-1 if sizes differ). */
static int differing(const ShortestPaths *a, const ShortestPaths *b)
{
    int v, count = 0;

    if (a->n != b->n) {
        return -1;
    }
    for (v = 0; v < a->n; v++) {
        if (a->dist[v] != b->dist[v]) {
            count++;
        }
    }
    return count;
}

static int finite_count(const ShortestPaths *sp)
{
    int v, count = 0;

    for (v = 0; v < sp->n; v++) {
        if (sp->dist[v] < DIST_INF) {
            count++;
        }
    }
    return count;
}

/* The scan formula from the top of this file. */
static long scan_comparisons(long n, long r)
{
    return r * n - r * (r - 1) / 2 + (n - r);
}

/* Total weight of walking path[0..len-1] using real arcs, or -1. */
static int path_cost(const Graph *g, const int *path, int len)
{
    int i, a, found, total = 0;

    for (i = 0; i + 1 < len; i++) {
        found = 0;
        for (a = g->first[path[i]]; a < g->first[path[i] + 1]; a++) {
            if (g->to[a] == path[i + 1]) {
                total += g->weight[a];
                found = 1;
                break;
            }
        }
        if (!found) {
            return -1;
        }
    }
    return total;
}

/* ================================================================== */
/* Dijkstra with array scan                                           */
/* ================================================================== */

/*
 * 6-node graph (node: arcs as ->target(weight)):
 *   0: ->1(4) ->2(1)    1: ->3(1)    2: ->1(2) ->3(5)    3: ->4(3)    4, 5: none
 * Scan rounds (unfinished nodes examined = comparisons):
 *   round 1: 6 nodes, pick 0      round 2: 5 nodes, pick 2
 *   round 3: 4 nodes, pick 1      round 4: 3 nodes, pick 3
 *   round 5: 2 nodes, pick 4      round 6: 1 node (5), none reachable, stop
 *   comparisons = 6+5+4+3+2+1 = 21   (formula: R=5, n=6: 30 - 10 + 1 = 21)
 * Arcs tested: node 0 has 2, node 2 has 2, node 1 has 1, node 3 has 1 = 6.
 * aux_bytes = dist + prev (6 ints each) + 6 flag bytes.
 */
static void group_scan_hand(void)
{
    int from[6] = { 0, 0, 2, 2, 1, 3 };
    int to[6]   = { 1, 2, 1, 3, 3, 4 };
    int w[6]    = { 4, 1, 2, 5, 1, 3 };
    int want_path[5] = { 0, 2, 1, 3, 4 };
    int sf[3] = { 0, 0, 0 }, st[3] = { 1, 2, 3 }, sw[3] = { 1, 2, 3 };
    int path[8], i, bad, len;
    Graph g;
    ShortestPaths sp;

    harness_suite("dijkstra scan: hand-made graphs");

    check_long("build the 6-node graph", 0, make_graph(&g, 6, 6, from, to, w));
    metrics_reset();
    check_long("scan from node 0", 0, dijkstra_scan(&g, 0, &sp));
    check_long("dist[0]", 0, sp.dist[0]);
    check_long("dist[1] is 3, not the direct arc 4", 3, sp.dist[1]);
    check_long("dist[2]", 1, sp.dist[2]);
    check_long("dist[3]", 4, sp.dist[3]);
    check_long("dist[4]", 7, sp.dist[4]);
    check_long("dist[5] (isolated) is DIST_INF", DIST_INF, sp.dist[5]);
    check_long("prev[1] is 2", 2, sp.prev[1]);
    check_long("prev[3] is 1", 1, sp.prev[3]);
    check_long("prev[4] is 3", 3, sp.prev[4]);
    check_long("prev[5]", -1, sp.prev[5]);
    check_long("comparisons (hand count 6+5+4+3+2+1)", 21, (long)g_metrics.comparisons);
    check_long("comparisons match the formula", scan_comparisons(6, 5),
               (long)g_metrics.comparisons);
    check_long("relaxations = arcs of reached nodes", 6, (long)g_metrics.relaxations);
    check_long("no heap pushes", 0, (long)g_metrics.heap_pushes);
    check_long("no heap pops", 0, (long)g_metrics.heap_pops);
    check_long("aux_bytes = dist + prev + 6 flags",
               (long)(2 * 6 * sizeof(int) + 6), (long)g_metrics.aux_bytes);
    len = sp_path(&sp, 4, path, 8);
    check_long("path 0 -> 4: number of nodes", 5, len);
    bad = 0;
    for (i = 0; i < 5; i++) {
        if (path[i] != want_path[i]) {
            bad++;
        }
    }
    check_long("path 0 -> 4 is 0,2,1,3,4", 0, bad);
    check_long("path cost equals dist[4]", 7, path_cost(&g, path, len));
    sp_free(&sp);
    graph_free(&g);

    /* star: 0 -> 1 (1), 0 -> 2 (2), 0 -> 3 (3); all 4 reachable.
     * rounds examine 4, 3, 2, 1 nodes = 10 comparisons; 3 arcs tested. */
    check_long("build the 4-node star", 0, make_graph(&g, 4, 3, sf, st, sw));
    metrics_reset();
    check_long("star: run", 0, dijkstra_scan(&g, 0, &sp));
    check_long("star: dist[3]", 3, sp.dist[3]);
    check_long("star: comparisons 4+3+2+1", 10, (long)g_metrics.comparisons);
    check_long("star: relaxations", 3, (long)g_metrics.relaxations);
    sp_free(&sp);
    graph_free(&g);
}

static void group_scan_special(void)
{
    Graph g;
    ShortestPaths sp;

    harness_suite("dijkstra scan: special cases and invalid use");

    {   /* one node, no arcs: one scan of 1 node, then nothing left */
        int f[1] = { 0 }, t[1] = { 0 }, w[1] = { 0 };
        make_graph(&g, 1, 0, f, t, w);
        metrics_reset();
        check_long("1 node: run", 0, dijkstra_scan(&g, 0, &sp));
        check_long("1 node: dist[0]", 0, sp.dist[0]);
        check_long("1 node: comparisons", 1, (long)g_metrics.comparisons);
        check_long("1 node: relaxations", 0, (long)g_metrics.relaxations);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* zero-weight arcs and a parallel arc */
        int f[4] = { 0, 1, 0, 0 }, t[4] = { 1, 2, 2, 2 }, w[4] = { 0, 0, 5, 7 };
        make_graph(&g, 3, 4, f, t, w);
        check_long("zero weights: run", 0, dijkstra_scan(&g, 0, &sp));
        check_long("zero weights: dist[2] is 0, not 5", 0, sp.dist[2]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* source in the middle of a one-way chain: node 0 unreachable */
        int f[2] = { 0, 1 }, t[2] = { 1, 2 }, w[2] = { 1, 1 };
        make_graph(&g, 3, 2, f, t, w);
        check_long("source in the middle: run", 0, dijkstra_scan(&g, 1, &sp));
        check_long("source in the middle: node 0 unreachable", DIST_INF, sp.dist[0]);
        check_long("source in the middle: dist[2]", 1, sp.dist[2]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* invalid weights */
        int f[1] = { 0 }, t[1] = { 1 }, wbad[1] = { DIJKSTRA_MAX_WEIGHT + 1 };
        int wneg[1] = { -1 }, wmax[1] = { DIJKSTRA_MAX_WEIGHT };

        make_graph(&g, 2, 1, f, t, wbad);
        check_long("weight 1001: rejected", -1, dijkstra_scan(&g, 0, &sp));
        check_long("weight 1001: sp is empty", 1, sp.dist == NULL && sp.prev == NULL);
        graph_free(&g);
        make_graph(&g, 2, 1, f, t, wneg);
        check_long("negative weight: rejected", -1, dijkstra_scan(&g, 0, &sp));
        graph_free(&g);
        make_graph(&g, 2, 1, f, t, wmax);
        check_long("weight 1000: accepted", 0, dijkstra_scan(&g, 0, &sp));
        check_long("weight 1000: dist[1]", 1000, sp.dist[1]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* invalid arguments */
        int f[1] = { 0 }, t[1] = { 1 }, w[1] = { 1 };
        Graph zero;

        memset(&zero, 0, sizeof zero);
        make_graph(&g, 2, 1, f, t, w);
        check_long("NULL graph", -1, dijkstra_scan(NULL, 0, &sp));
        check_long("NULL result struct", -1, dijkstra_scan(&g, 0, NULL));
        check_long("empty graph (n = 0)", -1, dijkstra_scan(&zero, 0, &sp));
        check_long("source -1", -1, dijkstra_scan(&g, -1, &sp));
        check_long("source = n", -1, dijkstra_scan(&g, 2, &sp));
        check_long("good run after failures", 0, dijkstra_scan(&g, 0, &sp));
        sp_free(&sp);
        graph_free(&g);
    }
}

/*
 * sample1.txt from either robot start: 48 nodes, R = 38 reachable (the 8
 * shelves and 2 obstacles are not), 94 arcs, all of them tested once.
 *   comparisons = 38*48 - 38*37/2 + 10 = 1824 - 703 + 10 = 1131
 * The open 30 x 30 grid: n = R = 900, comparisons = 900*901/2 = 405450;
 * arcs = 2 * (30*29 + 29*30) = 3480.
 */
static void group_scan_maps(void)
{
    Problem p;
    Graph g;
    ShortestPaths hp, sc;
    int r1, r2;

    harness_suite("dijkstra scan: maps, against the heap version");

    check_long("parse sample1", PARSE_OK,
               parse_problem_file("data/small/sample1.txt", &p, NULL, 0));
    check_long("graph_build ok", 0, graph_build(&p, &g));
    r1 = graph_node(&g, 2, 1);
    r2 = graph_node(&g, 4, 5);

    check_long("sample1 robot 1: heap run", 0, dijkstra(&g, r1, &hp));
    metrics_reset();
    check_long("sample1 robot 1: scan run", 0, dijkstra_scan(&g, r1, &sc));
    check_long("sample1 robot 1: same distances as heap", 0, differing(&hp, &sc));
    check_long("sample1 robot 1: comparisons", 1131, (long)g_metrics.comparisons);
    check_long("sample1 robot 1: relaxations", 94, (long)g_metrics.relaxations);
    check_long("sample1 robot 1: no heap pushes", 0, (long)g_metrics.heap_pushes);
    check_long("sample1 robot 1: dist to T1 (2,6)", 13, sc.dist[graph_node(&g, 2, 6)]);
    sp_free(&hp);
    sp_free(&sc);

    check_long("sample1 robot 2: heap run", 0, dijkstra(&g, r2, &hp));
    metrics_reset();
    check_long("sample1 robot 2: scan run", 0, dijkstra_scan(&g, r2, &sc));
    check_long("sample1 robot 2: same distances as heap", 0, differing(&hp, &sc));
    check_long("sample1 robot 2: comparisons", 1131, (long)g_metrics.comparisons);
    check_long("sample1 robot 2: dist to T3 (5,6)", 2, sc.dist[graph_node(&g, 5, 6)]);
    sp_free(&hp);
    sp_free(&sc);
    graph_free(&g);
    problem_free(&p);

    if (make_open(&p, 30, 30) == 0) {
        graph_build(&p, &g);
        check_long("open 30x30: heap run", 0, dijkstra(&g, 0, &hp));
        metrics_reset();
        check_long("open 30x30: scan run", 0, dijkstra_scan(&g, 0, &sc));
        check_long("open 30x30: same distances as heap", 0, differing(&hp, &sc));
        check_long("open 30x30: far corner is 58", 58, sc.dist[graph_node(&g, 29, 29)]);
        check_long("open 30x30: comparisons 900*901/2", 405450, (long)g_metrics.comparisons);
        check_long("open 30x30: relaxations (all arcs)", 3480, (long)g_metrics.relaxations);
        sp_free(&hp);
        sp_free(&sc);
        graph_free(&g);
    }
    problem_free(&p);
}

void test_dijkstra_scan(void)
{
    group_scan_hand();
    group_scan_special();
    group_scan_maps();
}

/* ================================================================== */
/* Bellman-Ford                                                       */
/* ================================================================== */

/*
 * Same 6-node graph as above, from node 0. Hand run, node order 0..5,
 * every reached node has a finite distance when it is processed, so each
 * pass tests all 6 arcs that start at reachable nodes:
 *   pass 1: 0->1: 4; 0->2: 1; 1->3: 5; 2->1: 3 (better); 2->3: 6 (no);
 *           3->4: 8                                    changed
 *   pass 2: 1->3: 3+1 = 4 (better); 3->4: 7 (better); others no  changed
 *   pass 3: nothing improves                           stop
 *   relaxations = 3 passes * 6 = 18; result 0,3,1,4,7,INF.
 */
static void group_bf_hand(void)
{
    int from[6] = { 0, 0, 2, 2, 1, 3 };
    int to[6]   = { 1, 2, 1, 3, 3, 4 };
    int w[6]    = { 4, 1, 2, 5, 1, 3 };
    Graph g;
    ShortestPaths sp;

    harness_suite("bellman-ford: hand-made graphs");

    check_long("build the 6-node graph", 0, make_graph(&g, 6, 6, from, to, w));
    metrics_reset();
    check_long("bellman-ford from node 0", 0, bellman_ford(&g, 0, &sp));
    check_long("dist[0]", 0, sp.dist[0]);
    check_long("dist[1]", 3, sp.dist[1]);
    check_long("dist[2]", 1, sp.dist[2]);
    check_long("dist[3]", 4, sp.dist[3]);
    check_long("dist[4]", 7, sp.dist[4]);
    check_long("dist[5] (isolated) is DIST_INF", DIST_INF, sp.dist[5]);
    check_long("prev[1] is 2", 2, sp.prev[1]);
    check_long("prev[4] is 3", 3, sp.prev[4]);
    check_long("relaxations (3 passes * 6 arcs)", 18, (long)g_metrics.relaxations);
    check_long("no comparisons", 0, (long)g_metrics.comparisons);
    check_long("no heap pushes", 0, (long)g_metrics.heap_pushes);
    check_long("aux_bytes = dist + prev", (long)(2 * 6 * sizeof(int)),
               (long)g_metrics.aux_bytes);
    sp_free(&sp);
    graph_free(&g);
}

/*
 * Negative arc: 0->1 (4), 0->2 (5), 2->1 (-3), 1->3 (2).
 * Best route to 1 is 0->2->1 = 2, to 3 is 4. Dijkstra cannot be trusted
 * here and refuses it. Hand run (arcs per node: 0: [->1, ->2], 1: [->3],
 * 2: [->1]):
 *   pass 1: 1=4, 2=5, 3=6 (from 1 at 4), 1=2 (from 2)       changed
 *   pass 2: 3=4 (from 1 at 2); the rest unchanged            changed
 *   pass 3: nothing                                          stop
 *   relaxations = 3 passes * 4 arcs = 12.
 */
static void group_bf_negative(void)
{
    int from[4] = { 0, 0, 2, 1 }, to[4] = { 1, 2, 1, 3 }, w[4] = { 4, 5, -3, 2 };
    int want[4] = { 0, 2, 5, 4 };
    int path[6], len, bad, i;
    Graph g;
    ShortestPaths sp;

    harness_suite("bellman-ford: negative weights and cycles");

    make_graph(&g, 4, 4, from, to, w);
    check_long("dijkstra refuses a negative weight", -1, dijkstra(&g, 0, &sp));
    metrics_reset();
    check_long("bellman-ford: run", 0, bellman_ford(&g, 0, &sp));
    bad = 0;
    for (i = 0; i < 4; i++) {
        if (sp.dist[i] != want[i]) {
            bad++;
        }
    }
    check_long("distances are 0,2,5,4", 0, bad);
    check_long("relaxations (3 passes * 4 arcs)", 12, (long)g_metrics.relaxations);
    len = sp_path(&sp, 3, path, 6);
    check_long("path 0 -> 3 has 4 nodes (0,2,1,3)", 4, len);
    check_long("path cost is 4", 4, path_cost(&g, path, len));
    sp_free(&sp);
    graph_free(&g);

    {   /* negative cycle reachable: 0->1 (1), 1->2 (-2), 2->1 (1): cycle -1 */
        int f[3] = { 0, 1, 2 }, t[3] = { 1, 2, 1 }, ww[3] = { 1, -2, 1 };
        make_graph(&g, 3, 3, f, t, ww);
        check_long("negative cycle: detected", BF_NEG_CYCLE, bellman_ford(&g, 0, &sp));
        sp_free(&sp);
        check_long("negative cycle: sp_free is safe", 1, sp.dist == NULL);
        graph_free(&g);
    }
    {   /* the same cycle, but the source cannot reach it */
        int f[3] = { 1, 2, 1 }, t[3] = { 2, 1, 3 }, ww[3] = { -2, 1, 0 };
        make_graph(&g, 4, 3, f, t, ww);
        check_long("unreachable negative cycle: ok", 0, bellman_ford(&g, 0, &sp));
        check_long("unreachable negative cycle: node 1 is INF", DIST_INF, sp.dist[1]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* negative self-loop, and a harmless zero self-loop */
        int f[1] = { 0 }, t[1] = { 0 }, wneg[1] = { -1 }, wzero[1] = { 0 };
        make_graph(&g, 1, 1, f, t, wneg);
        check_long("negative self-loop: detected", BF_NEG_CYCLE, bellman_ford(&g, 0, &sp));
        sp_free(&sp);
        graph_free(&g);
        make_graph(&g, 1, 1, f, t, wzero);
        check_long("zero self-loop: ok", 0, bellman_ford(&g, 0, &sp));
        check_long("zero self-loop: dist[0]", 0, sp.dist[0]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* the largest negative weight */
        int f[1] = { 0 }, t[1] = { 1 }, ww[1] = { -BF_MAX_WEIGHT };
        make_graph(&g, 2, 1, f, t, ww);
        check_long("weight -1000: run", 0, bellman_ford(&g, 0, &sp));
        check_long("weight -1000: dist[1]", -1000, sp.dist[1]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* ring of 1000 nodes, every arc -1000: the overflow guard must stop it */
        int f[1000], t[1000], ww[1000], i2;
        for (i2 = 0; i2 < 1000; i2++) {
            f[i2] = i2;
            t[i2] = (i2 + 1) % 1000;
            ww[i2] = -BF_MAX_WEIGHT;
        }
        make_graph(&g, 1000, 1000, f, t, ww);
        check_long("negative ring of 1000: detected", BF_NEG_CYCLE,
                   bellman_ford(&g, 0, &sp));
        sp_free(&sp);
        graph_free(&g);
    }
}

static void group_bf_invalid(void)
{
    int f[1] = { 0 }, t[1] = { 1 }, wbig[1] = { BF_MAX_WEIGHT + 1 };
    int wlow[1] = { -BF_MAX_WEIGHT - 1 }, w[1] = { 1 };
    Graph g, zero;
    ShortestPaths sp;

    harness_suite("bellman-ford: invalid use");

    memset(&zero, 0, sizeof zero);
    make_graph(&g, 2, 1, f, t, w);
    check_long("NULL graph", -1, bellman_ford(NULL, 0, &sp));
    check_long("NULL result struct", -1, bellman_ford(&g, 0, NULL));
    check_long("empty graph (n = 0)", -1, bellman_ford(&zero, 0, &sp));
    check_long("source -1", -1, bellman_ford(&g, -1, &sp));
    check_long("source = n", -1, bellman_ford(&g, 2, &sp));
    check_long("failed run leaves sp empty", 1, sp.dist == NULL && sp.prev == NULL);
    graph_free(&g);
    make_graph(&g, 2, 1, f, t, wbig);
    check_long("weight 1001: rejected", -1, bellman_ford(&g, 0, &sp));
    graph_free(&g);
    make_graph(&g, 2, 1, f, t, wlow);
    check_long("weight -1001: rejected", -1, bellman_ford(&g, 0, &sp));
    graph_free(&g);
}

/*
 * Open 60 x 60 grid from the top-left corner. Node (r, c) has id 60r + c.
 * Its best predecessor is (r-1, c) or (r, c-1), both with a smaller id, so
 * in pass 1 every node is final before it is processed and every one of the
 * arcs is tested. Pass 2 changes nothing. arcs = 2*(60*59 + 59*60) = 14160,
 * relaxations = 2 * 14160 = 28320, and the far corner is 118 away.
 * From the opposite corner the arcs point against the node order, so it
 * needs many passes: only the distances are checked there.
 */
static void group_bf_grid(void)
{
    Problem p;
    Graph g;
    ShortestPaths hp, bf;

    harness_suite("bellman-ford: open grids");

    if (make_open(&p, 60, 60) == 0) {
        graph_build(&p, &g);
        metrics_reset();
        check_long("60x60 from top-left: run", 0, bellman_ford(&g, 0, &bf));
        check_long("60x60 from top-left: far corner is 118", 118,
                   bf.dist[graph_node(&g, 59, 59)]);
        check_long("60x60 from top-left: relaxations (2 * 14160)", 28320,
                   (long)g_metrics.relaxations);
        sp_free(&bf);
        graph_free(&g);
    }
    problem_free(&p);

    if (make_open(&p, 40, 40) == 0) {
        graph_build(&p, &g);
        check_long("40x40 from bottom-right: heap run", 0,
                   dijkstra(&g, graph_node(&g, 39, 39), &hp));
        metrics_reset();
        check_long("40x40 from bottom-right: bf run", 0,
                   bellman_ford(&g, graph_node(&g, 39, 39), &bf));
        check_long("40x40 from bottom-right: same distances", 0, differing(&hp, &bf));
        check_long("40x40 from bottom-right: far corner is 78", 78,
                   bf.dist[0]);
        check_long("40x40 from bottom-right: more work than dijkstra (> 6240)", 1,
                   g_metrics.relaxations > 6240);
        sp_free(&hp);
        sp_free(&bf);
        graph_free(&g);
    }
    problem_free(&p);
}

static void group_bf_sample(void)
{
    Problem p;
    Graph g;
    ShortestPaths hp, bf;
    int r1;

    harness_suite("bellman-ford: data/small/sample1.txt");

    check_long("parse sample1", PARSE_OK,
               parse_problem_file("data/small/sample1.txt", &p, NULL, 0));
    check_long("graph_build ok", 0, graph_build(&p, &g));
    r1 = graph_node(&g, 2, 1);
    check_long("robot 1: heap run", 0, dijkstra(&g, r1, &hp));
    metrics_reset();
    check_long("robot 1: bellman-ford run", 0, bellman_ford(&g, r1, &bf));
    check_long("robot 1: same distances as heap", 0, differing(&hp, &bf));
    check_long("robot 1: reaches 38 cells", 38, finite_count(&bf));
    check_long("robot 1: more relaxations than dijkstra's 94", 1,
               g_metrics.relaxations > 94);
    sp_free(&hp);
    sp_free(&bf);
    graph_free(&g);
    problem_free(&p);
}

/* 200 random non-negative graphs: heap, scan and Bellman-Ford must agree,
 * and the scan's counters must follow their rules. */
static void group_random_three_way(void)
{
    int from[256], to[256], w[256];
    Graph g;
    ShortestPaths hp, sc, bf;
    int trial, n, m, i, source, r;
    int bad_rc = 0, bad_scan = 0, bad_bf = 0, bad_count = 0;
    unsigned long heap_relax, scan_relax, scan_comps, scan_pushes;

    harness_suite("three algorithms agree on 200 random graphs");

    g_seed = 4242UL;
    for (trial = 0; trial < 200; trial++) {
        n = 2 + next_rand(59);               /* 2 .. 60 nodes   */
        m = next_rand(4 * n + 1);            /* 0 .. 4n arcs    */
        for (i = 0; i < m; i++) {
            from[i] = next_rand(n);
            to[i]   = next_rand(n);
            w[i]    = next_rand(21);         /* weights 0 .. 20 */
        }
        source = next_rand(n);
        if (make_graph(&g, n, m, from, to, w) != 0) {
            bad_rc++;
            continue;
        }
        memset(&bf, 0, sizeof bf);           /* bellman_ford may not run */
        metrics_reset();
        i = dijkstra(&g, source, &hp);
        heap_relax = g_metrics.relaxations;
        metrics_reset();
        r = dijkstra_scan(&g, source, &sc);
        scan_relax  = g_metrics.relaxations;
        scan_comps  = g_metrics.comparisons;
        scan_pushes = g_metrics.heap_pushes;
        if (i != 0 || r != 0 || bellman_ford(&g, source, &bf) != 0) {
            bad_rc++;
        } else {
            bad_scan += differing(&hp, &sc);
            bad_bf   += differing(&hp, &bf);
            r = finite_count(&hp);
            if (scan_relax != heap_relax || scan_pushes != 0
                || scan_comps != (unsigned long)scan_comparisons(n, r)) {
                bad_count++;
            }
        }
        sp_free(&hp);
        sp_free(&sc);
        sp_free(&bf);
        graph_free(&g);
    }
    check_long("200 random graphs: every run succeeded", 0, bad_rc);
    check_long("200 random graphs: scan distances wrong", 0, bad_scan);
    check_long("200 random graphs: bellman-ford distances wrong", 0, bad_bf);
    check_long("200 random graphs: scan counter rules broken", 0, bad_count);
}

/*
 * Negative weights without a negative cycle, checked without trusting
 * Bellman-Ford itself. Give every node a number h[v] (0..10) and replace
 * each weight w by  w2 = w + h[u] - h[v]  (can be negative). The sum along
 * any path then changes by exactly h[source] - h[target], so for every
 * reachable v:
 *     dist_with_w2[v] = dist_with_w[v] + h[source] - h[v]
 * and a cycle keeps its weight, so there is still no negative cycle.
 * dist_with_w comes from heap Dijkstra (all w are >= 0).
 */
static void group_negative_random(void)
{
    int from[256], to[256], w[256], w2[256], h[64];
    Graph g1, g2;
    ShortestPaths hp, bf;
    int trial, n, m, i, source, v;
    int bad_rc = 0, bad_dist = 0, saw_negative = 0;

    harness_suite("bellman-ford: 200 random graphs with negative arcs");

    g_seed = 99UL;
    for (trial = 0; trial < 200; trial++) {
        n = 2 + next_rand(39);               /* 2 .. 40 nodes   */
        m = next_rand(4 * n + 1);
        for (i = 0; i < n; i++) {
            h[i] = next_rand(11);
        }
        for (i = 0; i < m; i++) {
            from[i] = next_rand(n);
            to[i]   = next_rand(n);
            w[i]    = next_rand(21);
            w2[i]   = w[i] + h[from[i]] - h[to[i]];
            if (w2[i] < 0) {
                saw_negative = 1;
            }
        }
        source = next_rand(n);
        if (make_graph(&g1, n, m, from, to, w) != 0) {
            bad_rc++;
            continue;
        }
        if (make_graph(&g2, n, m, from, to, w2) != 0) {
            graph_free(&g1);
            bad_rc++;
            continue;
        }
        memset(&bf, 0, sizeof bf);           /* bellman_ford may not run */
        if (dijkstra(&g1, source, &hp) != 0 || bellman_ford(&g2, source, &bf) != 0) {
            bad_rc++;
        } else {
            for (v = 0; v < n; v++) {
                if (hp.dist[v] >= DIST_INF) {
                    if (bf.dist[v] != DIST_INF) {
                        bad_dist++;
                    }
                } else if (bf.dist[v] != hp.dist[v] + h[source] - h[v]) {
                    bad_dist++;
                }
            }
        }
        sp_free(&hp);
        sp_free(&bf);
        graph_free(&g1);
        graph_free(&g2);
    }
    check_long("negative arcs really occurred", 1, saw_negative);
    check_long("every run succeeded (no false negative cycle)", 0, bad_rc);
    check_long("distances differ from the potential formula", 0, bad_dist);
}

void test_bellman_ford(void)
{
    group_bf_hand();
    group_bf_negative();
    group_bf_invalid();
    group_bf_grid();
    group_bf_sample();
    group_random_three_way();
    group_negative_random();
}
