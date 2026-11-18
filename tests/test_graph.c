/*
 * test_graph.c - tests for graph.c (item 3).
 *
 * Expected numbers for data/small/sample1.txt were counted by hand from the
 * map (not by the code under test): 48 cells, 38 walkable, 27 horizontal +
 * 20 vertical edges = 47 edges = 94 arcs.
 *
 * Run from the project root (make test does this).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "harness.h"
#include "io.h"
#include "problem.h"
#include "tests.h"

#define ONE_ROBOT  "ROBOT 5 10\n"
#define ONE_TASK   "TASK 1 1 1\n"

/* Parse text and build its graph. 0 = ok, -1 = failed. */
static int build_text(const char *text, Problem *p, Graph *g)
{
    char err[200];

    if (parse_problem_text(text, p, err, sizeof err) != PARSE_OK) {
        memset(g, 0, sizeof *g);
        return -1;
    }
    return graph_build(p, g);
}

/* Number of arcs leaving the node at (row, col). */
static int degree(const Graph *g, int row, int col)
{
    int u = graph_node(g, row, col);
    return g->first[u + 1] - g->first[u];
}

/* Arcs u->v with no matching arc v->u. A correct graph has 0. */
static int count_asymmetric(const Graph *g)
{
    int u, a, b, v, found, bad = 0;

    for (u = 0; u < g->n; u++) {
        for (a = g->first[u]; a < g->first[u + 1]; a++) {
            v = g->to[a];
            found = 0;
            for (b = g->first[v]; b < g->first[v + 1]; b++) {
                if (g->to[b] == u) {
                    found = 1;
                }
            }
            if (!found) {
                bad++;
            }
        }
    }
    return bad;
}

/* Arcs that start or end on a cell a robot cannot enter. Should be 0. */
static int count_bad_ends(const Problem *p, const Graph *g)
{
    int u, a, bad = 0;

    for (u = 0; u < g->n; u++) {
        for (a = g->first[u]; a < g->first[u + 1]; a++) {
            if (!cell_is_walkable(p->cells[u])
                || !cell_is_walkable(p->cells[g->to[a]])) {
                bad++;
            }
        }
    }
    return bad;
}

/* Arcs whose weight is not 1. Should be 0 for now. */
static int count_bad_weights(const Graph *g)
{
    int a, bad = 0;

    for (a = 0; a < g->n_arcs; a++) {
        if (g->weight[a] != 1) {
            bad++;
        }
    }
    return bad;
}

/* Build text, check walkable and arc counts, clean up. */
static void expect_counts(const char *label, const char *text,
                          int walkable, int arcs)
{
    Problem p;
    Graph g;
    char name[80];
    int rc = build_text(text, &p, &g);

    sprintf(name, "%s: build ok", label);
    check_long(name, 0, rc);
    sprintf(name, "%s: walkable cells", label);
    check_long(name, walkable, g.n_walkable);
    sprintf(name, "%s: arcs", label);
    check_long(name, arcs, g.n_arcs);
    graph_free(&g);
    problem_free(&p);
}

/* Build text, check how many tasks are unreachable, clean up. */
static void expect_unreachable(const char *label, const char *text, int want)
{
    Problem p;
    Graph g;

    if (build_text(text, &p, &g) != 0) {
        check_long(label, want, -99);
    } else {
        check_long(label, want, graph_unreachable_tasks(&p, &g, NULL));
    }
    graph_free(&g);
    problem_free(&p);
}

/* A rows x cols Problem built by hand: every cell is fill, plus one R at
 * (0,0). Used for the largest map without going through the parser. */
static int make_filled(Problem *p, int rows, int cols, char fill)
{
    problem_init(p);
    p->rows = rows;
    p->cols = cols;
    p->cells = (char *)malloc((size_t)rows * (size_t)cols);
    if (p->cells == NULL) {
        return -1;
    }
    memset(p->cells, fill, (size_t)rows * (size_t)cols);
    p->cells[0] = CELL_ROBOT;
    p->n_robots = 1;
    p->robots[0].row = 0;
    p->robots[0].col = 0;
    p->robots[0].capacity = 1;
    p->robots[0].battery = 1;
    return 0;
}

/* ---------------------------------------------------------------- */

static void group_symbols(void)
{
    harness_suite("graph: which cells are walkable");

    check_long("'.' free is walkable", 1, cell_is_walkable('.'));
    check_long("'C' charger is walkable", 1, cell_is_walkable('C'));
    check_long("'D' dock is walkable", 1, cell_is_walkable('D'));
    check_long("'R' robot start is walkable", 1, cell_is_walkable('R'));
    check_long("'T' task pickup is walkable", 1, cell_is_walkable('T'));
    check_long("'#' obstacle is not walkable", 0, cell_is_walkable('#'));
    check_long("'S' shelf is not walkable", 0, cell_is_walkable('S'));
    check_long("'\\0' is not walkable", 0, cell_is_walkable('\0'));
}

