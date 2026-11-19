/*
 * test_bench.c - tests for bench.c (item 14).
 *
 * Run from the project root (it reads the data/ files). Only the repeatable
 * counters and results are checked, never the times. Expected values:
 *  - table shape: 5 data files x (3 path rows + 2 planner rows) = 25, plus
 *    6 knapsack cases x 2 = 12, plus 7 pickup sizes x 2 = 14, plus the
 *    header: 52 lines in CSV mode.
 *  - classic knapsack (capacity 50, weights 10,20,30): the DP loop visits
 *    (50-10+1) + (50-20+1) + (50-30+1) = 41 + 31 + 21 = 93 cells, memory
 *    51 ints (204 bytes) + 3 x 51 flag bytes (153) = 357; DP value 220,
 *    greedy value 160.
 *  - nearest neighbour counts n(n+1)/2 comparisons: 21 for n=6, 20100 for
 *    n=200. Held-Karp counts n(n-1)2^(n-2) + n: 486 for n=6, 23050 for n=10.
 *  - sample1, robot 1: the counters worked out in the dijkstra tests
 *    (heap: 121 comparisons, 94 relaxations, 38 pushes + 38 pops = 76,
 *    1144 bytes; scan: 1131 comparisons; Bellman-Ford: 401 relaxations).
 *  - planner on sample1: both strategy sets serve 3 tasks, value 65, cost 15.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "harness.h"
#include "tests.h"

#define BENCH_FILE "test_bench.csv"

static char g_text[200000];

/* Load the whole CSV into g_text. Returns its length, or -1. */
static int slurp(const char *path)
{
    FILE *f = fopen(path, "r");
    size_t n;

    if (f == NULL) {
        return -1;
    }
    n = fread(g_text, 1, sizeof g_text - 1, f);
    g_text[n] = '\0';
    fclose(f);
    return (int)n;
}

/* Pointer to the first line that starts with prefix, or NULL. */
static const char *find_line(const char *prefix)
{
    size_t len = strlen(prefix);
    const char *p = g_text;

    while (p != NULL && *p != '\0') {
        if (strncmp(p, prefix, len) == 0) {
            return p;
        }
        p = strchr(p, '\n');
        if (p != NULL) {
            p++;
        }
    }
    return NULL;
}

/* Number in comma-separated field idx (0-based) of the line starting with
 * prefix. For the result column (3) the number after '=' is used.
 * Returns -1 when the line or a number is missing. */
static long field_num(const char *prefix, int idx)
{
    const char *p = find_line(prefix);
    const char *eq;
    int i;

    if (p == NULL) {
        return -1;
    }
    for (i = 0; i < idx; i++) {
        p = strchr(p, ',');
        if (p == NULL) {
            return -1;
        }
        p++;
    }
    if (idx == 3) {
        eq = strchr(p, '=');
        if (eq == NULL || eq > strchr(p, ',')) {
            return -1;
        }
        p = eq + 1;
    }
    return strtol(p, NULL, 10);
}

static int count_newlines(void)
{
    int n = 0;
    const char *p;

    for (p = g_text; *p != '\0'; p++) {
        if (*p == '\n') {
            n++;
        }
    }
    return n;
}

