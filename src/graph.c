/* graph.c - grid to graph conversion. See graph.h for the design. */
#include "graph.h"

#include <stdlib.h>
#include <string.h>

/* The four moves, in the order the arcs are stored: up, down, left, right. */
static const int DROW[4] = { -1, 1, 0, 0 };
static const int DCOL[4] = { 0, 0, -1, 1 };

int cell_is_walkable(char c)
{
    return c == CELL_FREE || c == CELL_CHARGER || c == CELL_DOCK
        || c == CELL_ROBOT || c == CELL_TASK;
}

/* Walkable test by position. problem_cell gives '\0' outside the grid, and
 * '\0' is not walkable, so no separate bounds check is needed. */
static int walkable_at(const Problem *p, int row, int col)
{
    return cell_is_walkable(problem_cell(p, row, col));
}

void graph_free(Graph *g)
{
    if (g == NULL) {
        return;
    }
    free(g->first);
    free(g->to);
    free(g->weight);
    memset(g, 0, sizeof *g);
}

int graph_build(const Problem *p, Graph *g)
{
    int *fill = NULL;      /* next free slot in each node's arc list */
    int r, c, k, u, v, n;

    if (g == NULL) {
        return -1;
    }
    memset(g, 0, sizeof *g);
    if (p == NULL || p->cells == NULL || p->rows < 1 || p->cols < 1) {
        return -1;
    }

    g->rows = p->rows;
    g->cols = p->cols;
    n = p->rows * p->cols;
    g->n = n;

    g->first = (int *)calloc((size_t)n + 1, sizeof(int));
    if (g->first == NULL) {
        graph_free(g);
        return -1;
    }

    /* Pass 1: count the arcs leaving each node. The count for node u is
     * kept in first[u + 1], so the prefix sum below turns it into starts. */
    for (r = 0; r < p->rows; r++) {
        for (c = 0; c < p->cols; c++) {
            if (!walkable_at(p, r, c)) {
                continue;
            }
            g->n_walkable++;
            u = r * p->cols + c;
            for (k = 0; k < 4; k++) {
                if (walkable_at(p, r + DROW[k], c + DCOL[k])) {
                    g->first[u + 1]++;
                }
            }
        }
    }

    /* Prefix sum: first[u] = total arcs of nodes 0 .. u-1. */
    for (u = 0; u < n; u++) {
        g->first[u + 1] += g->first[u];
    }
    g->n_arcs = g->first[n];

    /* +1 so that a graph with no arcs does not ask malloc for 0 bytes. */
    g->to     = (int *)malloc(((size_t)g->n_arcs + 1) * sizeof(int));
    g->weight = (int *)malloc(((size_t)g->n_arcs + 1) * sizeof(int));
    fill      = (int *)malloc((size_t)n * sizeof(int));
    if (g->to == NULL || g->weight == NULL || fill == NULL) {
        free(fill);
        graph_free(g);
        return -1;
    }
    memcpy(fill, g->first, (size_t)n * sizeof(int));

    /* Pass 2: write each arc into its node's slot range. */
    for (r = 0; r < p->rows; r++) {
        for (c = 0; c < p->cols; c++) {
            if (!walkable_at(p, r, c)) {
                continue;
            }
            u = r * p->cols + c;
            for (k = 0; k < 4; k++) {
                if (walkable_at(p, r + DROW[k], c + DCOL[k])) {
                    v = (r + DROW[k]) * p->cols + (c + DCOL[k]);
                    g->to[fill[u]]     = v;
                    g->weight[fill[u]] = 1;
                    fill[u]++;
                }
            }
        }
    }

    free(fill);
    return 0;
}

int graph_node(const Graph *g, int row, int col)
{
    if (g == NULL || row < 0 || row >= g->rows || col < 0 || col >= g->cols) {
        return -1;
    }
    return row * g->cols + col;
}

int graph_unreachable_tasks(const Problem *p, const Graph *g, int *flags)
{
    char *seen;        /* seen[u] = 1 once u has been put in the queue */
    int  *queue;       /* BFS queue: each node enters at most once     */
    int head = 0, tail = 0;
    int i, u, v, a, node, count = 0;

    if (p == NULL || g == NULL || g->first == NULL || g->n < 1) {
        return -1;
    }
    seen  = (char *)calloc((size_t)g->n, 1);
    queue = (int *)malloc((size_t)g->n * sizeof(int));
    if (seen == NULL || queue == NULL) {
        free(seen);
        free(queue);
        return -1;
    }

    /* Start the BFS from every robot at once. */
    for (i = 0; i < p->n_robots; i++) {
        node = graph_node(g, p->robots[i].row, p->robots[i].col);
        if (node >= 0 && !seen[node]) {
            seen[node] = 1;
            queue[tail++] = node;
        }
    }

    while (head < tail) {
        u = queue[head++];
        for (a = g->first[u]; a < g->first[u + 1]; a++) {
            v = g->to[a];
            if (!seen[v]) {
                seen[v] = 1;
                queue[tail++] = v;
            }
        }
    }

    for (i = 0; i < p->n_tasks; i++) {
        node = graph_node(g, p->tasks[i].row, p->tasks[i].col);
        if (node < 0 || !seen[node]) {
            count++;
            if (flags != NULL) {
                flags[i] = 1;
            }
        } else if (flags != NULL) {
            flags[i] = 0;
        }
    }

    free(seen);
    free(queue);
    return count;
}
