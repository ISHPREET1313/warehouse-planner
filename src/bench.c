/* bench.c - benchmark table. See bench.h. */
#include "bench.h"

#include <stdlib.h>
#include <string.h>

#include "bellman_ford.h"
#include "dijkstra.h"
#include "graph.h"
#include "io.h"
#include "knapsack.h"
#include "metrics.h"
#include "planner.h"
#include "problem.h"
#include "tsp.h"

/* ---------- output rows ---------- */

typedef struct {
    FILE *out;
    int   csv;
} Sink;

static void print_header(const Sink *k)
{
    if (k->csv) {
        fprintf(k->out, "section,case,algorithm,result,comparisons,relaxations,heap_ops,aux_bytes,ms_per_run\n");
    } else {
        fprintf(k->out, "%-13s %-22s %-24s %-30s %12s %11s %10s %10s %10s\n",
                "section", "case", "algorithm", "result", "comparisons",
                "relaxations", "heap_ops", "aux_bytes", "ms/run");
    }
}

static void emit(const Sink *k, const char *section, const char *cs,
                 const char *algo, const char *result, const Metrics *m,
                 double ms)
{
    unsigned long heap_ops = m->heap_pushes + m->heap_pops;

    if (k->csv) {
        fprintf(k->out, "%s,%s,%s,%s,%lu,%lu,%lu,%lu,%.3f\n", section, cs, algo,
                result, m->comparisons, m->relaxations, heap_ops, m->aux_bytes, ms);
    } else {
        fprintf(k->out, "%-13s %-22s %-24s %-30s %12lu %11lu %10lu %10lu %10.3f\n",
                section, cs, algo, result, m->comparisons, m->relaxations,
                heap_ops, m->aux_bytes, ms);
    }
}

static void emit_note(const Sink *k, const char *section, const char *cs,
                      const char *algo, const char *note)
{
    Metrics zero;

    memset(&zero, 0, sizeof zero);
    emit(k, section, cs, algo, note, &zero, 0.0);
}

/* ---------- measuring ---------- */

typedef int (*BodyFn)(void *ctx);

/* Run fn once with fresh counters (copied to *m), then time repeated runs.
 * Returns 0 on success; *ms = average milliseconds per run. */
static int measure(BodyFn fn, void *ctx, Metrics *m, double *ms)
{
    Timer t;
    int reps = 0;
    double total;

    metrics_reset();
    if (fn(ctx) != 0) {
        return -1;
    }
    *m = g_metrics;
    timer_start(&t);
    do {
        if (fn(ctx) != 0) {
            return -1;
        }
        reps++;
        total = timer_elapsed_ms(&t);
    } while (total < 30.0 && reps < 2000);
    *ms = total / reps;
    return 0;
}

/* Small seeded random numbers (same recipe as the tests). */
static unsigned long g_seed;

static void seed_rand(unsigned long s)
{
    g_seed = s;
}

