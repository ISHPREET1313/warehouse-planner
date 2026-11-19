/*
 * test_export.c - tests for export.c (item 13).
 *
 * Same corridor "R.T.T.T" as the planner tests (tasks at columns 2, 4, 6,
 * each weight 10, value 5, priority 1). Expected lines worked out by hand:
 *  everything fits (capacity 100, battery 100): order 1,2,3, cost 6, load 30,
 *     value 15, so robot row "A,1,3,30,100,15,6,100";
 *  battery 5: task 3 is dropped, robot row "A,1,2,20,100,10,4,5" and the
 *     unserved row "A,none,0,3,0,6,10,5,1" comes after the served rows;
 *  append = 1 adds rows without a second header.
 */
#include <stdio.h>
#include <string.h>

#include "export.h"
#include "graph.h"
#include "harness.h"
#include "io.h"
#include "planner.h"
#include "problem.h"
#include "tests.h"

#define TASKS_FILE  "test_export_tasks.csv"
#define ROBOTS_FILE "test_export_robots.csv"

static Plan g_plan;

/* Build a plan for text; returns 0 if everything worked. The Problem is
 * returned in *p (caller frees). */
static int make_plan(const char *text, Problem *p)
{
    Graph g;
    PlanOptions opt = {1, 1};
    char err[200];
    int rc;

    if (parse_problem_text(text, p, err, sizeof err) != PARSE_OK) {
        return -2;
    }
    if (graph_build(p, &g) != 0) {
        return -2;
    }
    rc = plan_build(p, &g, opt, &g_plan);
    graph_free(&g);
    return rc;
}

/* Copy line number n (0-based) of a file into buf; "(missing)" if absent. */
static void line_of(const char *path, int n, char *buf, size_t size)
{
    FILE *f = fopen(path, "r");
    int i = 0;

    strcpy(buf, "(missing)");
    if (f == NULL) {
        return;
    }
    while (fgets(buf, (int)size, f) != NULL) {
        if (i == n) {
            buf[strcspn(buf, "\r\n")] = '\0';
            fclose(f);
            return;
        }
        i++;
    }
    strcpy(buf, "(missing)");
    fclose(f);
}

static int count_lines(const char *path)
{
    FILE *f = fopen(path, "r");
    char buf[200];
    int n = 0;

    if (f == NULL) {
        return -1;
    }
    while (fgets(buf, sizeof buf, f) != NULL) {
        n++;
    }
    fclose(f);
    return n;
}

void test_export(void)
{
    Problem p;
    char buf[200];

    harness_suite("export");

    /* everything fits */
    check_long("fits: plan built", 0,
               make_plan("MAP 1 7\nR.T.T.T\nROBOT 100 100\n"
                         "TASK 10 5 1\nTASK 10 5 1\nTASK 10 5 1\n", &p));
    check_long("fits: export code", 0,
               export_plan_csv(TASKS_FILE, ROBOTS_FILE, "A", &p, &g_plan, 0));
    check_long("fits: tasks file lines", 4, count_lines(TASKS_FILE));
    line_of(TASKS_FILE, 0, buf, sizeof buf);
    check_str("fits: tasks header", "strategy,robot,order,task,row,col,weight,value,priority", buf);
    line_of(TASKS_FILE, 1, buf, sizeof buf);
    check_str("fits: tasks row 1", "A,1,1,1,0,2,10,5,1", buf);
    line_of(TASKS_FILE, 2, buf, sizeof buf);
    check_str("fits: tasks row 2", "A,1,2,2,0,4,10,5,1", buf);
    line_of(TASKS_FILE, 3, buf, sizeof buf);
    check_str("fits: tasks row 3", "A,1,3,3,0,6,10,5,1", buf);
    check_long("fits: robots file lines", 2, count_lines(ROBOTS_FILE));
    line_of(ROBOTS_FILE, 0, buf, sizeof buf);
    check_str("fits: robots header", "strategy,robot,stops,load,capacity,value,cost,battery", buf);
    line_of(ROBOTS_FILE, 1, buf, sizeof buf);
    check_str("fits: robots row", "A,1,3,30,100,15,6,100", buf);
    problem_free(&p);

    /* battery drop leaves one task unserved; second plan appended */
    check_long("battery: plan built", 0,
               make_plan("MAP 1 7\nR.T.T.T\nROBOT 100 5\n"
                         "TASK 10 5 1\nTASK 10 5 1\nTASK 10 5 1\n", &p));
    check_long("battery: export code", 0,
               export_plan_csv(TASKS_FILE, ROBOTS_FILE, "A", &p, &g_plan, 0));
    check_long("battery: tasks file lines", 4, count_lines(TASKS_FILE));
    line_of(TASKS_FILE, 2, buf, sizeof buf);
    check_str("battery: second served row", "A,1,2,2,0,4,10,5,1", buf);
    line_of(TASKS_FILE, 3, buf, sizeof buf);
    check_str("battery: unserved row", "A,none,0,3,0,6,10,5,1", buf);
    line_of(ROBOTS_FILE, 1, buf, sizeof buf);
    check_str("battery: robots row", "A,1,2,20,100,10,4,5", buf);
    check_long("append: export code", 0,
               export_plan_csv(TASKS_FILE, ROBOTS_FILE, "B", &p, &g_plan, 1));
    check_long("append: tasks file lines", 7, count_lines(TASKS_FILE));
    check_long("append: robots file lines", 3, count_lines(ROBOTS_FILE));
    line_of(TASKS_FILE, 4, buf, sizeof buf);
    check_str("append: first new row has no header", "B,1,1,1,0,2,10,5,1", buf);
    line_of(ROBOTS_FILE, 2, buf, sizeof buf);
    check_str("append: robots row", "B,1,2,20,100,10,4,5", buf);

    /* invalid arguments */
    check_long("invalid: NULL path", -1,
               export_plan_csv(NULL, ROBOTS_FILE, "A", &p, &g_plan, 0));
    check_long("invalid: NULL plan", -1,
               export_plan_csv(TASKS_FILE, ROBOTS_FILE, "A", &p, NULL, 0));
    check_long("invalid: NULL label", -1,
               export_plan_csv(TASKS_FILE, ROBOTS_FILE, NULL, &p, &g_plan, 0));
    check_long("invalid: folder does not exist", -1,
               export_plan_csv("no_such_folder/t.csv", ROBOTS_FILE, "A", &p, &g_plan, 0));
    problem_free(&p);

    remove(TASKS_FILE);
    remove(ROBOTS_FILE);
}
