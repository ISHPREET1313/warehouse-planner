/*
 * gen_testdata.c - how the files in data/ and the expected numbers in
 * tests/test_files.c were made (item 7). Plain C11, standard library only.
 * It replaces the earlier Python script, so the whole project is C.
 *
 * It writes the generated map files into a folder and prints, for every map,
 * the size, the number of robots and tasks, and the shortest-path cost from
 * each robot to each task. Those costs come from a plain breadth-first search
 * written HERE. This file includes nothing from src/ (no graph, no heap, no
 * Dijkstra), so the C tests in tests/test_files.c compare two independent
 * implementations: if the planner's Dijkstra were wrong, this BFS would
 * disagree.
 *
 * Build and run (from the project root; the folder must exist):
 *     make gen
 *     mkdir out
 *     gen_testdata out            (then compare out/ with data/)
 * With no argument the folder is "out".
 *
 * Random numbers: a small linear congruential generator, the same recipe as
 * the C tests, so the files are repeatable. Why BFS and not Dijkstra: every
 * step costs 1, so BFS finds shortest paths in Theta(V + E) and is far
 * simpler to check by eye.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXR 64
#define MAXC 80

typedef struct {
    int  rows, cols;
    char c[MAXR][MAXC];
} Grid;

typedef struct { int r, c; } Cell;

/* ---------- repeatable random numbers ---------- */

typedef struct { unsigned long long s; } Lcg;

/* s = (s * 1103515245 + 12345) mod 2^31, result = (s >> 8) mod n.
 * 64-bit arithmetic: the product of a 31-bit and a 31-bit number fits. */
static int lcg_next(Lcg *g, int n)
{
    g->s = (g->s * 1103515245ULL + 12345ULL) & 0x7FFFFFFFULL;
    return (int)((g->s >> 8) % (unsigned long long)n);
}

/* ---------- helpers ---------- */

static int walkable(char ch)
{
    return ch == '.' || ch == 'C' || ch == 'D' || ch == 'R' || ch == 'T';
}

static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

/* All cells holding ch, in reading order (row by row). Returns the count. */
static int cells_of(const Grid *g, char ch, Cell *out)
{
    int r, c, n = 0;

    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            if (g->c[r][c] == ch) {
                out[n].r = r;
                out[n].c = c;
                n++;
            }
        }
    }
    return n;
}

/* Breadth-first search from (r0, c0): dist[r][c] = steps, or -1. */
static void bfs(const Grid *g, int r0, int c0, int dist[MAXR][MAXC])
{
    static Cell queue[MAXR * MAXC];
    int head = 0, tail = 0, r, c, k, nr, nc;

    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            dist[r][c] = -1;
        }
    }
    dist[r0][c0] = 0;
    queue[tail].r = r0;
    queue[tail].c = c0;
    tail++;
    while (head < tail) {
        r = queue[head].r;
        c = queue[head].c;
        head++;
        for (k = 0; k < 4; k++) {
            nr = r + DR[k];
            nc = c + DC[k];
            if (nr >= 0 && nr < g->rows && nc >= 0 && nc < g->cols &&
                walkable(g->c[nr][nc]) && dist[nr][nc] < 0) {
                dist[nr][nc] = dist[r][c] + 1;
                queue[tail].r = nr;
                queue[tail].c = nc;
                tail++;
            }
        }
    }
}

/* ---------- the warehouse map ---------- */

/* Shelf blocks two rows thick with a cross aisle every 11 columns, a dock and
 * two chargers, robots spread over the free floor and tasks next to shelves. */