static int next_rand(int range)
{
    g_seed = (g_seed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (int)((g_seed >> 8) % (unsigned long)range);
}

/* ---------- shortest paths ---------- */

typedef int (*SolverFn)(const Graph *g, int source, ShortestPaths *sp);

typedef struct {
    const Graph *g;
    int          source;
    SolverFn     solve;
    long         sum;       /* sum of all finite distances (agreement check) */
} SpCtx;

static int sp_body(void *vctx)
{
    SpCtx *c = (SpCtx *)vctx;
    ShortestPaths sp;
    int v;

    if (c->solve(c->g, c->source, &sp) != 0) {
        sp_free(&sp);
        return -1;
    }
    c->sum = 0;
    for (v = 0; v < sp.n; v++) {
        if (sp.dist[v] < DIST_INF) {
            c->sum += sp.dist[v];
        }
    }
    sp_free(&sp);
    return 0;
}

static void bench_paths(const Sink *k, const char *cs, const Graph *g, int source)
{
    static const char *name[3] = {"dijkstra heap", "dijkstra scan", "bellman-ford"};
    SolverFn fn[3];
    SpCtx c;
    Metrics m;
    double ms;
    char res[64];
    int i;

    fn[0] = dijkstra;
    fn[1] = dijkstra_scan;
    fn[2] = bellman_ford;
    for (i = 0; i < 3; i++) {
        c.g = g;
        c.source = source;
        c.solve = fn[i];
        c.sum = 0;
        if (measure(sp_body, &c, &m, &ms) != 0) {
            emit_note(k, "paths", cs, name[i], "failed");
            continue;
        }
        sprintf(res, "sumdist=%ld", c.sum);
        emit(k, "paths", cs, name[i], res, &m, ms);
    }
}

/* ---------- planner ---------- */

typedef struct {
    const Problem *p;
    const Graph   *g;
    PlanOptions    opt;
    Plan          *plan;
} PlanCtx;

static int plan_body(void *vctx)
{
    PlanCtx *c = (PlanCtx *)vctx;

    return plan_build(c->p, c->g, c->opt, c->plan);
}

static void bench_plans(const Sink *k, const char *cs, const Problem *p, const Graph *g)
{
    static Plan plan;
    PlanCtx c;
    Metrics m;
    double ms;
    char res[64];
    int i;

    for (i = 0; i < 2; i++) {
        c.p = p;
        c.g = g;
        c.opt.use_dp_knapsack = (i == 0);
        c.opt.use_held_karp = (i == 0);
        c.plan = &plan;
        if (measure(plan_body, &c, &m, &ms) != 0) {
            emit_note(k, "planner", cs, i == 0 ? "A: DP + Held-Karp" : "B: greedy + nearest", "failed");
            continue;
        }
        sprintf(res, "served=%d value=%d cost=%d", plan.served, plan.total_value,
                plan.total_cost);
        emit(k, "planner", cs, i == 0 ? "A: DP + Held-Karp" : "B: greedy + nearest",
             res, &m, ms);
    }
}

/* ---------- data files: paths and planner ---------- */

static const char *base_name(const char *path)
{
    const char *s = strrchr(path, '/');

    return s ? s + 1 : path;
}

static void bench_file(const Sink *k, const char *path)
{
    Problem p;
    Graph g;
    char err[200];
    char cs[40];

    if (parse_problem_file(path, &p, err, sizeof err) != PARSE_OK) {
        emit_note(k, "paths", base_name(path), "-", "skipped (cannot load)");
        return;
    }
    if (graph_build(&p, &g) != 0) {
        problem_free(&p);
        emit_note(k, "paths", base_name(path), "-", "skipped (graph failed)");
        return;
    }
    strncpy(cs, base_name(path), sizeof cs - 1);
    cs[sizeof cs - 1] = '\0';
    if (strlen(cs) > 4) {
        cs[strlen(cs) - 4] = '\0';          /* drop ".txt" */
    }
    bench_paths(k, cs, &g, graph_node(&g, p.robots[0].row, p.robots[0].col));
    bench_plans(k, cs, &p, &g);
    graph_free(&g);
    problem_free(&p);
}

/* ---------- knapsack ---------- */

typedef struct {
    int weight[KNAP_MAX_ITEMS], value[KNAP_MAX_ITEMS];
    int n, cap, dp, best;
} KnapCtx;

static int knap_body(void *vctx)
{
    KnapCtx *c = (KnapCtx *)vctx;
    int chosen[KNAP_MAX_ITEMS];

    if (c->dp) {
        return knapsack_dp(c->weight, c->value, c->n, c->cap, chosen, &c->best);
    }
    return knapsack_greedy(c->weight, c->value, c->n, c->cap, chosen, &c->best);
}

static void bench_knap_case(const Sink *k, const char *cs, KnapCtx *c)
{
    Metrics m;
    double ms;
    char res[64];
    int i;

    for (i = 0; i < 2; i++) {
        c->dp = (i == 0);
        if (measure(knap_body, c, &m, &ms) != 0) {
            emit_note(k, "knapsack", cs, i == 0 ? "DP (exact)" : "greedy", "failed");
            continue;
        }
        sprintf(res, "value=%d", c->best);
        emit(k, "knapsack", cs, i == 0 ? "DP (exact)" : "greedy", res, &m, ms);
    }
}

static void bench_knapsack(const Sink *k)
{
    static const int size[5][2] = {{10, 50}, {50, 300}, {100, 1000}, {200, 5000}, {256, 20000}};
    static KnapCtx c;
    char cs[40];
    int i, j;

    c.n = 3;
    c.cap = 50;
    c.weight[0] = 10; c.value[0] = 60;
    c.weight[1] = 20; c.value[1] = 100;
    c.weight[2] = 30; c.value[2] = 120;
    bench_knap_case(k, "classic n=3 cap=50", &c);

    for (i = 0; i < 5; i++) {
        seed_rand(2026UL + (unsigned long)i);
        c.n = size[i][0];
        c.cap = size[i][1];
        for (j = 0; j < c.n; j++) {
            c.weight[j] = 1 + next_rand(20);
            c.value[j] = 1 + next_rand(100);
        }
        sprintf(cs, "random n=%d cap=%d", c.n, c.cap);
        bench_knap_case(k, cs, &c);
    }
}

/* ---------- pickup order ---------- */

static int g_dist[(TSP_NN_MAX_STOPS + 1) * (TSP_NN_MAX_STOPS + 1)];

typedef struct {
    int n, hk, cost;
} TspCtx;

static int tsp_body(void *vctx)
{
    TspCtx *c = (TspCtx *)vctx;
    int order[TSP_NN_MAX_STOPS];

    if (c->hk) {
        return tsp_held_karp(g_dist, c->n, order, &c->cost);
    }
    return tsp_nearest(g_dist, c->n, order, &c->cost);
}

static void bench_tsp(const Sink *k)
{
    static const int sizes[7] = {6, 10, 12, 14, 15, 50, 200};
    int x[TSP_NN_MAX_STOPS + 1], y[TSP_NN_MAX_STOPS + 1];
    TspCtx c;
    Metrics m;
    double ms;
    char cs[40], res[40];
    int s, n, a, b, dx, dy, i;

    for (s = 0; s < 7; s++) {
        n = sizes[s];
        seed_rand(77UL + (unsigned long)n);
        for (a = 0; a <= n; a++) {
            x[a] = next_rand(100);
            y[a] = next_rand(100);
        }
        for (a = 0; a <= n; a++) {
            for (b = 0; b <= n; b++) {
                dx = x[a] > x[b] ? x[a] - x[b] : x[b] - x[a];
                dy = y[a] > y[b] ? y[a] - y[b] : y[b] - y[a];
                g_dist[a * (n + 1) + b] = dx + dy;
            }
        }
        sprintf(cs, "random stops=%d", n);
        for (i = 0; i < 2; i++) {
            c.n = n;
            c.hk = (i == 1);
            c.cost = 0;
            if (c.hk && n > TSP_HK_MAX_STOPS) {
                emit_note(k, "pickup-order", cs, "Held-Karp (exact)", "n/a (above 15 stops)");
                continue;
            }
            if (measure(tsp_body, &c, &m, &ms) != 0) {
                emit_note(k, "pickup-order", cs, i ? "Held-Karp (exact)" : "nearest neighbour", "failed");
                continue;
            }
            sprintf(res, "cost=%d", c.cost);
            emit(k, "pickup-order", cs, i ? "Held-Karp (exact)" : "nearest neighbour", res, &m, ms);
        }
    }
}

/* ---------- the whole table ---------- */

int bench_run(FILE *out, int csv)
{
    static const char *files[5] = {
        "data/small/sample1.txt",
        "data/small/sample2.txt",
        "data/medium/warehouse_30x40.txt",
        "data/large/warehouse_62x80.txt",
        "data/adversarial/snake_21x21.txt"
    };
    Sink k;
    int i;

    if (out == NULL) {
        return -1;
    }
    k.out = out;
    k.csv = csv;
    print_header(&k);
    for (i = 0; i < 5; i++) {
        bench_file(&k, files[i]);
    }
    bench_knapsack(&k);
    bench_tsp(&k);
    return 0;
}
