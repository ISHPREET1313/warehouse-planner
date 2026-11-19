/* export.c - CSV output of a Plan. See export.h. */
#include "export.h"

#include <stdio.h>

static int write_tasks(FILE *f, const char *label, const Problem *p,
                       const Plan *plan, int header)
{
    int r, i, t;

    if (header && fprintf(f, "strategy,robot,order,task,row,col,weight,value,priority\n") < 0) {
        return -1;
    }
    for (r = 0; r < plan->n_robots; r++) {
        for (i = 0; i < plan->robot[r].n_stops; i++) {
            t = plan->robot[r].stop[i];
            if (fprintf(f, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", label, r + 1, i + 1,
                        t + 1, p->tasks[t].row, p->tasks[t].col,
                        p->tasks[t].weight, p->tasks[t].value,
                        p->tasks[t].priority) < 0) {
                return -1;
            }
        }
    }
    for (t = 0; t < plan->n_tasks; t++) {
        if (plan->assigned[t] < 0 &&
            fprintf(f, "%s,none,0,%d,%d,%d,%d,%d,%d\n", label, t + 1,
                    p->tasks[t].row, p->tasks[t].col, p->tasks[t].weight,
                    p->tasks[t].value, p->tasks[t].priority) < 0) {
            return -1;
        }
    }
    return 0;
}

static int write_robots(FILE *f, const char *label, const Problem *p,
                        const Plan *plan, int header)
{
    int r;

    if (header && fprintf(f, "strategy,robot,stops,load,capacity,value,cost,battery\n") < 0) {
        return -1;
    }
    for (r = 0; r < plan->n_robots; r++) {
        if (fprintf(f, "%s,%d,%d,%d,%d,%d,%d,%d\n", label, r + 1,
                    plan->robot[r].n_stops, plan->robot[r].load,
                    p->robots[r].capacity, plan->robot[r].value,
                    plan->robot[r].cost, p->robots[r].battery) < 0) {
            return -1;
        }
    }
    return 0;
}

int export_plan_csv(const char *tasks_path, const char *robots_path,
                    const char *label, const Problem *p, const Plan *plan,
                    int append)
{
    FILE *ft, *fr;
    int rc = 0;

    if (tasks_path == NULL || robots_path == NULL || label == NULL ||
        p == NULL || plan == NULL || plan->n_robots > p->n_robots ||
        plan->n_tasks > p->n_tasks) {
        return -1;
    }
    ft = fopen(tasks_path, append ? "a" : "w");
    if (ft == NULL) {
        return -1;
    }
    fr = fopen(robots_path, append ? "a" : "w");
    if (fr == NULL) {
        fclose(ft);
        return -1;
    }
    if (write_tasks(ft, label, p, plan, !append) != 0 ||
        write_robots(fr, label, p, plan, !append) != 0) {
        rc = -1;
    }
    if (fclose(ft) != 0) {
        rc = -1;
    }
    if (fclose(fr) != 0) {
        rc = -1;
    }
    return rc;
}
