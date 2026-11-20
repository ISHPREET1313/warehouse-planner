/*
 * main.c - entry point and CLI menu (item 13).
 *
 *   planner <input-file>   one-shot report: problem, graph, reachability,
 *                          the three shortest-path algorithms per robot, and
 *                          the plan with both strategy sets (A and B).
 *   planner --bench        the benchmark table (bench.h) on the screen.
 *   planner --export-replay <input> <output>
 *                          plan with both strategy sets, build the cell-by-cell
 *                          routes (route.h) and write a replay file (replay.h).
 *   planner --play <replay-file>
 *                          watch a replay file in the terminal: the real-time
 *                          player (playrun.h) on a Windows console, otherwise
 *                          the line-based player (play.h).
 *   planner                interactive menu (works with typed or piped input).
 *
 * Strategy sets (same input, compared on results and operation counts):
 *   A = knapsack DP + Held-Karp (exact per robot)
 *   B = knapsack greedy + nearest neighbour (fast heuristics)
 * No algorithm lives here: main.c only calls the modules and prints.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bellman_ford.h"
#include "bench.h"
#include "dijkstra.h"
#include "export.h"
#include "graph.h"
#include "io.h"
#include "metrics.h"
#include "planner.h"
#include "play.h"
#include "playrun.h"
#include "problem.h"
#include "replay.h"
#include "route.h"

#define TASKS_CSV  "plan_tasks.csv"
#define PLAY_DELAY_MS 200            /* pause between frames in auto mode */
#define MENU_REPLAY "menu_play.replay"
#define ROBOTS_CSV "plan_robots.csv"

/* ---------- shortest-path report ---------- */

/* All three algorithms have this shape, so one helper can run any of them. */
typedef int (*SolverFn)(const Graph *g, int source, ShortestPaths *sp);

/* 1 if both results have the same distance for every node, otherwise 0. */
static int same_distances(const ShortestPaths *a, const ShortestPaths *b)
{
    int v;

    if (a->n != b->n) {
        return 0;
    }
    for (v = 0; v < a->n; v++) {
        if (a->dist[v] != b->dist[v]) {
            return 0;
        }
    }
    return 1;
}

/* Run one algorithm, print its time and counters, and return 1 if its
 * distances equal those of the reference run (the heap version). */
static int run_solver(const char *name, SolverFn solve, const Graph *g,
                      int source, const ShortestPaths *ref)
{
    ShortestPaths sp;
    Timer t;
    double ms;
    int rc, agree;

    metrics_reset();
    timer_start(&t);
    rc = solve(g, source, &sp);
    ms = timer_elapsed_ms(&t);
    if (rc != 0) {
        printf("    %-14s failed (code %d)\n", name, rc);
        sp_free(&sp);
        return 0;
    }
    agree = same_distances(ref, &sp);
    printf("    %-14s elapsed_ms=%.3f ", name, ms);
    metrics_print();
    sp_free(&sp);
    return agree;
}

/* For each robot: Dijkstra with heap (prints the cost to every task), then
 * the array-scan version and Bellman-Ford for comparison. */
static void report_distances(const Problem *p, const Graph *g)
{
    ShortestPaths ref;
    Timer t;
    double ms;
    int r, k, d, source, agree;

    printf("shortest path costs (three algorithms per robot):\n");
    for (r = 0; r < p->n_robots; r++) {
        source = graph_node(g, p->robots[r].row, p->robots[r].col);
        metrics_reset();
        timer_start(&t);
        if (dijkstra(g, source, &ref) != 0) {
            printf("  robot %d: dijkstra failed\n", r + 1);
            continue;
        }
        ms = timer_elapsed_ms(&t);
        printf("  robot %d:", r + 1);
        for (k = 0; k < p->n_tasks; k++) {
            d = ref.dist[graph_node(g, p->tasks[k].row, p->tasks[k].col)];
            if (d >= DIST_INF) {
                printf(" T%d=unreachable", k + 1);
            } else {
                printf(" T%d=%d", k + 1, d);
            }
        }
        printf("\n    %-14s elapsed_ms=%.3f ", "dijkstra heap", ms);
        metrics_print();

        agree = 1;
        agree &= run_solver("dijkstra scan", dijkstra_scan, g, source, &ref);
        agree &= run_solver("bellman-ford", bellman_ford, g, source, &ref);
        printf("    all three give the same distances: %s\n", agree ? "yes" : "NO");
        sp_free(&ref);
    }
}

