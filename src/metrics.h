/*
 * metrics.h - operation counters and a simple timer.
 *
 * Every algorithm bumps these counters through the macros below, so we can
 * compare algorithms by HOW MUCH WORK they do, not only by wall-clock time.
 *
 * What each counter means (keep this definition fixed, see DECISIONS.md):
 *   comparisons  - one comparison between two keys: a heap sift compare,
 *                  or one "is this smaller than the best so far?" test
 *                  in an array scan, or a comparison in a sort.
 *   relaxations  - one edge relaxation attempt: the test
 *                  "dist[u] + w < dist[v]?", whether or not it succeeds.
 *   heap_pushes  - one insert into the min-heap.
 *   heap_pops    - one extract-min from the min-heap.
 *   aux_bytes    - auxiliary memory an algorithm asked for (dist arrays,
 *                  heap array, ...), added by hand when it allocates.
 */
#ifndef METRICS_H
#define METRICS_H

#include <time.h>

typedef struct {
    unsigned long comparisons;
    unsigned long relaxations;
    unsigned long heap_pushes;
    unsigned long heap_pops;
    unsigned long aux_bytes;
} Metrics;

/* One global set of counters, defined in metrics.c. */
extern Metrics g_metrics;

/* Counting macros: algorithms call these, nothing else. */
#define COUNT_COMPARISON()  (g_metrics.comparisons++)
#define COUNT_RELAXATION()  (g_metrics.relaxations++)
#define COUNT_HEAP_PUSH()   (g_metrics.heap_pushes++)
#define COUNT_HEAP_POP()    (g_metrics.heap_pops++)
#define ADD_AUX_BYTES(n)    (g_metrics.aux_bytes += (unsigned long)(n))

/* Set every counter back to zero. Call before each measured run. */
void metrics_reset(void);

/* Print all counters on one line to stdout. */
void metrics_print(void);

/* ---- Timer (wall/CPU time via clock(), resolution about 1 ms on MinGW) ---- */
typedef struct {
    clock_t start;
} Timer;

void   timer_start(Timer *t);
/* Milliseconds since timer_start. Returns 0.0 if t is NULL. */
double timer_elapsed_ms(const Timer *t);

#endif /* METRICS_H */
