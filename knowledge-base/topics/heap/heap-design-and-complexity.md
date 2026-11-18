---
id: 20261119-heap-design-and-complexity
title: Min-heap design and complexity
tags: [heap, complexity]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-graph-design-and-complexity, 20261119-heap-viva-questions, 20261119-operation-counters]
summary: Array-based binary min-heap of (key, node) pairs with lazy deletion, how push and pop work, and best/average/worst time and space.
---

# Min-heap design and complexity

> Summary: Array-based binary min-heap of (key, node) pairs with lazy deletion, how push and pop work, and best/average/worst time and space.

## Context
Item 4. Dijkstra (item 5) needs "give me the unvisited node with the smallest distance" many times. A heap does that in O(log n) instead of the O(n) scan of the array version (item 6). Code: `src/heap.[ch]`; tests: `tests/test_heap.c`.

## Details
Layout
- One array `items[0..size-1]` of `{key, node}`. Item `i` has parent `(i-1)/2` and children `2i+1`, `2i+2`.
- Min-heap rule: parent key <= child key for every item, so `items[0]` is always the smallest. Equal keys come out in no fixed order.
- Capacity is fixed at `heap_init` (1 to 50,000,000). A push on a full heap returns -1; nothing is reallocated.

No decrease-key (lazy deletion)
- When Dijkstra finds a shorter distance to a node it pushes a second `(newdist, node)` pair. The old pair stays in the heap and is skipped when popped (its key is larger than the node's current best distance).
- So the heap needs room for one push per successful relaxation: at most `n_arcs + 1` pushes. Chosen because decrease-key needs a position table and extra code to defend.

Push
1. Write the pair into the first free slot, `size++`.
2. `sift_up`: while the item is smaller than its parent, swap with the parent. One comparison per level.

Pop
1. Take the root (the answer). Move the last item to the root, `size--`.
2. `sift_down`: swap the item with its smaller child until neither child is smaller. Two comparisons per level (child vs child, item vs child).

Counters
- Each key comparison -> `COUNT_COMPARISON`, each successful push -> `COUNT_HEAP_PUSH`, each successful pop -> `COUNT_HEAP_POP`. `heap_init` adds the array size to `aux_bytes`. Failed operations are not counted.

Complexity (n items in the heap)
- Push: worst O(log n) (new key is the smallest, climbs to the root); best O(1) (ascending keys, 1 comparison); average O(1) for random keys (most items stay near the bottom). Auxiliary space O(1).
- Pop: worst and average Theta(log n) (the moved item came from the bottom, so it usually sinks back down); best O(1) when all keys are equal (the item stops at once). Auxiliary space O(1).
- Init: O(1) time (one malloc), O(capacity) space. Height of the heap is floor(log2 n).
- Dijkstra with this heap: O((V + E) log V) time overall, because there are at most E + 1 pushes and pops each costing O(log E) = O(log V).

Worked count (used as a test)
- Push 1,2,3 -> 2 comparisons; pop -> 1 more: array [1,2,3] becomes [3,2] then swaps to [2,3].
- 7 pushes in descending order -> 0+1+1+2+2+2+2 = 10 comparisons (every new item climbs to the root).

## Related
- [Graph design and complexity](../graph/graph-design-and-complexity.md) — prerequisite: the graph the heap will serve.
- [Operation counters](../metrics/operation-counters.md) — see-also: meaning of each counter.
- [Heap viva questions](heap-viva-questions.md) — see-also: defence prep.