static void group_sample(void)
{
    Problem p;
    Graph g;
    int rc, u;

    harness_suite("graph: data/small/sample1.txt");

    rc = parse_problem_file("data/small/sample1.txt", &p, NULL, 0);
    check_long("parse sample1", PARSE_OK, rc);
    rc = graph_build(&p, &g);
    check_long("graph_build ok", 0, rc);

    check_long("nodes = 6 * 8", 48, g.n);
    check_long("walkable cells (hand count)", 38, g.n_walkable);
    check_long("arcs (hand count 2 * 47)", 94, g.n_arcs);
    check_long("first[0] is 0", 0, g.first[0]);
    check_long("first[n] equals n_arcs", 94, g.first[g.n]);

    check_long("degree of R (2,1): 2", 2, degree(&g, 2, 1));
    check_long("degree of D corner (0,0): 2", 2, degree(&g, 0, 0));
    check_long("degree of open cell (4,3): 4", 4, degree(&g, 4, 3));
    check_long("degree of shelf (1,1): 0", 0, degree(&g, 1, 1));
    check_long("degree of obstacle (2,3): 0", 0, degree(&g, 2, 3));

    u = graph_node(&g, 4, 3);
    check_long("node id of (4,3)", 35, u);
    check_long("arc 1 of (4,3) goes up to 27", 27, g.to[g.first[u]]);
    check_long("arc 2 of (4,3) goes down to 43", 43, g.to[g.first[u] + 1]);
    check_long("arc 3 of (4,3) goes left to 34", 34, g.to[g.first[u] + 2]);
    check_long("arc 4 of (4,3) goes right to 36", 36, g.to[g.first[u] + 3]);

    check_long("every arc has a reverse arc", 0, count_asymmetric(&g));
    check_long("no arc touches # or S", 0, count_bad_ends(&p, &g));
    check_long("every weight is 1", 0, count_bad_weights(&g));
    check_long("unreachable tasks in sample1", 0,
               graph_unreachable_tasks(&p, &g, NULL));

    check_long("graph_node(2,1)", 17, graph_node(&g, 2, 1));
    check_long("graph_node row too big", -1, graph_node(&g, 6, 0));
    check_long("graph_node col too big", -1, graph_node(&g, 0, 8));
    check_long("graph_node negative", -1, graph_node(&g, -1, 0));

    graph_free(&g);
    problem_free(&p);
}

static void group_small(void)
{
    Problem p;
    Graph g;

    harness_suite("graph: small and boundary maps");

    expect_counts("1x1 map", "MAP 1 1\nR\n" ONE_ROBOT, 1, 0);
    expect_counts("R with two obstacles", "MAP 1 3\nR##\n" ONE_ROBOT, 1, 0);
    expect_counts("2x2 all open", "MAP 2 2\nR.\n..\n" ONE_ROBOT, 4, 8);
    expect_counts("3x3 all open (12 edges)", "MAP 3 3\n.R.\n...\n...\n"
                  ONE_ROBOT, 9, 24);
    expect_counts("shelf blocks the aisle", "MAP 1 3\nRST\n" ONE_ROBOT ONE_TASK,
                  2, 0);
    expect_counts("charger and dock connect", "MAP 1 3\nCRD\n" ONE_ROBOT, 3, 4);

    build_text("MAP 3 3\n.R.\n...\n...\n" ONE_ROBOT, &p, &g);
    check_long("3x3: corner degree", 2, degree(&g, 0, 0));
    check_long("3x3: edge degree", 3, degree(&g, 0, 1));
    check_long("3x3: centre degree", 4, degree(&g, 1, 1));
    check_long("3x3: every arc has a reverse", 0, count_asymmetric(&g));
    graph_free(&g);
    problem_free(&p);
}

