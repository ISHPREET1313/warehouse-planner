# Warehouse Robot Path and Task Planner

A pure-software simulation in C for the course CSEG3053, Design and Analysis of Algorithms. Given a warehouse map, robots with capacity and battery limits, and pickup tasks with weight, value and priority, it decides which robot does which task, what each robot loads, and in which order it visits its stops. Every decision is made by an algorithm that is also available in a cheaper or simpler alternative form, and the program compares the alternatives by operation counts, time and memory.

C11, standard library only, fully offline. Developed and tested with MinGW GCC 6.3 and GNU Make 3.81 on Windows; it also builds on Linux.

## Quick start

Run everything from the project root.

    make                              build planner
    make clean && make test           build and run the test suite (more than 1400 checks)
    planner                           interactive menu
    planner data/small/sample1.txt    one-shot report for one input file
    planner --bench                   the benchmark table
    planner --export-replay data/small/sample1.txt sample1.replay
                                      cell-by-cell routes of plans A and B into a replay file
    planner --play sample1.replay     watch the replay in the terminal

On Linux use `./planner`. Sanitizers: `make clean && make test SAN=1` (needs GCC with ASan/UBSan, for example under WSL).

## Example

`planner data/small/sample1.txt` (6 x 8 map, 2 robots, 3 tasks) prints the shortest-path costs from each robot to each task, for example `robot 1: T1=13 T2=5 T3=10`, and then the plan under two strategy sets on the same input:

    A: knapsack DP + Held-Karp (exact per robot)
      robot 1: load 7/10, value 35, cost 13/40, stops: T2 T1
      robot 2: load 5/6, value 30, cost 2/30, stops: T3
      TOTAL: served 3 of 3, value 65, cost 15
    B: knapsack greedy + nearest neighbour (fast heuristics)
      TOTAL: served 3 of 3, value 65, cost 15

Here both strategies agree. On `data/scenarios/knapsack_trap.txt` they do not: greedy loads value 160, the exact method 220.

## Interactive menu

    1  load a problem file                 6  export the plan(s) to CSV
    2  show the problem                    7  scenarios (4 ready-made inputs)
    3  shortest paths (3 algorithms)       8  benchmark table on the screen
    4  plan, both strategy sets            9  benchmark table to benchmark.csv
    5  plan, one strategy set of my choice 0  quit
    10 watch the robots drive (ASCII replay of plans A and B)

Menu 6 writes `plan_tasks.csv` and `plan_robots.csv`.

## Routes and the terminal replay

The planner decides which robot serves which task and in what order. `route.c` then turns that decision into the actual cell-by-cell path of each robot: for every leg (start, stop 1, stop 2, ...) it runs Dijkstra and reads the path back with `sp_path`, so no new algorithm is involved. The routes are checked against the plan: every step goes to a walkable 4-neighbour, the number of steps equals the plan's route cost, and the stops are reached in plan order.

`planner --export-replay <input> <output>` (or menu 10) writes a plain-text replay file: the map, the robots, the tasks and, for plans A and B, the cell of every robot at every tick plus the tick of every pick-up. The file format is described in `src/replay.h`, and the reader validates it with line-numbered errors.

`planner --play <file>` (or menu 10) plays the replay in the terminal. The player only draws a recording; it never plans, so removing it leaves the planner output unchanged.

    1-9, a-w  robots     T  waiting task    x  picked task
    .  floor    #  wall    =  shelf    C  charger    D  dock

Commands, one per line: Enter = one tick forward, `b` back, `a` auto-play, `+` / `-` speed, `p` switch between plan A and B, `u` / `d` scroll, `q` quit. Maps taller than 20 rows are shown through a scrolling window (so the 62 x 80 scenario works); maps wider than 100 columns are refused. On a console that understands ANSI codes the symbols are coloured.

## Algorithms

| Step | Algorithm | Time | Alternative compared |
|---|---|---|---|
| Reachability | multi-source BFS | Theta(V + E) | |
| Shortest paths | Dijkstra with a binary min-heap (lazy deletion) | O((V + E) log V) | Dijkstra with array scan, Theta(V^2 + E); Bellman-Ford, O(VE) worst case, accepts negative weights |
| Load of one robot | 0/1 knapsack, dynamic programming (exact) | Theta(n C) | greedy by value/weight, fast but can be wrong |
| Pickup order | Held-Karp, dynamic programming over subsets (exact, up to 15 stops) | Theta(n^2 2^n) | nearest neighbour, Theta(n^2) |
| Task assignment | nearest robot with capacity, tasks offered by priority, battery trimming | (R + T) Dijkstra runs plus knapsack and routing per round | |

