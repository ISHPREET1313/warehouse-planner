/*
 * test_files.c - item 7: end-to-end tests on the files in data/, plus
 * adversarial graph shapes.
 *
 * File tests: each file is parsed, turned into a graph, and every robot is
 * run through all three algorithms (heap, scan, Bellman-Ford). The expected
 * costs from each robot to each task were NOT taken from the C code: they
 * come from a separate breadth-first search written in Python
 * (see docs/TESTING.md for how to regenerate them). The same script gives the
 * independent walkable-cell and arc counts, which also catch a typo in a
 * map file.
 *
 * Adversarial shapes (built by hand, counts worked out on paper):
 *  - "fan": makes the heap hold as many pairs as the capacity allows
 *    (arcs + 1) and leaves m - 1 stale pairs behind,
 *  - "reverse chain": arcs point against the node numbering, the slow case
 *    for Bellman-Ford (n passes).
 *
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

/* ---------------- expected costs, robot by robot (-1 = unreachable) ---------------- */
static const int D_SAMPLE1[] = { 13, 5, 10,   5, 3, 2 };
static const int D_SAMPLE2[] = { 7, 11, 8, 5 };
static const int D_MEDIUM[]  = {
    23, 11, 13, 14, 15, 23, 34, 31,
    15, 19, 21, 22, 13, 3, 14, 17,
    38, 26, 26, 23, 20, 22, 17, 14
};
static const int D_LARGE[] = {
    27, 21, 43, 25, 16, 29, 22, 43, 44, 49, 73, 55, 44, 59, 83, 57, 62, 70, 100, 98,
    65, 27, 37, 55, 22, 11, 16, 5, 6, 49, 35, 55, 34, 27, 45, 49, 52, 38, 62, 60,
    48, 72, 88, 38, 51, 56, 43, 56, 55, 16, 68, 6, 17, 38, 62, 24, 31, 49, 79, 77,
    113, 65, 47, 103, 70, 57, 64, 43, 42, 81, 27, 71, 50, 27, 13, 41, 42, 24, 14, 12
};
static const int D_SNAKE[]  = { 240 };
static const int D_SEALED[] = { 3, -1 };
static const int D_BOXED[]  = { -1, -1,   1, 3 };

typedef struct {
    const char *path;
    int rows, cols, n_robots, n_tasks;
    int walkable, arcs;        /* independent counts                      */
    int unreachable;           /* tasks no robot can reach                */
    const int *dist;           /* n_robots * n_tasks expected costs        */
} FileCase;

static const FileCase CASES[] = {
    { "data/small/sample1.txt",           6,  8, 2,  3,   38,   94, 0, D_SAMPLE1 },
    { "data/small/sample2.txt",           7, 12, 1,  4,   38,   90, 0, D_SAMPLE2 },
    { "data/medium/warehouse_30x40.txt", 30, 40, 3,  8,  630, 1842, 0, D_MEDIUM  },
    { "data/large/warehouse_62x80.txt",  62, 80, 4, 20, 2640, 7824, 0, D_LARGE   },
    { "data/adversarial/snake_21x21.txt",  21, 21, 1, 1, 241, 480, 0, D_SNAKE  },
    { "data/adversarial/sealed_task.txt",   7,  9, 1, 2,  23,  54, 1, D_SEALED },
    { "data/adversarial/boxed_robot.txt",   5, 11, 2, 2,  24,  68, 0, D_BOXED  },
    { "data/adversarial/no_tasks.txt",      4,  5, 1, 0,   6,  14, 0, NULL     }
};

/* ---------------- helpers ---------------- */

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

/* Scan comparisons for n nodes of which r are reachable (see test_compare.c). */
static unsigned long scan_comparisons(long n, long r)
{
    return (unsigned long)(r * n - r * (r - 1) / 2 + (n - r));
}

/* Build a Graph by hand from m arcs (from[i] -> to[i], weight w[i]). 0 = ok. */
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