void test_bench(void)
{
    FILE *f;
    int rc;
    long nn, hk;

    harness_suite("bench");

    check_long("invalid: NULL output", -1, bench_run(NULL, 1));

    f = fopen(BENCH_FILE, "w");
    rc = (f == NULL) ? -9 : bench_run(f, 1);
    if (f != NULL) {
        fclose(f);
    }
    check_long("csv run: return code", 0, rc);
    check_long("csv run: file read", 1, slurp(BENCH_FILE) > 0);
    check_long("csv run: 52 lines", 52, count_newlines());
    check_long("csv run: header",
               1, find_line("section,case,algorithm,result,comparisons,relaxations,heap_ops,aux_bytes,ms_per_run\n") == g_text);

    /* knapsack, classic case */
    check_long("classic DP: value", 220,
               field_num("knapsack,classic n=3 cap=50,DP (exact),", 3));
    check_long("classic DP: comparisons", 93,
               field_num("knapsack,classic n=3 cap=50,DP (exact),", 4));
    check_long("classic DP: aux bytes", 357,
               field_num("knapsack,classic n=3 cap=50,DP (exact),", 7));
    check_long("classic greedy: value", 160,
               field_num("knapsack,classic n=3 cap=50,greedy,", 3));

    /* pickup order counters and quality */
    check_long("NN n=6: comparisons", 21,
               field_num("pickup-order,random stops=6,nearest neighbour,", 4));
    check_long("NN n=200: comparisons", 20100,
               field_num("pickup-order,random stops=200,nearest neighbour,", 4));
    check_long("HK n=6: comparisons", 486,
               field_num("pickup-order,random stops=6,Held-Karp (exact),", 4));
    check_long("HK n=10: comparisons", 23050,
               field_num("pickup-order,random stops=10,Held-Karp (exact),", 4));
    check_long("HK n=200 is skipped", 1,
               find_line("pickup-order,random stops=200,Held-Karp (exact),n/a") != NULL);
    nn = field_num("pickup-order,random stops=10,nearest neighbour,", 3);
    hk = field_num("pickup-order,random stops=10,Held-Karp (exact),", 3);
    check_long("n=10: exact cost <= heuristic cost", 1, hk > 0 && hk <= nn);
    nn = field_num("pickup-order,random stops=14,nearest neighbour,", 3);
    hk = field_num("pickup-order,random stops=14,Held-Karp (exact),", 3);
    check_long("n=14: exact cost <= heuristic cost", 1, hk > 0 && hk <= nn);

    /* shortest paths: counters of sample1 and agreement on every file */
    check_long("sample1 heap: comparisons", 121,
               field_num("paths,sample1,dijkstra heap,", 4));
    check_long("sample1 heap: relaxations", 94,
               field_num("paths,sample1,dijkstra heap,", 5));
    check_long("sample1 heap: heap ops", 76,
               field_num("paths,sample1,dijkstra heap,", 6));
    check_long("sample1 heap: aux bytes", 1144,
               field_num("paths,sample1,dijkstra heap,", 7));
    check_long("sample1 scan: comparisons", 1131,
               field_num("paths,sample1,dijkstra scan,", 4));
    check_long("sample1 bellman-ford: relaxations", 401,
               field_num("paths,sample1,bellman-ford,", 5));
    check_long("sample1: scan agrees with heap", field_num("paths,sample1,dijkstra heap,", 3),
               field_num("paths,sample1,dijkstra scan,", 3));
    check_long("sample1: bellman-ford agrees with heap", field_num("paths,sample1,dijkstra heap,", 3),
               field_num("paths,sample1,bellman-ford,", 3));
    check_long("large: scan agrees with heap", field_num("paths,warehouse_62x80,dijkstra heap,", 3),
               field_num("paths,warehouse_62x80,dijkstra scan,", 3));
    check_long("large: bellman-ford agrees with heap", field_num("paths,warehouse_62x80,dijkstra heap,", 3),
               field_num("paths,warehouse_62x80,bellman-ford,", 3));
    check_long("snake: scan agrees with heap", field_num("paths,snake_21x21,dijkstra heap,", 3),
               field_num("paths,snake_21x21,dijkstra scan,", 3));
    check_long("snake: bellman-ford agrees with heap", field_num("paths,snake_21x21,dijkstra heap,", 3),
               field_num("paths,snake_21x21,bellman-ford,", 3));
    check_long("large: heap does fewer comparisons than scan", 1,
               field_num("paths,warehouse_62x80,dijkstra heap,", 4) <
               field_num("paths,warehouse_62x80,dijkstra scan,", 4));

    /* planner rows */
    check_long("sample1 planner A: row", 1,
               find_line("planner,sample1,A: DP + Held-Karp,served=3 value=65 cost=15,") != NULL);
    check_long("sample1 planner B: row", 1,
               find_line("planner,sample1,B: greedy + nearest,served=3 value=65 cost=15,") != NULL);

    /* text mode runs too and has the same number of lines */
    f = fopen(BENCH_FILE, "w");
    rc = (f == NULL) ? -9 : bench_run(f, 0);
    if (f != NULL) {
        fclose(f);
    }
    check_long("text run: return code", 0, rc);
    check_long("text run: file read", 1, slurp(BENCH_FILE) > 0);
    check_long("text run: 52 lines", 52, count_newlines());

    remove(BENCH_FILE);
}
