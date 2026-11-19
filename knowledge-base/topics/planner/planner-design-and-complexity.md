---
id: 20261119-planner-design-and-complexity
title: Planner pipeline design and complexity
tags: [planner, complexity, dijkstra, knapsack, tsp]
updated: 2026-11-19
---

# Planner pipeline (src/planner.c)

No I/O inside the planner; main.c prints. Options choose the strategy so the same input can be run twice and compared: knapsack DP or greedy, Held-Karp or nearest neighbour.

## The five steps
1. **Distances.** Dijkstra (heap) from each robot start and each reachable task cell. Gives robot-to-task `rt` and task-to-task `tt`.
2. **Assignment in rounds.** Offer unserved tasks in order: priority high to low, then value high to low, then number. Each goes to the nearest robot that can reach it and has not refused it before (tie: lower robot number).
3. **Loading.** Each robot runs the knapsack on its candidates (weight, value, capacity). Tasks left out become unserved and are offered to their next nearest robot in the next round. Stop when no unserved task has an untried reachable robot.
4. **Routing.** Open path from the robot's start over the chosen stops: Held-Karp for up to 15 stops, otherwise nearest neighbour.
5. **Battery.** If route cost > battery, drop the lowest priority task (tie: lowest value, then highest task number), route again, repeat.

## Why this design
- Every step reuses a tested algorithm, so the pipeline itself is short and easy to explain.
- Nearest-robot assignment is a greedy heuristic: fast but not optimal globally. Say so in the viva.
- Priority is not part of the knapsack value (limit 1,000,000 per item); it orders offers and drops (QUESTIONS Q29).

## Complexity
R robots, T tasks, V cells. Step 1: (R + T) Dijkstras, O((R + T) V log V). Each round at most R knapsacks of O(T*C), plus routes: Held-Karp O(k^2 2^k) with k <= 15. Rounds: each round marks at least one (task, robot) pair as tried, so at most R*T rounds (in practice a handful). Space: R*T + T*T ints, R*T flags.

## Worked example (test group B in tests/test_planner.c)
Corridor `R.T.T.T`, capacity 50, tasks (w10 v60) (w20 v100) (w30 v120). DP loads tasks 1 and 2 (value 220), route 1,2 cost 6. Greedy loads 0 and 1 (value 160), cost 4. Same input, different strategy, different plan.