/* ---------------- file tests ---------------- */

static void run_file_case(const FileCase *fc)
{
    Problem p;
    Graph g;
    ShortestPaths hp, sc, bf;
    char err[200];
    ParseStatus st;
    int r, k, v, src, node, want, n_reach, rc_h, rc_s, rc_b;
    int bad_rc = 0, bad_heap = 0, bad_scan = 0, bad_bf = 0;
    int bad_cross = 0, bad_counts = 0;
    unsigned long heap_relax, heap_push, heap_pop, scan_relax, scan_comp, bf_relax;

    harness_suite(fc->path);

    st = parse_problem_file(fc->path, &p, err, sizeof err);
    check_str("parse", "PARSE_OK", parse_status_name(st));
    if (st != PARSE_OK) {
        return;
    }
    check_long("rows", fc->rows, p.rows);
    check_long("cols", fc->cols, p.cols);
    check_long("robots", fc->n_robots, p.n_robots);
    check_long("tasks", fc->n_tasks, p.n_tasks);

    if (graph_build(&p, &g) != 0) {
        check_long("graph_build", 0, -1);
        problem_free(&p);
        return;
    }
    check_long("walkable cells (independent count)", fc->walkable, g.n_walkable);
    check_long("arcs (independent count)", fc->arcs, g.n_arcs);
    check_long("tasks no robot can reach", fc->unreachable,
               graph_unreachable_tasks(&p, &g, NULL));

    for (r = 0; r < p.n_robots; r++) {
        src = graph_node(&g, p.robots[r].row, p.robots[r].col);

        metrics_reset();
        rc_h = dijkstra(&g, src, &hp);
        heap_relax = g_metrics.relaxations;
        heap_push  = g_metrics.heap_pushes;
        heap_pop   = g_metrics.heap_pops;

        metrics_reset();
        rc_s = dijkstra_scan(&g, src, &sc);
        scan_relax = g_metrics.relaxations;
        scan_comp  = g_metrics.comparisons;

        metrics_reset();
        rc_b = bellman_ford(&g, src, &bf);
        bf_relax = g_metrics.relaxations;

        if (rc_h != 0 || rc_s != 0 || rc_b != 0) {
            bad_rc++;
        } else {
            for (k = 0; k < p.n_tasks; k++) {
                want = fc->dist[r * p.n_tasks + k];
                if (want < 0) {
                    want = DIST_INF;
                }
                node = graph_node(&g, p.tasks[k].row, p.tasks[k].col);
                if (hp.dist[node] != want) {
                    bad_heap++;
                }
                if (sc.dist[node] != want) {
                    bad_scan++;
                }
                if (bf.dist[node] != want) {
                    bad_bf++;
                }
            }
            for (v = 0; v < g.n; v++) {
                if (hp.dist[v] != sc.dist[v] || hp.dist[v] != bf.dist[v]) {
                    bad_cross++;
                }
            }
            n_reach = finite_count(&hp);
            if (heap_pop != heap_push
                || heap_push > (unsigned long)g.n_arcs + 1
                || scan_relax != heap_relax
                || scan_comp != scan_comparisons(g.n, n_reach)
                || bf_relax < heap_relax) {
                bad_counts++;
            }
        }
        sp_free(&hp);
        sp_free(&sc);
        sp_free(&bf);
    }
    check_long("every algorithm run succeeded", 0, bad_rc);
    check_long("heap: wrong task costs", 0, bad_heap);
    check_long("scan: wrong task costs", 0, bad_scan);
    check_long("bellman-ford: wrong task costs", 0, bad_bf);
    check_long("nodes where the 3 algorithms differ", 0, bad_cross);
    check_long("counter rules broken", 0, bad_counts);

    graph_free(&g);
    problem_free(&p);
}