/* Print graph size and any unreachable tasks. Returns 0, or 1 on failure. */
static int report_reachability(const Problem *p, const Graph *g)
{
    int flags[MAX_TASKS];
    int bad, i;

    printf("graph: nodes=%d walkable=%d arcs=%d\n", g->n, g->n_walkable, g->n_arcs);
    bad = graph_unreachable_tasks(p, g, flags);
    if (bad < 0) {
        printf("graph: reachability check failed (out of memory?)\n");
        return 1;
    }
    if (bad == 0) {
        printf("reachability: every task can be reached by some robot\n");
        return 0;
    }
    printf("reachability: %d task(s) cannot be reached by any robot:\n", bad);
    for (i = 0; i < p->n_tasks; i++) {
        if (flags[i]) {
            printf("  task %d at (%d,%d)\n", i + 1, p->tasks[i].row, p->tasks[i].col);
        }
    }
    return 0;
}

/* ---------- planning ---------- */

/* Run the planner with one pair of strategies into *plan and print it.
 * Returns 0 on success. */
static int run_plan(const char *label, const Problem *p, const Graph *g,
                    int dp, int hk, Plan *plan)
{
    PlanOptions opt;
    Timer t;
    double ms;
    int r, i;

    opt.use_dp_knapsack = dp;
    opt.use_held_karp = hk;
    metrics_reset();
    timer_start(&t);
    if (plan_build(p, g, opt, plan) != 0) {
        printf("  %s: planning failed\n", label);
        return 1;
    }
    ms = timer_elapsed_ms(&t);
    printf("  %s\n", label);
    for (r = 0; r < plan->n_robots; r++) {
        printf("    robot %d: load %d/%d, value %d, cost %d/%d, stops:", r + 1,
               plan->robot[r].load, p->robots[r].capacity, plan->robot[r].value,
               plan->robot[r].cost, p->robots[r].battery);
        if (plan->robot[r].n_stops == 0) {
            printf(" none");
        }
        for (i = 0; i < plan->robot[r].n_stops; i++) {
            printf(" T%d", plan->robot[r].stop[i] + 1);
        }
        printf("\n");
    }
    printf("    unserved:");
    for (i = 0; i < plan->n_tasks; i++) {
        if (plan->assigned[i] < 0) {
            printf(" T%d", i + 1);
        }
    }
    printf("\n    TOTAL: served %d of %d, value %d, cost %d\n", plan->served,
           plan->n_tasks, plan->total_value, plan->total_cost);
    printf("    elapsed_ms=%.3f ", ms);
    metrics_print();
    return 0;
}

#define LABEL_A "A: knapsack DP + Held-Karp (exact per robot)"
#define LABEL_B "B: knapsack greedy + nearest neighbour (fast heuristics)"

/* The two strategy sets on the same input; both plans are kept in plans[]. */
static int report_plans(const Problem *p, const Graph *g, Plan plans[2])
{
    int rc;

    printf("plan (same input, two strategy sets):\n");
    rc = run_plan(LABEL_A, p, g, 1, 1, &plans[0]);
    rc |= run_plan(LABEL_B, p, g, 0, 0, &plans[1]);   /* separate statements: A prints first */
    return rc;
}

/* ---------- one-shot mode: planner <file> ---------- */

