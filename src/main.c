/*
 * main.c - entry point. Later this becomes the CLI menu (item 8).
 *
 * For now:  planner <input-file>   parses and validates the file and prints
 *           the problem, or prints the error message.
 *           planner                 prints the skeleton banner.
 */
#include <stdio.h>

#include "io.h"
#include "metrics.h"
#include "problem.h"

int main(int argc, char **argv)
{
    Timer t;
    Problem p;
    char err[200];
    ParseStatus st;

    printf("Warehouse Robot Path and Task Planner\n");

    if (argc >= 2) {
        st = parse_problem_file(argv[1], &p, err, sizeof err);
        if (st != PARSE_OK) {
            printf("%s: %s\n", parse_status_name(st), err);
            return 1;
        }
        problem_print(&p);
        problem_free(&p);
        return 0;
    }

    printf("Skeleton build OK. Usage: planner <input-file>\n");
    metrics_reset();
    timer_start(&t);
    /* nothing to measure yet */
    printf("elapsed_ms=%.3f\n", timer_elapsed_ms(&t));
    metrics_print();
    return 0;
}
