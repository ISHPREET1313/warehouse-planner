/*
 * graph.h - turn the grid of a Problem into a graph (item 3).
 *
 * Nodes: every grid cell is a node, with id = row * cols + col. Cells a
 * robot cannot enter (# obstacle, S shelf) are still nodes but have no arcs,
 * so they are isolated and no search can reach them.
 *
 * Walkable cells: . C D R T
 * Arcs: between 4-neighbours (up, down, left, right) that are both
 * walkable, one arc in each direction, cost 1. The weight is stored per arc
 * so other algorithms (Bellman-Ford) can use other weights later.
 *
 * Storage (compact adjacency lists, "CSR"): the arcs leaving node u are
 *     to[first[u]] ... to[first[u+1] - 1]      (weight[] in the same places)
 * so the degree of u is first[u+1] - first[u]. Arcs of one node are in the
 * order up, down, left, right.
 *
 * Build: time O(rows*cols), space O(V + E) for the graph plus an O(V)
 * temporary array. V = rows*cols, E = number of arcs (at most 4V).
 * No algorithm and no printing live here.
 */
#ifndef GRAPH_H
#define GRAPH_H

#include "problem.h"

typedef struct {
    int  rows, cols;
    int  n;             /* number of nodes = rows * cols                 */
    int  n_walkable;    /* how many cells are walkable                   */
    int  n_arcs;        /* total arcs (each edge counts twice)           */
    int *first;         /* n + 1 entries: start of each node's arc list  */
    int *to;            /* n_arcs entries: node the arc goes to          */
    int *weight;        /* n_arcs entries: cost of the arc (1 for now)   */
} Graph;

/* 1 if a robot may stand on this symbol (. C D R T), otherwise 0. */
int cell_is_walkable(char c);

/*
 * Build the graph of p. Returns 0 on success, -1 if p is NULL or empty or
 * memory runs out (g is then empty). Call graph_free(g) when done.
 */
int graph_build(const Problem *p, Graph *g);

/* Free the arrays and reset g. Safe on NULL and when called twice. */
void graph_free(Graph *g);

/* Node id of (row, col), or -1 if the position is outside the grid. */
int graph_node(const Graph *g, int row, int col);

/*
 * Multi-source BFS from every robot start. A task is unreachable if no
 * robot can walk to its cell. If flags is not NULL it must have room for
 * p->n_tasks ints: flags[i] = 1 if task i is unreachable, else 0.
 * Returns how many tasks are unreachable, or -1 if memory runs out.
 * Time O(V + E), auxiliary space O(V).
 */
int graph_unreachable_tasks(const Problem *p, const Graph *g, int *flags);

#endif /* GRAPH_H */
