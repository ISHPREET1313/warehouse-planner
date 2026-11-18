/* test_metrics.c - tests for the counters and the timer. */
#include "harness.h"
#include "metrics.h"
#include "tests.h"

void test_metrics(void)
{
    Timer t;
    double first, later;
    volatile long sink = 0;   /* volatile: stops the compiler deleting the busy loop */
    long i;

    harness_suite("metrics");

    /* ---- normal: each macro bumps only its own counter ---- */
    metrics_reset();
    COUNT_COMPARISON();
    COUNT_COMPARISON();
    COUNT_COMPARISON();
    COUNT_RELAXATION();
    COUNT_HEAP_PUSH();
    COUNT_HEAP_PUSH();
    COUNT_HEAP_POP();
    ADD_AUX_BYTES(100);
    ADD_AUX_BYTES(28);
    check_long("normal: comparisons after 3 calls",     3,   (long)g_metrics.comparisons);
    check_long("normal: relaxations after 1 call",      1,   (long)g_metrics.relaxations);
    check_long("normal: heap_pushes after 2 calls",     2,   (long)g_metrics.heap_pushes);
    check_long("normal: heap_pops after 1 call",        1,   (long)g_metrics.heap_pops);
    check_long("normal: aux_bytes after adding 100, 28", 128, (long)g_metrics.aux_bytes);

    /* ---- boundary: reset behaviour ---- */
    metrics_reset();
    check_long("boundary: comparisons right after reset", 0, (long)g_metrics.comparisons);
    check_long("boundary: aux_bytes right after reset",   0, (long)g_metrics.aux_bytes);
    COUNT_HEAP_PUSH();
    metrics_reset();
    check_long("boundary: reset clears a used counter",   0, (long)g_metrics.heap_pushes);

    /* ---- adversarial: many increments must not be lost ---- */
    metrics_reset();
    for (i = 0; i < 1000000; i++) {
        COUNT_COMPARISON();
    }
    check_long("adversarial: 1,000,000 comparisons", 1000000, (long)g_metrics.comparisons);

    /* ---- invalid: NULL timer must not crash ---- */
    check_long("invalid: NULL timer reports 0 ms", 1, timer_elapsed_ms(NULL) == 0.0);

    /* ---- timer sanity (exact ms cannot be tested, so test the ordering) ---- */
    timer_start(&t);
    first = timer_elapsed_ms(&t);
    for (i = 0; i < 5000000; i++) {
        sink += i;
    }
    later = timer_elapsed_ms(&t);
    check_long("timer: first reading is not negative", 1, first >= 0.0);
    check_long("timer: later reading is not smaller",  1, later >= first);
}