static void warehouse(Grid *g, int rows, int cols, int n_robots, int n_tasks,
                      unsigned long long seed)
{
    static Cell free_cell[MAXR * MAXC], near_cell[MAXR * MAXC];
    Lcg rng;
    int r, c, i, n_free = 0, n_near = 0, step, placed = 0;

    g->rows = rows;
    g->cols = cols;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            g->c[r][c] = '#';
        }
    }
    for (r = 1; r < rows - 1; r++) {
        for (c = 1; c < cols - 1; c++) {
            g->c[r][c] = '.';
        }
    }
    for (r = 2; r < rows - 2; r++) {
        if (r % 4 == 2 || r % 4 == 3) {
            for (c = 3; c < cols - 3; c++) {
                if ((c - 3) % 11 != 10) {
                    g->c[r][c] = 'S';
                }
            }
        }
    }
    g->c[1][1] = 'D';
    g->c[1][cols - 2] = 'C';
    g->c[rows - 2][1] = 'C';

    rng.s = seed;
    for (r = 1; r < rows - 1; r++) {
        for (c = 1; c < cols - 1; c++) {
            if (g->c[r][c] == '.') {
                free_cell[n_free].r = r;
                free_cell[n_free].c = c;
                n_free++;
            }
        }
    }
    step = n_free / n_robots;
    for (i = 0; i < n_robots; i++) {
        int pick = i * step + lcg_next(&rng, step / 2 + 1);

        g->c[free_cell[pick].r][free_cell[pick].c] = 'R';
    }
    for (i = 0; i < n_free; i++) {
        r = free_cell[i].r;
        c = free_cell[i].c;
        if (g->c[r][c] == '.' && (g->c[r - 1][c] == 'S' || g->c[r + 1][c] == 'S')) {
            near_cell[n_near++] = free_cell[i];
        }
    }
    while (placed < n_tasks) {
        int pick = lcg_next(&rng, n_near);

        if (g->c[near_cell[pick].r][near_cell[pick].c] == '.') {
            g->c[near_cell[pick].r][near_cell[pick].c] = 'T';
            placed++;
        }
    }
}

/* ---------- output ---------- */

/* The input file text: header comments, MAP, rows, ROBOT lines, TASK lines. */
static void render(FILE *f, const Grid *g, const char *const header[],
                   int n_header, unsigned long long seed)
{
    Cell robots[MAXR * MAXC], tasks[MAXR * MAXC];
    Lcg rng;
    int i, r, n_robots, n_tasks, cap, battery, weight, value, priority;

    for (i = 0; i < n_header; i++) {
        fprintf(f, "# %s\n", header[i]);
    }
    fprintf(f, "MAP %d %d\n", g->rows, g->cols);
    for (r = 0; r < g->rows; r++) {
        fprintf(f, "%.*s\n", g->cols, g->c[r]);
    }
    rng.s = seed + 1;
    n_robots = cells_of(g, 'R', robots);
    for (i = 0; i < n_robots; i++) {
        cap = 8 + lcg_next(&rng, 13);             /* separate statements: the */
        battery = 100 + lcg_next(&rng, 301);      /* order of draws matters   */
        fprintf(f, "ROBOT %d %d\n", cap, battery);
    }
    n_tasks = cells_of(g, 'T', tasks);
    for (i = 0; i < n_tasks; i++) {
        weight = 1 + lcg_next(&rng, 9);
        value = 5 + lcg_next(&rng, 46);
        priority = 1 + lcg_next(&rng, 5);
        fprintf(f, "TASK %d %d %d\n", weight, value, priority);
    }
}

/* Print the numbers the C tests expect for this map. If dir is not NULL the
 * input file is also written to dir/name. Returns 0, or 1 if it cannot be
 * written. */
static int report(const char *dir, const char *name, const Grid *g,
                  const char *const header[], int n_header,
                  unsigned long long seed)
{
    static int dist[MAXR][MAXC];
    Cell robots[MAXR * MAXC], tasks[MAXR * MAXC];
    int n_robots, n_tasks, n_walk = 0, n_arcs = 0, r, c, k, i, j;

    if (dir != NULL) {
        char path[512];
        FILE *f;

        if (strlen(dir) + strlen(name) + 2 > sizeof path) {
            printf("path too long\n");
            return 1;
        }
        sprintf(path, "%s/%s", dir, name);
        f = fopen(path, "w");
        if (f == NULL) {
            printf("cannot write %s (does the folder exist? try: mkdir %s)\n",
                   path, dir);
            return 1;
        }
        render(f, g, header, n_header, seed);
        if (fclose(f) != 0) {
            printf("writing %s failed\n", path);
            return 1;
        }
    }
    n_robots = cells_of(g, 'R', robots);
    n_tasks = cells_of(g, 'T', tasks);
    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            if (!walkable(g->c[r][c])) {
                continue;
            }
            n_walk++;
            for (k = 0; k < 4; k++) {
                int nr = r + DR[k], nc = c + DC[k];

                if (nr >= 0 && nr < g->rows && nc >= 0 && nc < g->cols &&
                    walkable(g->c[nr][nc])) {
                    n_arcs++;
                }
            }
        }
    }
    printf("== %s: rows=%d cols=%d robots=%d tasks=%d walkable=%d arcs=%d\n",
           name, g->rows, g->cols, n_robots, n_tasks, n_walk, n_arcs);
    for (i = 0; i < n_robots; i++) {
        bfs(g, robots[i].r, robots[i].c, dist);
        printf("   {");
        for (j = 0; j < n_tasks; j++) {
            printf("%s%d", j > 0 ? ", " : "", dist[tasks[j].r][tasks[j].c]);
        }
        printf("},\n");
    }
    return 0;
}

