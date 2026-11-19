/*
 * test_route.c - tests for route.c (item 24). Expected values are worked
 * out by hand from the maps below, not copied from program output.
 *
 * Corridor "R.T.T.T" (1 x 7): robot at column 0, tasks at columns 2, 4, 6.
 *   order T1 T2 T3: route is nodes 0..6, cost 6, picks at ticks 2, 4, 6.
 *   order T3 T1 T2: 6 steps to column 6, 4 back to column 2, 2 forward to
 *     column 4: cost 12, 13 nodes, picks at ticks 6, 10, 12.
 * sample1 (6 x 8), node id = row * 8 + col. Robot 1 starts at (2,1) = 17,
 *   stops T2 at (4,2) = 34, then T1 at (2,6) = 22. Shelves at (3,1), (3,2)
 *   force the way down column 0, so the first leg is the unique 5-step path
 *   17,16,24,32,33,34. Then the second leg must run along row 4 to column 7
 *   and up to (2,6) through (2,7), because (2,5) is a dead end: the unique
 *   8-step path 35,36,37,38,39,31,23,22. Cost 13, 14 nodes, picks at ticks
 *   5 and 13. Robot 2 starts at (4,5) = 37 and takes T3 at (5,6) = 46 in 2
 *   steps (two equal paths exist, so only the ends and the length are fixed).
 * Open 500 x 500 map, corner to corner: 499 + 499 = 998 steps, last node
 *   499 * 500 + 499 = 249999.
 */
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "harness.h"
#include "io.h"
#include "planner.h"
#include "problem.h"
#include "route.h"
#include "tests.h"

/* Number of steps in rt that are not a move to a 4-neighbour walkable cell,
 * plus one if the start cell is not walkable. 0 means a valid route. */
static int bad_steps(const Problem *p, const RobotRoute *rt)
{
    int i, bad = 0, dr, dc;

    if (!cell_is_walkable(p->cells[rt->node[0]])) {
        bad++;
    }
    for (i = 1; i <= rt->n_steps; i++) {
        dr = rt->node[i - 1] / p->cols - rt->node[i] / p->cols;
        dc = rt->node[i - 1] % p->cols - rt->node[i] % p->cols;
        if (dr * dr + dc * dc != 1 || !cell_is_walkable(p->cells[rt->node[i]])) {
            bad++;
        }
    }
    return bad;
}

/* A Plan with the given stops for robot 0 and nothing for other robots. */
static void one_robot_plan(Plan *plan, int n_robots, int n_tasks,
                           const int *stops, int n_stops, int cost)
{
    int i;

    memset(plan, 0, sizeof *plan);
    plan->n_robots = n_robots;
    plan->n_tasks = n_tasks;
    for (i = 0; i < n_tasks; i++) {
        plan->assigned[i] = -1;
    }
    for (i = 0; i < n_stops; i++) {
        plan->robot[0].stop[i] = stops[i];
        plan->assigned[stops[i]] = 0;
    }
    plan->robot[0].n_stops = n_stops;
    plan->robot[0].cost = cost;
}

static int load_text(const char *text, Problem *p, Graph *g)
{
    char err[200];

    if (parse_problem_text(text, p, err, sizeof err) != PARSE_OK) {
        return -1;
    }
    if (graph_build(p, g) != 0) {
        problem_free(p);
        return -1;
    }
    return 0;
}

static const char CORRIDOR[] = "MAP 1 7\nR.T.T.T\nROBOT 100 100\n"
                               "TASK 10 5 1\nTASK 10 5 1\nTASK 10 5 1\n";