static int run_file(const char *path)
{
    static Plan plans[2];
    Problem p;
    Graph g;
    char err[200];
    ParseStatus st;
    int rc;

    st = parse_problem_file(path, &p, err, sizeof err);
    if (st != PARSE_OK) {
        printf("%s: %s\n", parse_status_name(st), err);
        return 1;
    }
    problem_print(&p);
    if (graph_build(&p, &g) != 0) {
        printf("graph: could not be built (out of memory?)\n");
        problem_free(&p);
        return 1;
    }
    rc = report_reachability(&p, &g);
    report_distances(&p, &g);
    rc |= report_plans(&p, &g, plans);
    graph_free(&g);
    problem_free(&p);
    return rc;
}

/* Plans A and B on the same input, their cell-by-cell routes, and one replay
 * file holding both. Returns 0 on success, 1 on failure (a message is printed). */
static int write_replay_for(const Problem *p, const Graph *g, const char *out_path)
{
    static Plan plans[2];
    static RouteSet routes[2];
    const char *labels[2] = {"A", "B"};
    PlanOptions opt;
    int k, rc = 0;

    memset(routes, 0, sizeof routes);
    for (k = 0; k < 2 && rc == 0; k++) {
        opt.use_dp_knapsack = (k == 0);
        opt.use_held_karp = (k == 0);
        if (plan_build(p, g, opt, &plans[k]) != 0) {
            printf("plan %s: planning failed\n", labels[k]);
            rc = 1;
        } else if (route_build(p, g, &plans[k], &routes[k]) != 0) {
            printf("plan %s: routes could not be built\n", labels[k]);
            rc = 1;
        }
    }
    if (rc == 0 && replay_write(out_path, p, 2, labels, plans, routes) != 0) {
        printf("cannot write the replay file %s here\n", out_path);
        rc = 1;
    }
    route_free(&routes[0]);
    route_free(&routes[1]);
    return rc;
}

static int run_export_replay(const char *in_path, const char *out_path)
{
    Problem p;
    Graph g;
    char err[200];
    ParseStatus st;
    int rc;

    st = parse_problem_file(in_path, &p, err, sizeof err);
    if (st != PARSE_OK) {
        printf("%s: %s\n", parse_status_name(st), err);
        return 1;
    }
    if (graph_build(&p, &g) != 0) {
        printf("graph: could not be built (out of memory?)\n");
        problem_free(&p);
        return 1;
    }
    rc = write_replay_for(&p, &g, out_path);
    if (rc == 0) {
        printf("wrote the replay of plans A and B to %s\n", out_path);
    }
    graph_free(&g);
    problem_free(&p);
    return rc;
}

/* planner --play <file>: read a replay file and play it. */
static int run_play_file(const char *path)
{
    static Replay rp;
    char err[200];
    int rc;

    err[0] = '\0';
    rc = replay_read(path, &rp, err, sizeof err);
    if (rc != 0) {
        printf("cannot play %s: %s\n", path, err[0] ? err : "file cannot be read");
        return 1;
    }
    rc = playrun_live(&rp, 0);                     /* real-time player on a console */
    if (rc == PLAYRUN_UNAVAILABLE) {
        rc = play_run(&rp, 0, stdin, stdout, PLAY_DELAY_MS, 1);   /* line-based */
    }
    replay_free(&rp);
    return rc == 0 ? 0 : 1;
}

/* ---------- interactive menu ---------- */

typedef struct {
    Problem p;
    Graph   g;
    int     loaded;
    char    name[260];
    Plan    plans[2];       /* plans kept for export                   */
    const char *labels[2];  /* strategy label of each kept plan        */
    int     n_plans;
} Session;

static Session s;

/* Prompt and read one line without the newline. Returns 0 at end of input. */
static int read_line(const char *prompt, char *buf, size_t size)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

/* Prompt for a whole number. Returns -1 on end of input or bad text. */
static int read_choice(const char *prompt)
{
    char buf[64], *end;
    long v;

    if (!read_line(prompt, buf, sizeof buf)) {
        return -1;
    }
    v = strtol(buf, &end, 10);
    if (end == buf) {
        return -2;                  /* no digits at all (also blank lines) */
    }
    while (*end == ' ' || *end == '\t') {
        end++;                      /* a trailing space is harmless */
    }
    if (*end != '\0' || v < 0 || v > 1000) {
        return -2;
    }
    return (int)v;
}

