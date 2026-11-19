/* tests.h - one declaration per test file. run_tests.c calls them all. */
#ifndef TESTS_H
#define TESTS_H

void test_metrics(void);
void test_parser(void);
void test_graph(void);
void test_heap(void);
void test_dijkstra(void);
void test_dijkstra_scan(void);
void test_bellman_ford(void);
void test_files(void);
void test_knapsack(void);
void test_tsp(void);
void test_planner(void);
void test_export(void);
void test_bench(void);
void test_route(void);
void test_replay(void);
void test_play(void);
void test_playview(void);

#endif /* TESTS_H */