static void corridor_tests(void)
{
    static Plan plan;
    static RouteSet rs;
    Problem p;
    Graph g;
    int fwd[3] = {0, 1, 2}, back[3] = {2, 0, 1}, i;

    if (load_text(CORRIDOR, &p, &g) != 0) {
        check_long("corridor: loaded", 0, 1);
        return;
    }
    one_robot_plan(&plan, 1, 3, fwd, 3, 6);
    check_long("fwd: route_build code", 0, route_build(&p, &g, &plan, &rs));
    check_long("fwd: robots", 1, rs.n_robots);
    check_long("fwd: steps", 6, rs.robot[0].n_steps);
    check_long("fwd: picks", 3, rs.robot[0].n_picks);
    for (i = 0; i <= 6; i++) {
        check_long("fwd: node i is column i", i, rs.robot[0].node[i]);
    }
    check_long("fwd: pick tick 1", 2, rs.robot[0].pick_tick[0]);
    check_long("fwd: pick tick 2", 4, rs.robot[0].pick_tick[1]);
    check_long("fwd: pick tick 3", 6, rs.robot[0].pick_tick[2]);
    check_long("fwd: bad steps", 0, bad_steps(&p, &rs.robot[0]));
    route_free(&rs);
    check_long("free: node pointer reset", 1, rs.robot[0].node == NULL);
    route_free(&rs);                       /* second call is harmless */
    route_free(NULL);

    one_robot_plan(&plan, 1, 3, back, 3, 12);
    check_long("back: route_build code", 0, route_build(&p, &g, &plan, &rs));
    check_long("back: steps", 12, rs.robot[0].n_steps);
    check_long("back: pick tick 1", 6, rs.robot[0].pick_tick[0]);
    check_long("back: pick tick 2", 10, rs.robot[0].pick_tick[1]);
    check_long("back: pick tick 3", 12, rs.robot[0].pick_tick[2]);
    check_long("back: node at tick 6", 6, rs.robot[0].node[6]);
    check_long("back: node at tick 10", 2, rs.robot[0].node[10]);
    check_long("back: node at tick 12", 4, rs.robot[0].node[12]);
    check_long("back: node at tick 7 (turning back)", 5, rs.robot[0].node[7]);
    check_long("back: bad steps", 0, bad_steps(&p, &rs.robot[0]));
    route_free(&rs);

    one_robot_plan(&plan, 1, 3, fwd, 0, 0);          /* no stops at all */
    check_long("empty: route_build code", 0, route_build(&p, &g, &plan, &rs));
    check_long("empty: steps", 0, rs.robot[0].n_steps);
    check_long("empty: picks", 0, rs.robot[0].n_picks);
    check_long("empty: start node", 0, rs.robot[0].node[0]);
    route_free(&rs);

    one_robot_plan(&plan, 1, 3, fwd, 3, 5);          /* cost does not match */
    check_long("wrong cost 5: code -2", -2, route_build(&p, &g, &plan, &rs));
    check_long("wrong cost 5: rs left empty", 1, rs.robot[0].node == NULL);
    one_robot_plan(&plan, 1, 3, fwd, 3, 7);
    check_long("wrong cost 7: code -2", -2, route_build(&p, &g, &plan, &rs));

    /* invalid arguments */
    one_robot_plan(&plan, 1, 3, fwd, 3, 6);
    check_long("invalid: NULL rs", -1, route_build(&p, &g, &plan, NULL));
    check_long("invalid: NULL problem", -1, route_build(NULL, &g, &plan, &rs));
    check_long("invalid: NULL graph", -1, route_build(&p, NULL, &plan, &rs));
    check_long("invalid: NULL plan", -1, route_build(&p, &g, NULL, &rs));
    plan.n_robots = 2;
    check_long("invalid: more plan robots than problem robots", -1,
               route_build(&p, &g, &plan, &rs));
    plan.n_robots = 1;
    plan.robot[0].stop[1] = 3;                       /* task 4 does not exist */
    check_long("invalid: stop out of range", -1, route_build(&p, &g, &plan, &rs));
    plan.robot[0].stop[1] = -1;
    check_long("invalid: negative stop", -1, route_build(&p, &g, &plan, &rs));
    plan.robot[0].stop[1] = 1;
    plan.robot[0].cost = -1;
    check_long("invalid: negative cost", -1, route_build(&p, &g, &plan, &rs));
    plan.robot[0].cost = 6;
    plan.robot[0].n_stops = MAX_TASKS + 1;
    check_long("invalid: too many stops", -1, route_build(&p, &g, &plan, &rs));
    graph_free(&g);
    problem_free(&p);
}

static void unreachable_test(void)
{
    static Plan plan;
    static RouteSet rs;
    Problem p;
    Graph g;
    int stops[1] = {0};

    if (load_text("MAP 1 3\nR#T\nROBOT 10 10\nTASK 1 1 1\n", &p, &g) != 0) {
        check_long("sealed: loaded", 0, 1);
        return;
    }
    one_robot_plan(&plan, 1, 1, stops, 1, 5);
    check_long("sealed task: code -1", -1, route_build(&p, &g, &plan, &rs));
    check_long("sealed task: rs left empty", 1, rs.robot[0].node == NULL);
    graph_free(&g);
    problem_free(&p);
}

static void sample1_tests(void)
{
    static Plan plan;
    static RouteSet rs;
    static const int want[14] = {17, 16, 24, 32, 33, 34, 35, 36, 37, 38, 39,
                                 31, 23, 22};
    Problem p;
    Graph g;
    char err[200];
    int i;

    if (parse_problem_file("data/small/sample1.txt", &p, err, sizeof err) != PARSE_OK ||
        graph_build(&p, &g) != 0) {
        check_long("sample1: loaded", 0, 1);
        return;
    }
    memset(&plan, 0, sizeof plan);
    plan.n_robots = 2;
    plan.n_tasks = 3;
    plan.robot[0].n_stops = 2;
    plan.robot[0].stop[0] = 1;            /* T2 first */
    plan.robot[0].stop[1] = 0;            /* then T1  */
    plan.robot[0].cost = 13;
    plan.robot[1].n_stops = 1;
    plan.robot[1].stop[0] = 2;            /* robot 2 takes T3 */
    plan.robot[1].cost = 2;
    check_long("sample1: route_build code", 0, route_build(&p, &g, &plan, &rs));
    check_long("sample1: robots", 2, rs.n_robots);
    check_long("sample1 robot 1: steps", 13, rs.robot[0].n_steps);
    for (i = 0; i < 14; i++) {
        check_long("sample1 robot 1: node", want[i], rs.robot[0].node[i]);
    }
    check_long("sample1 robot 1: pick tick T2", 5, rs.robot[0].pick_tick[0]);
    check_long("sample1 robot 1: pick tick T1", 13, rs.robot[0].pick_tick[1]);
    check_long("sample1 robot 1: bad steps", 0, bad_steps(&p, &rs.robot[0]));
    check_long("sample1 robot 2: steps", 2, rs.robot[1].n_steps);
    check_long("sample1 robot 2: start", 37, rs.robot[1].node[0]);
    check_long("sample1 robot 2: end", 46, rs.robot[1].node[2]);
    check_long("sample1 robot 2: pick tick", 2, rs.robot[1].pick_tick[0]);
    check_long("sample1 robot 2: bad steps", 0, bad_steps(&p, &rs.robot[1]));
    route_free(&rs);
    graph_free(&g);
    problem_free(&p);
}

