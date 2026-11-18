/*
 * test_dijkstra.c - tests for dijkstra.c (item 5).
 *
 * The expected numbers were worked out by hand, not copied from the code:
 *  - the 6-node weighted graph and its counters (see group_weighted),
 *  - the distances on data/small/sample1.txt (see group_sample),
 *  - snake and open-grid distances from the shape of the map,
 * and 200 random graphs are checked against a simple Bellman-Ford written
 * inside this file (an independent way to get the right answer).
 *
 * Run from the project root (make test does this).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dijkstra.h"
#include "graph.h"
#include "harness.h"
#include "heap.h"
#include "io.h"
#include "metrics.h"
#include "problem.h"
#include "tests.h"

#define ONE_ROBOT  "ROBOT 5 10\n"
#define ONE_TASK   "TASK 1 1 1\n"

/* ---- small repeatable pseudo-random numbers ---- */
static unsigned long g_seed = 777UL;

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

/* Build a graph by hand and run Dijkstra on it. Returns dijkstra's result,
 * or -2 if the graph could not be built. The caller frees g and sp. */
static int run_hand(Graph *g, ShortestPaths *sp, int n, int m,
                    const int *from, const int *to, const int *w, int source)
{
    memset(sp, 0, sizeof *sp);
    if (make_graph(g, n, m, from, to, w) != 0) {
        return -2;
    }
    return dijkstra(g, source, sp);
}

/* Parse a map text, build its graph. 0 = ok. */
static int build_text(const char *text, Problem *p, Graph *g)
{
    char err[200];

    if (parse_problem_text(text, p, err, sizeof err) != PARSE_OK) {
        memset(g, 0, sizeof *g);
        return -1;
    }
    return graph_build(p, g);
}

/* How many nodes have a finite distance. */
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

/* Self-check of a result, independent of the expected numbers:
 *  1. dist[source] is 0,
 *  2. no arc could still shorten any distance,
 *  3. every reached node's prev is a node with a real arc that explains
 *     dist exactly, and unreached nodes have prev -1.
 * Returns the number of broken rules (0 = all fine). */
static int count_violations(const Graph *g, const ShortestPaths *sp, int source)
{
    int u, v, a, found, bad = 0;

    if (sp->dist[source] != 0) {
        bad++;
    }
    for (u = 0; u < g->n; u++) {
        if (sp->dist[u] >= DIST_INF) {
            continue;
        }
        for (a = g->first[u]; a < g->first[u + 1]; a++) {
            if (sp->dist[g->to[a]] > sp->dist[u] + g->weight[a]) {
                bad++;
            }
        }
    }
    for (v = 0; v < g->n; v++) {
        if (v == source) {
            continue;
        }
        if (sp->dist[v] >= DIST_INF) {
            if (sp->prev[v] != -1) {
                bad++;
            }
            continue;
        }
        u = sp->prev[v];
        if (u < 0 || u >= g->n || sp->dist[u] >= DIST_INF) {
            bad++;
            continue;
        }
        found = 0;
        for (a = g->first[u]; a < g->first[u + 1]; a++) {
            if (g->to[a] == v && sp->dist[u] + g->weight[a] == sp->dist[v]) {
                found = 1;
            }
        }
        if (!found) {
            bad++;
        }
    }
    return bad;
}

/* Total weight of walking path[0..len-1] using real arcs, or -1 if some
 * step is not an arc. (On the grid every weight is 1 and arcs are unique.) */
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

