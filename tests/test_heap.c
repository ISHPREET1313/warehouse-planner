/*
 * test_heap.c - tests for heap.c (item 4).
 *
 * Counter expectations (comparisons, pushes, pops) were worked out by hand
 * from the rules in heap.c, not copied from the code's output. The
 * worked examples are written next to each check.
 */
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "heap.h"
#include "metrics.h"
#include "tests.h"

/* Small repeatable pseudo-random numbers (same on every machine). */
static unsigned long g_seed = 12345UL;

static int next_rand(int range)
{
    g_seed = (g_seed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (int)((g_seed >> 8) % (unsigned long)range);
}

/* Pop one item and check its key. A failed pop shows as a FAIL row. */
static void expect_pop(const char *name, MinHeap *h, int want_key)
{
    int key = 0, node = 0;

    if (heap_pop(h, &key, &node) != 0) {
        check_long(name, 1, 0);
    } else {
        check_long(name, want_key, key);
    }
}

/* Pop everything. Returns how many items came out; *sorted is 1 if the
 * keys never went down. */
static int drain(MinHeap *h, int *sorted)
{
    int key = 0, node = 0, prev = 0, count = 0;

    *sorted = 1;
    while (heap_pop(h, &key, &node) == 0) {
        if (count > 0 && key < prev) {
            *sorted = 0;
        }
        prev = key;
        count++;
    }
    return count;
}

/* ---------------------------------------------------------------- */

static void group_basic(void)
{
    MinHeap h;
    int keys[6] = { 5, 3, 8, 1, 9, 2 };
    int want[6] = { 1, 2, 3, 5, 8, 9 };
    int key = 0, node = 0, i;
    char name[48];

    harness_suite("heap: basic behaviour");

    check_long("init with capacity 8", 0, heap_init(&h, 8));
    check_long("new heap: size", 0, heap_size(&h));
    check_long("new heap: is empty", 1, heap_is_empty(&h));
    check_long("pop from an empty heap fails", -1, heap_pop(&h, &key, &node));
    check_long("push (5, 50)", 0, heap_push(&h, 5, 50));
    check_long("size after one push", 1, heap_size(&h));
    check_long("not empty after a push", 0, heap_is_empty(&h));
    check_long("pop the only item: status", 0, heap_pop(&h, &key, &node));
    check_long("pop the only item: key", 5, key);
    check_long("pop the only item: node", 50, node);
    check_long("size is 0 again", 0, heap_size(&h));

    for (i = 0; i < 6; i++) {
        heap_push(&h, keys[i], keys[i] * 10);
    }
    check_long("6 pushes: size", 6, heap_size(&h));
    check_long("6 pushes: min-heap rule holds", 1, heap_is_valid(&h));
    for (i = 0; i < 6; i++) {
        heap_pop(&h, &key, &node);
        sprintf(name, "pop %d: key", i + 1);
        check_long(name, want[i], key);
        sprintf(name, "pop %d: node goes with its key", i + 1);
        check_long(name, want[i] * 10, node);
    }
    check_long("empty after 6 pops", 1, heap_is_empty(&h));
    heap_free(&h);
}

static void group_boundary(void)
{
    MinHeap h;
    int key = 0, node = 0, count = 0, bad = 0, sum = 0, i;

    harness_suite("heap: boundary input");

    /* capacity 1 */
    check_long("init capacity 1", 0, heap_init(&h, 1));
    check_long("capacity 1: first push", 0, heap_push(&h, 4, 0));
    check_long("capacity 1: push on a full heap fails", -1, heap_push(&h, 2, 1));
    check_long("capacity 1: size stays 1", 1, heap_size(&h));
    expect_pop("capacity 1: the first item is still there", &h, 4);
    check_long("capacity 1: push works after a pop", 0, heap_push(&h, 2, 1));
    heap_free(&h);

    /* capacity out of range */
    check_long("init capacity 0 is rejected", -1, heap_init(&h, 0));
    heap_free(&h);
    check_long("init capacity -5 is rejected", -1, heap_init(&h, -5));
    heap_free(&h);
    check_long("init capacity above the maximum is rejected", -1,
               heap_init(&h, HEAP_MAX_CAPACITY + 1));
    heap_free(&h);

    /* negative keys */
    heap_init(&h, 4);
    heap_push(&h, -5, 0);
    heap_push(&h, 0, 1);
    heap_push(&h, -100, 2);
    heap_push(&h, 7, 3);
    expect_pop("negative keys: first pop", &h, -100);
    expect_pop("negative keys: second pop", &h, -5);
    expect_pop("negative keys: third pop", &h, 0);
    expect_pop("negative keys: fourth pop", &h, 7);
    heap_free(&h);

    /* extreme keys: comparisons must not overflow */
    heap_init(&h, 3);
    heap_push(&h, INT_MAX, 0);
    heap_push(&h, INT_MIN, 1);
    heap_push(&h, 0, 2);
    expect_pop("INT_MIN comes out first", &h, INT_MIN);
    expect_pop("0 comes out second", &h, 0);
    expect_pop("INT_MAX comes out last", &h, INT_MAX);
    heap_free(&h);

    /* equal keys: every item must still come out exactly once */
    heap_init(&h, 5);
    for (i = 1; i <= 5; i++) {
        heap_push(&h, 4, i);
    }
    while (heap_pop(&h, &key, &node) == 0) {
        count++;
        sum += node;
        if (key != 4) {
            bad++;
        }
    }
    check_long("equal keys: 5 items come out", 5, count);
    check_long("equal keys: every key is 4", 0, bad);
    check_long("equal keys: nodes 1..5 all returned (sum 15)", 15, sum);
    heap_free(&h);
}

static void group_invalid(void)
{
    MinHeap h, zero;
    int key = 0, node = 0;

    harness_suite("heap: invalid use");

    check_long("init on a NULL heap", -1, heap_init(NULL, 4));
    check_long("push on a NULL heap", -1, heap_push(NULL, 1, 1));
    check_long("pop on a NULL heap", -1, heap_pop(NULL, &key, &node));
    check_long("size of a NULL heap", 0, heap_size(NULL));
    check_long("a NULL heap counts as empty", 1, heap_is_empty(NULL));
    check_long("a NULL heap is not valid", 0, heap_is_valid(NULL));
    heap_free(NULL);
    check_long("heap_free(NULL) is safe", 1, 1);

    memset(&zero, 0, sizeof zero);
    check_long("push on a never-initialised heap", -1, heap_push(&zero, 1, 1));
    check_long("pop on a never-initialised heap", -1, heap_pop(&zero, &key, &node));

    heap_init(&h, 4);
    heap_push(&h, 1, 1);
    heap_push(&h, 2, 2);
    check_long("pop with NULL outputs still works", 0, heap_pop(&h, NULL, NULL));
    check_long("... and removed one item", 1, heap_size(&h));
    key = 77;
    heap_pop(&h, NULL, NULL);
    check_long("pop on an empty heap leaves *key alone", 77,
               (heap_pop(&h, &key, &node), key));

    heap_push(&h, 1, 1);
    heap_free(&h);
    check_long("heap_free: size is 0", 0, heap_size(&h));
    check_long("heap_free: items is NULL", 1, h.items == NULL);
    check_long("push after heap_free fails", -1, heap_push(&h, 1, 1));
    check_long("pop after heap_free fails", -1, heap_pop(&h, &key, &node));
    heap_free(&h);
    check_long("heap_free twice is safe", 1, h.items == NULL);

    /* the validator itself must notice a broken heap */
    heap_init(&h, 3);
    heap_push(&h, 1, 0);
    heap_push(&h, 2, 1);
    heap_push(&h, 3, 2);
    check_long("validator: good heap", 1, heap_is_valid(&h));
    h.items[0].key = 99;               /* break the rule on purpose */
    check_long("validator: broken heap is detected", 0, heap_is_valid(&h));
    heap_free(&h);
}

static void group_counters(void)
{
    MinHeap h;
    int i;

    harness_suite("heap: operation counters");

    metrics_reset();
    heap_init(&h, 10);
    check_long("init adds capacity * item size to aux_bytes",
               (long)(10 * sizeof(HeapItem)), (long)g_metrics.aux_bytes);
    heap_free(&h);

    /* Worked example. Push 1, 2, 3:
     *   push 1: slot 0, no parent            -> 0 comparisons
     *   push 2: slot 1, parent 1 <= 2        -> 1 comparison, stop
     *   push 3: slot 2, parent 1 <= 3        -> 1 comparison, stop
     * Pop (removes 1): last item 3 moves to the root, array [3, 2].
     *   sift down: one child (2), no child-vs-child test,
     *   3 <= 2 is false -> 1 comparison, swap; then a leaf.
     * Totals: 3 pushes, 1 pop, 3 comparisons. */
    heap_init(&h, 10);
    metrics_reset();
    heap_push(&h, 1, 0);
    heap_push(&h, 2, 1);
    heap_push(&h, 3, 2);
    check_long("3 ascending pushes: heap_pushes", 3, (long)g_metrics.heap_pushes);
    check_long("3 ascending pushes: comparisons", 2, (long)g_metrics.comparisons);
    expect_pop("pop returns the smallest key", &h, 1);
    check_long("after the pop: heap_pops", 1, (long)g_metrics.heap_pops);
    check_long("after the pop: comparisons (2 + 1)", 3, (long)g_metrics.comparisons);
    check_long("the other two items are 2 then 3 (valid)", 1, heap_is_valid(&h));
    heap_free(&h);

    /* Best case for push: ascending keys, one comparison each except the
     * first push. 100 pushes -> 99 comparisons. */
    heap_init(&h, 100);
    metrics_reset();
    for (i = 1; i <= 100; i++) {
        heap_push(&h, i, i);
    }
    check_long("100 ascending pushes: comparisons", 99, (long)g_metrics.comparisons);
    check_long("100 ascending pushes: heap_pushes", 100, (long)g_metrics.heap_pushes);
    heap_free(&h);

    /* Worst case for push: descending keys, each new key climbs to the root.
     * The item in slot s climbs floor(log2(s + 1)) levels:
     * slots 0..6 -> 0 + 1 + 1 + 2 + 2 + 2 + 2 = 10 comparisons. */
    heap_init(&h, 7);
    metrics_reset();
    for (i = 7; i >= 1; i--) {
        heap_push(&h, i, i);
    }
    check_long("7 descending pushes: comparisons", 10, (long)g_metrics.comparisons);
    check_long("7 descending pushes: the root is 1", 1, h.items[0].key);
    heap_free(&h);

    /* Failed operations are not counted. */
    heap_init(&h, 1);
    metrics_reset();
    heap_push(&h, 4, 0);
    heap_push(&h, 5, 1);               /* full: fails */
    heap_pop(&h, NULL, NULL);
    heap_pop(&h, NULL, NULL);          /* empty: fails */
    check_long("failed push is not counted", 1, (long)g_metrics.heap_pushes);
    check_long("failed pop is not counted", 1, (long)g_metrics.heap_pops);
    heap_free(&h);
}

static void group_stress(void)
{
    MinHeap h;
    int bad, sorted, count, i, round, key, node;
    char seen[1000];
    int dupes, wrong;

    harness_suite("heap: stress and adversarial input");

    g_seed = 12345UL;

    /* 100000 random keys */
    heap_init(&h, 100000);
    bad = 0;
    for (i = 0; i < 100000; i++) {
        if (heap_push(&h, next_rand(1000003), i) != 0) {
            bad++;
        }
    }
    check_long("100000 random pushes: all accepted", 0, bad);
    check_long("100000 random pushes: size", 100000, heap_size(&h));
    check_long("100000 random pushes: min-heap rule holds", 1, heap_is_valid(&h));
    count = drain(&h, &sorted);
    check_long("100000 random pops: how many", 100000, count);
    check_long("100000 random pops: keys never decrease", 1, sorted);
    check_long("100000 random pops: heap is empty", 1, heap_is_empty(&h));

    /* many equal keys (only 10 different values) */
    for (i = 0; i < 50000; i++) {
        heap_push(&h, next_rand(10), i);
    }
    count = drain(&h, &sorted);
    check_long("50000 keys from 10 values: how many", 50000, count);
    check_long("50000 keys from 10 values: sorted", 1, sorted);

    /* already sorted ascending, then descending */
    for (i = 0; i < 50000; i++) {
        heap_push(&h, i, i);
    }
    count = drain(&h, &sorted);
    check_long("50000 ascending keys: how many", 50000, count);
    check_long("50000 ascending keys: sorted", 1, sorted);
    for (i = 50000; i > 0; i--) {
        heap_push(&h, i, i);
    }
    count = drain(&h, &sorted);
    check_long("50000 descending keys: how many", 50000, count);
    check_long("50000 descending keys: sorted", 1, sorted);

    /* interleaved: push 3, pop 1, repeated; net +2 per round */
    bad = 0;
    for (round = 0; round < 20000; round++) {
        heap_push(&h, next_rand(1000003), round);
        heap_push(&h, next_rand(1000003), round);
        heap_push(&h, next_rand(1000003), round);
        heap_pop(&h, NULL, NULL);
        if (round % 1000 == 0 && !heap_is_valid(&h)) {
            bad++;
        }
    }
    check_long("interleaved: size is 2 * 20000", 40000, heap_size(&h));
    check_long("interleaved: rule held at every check", 0, bad);
    count = drain(&h, &sorted);
    check_long("interleaved: remaining items", 40000, count);
    check_long("interleaved: remaining keys sorted", 1, sorted);
    heap_free(&h);

    /* every pop must return a key together with its own node */
    heap_init(&h, 1000);
    memset(seen, 0, sizeof seen);
    for (i = 0; i < 1000; i++) {
        heap_push(&h, (i * 37) % 101, i);
    }
    dupes = 0;
    wrong = 0;
    count = 0;
    while (heap_pop(&h, &key, &node) == 0) {
        count++;
        if (key != (node * 37) % 101) {
            wrong++;
        }
        if (seen[node]) {
            dupes++;
        }
        seen[node] = 1;
    }
    check_long("pairs: 1000 items come out", 1000, count);
    check_long("pairs: every key still has its own node", 0, wrong);
    check_long("pairs: no node comes out twice", 0, dupes);
    heap_free(&h);

    /* the rule must hold after EVERY single push and pop */
    heap_init(&h, 300);
    bad = 0;
    for (i = 0; i < 300; i++) {
        heap_push(&h, next_rand(50), i);
        if (!heap_is_valid(&h)) {
            bad++;
        }
    }
    check_long("rule holds after each of 300 pushes", 0, bad);
    bad = 0;
    for (i = 0; i < 300; i++) {
        heap_pop(&h, NULL, NULL);
        if (!heap_is_valid(&h)) {
            bad++;
        }
    }
    check_long("rule holds after each of 300 pops", 0, bad);
    heap_free(&h);
}

void test_heap(void)
{
    group_basic();
    group_boundary();
    group_invalid();
    group_counters();
    group_stress();
}