static void unload(void)
{
    if (s.loaded) {
        graph_free(&s.g);
        problem_free(&s.p);
    }
    s.loaded = 0;
    s.n_plans = 0;
    s.name[0] = '\0';
}

/* Load and validate a file into the session. Returns 1 on success. */
static int load(const char *path)
{
    char err[200];
    ParseStatus st;

    unload();
    st = parse_problem_file(path, &s.p, err, sizeof err);
    if (st != PARSE_OK) {
        printf("%s: %s\n", parse_status_name(st), err);
        return 0;
    }
    if (graph_build(&s.p, &s.g) != 0) {
        printf("graph: could not be built (out of memory?)\n");
        problem_free(&s.p);
        return 0;
    }
    s.loaded = 1;
    strncpy(s.name, path, sizeof s.name - 1);
    s.name[sizeof s.name - 1] = '\0';
    printf("loaded %s: %dx%d map, %d robot(s), %d task(s)\n", s.name, s.p.rows,
           s.p.cols, s.p.n_robots, s.p.n_tasks);
    report_reachability(&s.p, &s.g);
    return 1;
}

static int need_loaded(void)
{
    if (!s.loaded) {
        printf("no problem loaded yet: choose 1 (load a file) or 7 (scenarios) first\n");
    }
    return s.loaded;
}

static void menu_plan_both(void)
{
    if (need_loaded() && report_plans(&s.p, &s.g, s.plans) == 0) {
        s.labels[0] = "A";
        s.labels[1] = "B";
        s.n_plans = 2;
    }
}

static void menu_plan_one(void)
{
    int k, m;

    if (!need_loaded()) {
        return;
    }
    k = read_choice("knapsack: 1 = DP (exact), 2 = greedy > ");
    m = read_choice("route:    1 = Held-Karp (exact), 2 = nearest neighbour > ");
    if ((k != 1 && k != 2) || (m != 1 && m != 2)) {
        printf("please answer 1 or 2 to both questions\n");
        return;
    }
    printf("plan (one strategy set):\n");
    if (run_plan(k == 1 ? "custom: knapsack DP" : "custom: knapsack greedy",
                 &s.p, &s.g, k == 1, m == 1, &s.plans[0]) == 0) {
        s.labels[0] = "custom";
        s.n_plans = 1;
    }
}

static void menu_export(void)
{
    int i;

    if (!need_loaded()) {
        return;
    }
    if (s.n_plans == 0) {
        printf("no plan yet: choose 4 or 5 first\n");
        return;
    }
    for (i = 0; i < s.n_plans; i++) {
        if (export_plan_csv(TASKS_CSV, ROBOTS_CSV, s.labels[i], &s.p,
                            &s.plans[i], i > 0) != 0) {
            printf("export failed: cannot write %s / %s here\n", TASKS_CSV, ROBOTS_CSV);
            return;
        }
    }
    printf("wrote %d plan(s) to %s and %s\n", s.n_plans, TASKS_CSV, ROBOTS_CSV);
}

static void menu_scenarios(void)
{
    static const char *file[4] = {
        "data/small/sample1.txt",
        "data/scenarios/shock_blocked_aisle.txt",
        "data/large/warehouse_62x80.txt",
        "data/scenarios/knapsack_trap.txt"
    };
    int c;

    printf("  1 normal day (sample1)\n  2 constraint shock (blocked aisle, capacity drop)\n"
           "  3 large stress (62x80)\n  4 knapsack trap (greedy picks the wrong load)\n");
    c = read_choice("scenario > ");
    if (c < 1 || c > 4) {
        printf("please choose 1 to 4\n");
        return;
    }
    if (load(file[c - 1])) {
        menu_plan_both();
    }
}

static void menu_show(void)
{
    if (need_loaded()) {
        problem_print(&s.p);
    }
}

static void menu_paths(void)
{
    if (need_loaded()) {
        report_distances(&s.p, &s.g);
    }
}

