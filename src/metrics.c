/* metrics.c - counters and timer. See metrics.h for what each counter means. */
#include "metrics.h"

#include <stdio.h>

Metrics g_metrics;   /* global variables start at zero in C */

void metrics_reset(void)
{
    g_metrics.comparisons = 0;
    g_metrics.relaxations = 0;
    g_metrics.heap_pushes = 0;
    g_metrics.heap_pops   = 0;
    g_metrics.aux_bytes   = 0;
}

void metrics_print(void)
{
    printf("comparisons=%lu relaxations=%lu heap_pushes=%lu heap_pops=%lu aux_bytes=%lu\n",
           g_metrics.comparisons, g_metrics.relaxations,
           g_metrics.heap_pushes, g_metrics.heap_pops,
           g_metrics.aux_bytes);
}

void timer_start(Timer *t)
{
    if (t == NULL) {
        return;
    }
    t->start = clock();
}

double timer_elapsed_ms(const Timer *t)
{
    if (t == NULL) {
        return 0.0;
    }
    /* clock() counts ticks; CLOCKS_PER_SEC ticks make one second. */
    return (double)(clock() - t->start) * 1000.0 / (double)CLOCKS_PER_SEC;
}
