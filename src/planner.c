/* planner.c - planner pipeline. See planner.h for the steps. */
#include "planner.h"

#include <stdlib.h>
#include <string.h>

#include "dijkstra.h"
#include "knapsack.h"
#include "tsp.h"

/* Everything the steps share. */
typedef struct {
    const Problem *p;
    const Graph   *g;
    PlanOptions    opt;
    int            nr, nt;
    int           *rt;        /* nr*nt: rt[r*nt+j] = distance robot r -> task j */
    int           *tt;        /* nt*nt: tt[i*nt+j] = distance task i -> task j  */
    char          *tried;     /* nt*nr: 1 if robot r already had task t as candidate */
    int           *cand;      /* nr*nt: candidate list of each robot             */
    int           *ncand;     /* nr: length of each candidate list               */
    int           *assigned;  /* nt: robot or -1                                 */
    Plan          *plan;
} Work;

/* Dijkstra from src_node; row[j] = distance to task j. 0 ok, -1 failure. */
static int fill_row(const Work *w, int src_node, int *row)
{
    ShortestPaths sp;
    int j;

    if (dijkstra(w->g, src_node, &sp) != 0) {
        sp_free(&sp);
        return -1;
    }
    for (j = 0; j < w->nt; j++) {
        row[j] = sp.dist[graph_node(w->g, w->p->tasks[j].row, w->p->tasks[j].col)];
    }
    sp_free(&sp);
    return 0;
}

/* Step 1: all distances. Task rows are only computed for reachable tasks. */
static int compute_distances(Work *w)
{
    int r, i, j, reachable;

    for (r = 0; r < w->nr; r++) {
        int node = graph_node(w->g, w->p->robots[r].row, w->p->robots[r].col);
        if (node < 0 || fill_row(w, node, w->rt + r * w->nt) != 0) {
            return -1;
        }
    }
    for (i = 0; i < w->nt; i++) {
        int node = graph_node(w->g, w->p->tasks[i].row, w->p->tasks[i].col);
        if (node < 0) {
            return -1;
        }
        reachable = 0;
        for (r = 0; r < w->nr; r++) {
            if (w->rt[r * w->nt + i] < DIST_INF) {
                reachable = 1;
            }
        }
        if (reachable) {
            if (fill_row(w, node, w->tt + i * w->nt) != 0) {
                return -1;
            }
        } else {
            for (j = 0; j < w->nt; j++) {
                w->tt[i * w->nt + j] = DIST_INF;
            }
        }
    }
    return 0;
}

/*
 * Step 4: best order of the k tasks in list (all reachable by robot r).
 * order_out[i] = task number visited i-th, *cost = route cost. 0 ok, -1 failure.
 */
static int route(const Work *w, int r, const int *list, int k,
                 int *order_out, int *cost)
{
    int n1 = k + 1;
    int a, b, i, rc;
    int *d;
    int ord[MAX_TASKS];

    d = (int *)malloc((size_t)n1 * (size_t)n1 * sizeof *d);
    if (d == NULL) {
        return -1;
    }
    for (a = 0; a < n1; a++) {
        for (b = 0; b < n1; b++) {
            if (a == b) {
                d[a * n1 + b] = 0;
            } else if (a == 0) {
                d[a * n1 + b] = w->rt[r * w->nt + list[b - 1]];
            } else if (b == 0) {
                d[a * n1 + b] = w->rt[r * w->nt + list[a - 1]];
            } else {
                d[a * n1 + b] = w->tt[list[a - 1] * w->nt + list[b - 1]];
            }
        }
    }
    if (w->opt.use_held_karp && k <= TSP_HK_MAX_STOPS) {
        rc = tsp_held_karp(d, k, ord, cost);
    } else {
        rc = tsp_nearest(d, k, ord, cost);
    }
    free(d);
    if (rc != 0) {
        return -1;
    }
    for (i = 0; i < k; i++) {
        order_out[i] = list[ord[i] - 1];
    }
    return 0;
}

/* Position in list of the task to drop when the battery is too low:
 * lowest priority, then lowest value, then highest task number. */
static int drop_victim(const Work *w, const int *list, int k)
{
    int best = 0, i;

    for (i = 1; i < k; i++) {
        const Task *a = &w->p->tasks[list[i]];
        const Task *b = &w->p->tasks[list[best]];
        if (a->priority < b->priority ||
            (a->priority == b->priority && a->value < b->value) ||
            (a->priority == b->priority && a->value == b->value &&
             list[i] > list[best])) {
            best = i;
        }
    }
    return best;
}

/* Steps 3 and 5 for one robot: knapsack on its candidates, then route and
 * battery trimming. Updates assigned[], cand[r], ncand[r] and plan->robot[r]. */