/* Menu 10: plan A and B with their routes, then play the replay. */
static void menu_play(void)
{
    static Replay rp;
    char err[200];
    int rc;

    if (!need_loaded()) {
        return;
    }
    if (write_replay_for(&s.p, &s.g, MENU_REPLAY) != 0) {
        return;
    }
    err[0] = '\0';
    rc = replay_read(MENU_REPLAY, &rp, err, sizeof err);
    if (rc != 0) {
        printf("the replay could not be read back: %s\n", err);
        return;
    }
    rc = playrun_live(&rp, 0);
    if (rc == PLAYRUN_UNAVAILABLE) {
        play_run(&rp, 0, stdin, stdout, PLAY_DELAY_MS, 1);
    }
    replay_free(&rp);
}

#define BENCH_CSV "benchmark.csv"

/* Benchmark table to the screen (to_csv = 0) or to BENCH_CSV (to_csv = 1). */
static void menu_bench(int to_csv)
{
    FILE *f;

    if (!to_csv) {
        printf("benchmark (counters are exact; times vary a little per run):\n");
        bench_run(stdout, 0);
        return;
    }
    f = fopen(BENCH_CSV, "w");
    if (f == NULL) {
        printf("cannot write %s here\n", BENCH_CSV);
        return;
    }
    bench_run(f, 1);
    if (fclose(f) != 0) {
        printf("writing %s failed\n", BENCH_CSV);
        return;
    }
    printf("wrote the benchmark table to %s\n", BENCH_CSV);
}

static void menu_load(void)
{
    char path[260];

    if (read_line("file path > ", path, sizeof path) && path[0] != '\0') {
        load(path);
    }
}

static int run_menu(void)
{
    int c;

    printf("\n");
    for (;;) {
        printf("\n=== Warehouse Planner ===  (loaded: %s)\n"
               " 1  load a problem file\n"
               " 2  show the problem\n"
               " 3  shortest paths (Dijkstra heap, Dijkstra scan, Bellman-Ford)\n"
               " 4  plan, both strategy sets side by side\n"
               " 5  plan, one strategy set of my choice\n"
               " 6  export the plan(s) to CSV\n"
               " 7  scenarios\n"
               " 8  benchmark table (all comparisons, on the screen)\n"
               " 9  benchmark table to CSV (" BENCH_CSV ")\n"
               " 10 watch the robots drive (ASCII replay, plan A and B)\n"
               " 0  quit\n",
               s.loaded ? s.name : "nothing");
        c = read_choice("choice > ");
        if (c == -1 || c == 0) {
            break;
        }
        switch (c) {
        case 1: menu_load(); break;
        case 2: menu_show(); break;
        case 3: menu_paths(); break;
        case 4: menu_plan_both(); break;
        case 5: menu_plan_one(); break;
        case 6: menu_export(); break;
        case 7: menu_scenarios(); break;
        case 8: menu_bench(0); break;
        case 9: menu_bench(1); break;
        case 10: menu_play(); break;
        default: printf("please type a number from the menu\n"); break;
        }
    }
    unload();
    printf("bye\n");
    return 0;
}

int main(int argc, char **argv)
{
    printf("Warehouse Robot Path and Task Planner\n");
    if (argc >= 2 && strcmp(argv[1], "--bench") == 0) {
        return bench_run(stdout, 0) == 0 ? 0 : 1;
    }
    if (argc >= 2 && strcmp(argv[1], "--export-replay") == 0) {
        if (argc != 4) {
            printf("usage: planner --export-replay <input-file> <output-file>\n");
            return 1;
        }
        return run_export_replay(argv[2], argv[3]);
    }
    if (argc >= 2 && strcmp(argv[1], "--play") == 0) {
        if (argc != 3) {
            printf("usage: planner --play <replay-file>\n");
            return 1;
        }
        return run_play_file(argv[2]);
    }
    if (argc >= 2) {
        return run_file(argv[1]);
    }
    return run_menu();
}
