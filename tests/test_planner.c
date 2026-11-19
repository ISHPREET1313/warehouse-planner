/*
 * test_planner.c - tests for planner.c (item 11).
 *
 * All maps are one-row corridors, so every distance is just the difference
 * of the column numbers (unit cost per step) and every number below was
 * worked out by hand. Columns start at 0. Task numbers in the checks are
 * 0-based (task 0 is the first T on the map).
 *
 *  A  "R.T.T.T"  one robot, three tasks at columns 2, 4, 6.
 *     Distances from R: 2, 4, 6. Best order 0,1,2 costs 2+2+2 = 6.
 *  B  same map, capacity 50, tasks (w10 v60) (w20 v100) (w30 v120):
 *     knapsack_dp takes tasks 1 and 2 (value 220), route 1,2 costs 4 + 2 = 6.
 *     knapsack_greedy takes tasks 0 and 1 (value 160), route costs 2 + 2 = 4.
 *  C  battery 5, three equal tasks: all three cost 6 > 5, so the highest
 *     numbered task (same priority and value) is dropped, leaving 0,1 at 4.
 *  D  "R.T.T.R" two robots: task at col 2 is nearest to robot 0 (2 vs 4),
 *     task at col 4 is nearest to robot 1 (2 vs 4). Cost 2 + 2 = 4.
 *  E  "RTT.R" robot 0 has capacity 10: both tasks (w10) are nearest to
 *     robot 0 (tie on task 1 goes to robot 0), the knapsack keeps the
 *     more valuable task 1 (v7), task 0 (v5) is offered to robot 1
 *     (distance 3). Robot 0 cost 2, robot 1 cost 3, total 5, value 12.
 *  F  "R#T" the task is walled off: nothing served, still a success.
 *  I  "R.T.T.T" battery 5; task 0 has priority 1 and value 100, tasks 1
 *     and 2 have priority 5 and value 1. Dropping goes by priority first:
 *     task 0 goes, then (equal priority and value) task 2. Task 1 stays,
 *     cost 4, value 1.
 */
#include <string.h>

#include "graph.h"
#include "harness.h"
#include "io.h"
#include "planner.h"
#include "problem.h"
#include "tests.h"

static Plan g_plan;

/* Parse text, build the graph, plan. Returns plan_build's code, or -2 if the
 * text itself does not parse (a mistake in the test). */
static int run(const char *text, int dp, int hk)
{
    Problem p;
    Graph g;
    PlanOptions opt;
    char err[200];
    int rc;

    opt.use_dp_knapsack = dp;
    opt.use_held_karp = hk;
    if (parse_problem_text(text, &p, err, sizeof err) != PARSE_OK) {
        return -2;
    }
    if (graph_build(&p, &g) != 0) {
        problem_free(&p);
        return -2;
    }
    rc = plan_build(&p, &g, opt, &g_plan);
    graph_free(&g);
    problem_free(&p);
    return rc;
}

#define LINE3 "MAP 1 7\nR.T.T.T\n"
#define EQUAL3 "TASK 10 5 1\nTASK 10 5 1\nTASK 10 5 1\n"