static int solve_robot(Work *w, int r)
{
    int *list = w->cand + r * w->nt;
    int k = w->ncand[r];
    int weight[MAX_TASKS], value[MAX_TASKS], chosen[MAX_TASKS];
    int order[MAX_TASKS];
    long sum_w = 0;
    int cap, i, kept, cost = 0, rc;
    RobotPlan *rp = &w->plan->robot[r];

    memset(order, 0, sizeof order);
    for (i = 0; i < k; i++) {
        weight[i] = w->p->tasks[list[i]].weight;
        value[i] = w->p->tasks[list[i]].value;
        sum_w += weight[i];
    }
    cap = w->p->robots[r].capacity;
    if (sum_w < cap) {
        cap = (int)sum_w;
    }
    if (w->opt.use_dp_knapsack) {
        rc = knapsack_dp(weight, value, k, cap, chosen, NULL);
    } else {
        rc = knapsack_greedy(weight, value, k, cap, chosen, NULL);
    }
    if (rc != 0) {
        return -1;
    }

    kept = 0;
    for (i = 0; i < k; i++) {
        if (chosen[i]) {
            list[kept++] = list[i];
        } else {
            w->assigned[list[i]] = -1;      /* refused: offered elsewhere later */
        }
    }
    k = kept;

    while (k > 0) {
        if (route(w, r, list, k, order, &cost) != 0) {
            return -1;
        }
        if (cost <= w->p->robots[r].battery) {
            break;
        }
        i = drop_victim(w, list, k);
        w->assigned[list[i]] = -1;
        list[i] = list[k - 1];
        k--;
    }

    w->ncand[r] = k;
    memset(rp, 0, sizeof *rp);
    rp->n_stops = k;
    for (i = 0; i < k; i++) {
        rp->stop[i] = order[i];
        w->assigned[list[i]] = r;
        rp->load += w->p->tasks[list[i]].weight;
        rp->value += w->p->tasks[list[i]].value;
    }
    rp->cost = (k > 0) ? cost : 0;
    return 0;
}

/* Offer order: priority high to low, then value high to low, then number. */
static void offer_order(const Problem *p, int *ord)
{
    int i, j, t;

    for (i = 0; i < p->n_tasks; i++) {
        ord[i] = i;
    }
    for (i = 1; i < p->n_tasks; i++) {
        t = ord[i];
        j = i - 1;
        while (j >= 0 &&
               (p->tasks[ord[j]].priority < p->tasks[t].priority ||
                (p->tasks[ord[j]].priority == p->tasks[t].priority &&
                 p->tasks[ord[j]].value < p->tasks[t].value))) {
            ord[j + 1] = ord[j];
            j--;
        }
        ord[j + 1] = t;
    }
}

int plan_build(const Problem *p, const Graph *g, PlanOptions opt, Plan *out)
{
    Work w;
    int ord[MAX_TASKS];
    char dirty[MAX_ROBOTS];
    int rc = -1, added, oi, t, r, best, bd, d;

    if (p == NULL || g == NULL || out == NULL || p->n_robots < 1 ||
        p->n_robots > MAX_ROBOTS || p->n_tasks < 0 || p->n_tasks > MAX_TASKS) {
        return -1;
    }
    memset(out, 0, sizeof *out);
    out->n_robots = p->n_robots;
    out->n_tasks = p->n_tasks;

    memset(&w, 0, sizeof w);
    w.p = p;
    w.g = g;
    w.opt = opt;
    w.nr = p->n_robots;
    w.nt = p->n_tasks;
    w.plan = out;
    for (t = 0; t < w.nt; t++) {
        out->assigned[t] = -1;
    }
    if (w.nt == 0) {
        return 0;
    }

    w.rt = (int *)malloc((size_t)w.nr * (size_t)w.nt * sizeof *w.rt);
    w.tt = (int *)malloc((size_t)w.nt * (size_t)w.nt * sizeof *w.tt);
    w.tried = (char *)calloc((size_t)w.nr * (size_t)w.nt, 1);
    w.cand = (int *)malloc((size_t)w.nr * (size_t)w.nt * sizeof *w.cand);
    w.ncand = (int *)calloc((size_t)w.nr, sizeof *w.ncand);
    w.assigned = (int *)malloc((size_t)w.nt * sizeof *w.assigned);
    if (w.rt == NULL || w.tt == NULL || w.tried == NULL || w.cand == NULL ||
        w.ncand == NULL || w.assigned == NULL) {
        goto done;
    }
    for (t = 0; t < w.nt; t++) {
        w.assigned[t] = -1;
    }
    if (compute_distances(&w) != 0) {
        goto done;
    }

    offer_order(p, ord);
    for (;;) {
        added = 0;
        memset(dirty, 0, sizeof dirty);
        for (oi = 0; oi < w.nt; oi++) {
            t = ord[oi];
            if (w.assigned[t] != -1) {
                continue;
            }
            best = -1;
            bd = 0;
            for (r = 0; r < w.nr; r++) {
                d = w.rt[r * w.nt + t];
                if (w.tried[t * w.nr + r] || d >= DIST_INF) {
                    continue;
                }
                if (best < 0 || d < bd) {
                    best = r;
                    bd = d;
                }
            }
            if (best >= 0) {
                w.tried[t * w.nr + best] = 1;
                w.cand[best * w.nt + w.ncand[best]++] = t;
                dirty[best] = 1;
                added = 1;
            }
        }
        if (!added) {
            break;
        }
        for (r = 0; r < w.nr; r++) {
            if (dirty[r] && solve_robot(&w, r) != 0) {
                goto done;
            }
        }
    }

    for (t = 0; t < w.nt; t++) {
        out->assigned[t] = w.assigned[t];
        if (w.assigned[t] >= 0) {
            out->served++;
            out->total_value += p->tasks[t].value;
        }
    }
    for (r = 0; r < w.nr; r++) {
        out->total_cost += out->robot[r].cost;
    }
    rc = 0;

done:
    free(w.rt);
    free(w.tt);
    free(w.tried);
    free(w.cand);
    free(w.ncand);
    free(w.assigned);
    return rc;
}