/* Open 500 x 500 map, one robot in a corner, one task in the opposite one. */
static void big_open_test(void)
{
    static Plan plan;
    static RouteSet rs;
    Problem p;
    Graph g;
    int stops[1] = {0};

    problem_init(&p);
    p.rows = 500;
    p.cols = 500;
    p.cells = (char *)malloc(250000);
    if (p.cells == NULL) {
        check_long("500x500: allocated", 0, 1);
        return;
    }
    memset(p.cells, '.', 250000);
    p.cells[0] = 'R';
    p.cells[249999] = 'T';
    p.n_robots = 1;
    p.robots[0].row = 0;
    p.robots[0].col = 0;
    p.robots[0].capacity = 10;
    p.robots[0].battery = 2000;
    p.n_tasks = 1;
    p.tasks[0].row = 499;
    p.tasks[0].col = 499;
    p.tasks[0].weight = 1;
    p.tasks[0].value = 1;
    p.tasks[0].priority = 1;
    if (graph_build(&p, &g) != 0) {
        check_long("500x500: graph built", 0, 1);
        problem_free(&p);
        return;
    }
    one_robot_plan(&plan, 1, 1, stops, 1, 998);
    check_long("500x500: route_build code", 0, route_build(&p, &g, &plan, &rs));
    check_long("500x500: steps", 998, rs.robot[0].n_steps);
    check_long("500x500: last node", 249999, rs.robot[0].node[998]);
    check_long("500x500: pick tick", 998, rs.robot[0].pick_tick[0]);
    check_long("500x500: bad steps", 0, bad_steps(&p, &rs.robot[0]));
    route_free(&rs);
    graph_free(&g);
    problem_free(&p);
}

/*
 * Real planner on real data files, both strategy sets. The invariants must
 * hold for every robot of every file: route_build returns 0 (so the route
 * length equals the plan's cost), every step is a legal move, each pick tick
 * stands on the right task cell, and the picks add up to the tasks served.
 */
static void planner_files_tests(void)
{
    static const char *file[] = {
        "data/small/sample1.txt",
        "data/small/sample2.txt",
        "data/medium/warehouse_30x40.txt",
        "data/large/warehouse_62x80.txt",
        "data/scenarios/knapsack_trap.txt",
        "data/scenarios/shock_blocked_aisle.txt"
    };
    static Plan plan;
    static RouteSet rs;
    PlanOptions opt;
    Problem p;
    Graph g;
    char err[200];
    int f, s, r, i, picks, bad, wrong_cell, t, node;

    for (f = 0; f < 6; f++) {
        if (parse_problem_file(file[f], &p, err, sizeof err) != PARSE_OK ||
            graph_build(&p, &g) != 0) {
            check_long(file[f], 0, 1);
            continue;
        }
        for (s = 0; s < 2; s++) {
            opt.use_dp_knapsack = s == 0;
            opt.use_held_karp = s == 0;
            if (plan_build(&p, &g, opt, &plan) != 0 ||
                route_build(&p, &g, &plan, &rs) != 0) {
                check_long(file[f], 0, 1);
                continue;
            }
            picks = 0;
            bad = 0;
            wrong_cell = 0;
            for (r = 0; r < rs.n_robots; r++) {
                bad += bad_steps(&p, &rs.robot[r]);
                picks += rs.robot[r].n_picks;
                for (i = 0; i < rs.robot[r].n_picks; i++) {
                    t = plan.robot[r].stop[i];
                    node = rs.robot[r].node[rs.robot[r].pick_tick[i]];
                    if (node != p.tasks[t].row * p.cols + p.tasks[t].col) {
                        wrong_cell++;
                    }
                    if (i > 0 && rs.robot[r].pick_tick[i] <= rs.robot[r].pick_tick[i - 1]) {
                        wrong_cell++;
                    }
                }
            }
            check_long("files: illegal steps", 0, bad);
            check_long("files: picks on wrong cell or out of order", 0, wrong_cell);
            check_long("files: picks equal tasks served", plan.served, picks);
            route_free(&rs);
        }
        graph_free(&g);
        problem_free(&p);
    }
}

void test_route(void)
{
    harness_suite("route");
    corridor_tests();
    unreachable_test();
    sample1_tests();
    big_open_test();
    planner_files_tests();
}
