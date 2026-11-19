/*
 * route.h - turn a Plan into cell-by-cell routes (item 24).
 *
 * The planner (planner.h) decides WHICH tasks a robot serves and in WHAT
 * ORDER; it never says which cells the robot walks through. This module
 * does: for each robot it walks the legs start -> stop 1 -> stop 2 -> ...,
 * runs Dijkstra (heap) from the start of each leg and reads the path back
 * with sp_path. The legs are joined without repeating the shared cell.
 * No new algorithm: it only reuses Dijkstra, and it has no I/O.
 *
 * Output per robot (RobotRoute):
 *   node[0..n_steps]  grid node ids (row * cols + col), one per tick;
 *                     node[0] is the robot's start cell, tick t = index t;
 *   pick_tick[i]      the tick at which the robot reaches stop i of the plan
 *                     (the task is picked up then): node[pick_tick[i]] is the
 *                     cell of task plan.robot[r].stop[i].
 * A robot with no stops has n_steps = 0 and only its start node.
 *
 * Invariants (tested):
 *   1. every step moves to a 4-neighbour walkable cell;
 *   2. n_steps equals plan.robot[r].cost (the open route has no return trip);
 *   3. the route reaches the stops in plan order and pick_tick increases.
 * If the length differs from the plan's cost, route_build returns -2: that
 * would be a bug in the planner or here, never silently accepted.
 *
 * Complexity per robot with k stops: k Dijkstra runs, O(k (V + E) log V),
 * plus O(V) for one temporary path buffer; the route itself takes
 * O(cost) memory. The Dijkstra runs add to the global counters (metrics.h).
 *
 * Memory: route_build allocates node arrays. Call route_free when done.
 * route_build resets the RouteSet itself, so it may start uninitialised,
 * but route_free needs a RouteSet that route_build (or memset 0) has set up.
 */
#ifndef ROUTE_H
#define ROUTE_H

#include "graph.h"
#include "planner.h"
#include "problem.h"

typedef struct {
    int  n_steps;               /* moves made; node has n_steps + 1 entries */
    int *node;                  /* malloc'd, NULL for an empty RobotRoute   */
    int  n_picks;               /* = number of stops in the plan            */
    int  pick_tick[MAX_TASKS];  /* tick of each stop, in plan order         */
} RobotRoute;

typedef struct {
    int        n_robots;
    RobotRoute robot[MAX_ROBOTS];
} RouteSet;

/*
 * Build the routes for plan. Returns 0 on success, -1 for NULL arguments,
 * a plan that does not fit p, a cell outside the graph, an unreachable
 * stop or out of memory (rs is then empty), -2 if a route's length differs
 * from the plan's cost (rs is then empty too).
 */
int route_build(const Problem *p, const Graph *g, const Plan *plan,
                RouteSet *rs);

/* Free every node array and reset rs. Safe on NULL and when called twice. */
void route_free(RouteSet *rs);

#endif /* ROUTE_H */
