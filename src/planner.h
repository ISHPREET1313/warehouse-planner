/*
 * planner.h - the planner pipeline (item 11): which robot does which task,
 * what does each robot load, and in what order does it visit its stops.
 * It only COMPUTES a Plan; printing is main.c's job (no I/O here).
 *
 * Inputs: a Problem (robots, tasks) and its Graph. PlanOptions choose the
 * strategies, so the same input can be run twice and compared:
 *     use_dp_knapsack  1 = knapsack_dp (exact)        0 = knapsack_greedy
 *     use_held_karp    1 = tsp_held_karp (exact, but only for up to
 *                          TSP_HK_MAX_STOPS stops; more stops fall back to
 *                          nearest neighbour)         0 = tsp_nearest
 *
 * Steps (every step is one algorithm we already built and tested):
 *  1. Dijkstra (heap) from every robot start and from every task cell gives
 *     all distances we need: robot -> task and task -> task.
 *  2. Assignment, in ROUNDS. Tasks are offered in the order: priority high
 *     to low, then value high to low, then task number. Each unserved task
 *     goes to the NEAREST robot that can reach it and has not already
 *     refused it (a tie goes to the lower robot number).
 *  3. Loading: each robot runs the knapsack on its candidate tasks (weight
 *     = task weight, value = task value, capacity = robot capacity). Tasks
 *     the knapsack leaves out become unserved again and are offered to
 *     the next nearest robot in the next round. Rounds stop when no
 *     unserved task has a robot left to try.
 *  4. Routing: the Held-Karp or nearest-neighbour order of the chosen
 *     stops, starting at the robot's start cell, no return trip.
 *  5. Battery: if the route cost is above the robot's battery, drop the
 *     task with the LOWEST priority (then lowest value, then highest task
 *     number), route again, and repeat until it fits.
 * Priority therefore decides who is offered first and who is dropped first;
 * the knapsack itself maximises value only (see docs/DECISIONS.md).
 *
 * Complexity (R robots, T tasks, V grid cells, rounds <= R*T): step 1
 * runs (R + T) Dijkstras, O((R + T) V log V). Each round runs at most R
 * knapsacks, O(T * C) each, and routes of k stops, O(k^2 2^k) with Held-Karp.
 * Auxiliary space: R*T + T*T distances, O(R*T) flags.
 */
#ifndef PLANNER_H
#define PLANNER_H

#include "graph.h"
#include "problem.h"

typedef struct {
    int use_dp_knapsack;
    int use_held_karp;
} PlanOptions;

typedef struct {
    int n_stops;               /* tasks this robot serves                  */
    int stop[MAX_TASKS];       /* task numbers (0-based) in visiting order */
    int cost;                  /* route cost from the start to the last stop */
    int load;                  /* total weight carried                     */
    int value;                 /* total value of the tasks served          */
} RobotPlan;

typedef struct {
    int       n_robots, n_tasks;
    RobotPlan robot[MAX_ROBOTS];
    int       assigned[MAX_TASKS];   /* robot number (0-based) or -1 = unserved */
    int       served;                /* tasks served                      */
    int       total_value;
    int       total_cost;
} Plan;

/*
 * Build the plan. Returns 0 on success (a plan with zero tasks served is
 * still a success) and -1 for NULL arguments, a robot or task cell outside
 * the graph, a knapsack capacity above KNAP_MAX_CAPACITY, or out of memory.
 */
int plan_build(const Problem *p, const Graph *g, PlanOptions opt, Plan *out);

#endif /* PLANNER_H */