V = grid cells, E = arcs, n = items or stops, C = capacity, R = robots, T = tasks. Above 15 stops per robot the planner falls back to nearest neighbour. Task assignment between robots is a heuristic (the full problem is NP-hard); each robot's load and route are exact in strategy set A.

All algorithms count comparisons, relaxations, heap pushes and pops and auxiliary bytes (`src/metrics.h`), so they are compared by the work they do and not only by clock time. Counters are exact and repeatable; times come from `clock()` (coarse on Windows) and are indicative. The benchmark averages repeated runs. Example from the benchmark: on the 62 x 80 map Dijkstra with a heap needs 24,352 comparisons and the array scan 9,613,240, with identical distances.

## Demo scenarios (menu 7)

1. Normal day: `data/small/sample1.txt`
2. Constraint shock (blocked aisle, capacity drop): `data/scenarios/shock_blocked_aisle.txt`
3. Large stress, 62 x 80 map with 4 robots and 20 tasks: `data/large/warehouse_62x80.txt`
4. Knapsack trap where greedy is wrong (160 against 220): `data/scenarios/knapsack_trap.txt`

## Input format

    MAP <rows> <cols>
    <rows lines of exactly <cols> characters from  . # S C D R T>
    ROBOT <capacity> <battery>        one line per R, in reading order
    TASK <weight> <value> <priority>  one line per T, in reading order

`.` free, `#` obstacle, `S` shelf, `C` charger, `D` dock, `R` robot start, `T` task pickup. Robots walk on `. C D R T` in four directions at cost 1 per step. Priority is 1 (low) to 5 (high). Lines starting with `#` before `MAP` are comments. A bad file is rejected with a status code and a line-numbered message, for example `PARSE_ERR_MAP: line 5, column 2 ...`. Limits: maps up to 500 x 500, 32 robots, 256 tasks, numbers up to 1,000,000, files up to 4 MB.

## Testing

`make test` runs more than 1400 checks, printed as an expected-versus-actual table, and returns a non-zero exit code if any check fails. Categories: normal, boundary, invalid input, stress (500 x 500 maps) and adversarial input (sealed task, boxed-in robot, 240-step snake corridor, equal keys, negative cycles). Results are checked against independent oracles: brute force over all subsets (knapsack), over all orders (pickup order), a plain 2D table (knapsack DP), Bellman-Ford against Dijkstra on 200 random graphs, and a separate Python generator and BFS (`tools/gen_testdata.py`) for the data files. Counters are checked against hand-derived formulas, for example 41 + 31 + 21 = 93 comparisons for the classic knapsack case.

## Project layout

    src/              problem, io (parser), graph, heap, dijkstra, bellman_ford, knapsack,
                      tsp, planner, route (cell-by-cell paths), replay (replay file),
                      play (terminal player), export (CSV), bench, metrics, main.
                      Algorithms do no printing; only main.c, bench.c, export.c,
                      replay.c and play.c do I/O.
    tests/            one test_*.c per module and a small harness
    data/             small, medium, large, adversarial and scenarios input files
    tools/            gen_testdata.py (independent oracle for the data files)
    knowledge-base/   per-module notes: design, complexity, viva questions (start at INDEX.md)
    docs/             local working notes; not tracked in git

## Known limitations

- Step cost is 1 and moves are four-directional.
- The planner and the replay give each robot's route cell by cell, but the routes are computed one robot at a time: there is no collision handling, so two robots can stand on the same cell at the same tick, and there is no waiting.
- The route is an open path: it ends at the last stop with no return trip.
- Priority orders which task is offered first and which is dropped first when the battery is short; it does not change the knapsack value.
- The exact knapsack maximizes value and breaks ties arbitrarily, so at equal value strategy A can have a longer route than B (on `data/medium/warehouse_30x40.txt` both serve 6 tasks for value 243, with route cost 95 against 57).
- Held-Karp is limited to 15 stops per robot and the knapsack capacity is capped at 20,000.

## License

See `LICENSE`.
