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
    /* later: test_bellman_ford(); ... */
    return harness_summary();
}
