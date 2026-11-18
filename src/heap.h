/*
 * heap.h - binary min-heap of (key, node) pairs (item 4).
 *
 * Dijkstra puts (distance, node) pairs in here and always takes out the
 * pair with the smallest distance. There is no decrease-key: when a shorter
 * distance is found, Dijkstra just pushes a second pair for that node and
 * ignores the stale pair when it comes out ("lazy deletion"). That keeps
 * this module small. Because of it, a caller needs room for one push per
 * successful relaxation, i.e. at most (number of arcs + 1) pushes.
 *
 * Layout: the heap is one array, item i has
 *     parent  (i - 1) / 2
 *     children 2*i + 1 and 2*i + 2
 * and the MIN-HEAP RULE holds for every item except the root:
 *     key of parent <= key of item
 * so the smallest key is always items[0].
 * Items with equal keys come out in no particular order.
 *
 * Counters (see metrics.h): every key comparison does COUNT_COMPARISON(),
 * every successful push COUNT_HEAP_PUSH(), every successful pop
 * COUNT_HEAP_POP(). heap_init adds the array size to aux_bytes.
 *
 * Complexity (n = items in the heap):
 *   push  worst O(log n), best O(1), average O(1) for random keys; space O(1)
 *   pop   worst and average Theta(log n), best O(1) (all keys equal);
 *         space O(1)
 *   init  O(1) time, O(capacity) space
 * No I/O lives here.
 */
#ifndef HEAP_H
#define HEAP_H

/* Largest capacity heap_init accepts. Keeps 2*i + 1 far from int overflow
 * and stops a typo from asking for gigabytes. */
#define HEAP_MAX_CAPACITY 50000000

typedef struct {
    int key;     /* what the heap orders by (a distance)  */
    int node;    /* the graph node that goes with the key */
} HeapItem;

typedef struct {
    HeapItem *items;     /* capacity entries, malloc'd, NULL if not initialised */
    int       size;      /* how many are in use                                 */
    int       capacity;  /* how many fit                                        */
} MinHeap;

/* Make an empty heap that can hold capacity items (1..HEAP_MAX_CAPACITY).
 * Returns 0 on success, -1 if h is NULL, the capacity is out of range or
 * memory runs out (h is then empty). */
int heap_init(MinHeap *h, int capacity);

/* Free the array and reset h. Safe on NULL and when called twice. */
void heap_free(MinHeap *h);

/* Insert (key, node). Returns 0, or -1 if h is NULL or the heap is full. */
int heap_push(MinHeap *h, int key, int node);

/* Remove the pair with the smallest key. Its key and node are stored in
 * *key and *node (either pointer may be NULL). Returns 0, or -1 if h is
 * NULL or the heap is empty (outputs are then not touched). */
int heap_pop(MinHeap *h, int *key, int *node);

/* Number of items; 0 for NULL. */
int heap_size(const MinHeap *h);

/* 1 if there are no items (or h is NULL), otherwise 0. */
int heap_is_empty(const MinHeap *h);

/* 1 if the min-heap rule holds for every item, 0 if it does not or h is
 * NULL. Meant for tests; it counts nothing and takes O(n). */
int heap_is_valid(const MinHeap *h);

#endif /* HEAP_H */