/* Reference answer: plain Bellman-Ford (repeat "try every arc" n-1 times). */
static void reference_distances(int n, int m, const int *from, const int *to,
                                const int *w, int source, int *ref)
{
    int i, pass;

    for (i = 0; i < n; i++) {
        ref[i] = DIST_INF;
    }
    ref[source] = 0;
    for (pass = 0; pass < n - 1; pass++) {
        for (i = 0; i < m; i++) {
            if (ref[from[i]] < DIST_INF
                && ref[from[i]] + w[i] < ref[to[i]]) {
                ref[to[i]] = ref[from[i]] + w[i];
            }
        }
    }
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

/* A one-corridor "snake": even rows are open, odd rows are walls with one
 * gap, at the right end for rows 1, 5, 9 ... and at the left end for rows
 * 3, 7, 11 ... Start at (0,0). rows must be odd. Returns 0 on success. */
static int make_snake(Problem *p, int rows, int cols)
{
    int r, gap;

    problem_init(p);
    p->rows = rows;
    p->cols = cols;
    p->cells = (char *)malloc((size_t)rows * (size_t)cols);
    if (p->cells == NULL) {
        return -1;
    }
    memset(p->cells, '#', (size_t)rows * (size_t)cols);
    for (r = 0; r < rows; r += 2) {
        memset(p->cells + (size_t)r * (size_t)cols, '.', (size_t)cols);
    }
    for (r = 1; r < rows; r += 2) {
        gap = ((r / 2) % 2 == 0) ? cols - 1 : 0;
        p->cells[(size_t)r * (size_t)cols + (size_t)gap] = '.';
    }
    return 0;
}

/* ---------------------------------------------------------------- */

/*
 * The 6-node graph used here (node: arcs as ->target(weight)):
 *   0: ->1(4) ->2(1)     1: ->3(1)     2: ->1(2) ->3(5)
 *   3: ->4(3)            4: none       5: none (isolated)
 * Hand run from node 0 (pair = (distance, node)):
 *   push (0,0).                                        pushes 1
 *   pop (0,0): relax 0->1: dist1=4, push (4,1)         pushes 2
 *              relax 0->2: dist2=1, push (1,2)         pushes 3
 *   pop (1,2): relax 2->1: 1+2=3 < 4: dist1=3, push (3,1)   pushes 4
 *              relax 2->3: dist3=6, push (6,3)         pushes 5
 *   pop (3,1): relax 1->3: 3+1=4 < 6: dist3=4, push (4,3)   pushes 6
 *   pop (4,1) and (4,3) (either order): (4,1) is stale (4 > 3);
 *              (4,3) relaxes 3->4: dist4=7, push (7,4)      pushes 7
 *   pop (6,3): stale.  pop (7,4): no arcs.
 * Result: dist = 0,3,1,4,7,INF. Totals: 7 pushes, 7 pops, 6 relaxations
 * (every arc once), one stale check per pop = 7 comparisons plus the
 * heap's own. The heap capacity is 6 + 1 = 7, so it is exactly full.
 */
static void group_weighted(void)
{
    int from[6] = { 0, 0, 2, 2, 1, 3 };
    int to[6]   = { 1, 2, 1, 3, 3, 4 };
    int w[6]    = { 4, 1, 2, 5, 1, 3 };
    int want_path[5] = { 0, 2, 1, 3, 4 };
    int path[8];
    Graph g;
    ShortestPaths sp;
    int rc, len, i, bad;

    harness_suite("dijkstra: hand-made weighted graph");

    memset(&sp, 0, sizeof sp);
    check_long("build the 6-node graph", 0, make_graph(&g, 6, 6, from, to, w));
    metrics_reset();
    rc = dijkstra(&g, 0, &sp);
    check_long("dijkstra from node 0", 0, rc);

    check_long("dist[0]", 0, sp.dist[0]);
    check_long("dist[1] is 3, not the direct arc 4", 3, sp.dist[1]);
    check_long("dist[2]", 1, sp.dist[2]);
    check_long("dist[3]", 4, sp.dist[3]);
    check_long("dist[4]", 7, sp.dist[4]);
    check_long("dist[5] (isolated) is DIST_INF", DIST_INF, sp.dist[5]);
    check_long("prev[0]", -1, sp.prev[0]);
    check_long("prev[1] is 2", 2, sp.prev[1]);
    check_long("prev[2] is 0", 0, sp.prev[2]);
    check_long("prev[3] is 1", 1, sp.prev[3]);
    check_long("prev[4] is 3", 3, sp.prev[4]);
    check_long("prev[5]", -1, sp.prev[5]);
    check_long("sp.n", 6, sp.n);
    check_long("sp.source", 0, sp.source);

    check_long("heap_pushes (hand count)", 7, (long)g_metrics.heap_pushes);
    check_long("heap_pops (hand count)", 7, (long)g_metrics.heap_pops);
    check_long("relaxations = one per arc", 6, (long)g_metrics.relaxations);
    check_long("comparisons >= one stale check per pop", 1,
               g_metrics.comparisons >= 7);
    check_long("aux_bytes = dist + prev + heap of 7",
               (long)(2 * 6 * sizeof(int) + 7 * sizeof(HeapItem)),
               (long)g_metrics.aux_bytes);

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
    check_long("path to unreachable node 5", 0, sp_path(&sp, 5, path, 8));
    check_long("path to the source itself", 1, sp_path(&sp, 0, path, 8));
    check_long("buffer too small (4 < 5)", -1, sp_path(&sp, 4, path, 4));
    check_long("target above the last node", -1, sp_path(&sp, 6, path, 8));
    check_long("negative target", -1, sp_path(&sp, -1, path, 8));
    check_long("NULL output buffer", -1, sp_path(&sp, 4, NULL, 8));
    check_long("self-check: no violations", 0, count_violations(&g, &sp, 0));

    sp_free(&sp);
    graph_free(&g);
}

static void group_special_weights(void)
{
    Graph g;
    ShortestPaths sp;
    int path[8];
    int len;

    harness_suite("dijkstra: special weights and shapes");

    {   /* zero-weight arcs: the long way is free */
        int f[3] = { 0, 1, 0 }, t[3] = { 1, 2, 2 }, w[3] = { 0, 0, 5 };
        check_long("zero weights: run", 0, run_hand(&g, &sp, 3, 3, f, t, w, 0));
        check_long("zero weights: dist[1]", 0, sp.dist[1]);
        check_long("zero weights: dist[2] is 0, not 5", 0, sp.dist[2]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* two arcs between the same nodes: the cheaper one wins */
        int f[2] = { 0, 0 }, t[2] = { 1, 1 }, w[2] = { 5, 2 };
        check_long("parallel arcs: run", 0, run_hand(&g, &sp, 2, 2, f, t, w, 0));
        check_long("parallel arcs: dist[1] is 2", 2, sp.dist[1]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* the largest allowed weight */
        int f[1] = { 0 }, t[1] = { 1 }, w[1] = { DIJKSTRA_MAX_WEIGHT };
        check_long("weight 1000: run", 0, run_hand(&g, &sp, 2, 1, f, t, w, 0));
        check_long("weight 1000: dist[1]", 1000, sp.dist[1]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* one above the limit is rejected and leaves sp empty */
        int f[1] = { 0 }, t[1] = { 1 }, w[1] = { DIJKSTRA_MAX_WEIGHT + 1 };
        check_long("weight 1001: rejected", -1, run_hand(&g, &sp, 2, 1, f, t, w, 0));
        check_long("weight 1001: sp is empty", 1, sp.dist == NULL);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* negative weights are rejected */
        int f[2] = { 0, 1 }, t[2] = { 1, 2 }, w[2] = { 3, -1 };
        check_long("negative weight: rejected", -1, run_hand(&g, &sp, 3, 2, f, t, w, 0));
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* a cycle and a self-loop must not confuse it */
        int f[4] = { 0, 1, 2, 1 }, t[4] = { 1, 2, 0, 1 }, w[4] = { 1, 1, 1, 3 };
        check_long("cycle and self-loop: run", 0, run_hand(&g, &sp, 3, 4, f, t, w, 0));
        check_long("cycle: dist[1]", 1, sp.dist[1]);
        check_long("cycle: dist[2]", 2, sp.dist[2]);
        check_long("cycle: dist[0] stays 0", 0, sp.dist[0]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* source in the middle of a one-way chain 0 -> 1 -> 2 */
        int f[2] = { 0, 1 }, t[2] = { 1, 2 }, w[2] = { 1, 1 };
        check_long("source in the middle: run", 0, run_hand(&g, &sp, 3, 2, f, t, w, 1));
        check_long("source in the middle: node 0 unreachable", DIST_INF, sp.dist[0]);
        check_long("source in the middle: dist[1]", 0, sp.dist[1]);
        check_long("source in the middle: dist[2]", 1, sp.dist[2]);
        sp_free(&sp);
        graph_free(&g);
    }
    {   /* two equal shortest paths: the distance is certain, the route is
         * either one, and the path must be a real route of that cost */
        int f[4] = { 0, 0, 1, 2 }, t[4] = { 1, 2, 3, 3 }, w[4] = { 1, 1, 1, 1 };
        check_long("diamond: run", 0, run_hand(&g, &sp, 4, 4, f, t, w, 0));
        check_long("diamond: dist[3]", 2, sp.dist[3]);
        len = sp_path(&sp, 3, path, 8);
        check_long("diamond: path has 3 nodes", 3, len);
        check_long("diamond: path is a real route costing 2", 2, path_cost(&g, path, len));
        sp_free(&sp);
        graph_free(&g);
    }
}

static void group_small_maps(void)
{
    Problem p;
    Graph g;
    ShortestPaths sp;
    int path[8];

    harness_suite("dijkstra: small maps");

    build_text("MAP 1 1\nR\n" ONE_ROBOT, &p, &g);
    check_long("1x1: run", 0, dijkstra(&g, 0, &sp));
    check_long("1x1: dist[0]", 0, sp.dist[0]);
    check_long("1x1: one reachable node", 1, finite_count(&sp));
    check_long("1x1: path to itself", 1, sp_path(&sp, 0, path, 8));
    sp_free(&sp);
    graph_free(&g);
    problem_free(&p);

    build_text("MAP 1 4\nR..T\n" ONE_ROBOT ONE_TASK, &p, &g);
    check_long("R..T: run", 0, dijkstra(&g, 0, &sp));
    check_long("R..T: dist to T is 3", 3, sp.dist[3]);
    check_long("R..T: path has 4 nodes", 4, sp_path(&sp, 3, path, 8));
    check_long("R..T: path cost is 3", 3, path_cost(&g, path, 4));
    sp_free(&sp);
    graph_free(&g);
    problem_free(&p);

    build_text("MAP 1 5\nR.#.T\n" ONE_ROBOT ONE_TASK, &p, &g);
    check_long("R.#.T: run", 0, dijkstra(&g, 0, &sp));
    check_long("R.#.T: cell next to R", 1, sp.dist[1]);
    check_long("R.#.T: T is behind the wall", DIST_INF, sp.dist[4]);
    check_long("R.#.T: only 2 cells reachable", 2, finite_count(&sp));
    check_long("R.#.T: path to T", 0, sp_path(&sp, 4, path, 8));
    sp_free(&sp);
    graph_free(&g);
    problem_free(&p);

    build_text("MAP 1 3\nR#T\n" ONE_ROBOT ONE_TASK, &p, &g);
    check_long("R#T: run", 0, dijkstra(&g, 0, &sp));
    check_long("R#T: only the source is reachable", 1, finite_count(&sp));
    check_long("R#T: T unreachable", DIST_INF, sp.dist[2]);
    sp_free(&sp);
    graph_free(&g);
    problem_free(&p);
}

/*
 * data/small/sample1.txt (row, col):
 *      c0 c1 c2 c3 c4 c5 c6 c7
 *  r0  D  .  .  .  .  .  .  C
 *  r1  .  S  S  .  .  S  S  .
 *  r2  .  R  .  #  #  .  T  .
 *  r3  .  S  S  .  .  S  S  .
 *  r4  .  .  T  .  .  R  .  .
 *  r5  C  .  .  .  .  .  T  .
 * Robot 1 at (2,1): its only exits are (2,0) (cost 1) and the dead end
 * (2,2). From (2,0) go down to (3,0), (4,0) (cost 3), then along row 4.
 *   to T2 (4,2): 3 + 2 = 5          to T3 (5,6): 3 + 1 + 6 = 10
 *   to T1 (2,6): the right edge column is reached at (4,7) cost 10, then
 *                (3,7) 11, (2,7) 12, (2,6) 13 (the top route is also 13)
 *   to (0,0): 3        to robot 2's start (4,5): 8        to (2,2): 1
 * Robot 2 at (4,5): T3 (5,6) is 2, T2 (4,2) is 3, T1 (2,6) is 5
 *   ((4,6), (4,7), (3,7), (2,7), (2,6)). Back to (2,1): 8.
 * With unit weights a node is never improved after it is first reached, so
 * from robot 1: 38 pushes (one per walkable cell), 38 pops, and 94
 * relaxations (every arc of every reached node, 94 arcs in total).
 */
static void group_sample(void)
{
    Problem p;
    Graph g;
    ShortestPaths sp;
    int path[64];
    int r1, r2, t1, t2, t3, len;

    harness_suite("dijkstra: data/small/sample1.txt");

    check_long("parse sample1", PARSE_OK,
               parse_problem_file("data/small/sample1.txt", &p, NULL, 0));
    check_long("graph_build ok", 0, graph_build(&p, &g));
    r1 = graph_node(&g, 2, 1);
    r2 = graph_node(&g, 4, 5);
    t1 = graph_node(&g, 2, 6);
    t2 = graph_node(&g, 4, 2);
    t3 = graph_node(&g, 5, 6);

    metrics_reset();
    check_long("robot 1: run", 0, dijkstra(&g, r1, &sp));
    check_long("robot 1 -> T1 (2,6)", 13, sp.dist[t1]);
    check_long("robot 1 -> T2 (4,2)", 5, sp.dist[t2]);
    check_long("robot 1 -> T3 (5,6)", 10, sp.dist[t3]);
    check_long("robot 1 -> (0,0)", 3, sp.dist[graph_node(&g, 0, 0)]);
    check_long("robot 1 -> robot 2's start (4,5)", 8, sp.dist[r2]);
    check_long("robot 1 -> dead end (2,2)", 1, sp.dist[graph_node(&g, 2, 2)]);
    check_long("robot 1 -> shelf (1,1) is unreachable", DIST_INF,
               sp.dist[graph_node(&g, 1, 1)]);
    check_long("robot 1 -> obstacle (2,3) is unreachable", DIST_INF,
               sp.dist[graph_node(&g, 2, 3)]);
    check_long("robot 1: reaches all 38 walkable cells", 38, finite_count(&sp));
    check_long("robot 1: self-check, no violations", 0,
               count_violations(&g, &sp, r1));
    check_long("robot 1: heap_pushes", 38, (long)g_metrics.heap_pushes);
    check_long("robot 1: heap_pops", 38, (long)g_metrics.heap_pops);
    check_long("robot 1: relaxations (all 94 arcs)", 94, (long)g_metrics.relaxations);
    len = sp_path(&sp, t1, path, 64);
    check_long("robot 1 -> T1: path has 14 nodes", 14, len);
    check_long("robot 1 -> T1: path cost is 13", 13, path_cost(&g, path, len));
    check_long("robot 1 -> T1: path starts at the robot", r1, path[0]);
    check_long("robot 1 -> T1: path ends at T1", t1, path[len - 1]);
    sp_free(&sp);

    check_long("robot 2: run", 0, dijkstra(&g, r2, &sp));
    check_long("robot 2 -> T1 (2,6)", 5, sp.dist[t1]);
    check_long("robot 2 -> T2 (4,2)", 3, sp.dist[t2]);
    check_long("robot 2 -> T3 (5,6)", 2, sp.dist[t3]);
    check_long("robot 2 -> robot 1's start (2,1)", 8, sp.dist[r1]);
    check_long("robot 2: reaches all 38 walkable cells", 38, finite_count(&sp));
    check_long("robot 2: self-check, no violations", 0,
               count_violations(&g, &sp, r2));
    sp_free(&sp);

    graph_free(&g);
    problem_free(&p);
}

static void group_invalid(void)
{
    int from[1] = { 0 }, to[1] = { 1 }, w[1] = { 1 };
    Graph g, zero;
    ShortestPaths sp, blank;
    int path[4];

    harness_suite("dijkstra: invalid use");

    memset(&zero, 0, sizeof zero);
    memset(&blank, 0, sizeof blank);
    make_graph(&g, 2, 1, from, to, w);

    check_long("NULL graph", -1, dijkstra(NULL, 0, &sp));
    check_long("NULL result struct", -1, dijkstra(&g, 0, NULL));
    check_long("empty graph (n = 0)", -1, dijkstra(&zero, 0, &sp));
    check_long("source -1", -1, dijkstra(&g, -1, &sp));
    check_long("source = n", -1, dijkstra(&g, 2, &sp));
    check_long("failed run leaves sp empty", 1, sp.dist == NULL && sp.prev == NULL);

    check_long("sp_path on an empty result", -1, sp_path(&blank, 0, path, 4));
    check_long("sp_path on NULL", -1, sp_path(NULL, 0, path, 4));
    sp_free(NULL);
    check_long("sp_free(NULL) is safe", 1, 1);

    check_long("good run after failures", 0, dijkstra(&g, 0, &sp));
    sp_free(&sp);
    check_long("sp_free clears dist", 1, sp.dist == NULL);
    sp_free(&sp);
    check_long("sp_free twice is safe", 1, sp.prev == NULL);
    check_long("sp_path after sp_free", -1, sp_path(&sp, 0, path, 4));

    graph_free(&g);
}

/* 200 random graphs against the Bellman-Ford reference, plus the counter
 * rules that must hold for any graph:
 *   pops == pushes, pushes <= arcs + 1,
 *   relaxations == number of arcs that start at a reachable node. */
static void group_random(void)
{
    int from[256], to[256], w[256], ref[64];
    Graph g;
    ShortestPaths sp;
    int trial, n, m, i, source, rc;
    int bad_rc = 0, bad_dist = 0, bad_self = 0, bad_count = 0;
    long reach_arcs;

    harness_suite("dijkstra: 200 random graphs vs Bellman-Ford");

    g_seed = 777UL;
    for (trial = 0; trial < 200; trial++) {
        n = 2 + next_rand(59);               /* 2 .. 60 nodes   */
        m = next_rand(4 * n + 1);            /* 0 .. 4n arcs    */
        for (i = 0; i < m; i++) {
            from[i] = next_rand(n);
            to[i]   = next_rand(n);
            w[i]    = next_rand(21);         /* weights 0 .. 20 */
        }
        source = next_rand(n);

        metrics_reset();
        rc = run_hand(&g, &sp, n, m, from, to, w, source);
        if (rc != 0) {
            bad_rc++;
            sp_free(&sp);
            graph_free(&g);
            continue;
        }
        reference_distances(n, m, from, to, w, source, ref);
        reach_arcs = 0;
        for (i = 0; i < m; i++) {
            if (ref[from[i]] < DIST_INF) {
                reach_arcs++;
            }
        }
        for (i = 0; i < n; i++) {
            if (sp.dist[i] != ref[i]) {
                bad_dist++;
            }
        }
        bad_self += count_violations(&g, &sp, source);
        if (g_metrics.heap_pops != g_metrics.heap_pushes
            || g_metrics.heap_pushes > (unsigned long)m + 1
            || (long)g_metrics.relaxations != reach_arcs) {
            bad_count++;
        }
        sp_free(&sp);
        graph_free(&g);
    }
    check_long("200 random graphs: every run succeeded", 0, bad_rc);
    check_long("200 random graphs: wrong distances", 0, bad_dist);
    check_long("200 random graphs: self-check violations", 0, bad_self);
    check_long("200 random graphs: counter rules broken", 0, bad_count);
}

static void group_stress(void)
{
    static int big_path[130000];
    Problem p;
    Graph g;
    ShortestPaths sp;
    int len;

    harness_suite("dijkstra: largest and worst-shaped maps");

    /* 500 x 500, everything open, start in a corner. The far corner is
     * 499 + 499 = 998 steps away. Every one of the 250000 nodes is pushed
     * and popped once; every one of the 998000 arcs is relaxed once. */
    if (make_open(&p, 500, 500) == 0) {
        graph_build(&p, &g);
        metrics_reset();
        check_long("500x500 open: run", 0, dijkstra(&g, 0, &sp));
        check_long("500x500 open: far corner is 998", 998,
                   sp.dist[graph_node(&g, 499, 499)]);
        check_long("500x500 open: top-right corner is 499", 499,
                   sp.dist[graph_node(&g, 0, 499)]);
        check_long("500x500 open: centre (250,250) is 500", 500,
                   sp.dist[graph_node(&g, 250, 250)]);
        check_long("500x500 open: all 250000 cells reached", 250000, finite_count(&sp));
        check_long("500x500 open: heap_pushes", 250000, (long)g_metrics.heap_pushes);
        check_long("500x500 open: heap_pops", 250000, (long)g_metrics.heap_pops);
        check_long("500x500 open: relaxations", 998000, (long)g_metrics.relaxations);
        check_long("500x500 open: self-check", 0, count_violations(&g, &sp, 0));
        sp_free(&sp);
        graph_free(&g);
    }
    problem_free(&p);

    /* Small snake, 5 rows x 3 cols (hand count in make_snake's comment):
     * 3 open rows * 2 steps + 2 gaps * 2 steps = 10 steps to (4,2);
     * walkable cells = 3 * 3 + 2 = 11. */
    if (make_snake(&p, 5, 3) == 0) {
        graph_build(&p, &g);
        check_long("snake 5x3: run", 0, dijkstra(&g, 0, &sp));
        check_long("snake 5x3: end of the corridor (4,2) is 10", 10,
                   sp.dist[graph_node(&g, 4, 2)]);
        check_long("snake 5x3: 11 reachable cells", 11, finite_count(&sp));
        sp_free(&sp);
        graph_free(&g);
    }
    problem_free(&p);

    /* Big snake, 499 rows x 500 cols: one long corridor. 250 open rows of
     * 499 steps + 249 gaps of 2 steps = 125248 steps to (498,0), which is
     * where the last row ends (odd-numbered open rows end on the left).
     * Walkable cells = 250 * 500 + 249 = 125249. */
    if (make_snake(&p, 499, 500) == 0) {
        graph_build(&p, &g);
        check_long("snake 499x500: run", 0, dijkstra(&g, 0, &sp));
        check_long("snake 499x500: end of the corridor is 125248", 125248,
                   sp.dist[graph_node(&g, 498, 0)]);
        check_long("snake 499x500: 125249 reachable cells", 125249, finite_count(&sp));
        len = sp_path(&sp, graph_node(&g, 498, 0), big_path, 130000);
        check_long("snake 499x500: path has 125249 nodes", 125249, len);
        check_long("snake 499x500: path cost is 125248", 125248,
                   path_cost(&g, big_path, len));
        check_long("snake 499x500: self-check", 0, count_violations(&g, &sp, 0));
        sp_free(&sp);
        graph_free(&g);
    }
    problem_free(&p);
}

void test_dijkstra(void)
{
    group_weighted();
    group_special_weights();
    group_small_maps();
    group_sample();
    group_invalid();
    group_random();
    group_stress();
}
