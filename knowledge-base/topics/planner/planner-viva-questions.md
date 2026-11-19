---
id: 20261119-planner-viva-questions
title: Planner explain-back and viva questions
tags: [planner, viva]
updated: 2026-11-19
---

# Explain it back (5 lines)
1. Dijkstra gives every distance the planner needs, from each robot and each task.
2. Tasks are offered by priority then value; each goes to the nearest robot that has not refused it.
3. A robot loads its candidates with the knapsack; refused tasks try the next robot in the next round.
4. The chosen stops are ordered by Held-Karp (up to 15) or nearest neighbour, no return trip.
5. If the battery is too low, the lowest priority task is dropped and the route recomputed.

# Likely questions
- **Is the plan optimal?** No. Each robot's load and route are exact (DP, Held-Karp), but the split of tasks among robots is greedy (nearest robot), which is a heuristic; the full problem is NP-hard.
- **Why can rounds not loop forever?** Each round marks at least one (task, robot) pair as tried, and there are only R*T pairs.
- **Why does priority not enter the knapsack?** value * priority can exceed the 1,000,000 item limit; priority orders offers and drops instead.
- **What if more than 15 stops?** Held-Karp needs about n*2^n memory, so it falls back to nearest neighbour above 15 stops.
- **What does an unreachable task do?** It is never offered (no robot reaches it); it stays unserved and the plan still succeeds.
