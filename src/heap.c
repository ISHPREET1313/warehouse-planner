/* heap.c - binary min-heap. See heap.h for the layout and the rules. */
#include "heap.h"

#include <stdlib.h>
#include <string.h>

#include "metrics.h"

static void swap_items(HeapItem *a, HeapItem *b)
{
    HeapItem t = *a;
    *a = *b;
    *b = t;
}

/* Item i may be smaller than its parent: swap it upwards until its parent
 * is not larger (or it is the root). One comparison per level climbed. */
static void sift_up(MinHeap *h, int i)
{
    int parent;

    while (i > 0) {
        parent = (i - 1) / 2;
        COUNT_COMPARISON();
        if (h->items[parent].key <= h->items[i].key) {
            break;
        }
        swap_items(&h->items[parent], &h->items[i]);
        i = parent;
    }
}

/* Item i may be larger than its children: swap it downwards with its
 * smaller child until neither child is smaller (or it is a leaf).
 * Two comparisons per level: child against child, then item against child. */
static void sift_down(MinHeap *h, int i)
{
    int left, right, smallest;

    for (;;) {
        left = 2 * i + 1;
        if (left >= h->size) {
            break;                      /* no children: i is a leaf */
        }
        smallest = left;
        right = left + 1;
        if (right < h->size) {
            COUNT_COMPARISON();
            if (h->items[right].key < h->items[left].key) {
                smallest = right;
            }
        }
        COUNT_COMPARISON();
        if (h->items[i].key <= h->items[smallest].key) {
            break;                      /* min-heap rule already holds */
        }
        swap_items(&h->items[i], &h->items[smallest]);
        i = smallest;
    }
}

int heap_init(MinHeap *h, int capacity)
{
    if (h == NULL) {
        return -1;
    }
    memset(h, 0, sizeof *h);
    if (capacity < 1 || capacity > HEAP_MAX_CAPACITY) {
        return -1;
    }
    h->items = (HeapItem *)malloc((size_t)capacity * sizeof(HeapItem));
    if (h->items == NULL) {
        return -1;
    }
    h->capacity = capacity;
    ADD_AUX_BYTES((size_t)capacity * sizeof(HeapItem));
    return 0;
}

void heap_free(MinHeap *h)
{
    if (h == NULL) {
        return;
    }
    free(h->items);
    memset(h, 0, sizeof *h);
}

int heap_push(MinHeap *h, int key, int node)
{
    if (h == NULL || h->size >= h->capacity) {
        return -1;
    }
    h->items[h->size].key  = key;       /* put it in the first free slot ... */
    h->items[h->size].node = node;
    h->size++;
    sift_up(h, h->size - 1);            /* ... and move it up to its place   */
    COUNT_HEAP_PUSH();
    return 0;
}

int heap_pop(MinHeap *h, int *key, int *node)
{
    if (h == NULL || h->size == 0) {
        return -1;
    }
    if (key != NULL) {
        *key = h->items[0].key;         /* the root is the smallest */
    }
    if (node != NULL) {
        *node = h->items[0].node;
    }
    h->size--;
    if (h->size > 0) {
        h->items[0] = h->items[h->size];  /* last item takes the root's place */
        sift_down(h, 0);                  /* ... and sinks to where it belongs */
    }
    COUNT_HEAP_POP();
    return 0;
}

int heap_size(const MinHeap *h)
{
    return (h == NULL) ? 0 : h->size;
}

int heap_is_empty(const MinHeap *h)
{
    return heap_size(h) == 0;
}

int heap_is_valid(const MinHeap *h)
{
    int i;

    if (h == NULL) {
        return 0;
    }
    for (i = 1; i < h->size; i++) {
        if (h->items[(i - 1) / 2].key > h->items[i].key) {
            return 0;
        }
    }
    return 1;
}