static void group_files(void)
{
    size_t i;

    for (i = 0; i < sizeof CASES / sizeof CASES[0]; i++) {
        run_file_case(&CASES[i]);
    }
}

/* Files that must be rejected, with the status each one must give. */
static void group_invalid_files(void)
{
    static const char *const paths[] = {
        "data/adversarial/bad_count.txt",
        "data/adversarial/bad_dims.txt",
        "data/adversarial/bad_symbol.txt",
        "data/adversarial/bad_value.txt",
        "data/adversarial/bad_keyword.txt",
        "data/adversarial/truncated_map.txt"
    };
    static const char *const want[] = {
        "PARSE_ERR_COUNT", "PARSE_ERR_MAP", "PARSE_ERR_MAP",
        "PARSE_ERR_VALUE", "PARSE_ERR_SYNTAX", "PARSE_ERR_MAP"
    };
    Problem p;
    char err[200];
    ParseStatus st;
    size_t i;

    harness_suite("invalid files in data/adversarial");
    for (i = 0; i < sizeof paths / sizeof paths[0]; i++) {
        st = parse_problem_file(paths[i], &p, err, sizeof err);
        check_str(paths[i] + 17, want[i], parse_status_name(st));   /* skip "data/adversarial/" */
        problem_free(&p);
    }
    st = parse_problem_file("data/adversarial/bad_symbol.txt", &p, err, sizeof err);
    check_long("bad_symbol message names line 5", 1, strstr(err, "line 5") != NULL);
    check_long("bad_symbol message names column 2", 1, strstr(err, "column 2") != NULL);
    check_long("failed file leaves no grid", 1, p.cells == NULL);
    (void)st;
    problem_free(&p);
}

/* ---------------- adversarial shapes ---------------- */

/*
 * Fan graph, m = 50: node 0 is the source, nodes 1..m are middle nodes, node
 * T = m + 1 = 51 is the target (n = 52, arcs = 2m = 100).
 *   0 -> i    weight i              (i = 1..m)
 *   i -> T    weight 2m - 2i        (so dist via i is 2m - i, falling as i grows)
 * Middle nodes come out of the heap in order 1..m, and every one of them
 * improves T, so T is pushed m times. Hand count:
 *   pushes = 1 (source) + m (middle nodes) + m (T) = 101 = arcs + 1,
 *   pops   = 101 (49 of them stale T pairs), relaxations = 100,
 *   dist[T] = 2m - m = 50 through node m (that arc has weight 0).
 * Scan: n = R = 52, comparisons = 52 * 53 / 2 = 1378.
 * Bellman-Ford: pass 1 tests all 100 arcs and already reaches the final
 * distances (T has the largest id), pass 2 confirms: 200 relaxations.
 */
static void group_fan(void)
{
    enum { M = 50 };
    int from[2 * M], to[2 * M], w[2 * M];
    Graph g;
    ShortestPaths sp;
    int i;

    harness_suite("adversarial: fan (heap full, stale pairs)");

    for (i = 1; i <= M; i++) {
        from[i - 1] = 0;      to[i - 1] = i;        w[i - 1] = i;
        from[M + i - 1] = i;  to[M + i - 1] = M + 1;  w[M + i - 1] = 2 * M - 2 * i;
    }
    check_long("build the fan graph", 0, make_graph(&g, M + 2, 2 * M, from, to, w));

    metrics_reset();
    check_long("heap: run", 0, dijkstra(&g, 0, &sp));
    check_long("heap: dist[T] is 50", 50, sp.dist[M + 1]);
    check_long("heap: prev[T] is node 50", M, sp.prev[M + 1]);
    check_long("heap: dist[node 7] is 7", 7, sp.dist[7]);
    check_long("heap: pushes = arcs + 1 (heap exactly full)", 101,
               (long)g_metrics.heap_pushes);
    check_long("heap: pops", 101, (long)g_metrics.heap_pops);
    check_long("heap: relaxations = all 100 arcs", 100, (long)g_metrics.relaxations);
    sp_free(&sp);

    metrics_reset();
    check_long("scan: run", 0, dijkstra_scan(&g, 0, &sp));
    check_long("scan: dist[T] is 50", 50, sp.dist[M + 1]);
    check_long("scan: comparisons 52*53/2", 1378, (long)g_metrics.comparisons);
    sp_free(&sp);

    metrics_reset();
    check_long("bellman-ford: run", 0, bellman_ford(&g, 0, &sp));
    check_long("bellman-ford: dist[T] is 50", 50, sp.dist[M + 1]);
    check_long("bellman-ford: relaxations 2 * 100", 200, (long)g_metrics.relaxations);
    sp_free(&sp);
    graph_free(&g);
}

