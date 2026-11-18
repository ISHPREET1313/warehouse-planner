/*
 * main.c - entry point. Later this becomes the CLI menu (item 8).
 *
 * For now:  planner <input-file>   parses and validates the file, prints the
 *           problem, builds the graph and reports tasks no robot can reach,
 *           or prints the error message.
 *           planner                 prints the skeleton banner.
 */
#include <stdio.h>

#include "dijkstra.h"
#include "graph.h"
#include "io.h"
#include "metrics.h"
#include "problem.h"

/* Run Dijkstra (heap version) once per robot and print the cost from the
 * robot's start to every task, then that run's time and operation counts. */
static void report_distances(const Problem *p, const Graph *g)
{
    ShortestPaths sp;
    Timer t;
    int r, k, d;

    printf("shortest path costs (Dijkstra with heap):\n");
    for (r = 0; r < p->n_robots; r++) {
        metrics_reset();
        timer_start(&t);
        if (dijkstra(g, graph_node(g, p->robots[r].row, p->robots[r].col), &sp) != 0) {
            printf("  robot %d: dijkstra failed\n", r + 1);
            continue;
        }
        printf("  robot %d:", r + 1);
        for (k = 0; k < p->n_tasks; k++) {
            d = sp.dist[graph_node(g, p->tasks[k].row, p->tasks[k].col)];
            if (d >= DIST_INF) {
                printf(" T%d=unreachable", k + 1);
            } else {
                printf(" T%d=%d", k + 1, d);
            }
        }
        printf("\n    elapsed_ms=%.3f ", timer_elapsed_ms(&t));
        metrics_print();
        sp_free(&sp);
    }
}

/* Build the graph of p and print its size and any unreachable tasks. */
static int report_graph(const Problem *p)
{
    Graph g;
    int flags[MAX_TASKS];
    int bad, i;

    if (graph_build(p, &g) != 0) {
        printf("graph: could not be built (out of memory?)\n");
        return 1;
    }
    printf("graph: nodes=%d walkable=%d arcs=%d\n", g.n, g.n_walkable, g.n_arcs);

    bad = graph_unreachable_tasks(p, &g, flags);
    if (bad < 0) {
        printf("graph: reachability check failed (out of memory?)\n");
        graph_free(&g);
        return 1;
    }
    if (bad == 0) {
        printf("reachability: every task can be reached by some robot\n");
    } else {
        printf("reachability: %d task(s) cannot be reached by any robot:\n", bad);
        for (i = 0; i < p->n_tasks; i++) {
            if (flags[i]) {
                printf("  task %d at (%d,%d)\n", i + 1,
                       p->tasks[i].row, p->tasks[i].col);
            }
        }
    }
    report_distances(p, &g);
    graph_free(&g);
    return 0;
}

int main(int argc, char **argv)
{
    Timer t;
    Problem p;
    char err[200];
    ParseStatus st;
    int rc;

    printf("Warehouse Robot Path and Task Planner\n");

    if (argc >= 2) {
        st = parse_problem_file(argv[1], &p, err, sizeof err);
        if (st != PARSE_OK) {
            printf("%s: %s\n", parse_status_name(st), err);
            return 1;
        }
        problem_print(&p);
        rc = report_graph(&p);
        problem_free(&p);
        return rc;
    }

    printf("Skeleton build OK. Usage: planner <input-file>\n");
    metrics_reset();
    timer_start(&t);
    /* nothing to measure yet */
    printf("elapsed_ms=%.3f\n", timer_elapsed_ms(&t));
    metrics_print();
    return 0;
}
