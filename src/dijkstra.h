/*
 * dijkstra.h - shortest paths from one start node, using the min-heap
 * (item 5, the core algorithm).
 *
 * Input : a Graph (graph.h) and a source node. Arc weights must be whole
 *         numbers from 0 to DIJKSTRA_MAX_WEIGHT (Dijkstra is wrong for
 *         negative weights, so they are rejected).
 * Output: dist[v] = length of the shortest path source -> v, or DIST_INF if
 *         v cannot be reached; prev[v] = the node before v on one such
 *         shortest path (-1 for the source and for unreachable nodes).
 *
 * How it works (lazy deletion, no decrease-key):
 *   dist[source] = 0, everything else DIST_INF, push (0, source).
 *   Repeat: pop the pair (d, u) with the smallest d.
 *     - If d > dist[u], a shorter way to u was found after this pair was
 *       pushed: the pair is stale, skip it.
 *     - Otherwise u is final. For each arc u -> v of weight w, RELAX it:
 *       if dist[u] + w < dist[v], set dist[v] = dist[u] + w, prev[v] = u
 *       and push (dist[v], v).
 *   Stop when the heap is empty.
 * Why it is correct: the pair popped has the smallest distance of all
 * unfinished nodes, and weights are >= 0, so no later path can be shorter.
 *
 * Counters (metrics.h): every arc tested = one relaxation; every stale
 * check "d > dist[u]" = one comparison (plus the heap's own comparisons);
 * pushes and pops come from the heap; dist and prev arrays are added to
 * aux_bytes.
 *
 * Complexity (V nodes, E arcs): each node is expanded once, so there are
 * at most E + 1 pushes and pops, each O(log V) (E <= 4V on a grid).
 *   time  O((V + E) log V) worst and average; best Theta(V + E) when the
 *         source is boxed in (arrays are still filled, weights checked);
 *   space O(V + E): dist, prev, and a heap of E + 1 pairs.
 * With the grid's unit weights, dist <= V - 1 <= 250000, so sums stay far
 * below DIST_INF and cannot overflow an int.
 */
#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"

#define DIST_INF            1000000000   /* "no path" marker              */
#define DIJKSTRA_MAX_WEIGHT 1000         /* largest arc weight accepted   */

typedef struct {
    int  n;        /* number of nodes (size of dist and prev)  */
    int  source;   /* the start node of this run               */
    int *dist;     /* n entries, DIST_INF if unreachable       */
    int *prev;     /* n entries, -1 if none                    */
} ShortestPaths;

/*
 * Run Dijkstra from source. Returns 0 on success. Returns -1 (and leaves sp
 * empty) if g or sp is NULL, the graph is empty, source is out of range,
 * an arc weight is outside 0..DIJKSTRA_MAX_WEIGHT, or memory runs out.
 * Call sp_free(sp) when done.
 */
int dijkstra(const Graph *g, int source, ShortestPaths *sp);

/* Free the arrays and reset sp. Safe on NULL and when called twice. */
void sp_free(ShortestPaths *sp);

/*
 * Write the nodes of the shortest path source -> target into out, in order
 * (out[0] is the source). Returns how many nodes were written (>= 1);
 * 0 if target is unreachable; -1 if sp is empty, target is out of range,
 * out is NULL or max_len is smaller than the path length.
 * Time O(path length).
 */
int sp_path(const ShortestPaths *sp, int target, int *out, int max_len);

#endif /* DIJKSTRA_H */
