---
id: 20261119-heap-viva-questions
title: Min-heap explain-back and viva questions
tags: [heap, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-heap-design-and-complexity]
summary: A 5-line explain-back of the min-heap and likely evaluator questions with short answers.
---

# Min-heap explain-back and viva questions

> Summary: A 5-line explain-back of the min-heap and likely evaluator questions with short answers.

## Context
Required after every module so the admin can defend the code.

## Details
Explain it back (5 lines):
1. The heap is one array; item `i` has parent `(i-1)/2` and children `2i+1`, `2i+2`, and every parent key is <= its children's keys.
2. So the smallest key is always at index 0; `pop` reads it without searching.
3. `push` puts the pair in the next free slot and `sift_up` swaps it with larger parents until the rule holds.
4. `pop` moves the last item to the root, shrinks the heap, and `sift_down` swaps it with its smaller child until the rule holds.
5. Both walk one root-to-leaf path, so they cost O(log n); there is no decrease-key, Dijkstra pushes duplicates instead.

Evaluator questions:
- Q: Why a heap and not a sorted array or a plain array scan? A: A sorted array needs O(n) to insert; a plain array needs O(n) to find the minimum. The heap does both in O(log n), which makes Dijkstra O((V + E) log V) instead of O(V^2).
- Q: Why does pop take Theta(log n) on average but push O(1) on average? A: The last item moved to the root is a bottom item, so it usually has to sink almost all the way. A new random item is usually larger than its parent, so it rarely climbs.
- Q: How do you build the heap tree from an array? A: You don't store pointers. Parent and child positions come from index arithmetic: `(i-1)/2`, `2i+1`, `2i+2`.
- Q: What happens with equal keys? A: The rule uses `<=`, so equal keys may sit anywhere; they come out in no fixed order, which is fine because Dijkstra only needs the smallest distance.
- Q: Why no decrease-key? A: It needs a table of every node's position in the heap, updated on every swap. Lazy deletion is simpler: push a second pair and skip the stale one later. The cost is a larger heap (up to E + 1 pairs).
- Q: Can you prove the heap rule still holds after pop? A: Only the moved item can be wrong, and it was the last item, so only along one path. `sift_down` always swaps with the smaller child, so the swapped-up child is smaller than its new sibling and its new children; the rule holds at every level it fixes.

## Related
- [Min-heap design and complexity](heap-design-and-complexity.md) — prerequisite.
