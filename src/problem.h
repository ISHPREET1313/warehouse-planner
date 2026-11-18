/*
 * problem.h - the in-memory description of one warehouse problem.
 *
 * A Problem is what the parser produces and what every later module reads:
 * the grid, the robots (with their start cells) and the tasks (with their
 * pickup cells).  It holds data only: no algorithm and no I/O live here.
 *
 * The grid is stored row by row in one array: cell (row, col) is
 * cells[row * cols + col].  Rows and columns start at 0.
 */
#ifndef PROBLEM_H
#define PROBLEM_H

/* ---- Limits. The validator rejects anything outside them. ---- */
#define MAX_DIM       500       /* rows and cols are each 1..500            */
#define MAX_ROBOTS    32
#define MAX_TASKS     256
#define MAX_PRIORITY  5         /* priority is 1 (low) .. 5 (high)          */
#define MAX_NUMBER    1000000   /* capacity, battery, weight, value <= this,
                                   so sums of a few thousand never overflow */

/* Map symbols */
#define CELL_FREE      '.'
#define CELL_OBSTACLE  '#'
#define CELL_SHELF     'S'
#define CELL_CHARGER   'C'
#define CELL_DOCK      'D'
#define CELL_ROBOT     'R'
#define CELL_TASK      'T'

typedef struct {
    int row, col;       /* start cell (where the R is on the map) */
    int capacity;       /* max total weight it can carry          */
    int battery;        /* max total path cost before recharging  */
} Robot;

typedef struct {
    int row, col;       /* pickup cell (where the T is on the map) */
    int weight;         /* load it adds to a robot                 */
    int value;          /* benefit of picking it up                */
    int priority;       /* 1..MAX_PRIORITY                         */
} Task;

typedef struct {
    int    rows, cols;
    char  *cells;       /* rows*cols characters, malloc'd; NULL if empty */
    int    n_robots;
    Robot  robots[MAX_ROBOTS];
    int    n_tasks;
    Task   tasks[MAX_TASKS];
} Problem;

/* Set everything to zero / NULL. Call before first use. */
void problem_init(Problem *p);

/* Free the grid and reset to the empty state. Safe on NULL and called twice. */
void problem_free(Problem *p);

/* The symbol at (row, col), or '\0' if the position is outside the grid. */
char problem_cell(const Problem *p, int row, int col);

/* 1 if c is one of . # S C D R T, otherwise 0. */
int problem_is_symbol(char c);

#endif /* PROBLEM_H */