void test_planner(void)
{
    harness_suite("planner");

    /* A: everything fits, both exact and heuristic routing */
    check_long("A hk: return code", 0,
               run(LINE3 "ROBOT 100 100\n" EQUAL3, 1, 1));
    check_long("A hk: served", 3, g_plan.served);
    check_long("A hk: total value", 15, g_plan.total_value);
    check_long("A hk: total cost", 6, g_plan.total_cost);
    check_long("A hk: stops of robot 0", 3, g_plan.robot[0].n_stops);
    check_long("A hk: first stop", 0, g_plan.robot[0].stop[0]);
    check_long("A hk: second stop", 1, g_plan.robot[0].stop[1]);
    check_long("A hk: third stop", 2, g_plan.robot[0].stop[2]);
    check_long("A hk: load", 30, g_plan.robot[0].load);
    check_long("A nn: return code", 0,
               run(LINE3 "ROBOT 100 100\n" EQUAL3, 1, 0));
    check_long("A nn: total cost", 6, g_plan.total_cost);
    check_long("A nn: served", 3, g_plan.served);

    /* B: knapsack choice changes the plan (greedy is worse) */
    check_long("B dp: return code", 0,
               run(LINE3 "ROBOT 50 100\nTASK 10 60 1\nTASK 20 100 1\nTASK 30 120 1\n",
                   1, 1));
    check_long("B dp: served", 2, g_plan.served);
    check_long("B dp: total value", 220, g_plan.total_value);
    check_long("B dp: total cost", 6, g_plan.total_cost);
    check_long("B dp: task 0 unserved", -1, g_plan.assigned[0]);
    check_long("B dp: task 1 on robot 0", 0, g_plan.assigned[1]);
    check_long("B dp: task 2 on robot 0", 0, g_plan.assigned[2]);
    check_long("B dp: first stop", 1, g_plan.robot[0].stop[0]);
    check_long("B dp: second stop", 2, g_plan.robot[0].stop[1]);
    check_long("B dp: load", 50, g_plan.robot[0].load);
    check_long("B greedy: return code", 0,
               run(LINE3 "ROBOT 50 100\nTASK 10 60 1\nTASK 20 100 1\nTASK 30 120 1\n",
                   0, 1));
    check_long("B greedy: served", 2, g_plan.served);
    check_long("B greedy: total value", 160, g_plan.total_value);
    check_long("B greedy: total cost", 4, g_plan.total_cost);
    check_long("B greedy: task 2 unserved", -1, g_plan.assigned[2]);

    /* C: battery too low for all three, the last equal task is dropped */
    check_long("C battery: return code", 0,
               run(LINE3 "ROBOT 100 5\n" EQUAL3, 1, 1));
    check_long("C battery: served", 2, g_plan.served);
    check_long("C battery: total value", 10, g_plan.total_value);
    check_long("C battery: total cost", 4, g_plan.total_cost);
    check_long("C battery: task 2 unserved", -1, g_plan.assigned[2]);
    check_long("C battery: first stop", 0, g_plan.robot[0].stop[0]);
    check_long("C battery: second stop", 1, g_plan.robot[0].stop[1]);

    /* D: two robots, each takes the nearer task */
    check_long("D two robots: return code", 0,
               run("MAP 1 7\nR.T.T.R\nROBOT 100 100\nROBOT 100 100\n"
                   "TASK 1 1 1\nTASK 1 1 1\n", 1, 1));
    check_long("D two robots: task 0 on robot", 0, g_plan.assigned[0]);
    check_long("D two robots: task 1 on robot", 1, g_plan.assigned[1]);
    check_long("D two robots: total cost", 4, g_plan.total_cost);
    check_long("D two robots: robot 0 cost", 2, g_plan.robot[0].cost);
    check_long("D two robots: robot 1 cost", 2, g_plan.robot[1].cost);

    /* E: a refused task moves to the next robot in round two */
    check_long("E overflow: return code", 0,
               run("MAP 1 5\nRTT.R\nROBOT 10 100\nROBOT 100 100\n"
                   "TASK 10 5 1\nTASK 10 7 1\n", 1, 1));
    check_long("E overflow: served", 2, g_plan.served);
    check_long("E overflow: total value", 12, g_plan.total_value);
    check_long("E overflow: task 0 on robot", 1, g_plan.assigned[0]);
    check_long("E overflow: task 1 on robot", 0, g_plan.assigned[1]);
    check_long("E overflow: robot 0 cost", 2, g_plan.robot[0].cost);
    check_long("E overflow: robot 1 cost", 3, g_plan.robot[1].cost);
    check_long("E overflow: total cost", 5, g_plan.total_cost);

    /* F: unreachable task, and the empty cases */
    check_long("F walled task: return code", 0,
               run("MAP 1 3\nR#T\nROBOT 5 5\nTASK 1 1 1\n", 1, 1));
    check_long("F walled task: served", 0, g_plan.served);
    check_long("F walled task: unserved flag", -1, g_plan.assigned[0]);
    check_long("F walled task: total cost", 0, g_plan.total_cost);
    check_long("F no tasks: return code", 0,
               run("MAP 1 3\nR..\nROBOT 5 5\n", 1, 1));
    check_long("F no tasks: served", 0, g_plan.served);

    /* I: the battery drop goes by priority before value */
    check_long("I priority: return code", 0,
               run(LINE3 "ROBOT 100 5\nTASK 10 100 1\nTASK 10 1 5\nTASK 10 1 5\n",
                   1, 1));
    check_long("I priority: served", 1, g_plan.served);
    check_long("I priority: total value", 1, g_plan.total_value);
    check_long("I priority: total cost", 4, g_plan.total_cost);
    check_long("I priority: task 0 dropped", -1, g_plan.assigned[0]);
    check_long("I priority: task 1 kept", 0, g_plan.assigned[1]);
    check_long("I priority: task 2 dropped", -1, g_plan.assigned[2]);

    /* invalid arguments */
    {
        PlanOptions none = {1, 1};
        check_long("invalid: NULL everything", -1,
                   plan_build(NULL, NULL, none, NULL));
    }
}
