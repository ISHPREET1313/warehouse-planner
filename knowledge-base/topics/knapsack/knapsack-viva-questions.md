---
id: 20261119-knapsack-viva-questions
title: Knapsack explain-back and viva questions
tags: [knapsack, viva]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-knapsack-design-and-complexity]
summary: A 5-line explain-back of the knapsack module and likely evaluator questions with short answers.
---

# Knapsack explain-back and viva questions

> Summary: A 5-line explain-back of the knapsack module and likely evaluator questions with short answers.

## Context
Required after every module so the admin can defend the code.

## Details
Explain it back (5 lines):
1. A robot has a capacity and candidate tasks with weights and values; each task is taken whole or not at all.
2. The DP keeps `best[w]`, the best value for weight at most `w`, and updates it item by item.
3. The weight loop runs downwards so one item cannot be counted twice.
4. A flag table remembers where each item helped, so the chosen tasks are read back from the last item to the first.
5. Greedy sorts by value per weight and takes what fits: very fast, but it can miss the best answer.

Evaluator questions:
- Q: Why does the inner loop go from high weight to low? A: `best[w - weight[i]]` must still hold the previous row. Going upwards would reuse the item just added (unbounded knapsack).
- Q: Is O(n * C) polynomial? A: Only pseudo-polynomial: it is polynomial in the value of C, not in the number of bits needed to write C. The problem is NP-hard.
- Q: Give a case where greedy fails. A: Capacity 50, items (10,60) (20,100) (30,120): greedy 160, optimal 220.
- Q: Why cross-multiply the ratios? A: value[a]*weight[b] > value[b]*weight[a] compares the fractions without division or rounding.
- Q: Best, average and worst case of the DP? A: All Theta(n * C): the loops do not stop early.
- Q: How do you know which items were chosen? A: The flag table: start at the last item with full capacity; if it improved best at that weight, it was taken and the weight drops by its weight.
- Q: How can the greedy be made O(n log n)? A: Sort with merge sort instead of insertion sort.

## Related
- [Knapsack design and complexity](knapsack-design-and-complexity.md) — prerequisite.