static void group_reach(void)
{
    Problem p;
    Graph g;
    int flags[4] = { 7, 7, 7, 7 };
    int n;

    harness_suite("graph: unreachable tasks (BFS)");

    expect_unreachable("task next to the robot",
                       "MAP 1 4\nR..T\n" ONE_ROBOT ONE_TASK, 0);
    expect_unreachable("obstacle between robot and task",
                       "MAP 1 5\nR.#.T\n" ONE_ROBOT ONE_TASK, 1);
    expect_unreachable("shelf between robot and task",
                       "MAP 1 3\nRST\n" ONE_ROBOT ONE_TASK, 1);
    expect_unreachable("no tasks at all",
                       "MAP 1 2\nR.\n" ONE_ROBOT, 0);
    expect_unreachable("second robot reaches the task",
                       "MAP 1 5\nR#.RT\n" ONE_ROBOT ONE_ROBOT ONE_TASK, 0);
    expect_unreachable("long snake path is found",
                       "MAP 3 5\nR.#.T\n#.#.#\n.....\n" ONE_ROBOT ONE_TASK, 0);
    expect_unreachable("snake path cut by one #",
                       "MAP 3 5\nR.#.T\n#.#.#\n..#..\n" ONE_ROBOT ONE_TASK, 1);

    /* Sealed robot and sealed task: both tasks unreachable. */
    build_text("MAP 3 3\nR#T\n#S#\n..T\n" ONE_ROBOT ONE_TASK ONE_TASK, &p, &g);
    n = graph_unreachable_tasks(&p, &g, flags);
    check_long("sealed map: unreachable count", 2, n);
    check_long("sealed map: flags[0]", 1, flags[0]);
    check_long("sealed map: flags[1]", 1, flags[1]);
    check_long("sealed map: walkable cells", 5, g.n_walkable);
    check_long("sealed map: arcs", 4, g.n_arcs);
    graph_free(&g);
    problem_free(&p);

    /* One task reachable, one not: flags show which. */
    build_text("MAP 1 6\nRT#.T.\n" ONE_ROBOT ONE_TASK ONE_TASK, &p, &g);
    n = graph_unreachable_tasks(&p, &g, flags);
    check_long("mixed map: unreachable count", 1, n);
    check_long("mixed map: flags[0] reachable", 0, flags[0]);
    check_long("mixed map: flags[1] unreachable", 1, flags[1]);
    graph_free(&g);
    problem_free(&p);
}

static void group_invalid(void)
{
    Problem p;
    Graph g;
    Graph empty;

    harness_suite("graph: invalid and adversarial");

    memset(&g, 0, sizeof g);
    memset(&empty, 0, sizeof empty);

    check_long("graph_build(NULL problem)", -1, graph_build(NULL, &g));
    problem_init(&p);
    check_long("graph_build(empty problem)", -1, graph_build(&p, &g));
    check_long("graph_build(NULL graph)", -1, graph_build(&p, NULL));
    check_long("graph_node(NULL graph)", -1, graph_node(NULL, 0, 0));
    check_long("unreachable(NULL problem)", -1,
               graph_unreachable_tasks(NULL, &g, NULL));
    check_long("unreachable(empty graph)", -1,
               graph_unreachable_tasks(&p, &empty, NULL));

    build_text("MAP 1 2\nR.\n" ONE_ROBOT, &p, &g);
    graph_free(&g);
    check_long("graph_free clears first", 1, g.first == NULL);
    graph_free(&g);
    check_long("graph_free twice is safe", 1, g.first == NULL);
    graph_free(NULL);
    check_long("graph_free(NULL) is safe", 1, 1);
    problem_free(&p);
}

static void group_stress(void)
{
    Problem p;
    Graph g;

    harness_suite("graph: largest maps");

    /* 500 x 500, everything open: the most arcs a legal map can have.
     * Edges = 500*499 horizontal + 500*499 vertical = 499000; arcs = 998000. */
    if (make_filled(&p, 500, 500, '.') == 0) {
        check_long("500x500 open: build ok", 0, graph_build(&p, &g));
        check_long("500x500 open: nodes", 250000, g.n);
        check_long("500x500 open: walkable", 250000, g.n_walkable);
        check_long("500x500 open: arcs", 998000, g.n_arcs);
        check_long("500x500 open: corner degree", 2, degree(&g, 0, 0));
        check_long("500x500 open: far corner degree", 2, degree(&g, 499, 499));
        check_long("500x500 open: edge degree", 3, degree(&g, 0, 250));
        check_long("500x500 open: inner degree", 4, degree(&g, 250, 250));
        check_long("500x500 open: reverse arcs", 0, count_asymmetric(&g));
        check_long("500x500 open: no tasks unreachable", 0,
                   graph_unreachable_tasks(&p, &g, NULL));
        graph_free(&g);
    }
    problem_free(&p);

    /* 500 x 500 of obstacles with one robot: almost no arcs. */
    if (make_filled(&p, 500, 500, '#') == 0) {
        check_long("500x500 walls: build ok", 0, graph_build(&p, &g));
        check_long("500x500 walls: walkable", 1, g.n_walkable);
        check_long("500x500 walls: arcs", 0, g.n_arcs);
        graph_free(&g);
    }
    problem_free(&p);
}

void test_graph(void)
{
    group_symbols();
    group_sample();
    group_small();
    group_reach();
    group_invalid();
    group_stress();
}