/*
 * Reverse chain, n = 200: arcs i+1 -> i of weight 1, source = node 199. The
 * arcs point against the node numbering, so Bellman-Ford (which visits nodes
 * 0, 1, 2, ...) moves the distance only one step per pass.
 *   pass k (k = 1..199): k reached nodes, each with an arc = k relaxations
 *   pass 200: all reached, node 0 has no arc = 199 relaxations, no change
 *   total = 199 * 200 / 2 + 199 = 20099
 * Heap: 200 pushes, 200 pops, 199 relaxations (a chain has no stale pairs).
 * Scan: n = R = 200, comparisons = 200 * 201 / 2 = 20100.
 */
static void group_reverse_chain(void)
{
    enum { N = 200 };
    int from[N - 1], to[N - 1], w[N - 1];
    Graph g;
    ShortestPaths sp;
    int i;

    harness_suite("adversarial: reverse chain (slow Bellman-Ford)");

    for (i = 0; i < N - 1; i++) {
        from[i] = i + 1;
        to[i]   = i;
        w[i]    = 1;
    }
    check_long("build the chain", 0, make_graph(&g, N, N - 1, from, to, w));

    metrics_reset();
    check_long("heap: run", 0, dijkstra(&g, N - 1, &sp));
    check_long("heap: dist[0] is 199", 199, sp.dist[0]);
    check_long("heap: pushes", 200, (long)g_metrics.heap_pushes);
    check_long("heap: pops", 200, (long)g_metrics.heap_pops);
    check_long("heap: relaxations", 199, (long)g_metrics.relaxations);
    sp_free(&sp);

    metrics_reset();
    check_long("scan: run", 0, dijkstra_scan(&g, N - 1, &sp));
    check_long("scan: dist[0] is 199", 199, sp.dist[0]);
    check_long("scan: comparisons 200*201/2", 20100, (long)g_metrics.comparisons);
    check_long("scan: relaxations", 199, (long)g_metrics.relaxations);
    sp_free(&sp);

    metrics_reset();
    check_long("bellman-ford: run", 0, bellman_ford(&g, N - 1, &sp));
    check_long("bellman-ford: dist[0] is 199", 199, sp.dist[0]);
    check_long("bellman-ford: relaxations (hand count)", 20099,
               (long)g_metrics.relaxations);
    sp_free(&sp);

    /* the same chain read in the helpful direction: one pass + one check */
    for (i = 0; i < N - 1; i++) {
        from[i] = i;
        to[i]   = i + 1;
    }
    graph_free(&g);
    check_long("build the forward chain", 0, make_graph(&g, N, N - 1, from, to, w));
    metrics_reset();
    check_long("forward chain: run", 0, bellman_ford(&g, 0, &sp));
    check_long("forward chain: dist[199] is 199", 199, sp.dist[N - 1]);
    check_long("forward chain: 2 passes * 199 arcs", 398,
               (long)g_metrics.relaxations);
    sp_free(&sp);
    graph_free(&g);
}

void test_files(void)
{
    group_files();
    group_invalid_files();
    group_fan();
    group_reverse_chain();
}
