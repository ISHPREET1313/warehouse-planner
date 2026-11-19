/* route.c - cell-by-cell routes from a Plan. See route.h. */
#include "route.h"

#include <stdlib.h>
#include <string.h>

#include "dijkstra.h"

void route_free(RouteSet *rs)
{
    int r;

    if (rs == NULL) {
        return;
    }
    for (r = 0; r < MAX_ROBOTS; r++) {
        free(rs->robot[r].node);
    }
    memset(rs, 0, sizeof *rs);
}

/*
 * Build the route of robot r. tmp is a buffer of g->n ints for one leg.
 * Returns 0, -1 (bad input, unreachable stop, memory) or -2 (length differs
 * from the plan's cost).
 */
static int build_robot(const Problem *p, const Graph *g, const Plan *plan,
                       int r, int *tmp, RobotRoute *out)
{
    const RobotPlan *rp = &plan->robot[r];
    ShortestPaths sp;
    long cap_nodes;
    int cur, goal, len, i, t, m;

    if (rp->n_stops < 0 || rp->n_stops > MAX_TASKS || rp->cost < 0) {
        return -1;
    }
    /* a shortest leg has fewer than g->n nodes, so this bounds the route */
    cap_nodes = (long)g->n * (rp->n_stops + 1);
    if ((long)rp->cost + 1 > cap_nodes) {
        return -1;
    }
    out->node = (int *)malloc(((size_t)rp->cost + 1) * sizeof(int));
    if (out->node == NULL) {
        return -1;
    }
    cur = graph_node(g, p->robots[r].row, p->robots[r].col);
    if (cur < 0) {
        return -1;
    }
    out->node[0] = cur;
    len = 1;
    out->n_picks = rp->n_stops;
    for (i = 0; i < rp->n_stops; i++) {
        t = rp->stop[i];
        if (t < 0 || t >= p->n_tasks) {
            return -1;
        }
        goal = graph_node(g, p->tasks[t].row, p->tasks[t].col);
        if (goal < 0 || dijkstra(g, cur, &sp) != 0) {
            return -1;
        }
        m = sp_path(&sp, goal, tmp, g->n);   /* nodes cur .. goal, m >= 1 */
        sp_free(&sp);
        if (m <= 0) {
            return -1;                       /* unreachable stop           */
        }
        if (len + m - 1 > rp->cost + 1) {
            return -2;                       /* route longer than the cost */
        }
        memcpy(out->node + len, tmp + 1, (size_t)(m - 1) * sizeof(int));
        len += m - 1;
        out->pick_tick[i] = len - 1;
        cur = goal;
    }
    out->n_steps = len - 1;
    return out->n_steps == rp->cost ? 0 : -2;
}

int route_build(const Problem *p, const Graph *g, const Plan *plan,
                RouteSet *rs)
{
    int *tmp;
    int r, rc = 0;

    if (rs == NULL) {
        return -1;
    }
    memset(rs, 0, sizeof *rs);
    if (p == NULL || g == NULL || plan == NULL || g->n <= 0 ||
        plan->n_robots < 0 || plan->n_robots > p->n_robots ||
        plan->n_robots > MAX_ROBOTS || plan->n_tasks > p->n_tasks) {
        return -1;
    }
    tmp = (int *)malloc((size_t)g->n * sizeof(int));
    if (tmp == NULL) {
        return -1;
    }
    for (r = 0; r < plan->n_robots && rc == 0; r++) {
        rc = build_robot(p, g, plan, r, tmp, &rs->robot[r]);
    }
    free(tmp);
    if (rc != 0) {
        route_free(rs);
        return rc;
    }
    rs->n_robots = plan->n_robots;
    return 0;
}
