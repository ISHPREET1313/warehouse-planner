#!/usr/bin/env python3
"""
gen_testdata.py - how the files in data/ and the expected numbers in
tests/test_files.c were made (item 7). Needs only Python 3; the project
itself does not need Python.

It writes the map files into ./out/ and prints, for every file, the size,
the number of robots and tasks, and the shortest-path cost from each robot
to each task. Those costs come from a plain breadth-first search written
here, completely separate from the C code, so the C tests compare two
independent implementations.

Run:   python tools/gen_testdata.py      (then compare ./out with data/)
"""
import os
from collections import deque

WALK = set(".CDRT")          # same walkable symbols as src/graph.c


def bfs(grid, r0, c0):
    rows, cols = len(grid), len(grid[0])
    dist = {(r0, c0): 0}
    queue = deque([(r0, c0)])
    while queue:
        r, c = queue.popleft()
        for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nr, nc = r + dr, c + dc
            if (0 <= nr < rows and 0 <= nc < cols and grid[nr][nc] in WALK
                    and (nr, nc) not in dist):
                dist[(nr, nc)] = dist[(r, c)] + 1
                queue.append((nr, nc))
    return dist


class Lcg:
    """Small repeatable random numbers (same recipe as the C tests)."""
    def __init__(self, seed):
        self.s = seed

    def next(self, n):
        self.s = (self.s * 1103515245 + 12345) & 0x7FFFFFFF
        return (self.s >> 8) % n


def warehouse(rows, cols, n_robots, n_tasks, seed):
    g = [['#'] * cols for _ in range(rows)]
    for r in range(1, rows - 1):
        for c in range(1, cols - 1):
            g[r][c] = '.'
    for r in range(2, rows - 2):              # shelf blocks, two rows thick
        if r % 4 in (2, 3):
            for c in range(3, cols - 3):
                if (c - 3) % 11 != 10:        # a cross aisle every 11 columns
                    g[r][c] = 'S'
    g[1][1] = 'D'
    g[1][cols - 2] = 'C'
    g[rows - 2][1] = 'C'
    rng = Lcg(seed)
    free = [(r, c) for r in range(1, rows - 1) for c in range(1, cols - 1)
            if g[r][c] == '.']
    step = len(free) // n_robots
    for i in range(n_robots):
        p = free[i * step + rng.next(step // 2 + 1)]
        g[p[0]][p[1]] = 'R'
    near = [(r, c) for (r, c) in free
            if g[r][c] == '.' and (g[r - 1][c] == 'S' or g[r + 1][c] == 'S')]
    placed = 0
    while placed < n_tasks:
        p = near[rng.next(len(near))]
        if g[p[0]][p[1]] == '.':
            g[p[0]][p[1]] = 'T'
            placed += 1
    return g


def cells(g, ch):
    return [(r, c) for r in range(len(g)) for c in range(len(g[0])) if g[r][c] == ch]


def render(g, header, seed, n_extra_robots=0):
    out = ["# " + h for h in header]
    out.append("MAP %d %d" % (len(g), len(g[0])))
    out += ["".join(row) for row in g]
    rng = Lcg(seed + 1)
    for _ in cells(g, 'R'):
        out.append("ROBOT %d %d" % (8 + rng.next(13), 100 + rng.next(301)))
    for _ in cells(g, 'T'):
        out.append("TASK %d %d %d" % (1 + rng.next(9), 5 + rng.next(46), 1 + rng.next(5)))
    return "\n".join(out) + "\n"


def report(name, text, g):
    if text is not None:                      # None: hand-drawn map, file already in data/
        with open(os.path.join("out", name), "w") as f:
            f.write(text)
    robots, tasks = cells(g, 'R'), cells(g, 'T')
    walk = sum(1 for r in range(len(g)) for c in range(len(g[0])) if g[r][c] in WALK)
    arcs = 0
    for r in range(len(g)):
        for c in range(len(g[0])):
            if g[r][c] in WALK:
                for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    nr, nc = r + dr, c + dc
                    if 0 <= nr < len(g) and 0 <= nc < len(g[0]) and g[nr][nc] in WALK:
                        arcs += 1
    print("== %s: rows=%d cols=%d robots=%d tasks=%d walkable=%d arcs=%d"
          % (name, len(g), len(g[0]), len(robots), len(tasks), walk, arcs))
    for (r, c) in robots:
        d = bfs(g, r, c)
        print("   {" + ", ".join(str(d.get(t, -1)) for t in tasks) + "},")


def main():
    os.makedirs("out", exist_ok=True)

    g = warehouse(30, 40, 3, 8, 11)
    report("warehouse_30x40.txt", render(g, [
        "medium: 30 rows x 40 cols, shelf blocks with aisles and cross aisles,",
        "3 robots, 8 tasks. All tasks reachable."], 11), g)

    g = warehouse(62, 80, 4, 20, 23)
    report("warehouse_62x80.txt", render(g, [
        "large: 62 rows x 80 cols, shelf blocks with aisles and cross aisles,",
        "4 robots, 20 tasks. All tasks reachable."], 23), g)

    # snake corridor 21x21: one long path, shortest route is 240 steps
    rows = cols = 21
    g = [['#'] * cols for _ in range(rows)]
    for r in range(0, rows, 2):
        for c in range(cols):
            g[r][c] = '.'
    for r in range(1, rows, 2):
        g[r][cols - 1 if (r // 2) % 2 == 0 else 0] = '.'
    g[0][0] = 'R'
    g[20][20] = 'T'
    report("snake_21x21.txt", render(g, [
        "snake_21x21: one long corridor, the robot is at one end and the task at the other.",
        "The shortest path is 11 rows * 20 + 10 turns * 2 = 240 steps; BFS must not shortcut."], 1), g)

    # the hand-drawn maps (sample2, sealed_task, boxed_robot, no_tasks) are
    # small enough to check by eye; put them through the same report.
    for name, rows_txt in [
        ("sample2.txt", ["############", "#D...T.....#", "#.SSS.SSS..#",
                         "#.........R#", "#.SSS.SSS..#", "#T..T..T..C#", "############"]),
        ("sealed_task.txt", ["#########", "#R..T...#", "#.......#", "#########",
                             "#...#T#.#", "#...###.#", "#########"]),
        ("boxed_robot.txt", ["###########", "#R#.......#", "###..T....#",
                             "#....R..T.#", "###########"]),
        ("no_tasks.txt", ["#####", "#R..#", "#...#", "#####"]),
    ]:
        report(name, None, [list(x) for x in rows_txt])


if __name__ == "__main__":
    main()