/* Copy hand-drawn rows (all the same length) into a Grid. */
static void from_text(Grid *g, const char *const text[], int n_rows)
{
    int r;

    g->rows = n_rows;
    g->cols = (int)strlen(text[0]);
    for (r = 0; r < n_rows; r++) {
        memcpy(g->c[r], text[r], (size_t)g->cols);
    }
}

int main(int argc, char **argv)
{
    static Grid g;
    const char *dir = argc >= 2 ? argv[1] : "out";
    int rc = 0, r, c;

    {
        static const char *const h[] = {
            "medium: 30 rows x 40 cols, shelf blocks with aisles and cross aisles,",
            "3 robots, 8 tasks. All tasks reachable."};
        warehouse(&g, 30, 40, 3, 8, 11);
        rc |= report(dir, "warehouse_30x40.txt", &g, h, 2, 11);
    }
    {
        static const char *const h[] = {
            "large: 62 rows x 80 cols, shelf blocks with aisles and cross aisles,",
            "4 robots, 20 tasks. All tasks reachable."};
        warehouse(&g, 62, 80, 4, 20, 23);
        rc |= report(dir, "warehouse_62x80.txt", &g, h, 2, 23);
    }
    {
        /* snake corridor 21 x 21: one long path, shortest route 240 steps */
        static const char *const h[] = {
            "snake_21x21: one long corridor, the robot is at one end and the task at the other.",
            "The shortest path is 11 rows * 20 + 10 turns * 2 = 240 steps; BFS must not shortcut."};
        g.rows = g.cols = 21;
        for (r = 0; r < 21; r++) {
            for (c = 0; c < 21; c++) {
                g.c[r][c] = '#';
            }
        }
        for (r = 0; r < 21; r += 2) {
            for (c = 0; c < 21; c++) {
                g.c[r][c] = '.';
            }
        }
        for (r = 1; r < 21; r += 2) {
            g.c[r][(r / 2) % 2 == 0 ? 20 : 0] = '.';
        }
        g.c[0][0] = 'R';
        g.c[20][20] = 'T';
        rc |= report(dir, "snake_21x21.txt", &g, h, 2, 1);
    }

    /* The hand-drawn maps are small enough to check by eye; they go through
     * the same report but their files already exist in data/ (dir = NULL). */
    {
        static const char *const sample2[] = {
            "############", "#D...T.....#", "#.SSS.SSS..#", "#.........R#",
            "#.SSS.SSS..#", "#T..T..T..C#", "############"};
        static const char *const sealed[] = {
            "#########", "#R..T...#", "#.......#", "#########", "#...#T#.#",
            "#...###.#", "#########"};
        static const char *const boxed[] = {
            "###########", "#R#.......#", "###..T....#", "#....R..T.#",
            "###########"};
        static const char *const none[] = {
            "#####", "#R..#", "#...#", "#####"};

        from_text(&g, sample2, 7);
        rc |= report(NULL, "sample2.txt", &g, NULL, 0, 0);
        from_text(&g, sealed, 7);
        rc |= report(NULL, "sealed_task.txt", &g, NULL, 0, 0);
        from_text(&g, boxed, 5);
        rc |= report(NULL, "boxed_robot.txt", &g, NULL, 0, 0);
        from_text(&g, none, 4);
        rc |= report(NULL, "no_tasks.txt", &g, NULL, 0, 0);
    }
    return rc;
}
