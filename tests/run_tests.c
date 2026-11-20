/* run_tests.c - runs every test suite, then prints the summary. */
#include "harness.h"
#include "tests.h"

int main(void)
{
    test_metrics();
    test_parser();
    test_graph();
    test_heap();
    test_dijkstra();
    test_dijkstra_scan();
    test_bellman_ford();
    test_files();
    test_knapsack();
    test_tsp();
    test_planner();
    test_export();
    test_bench();
    test_route();
    test_replay();
    test_play();
    test_playview();
    test_playlive();
    return harness_summary();
}
