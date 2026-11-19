/*
 * replay.h - write and read the REPLAY file (item 24).
 *
 * A replay is a plain-text recording of what the planner decided: the map,
 * the robots, the tasks, and for each plan the cell every robot stands on at
 * every tick. Viewers (the ASCII player, item 25) only read it; they never
 * plan. Delete every viewer and the planner output is identical.
 *
 * FORMAT, version 1 (one record per line, upper-case keywords, spaces
 * between numbers, \n or \r\n line ends, '#' lines and blank lines are
 * ignored outside the map block; all ids are 1-based, rows and cols 0-based)
 *
 *   REPLAY 1
 *   GRID <rows> <cols>
 *   <rows lines of exactly <cols> map symbols>        same symbols as input
 *   ROBOT <id> <capacity> <battery> <row> <col>       id = 1, 2, ... in order
 *   TASK <id> <row> <col> <weight> <value> <priority> id = 1, 2, ... in order
 *   PLAN <label>                 one word; the records below belong to it
 *   STEP <tick> <robot> <row> <col>   robot position at that tick
 *   PICK <tick> <robot> <task>        robot picks up the task at that tick
 *   END
 * A file has one or more PLAN sections (REPLAY_MAX_PLANS at most). Within a
 * plan, records are sorted by tick, each robot has one STEP per tick from 0
 * to its last tick (it stays where it is afterwards), tick 0 is its start.
 *
 * What the reader checks (every failure gives a line-numbered message):
 *   header, GRID size 1..MAX_DIM, exact row widths, valid symbols, ids in
 *   order, numbers in range, STEP only on walkable cells, tick 0 on the
 *   robot's start cell, each STEP at most one cell from the previous one,
 *   PICK on the task's own cell, a task picked at most once per plan,
 *   records not going back in time, END present, nothing after END.
 *
 * replay_write returns 0, or -1 for bad arguments or a file that cannot be
 * written. replay_read returns 0, -1 (cannot read the file, out of memory)
 * or -2 (the content is invalid; err holds the message). On failure the
 * Replay is left empty. Time O(file size), space O(file size).
 */
#ifndef REPLAY_H
#define REPLAY_H

#include <stddef.h>

#include "planner.h"
#include "problem.h"
#include "route.h"

#define REPLAY_MAX_PLANS  4
#define REPLAY_LABEL_MAX  32     /* label length including the final '\0' */

typedef struct { int row, col, capacity, battery; } ReplayRobot;
typedef struct { int row, col, weight, value, priority; } ReplayTask;
typedef struct { int tick, robot, task; } ReplayPick;   /* robot, task 1-based */

typedef struct {
    char        label[REPLAY_LABEL_MAX];
    int         n_ticks;               /* last tick of any robot            */
    int         len[MAX_ROBOTS];       /* positions stored per robot        */
    int        *row[MAX_ROBOTS];       /* row[r][t], col[r][t] for t < len  */
    int        *col[MAX_ROBOTS];
    int         cap[MAX_ROBOTS];       /* private: allocated entries        */
    int         n_picks;
    ReplayPick *pick;                  /* n_picks entries, tick order       */
    int         pick_cap;              /* private                           */
} ReplayPlan;

typedef struct {
    int         rows, cols;
    char       *grid;                  /* rows * cols symbols               */
    int         n_robots;
    ReplayRobot robot[MAX_ROBOTS];
    int         n_tasks;
    ReplayTask  task[MAX_TASKS];
    int         n_plans;
    ReplayPlan  plan[REPLAY_MAX_PLANS];
} Replay;

/*
 * Write n_plans plans (label[k], plan[k], route[k]) of problem p to path.
 * Each route set must have been built for its plan and for p. Labels must be
 * one word shorter than REPLAY_LABEL_MAX without spaces.
 */
int replay_write(const char *path, const Problem *p, int n_plans,
                 const char *const label[], const Plan plan[],
                 const RouteSet route[]);

/* Read and validate a replay file. Call replay_free when done. */
int replay_read(const char *path, Replay *rp, char *err, size_t err_size);

/* Free everything and reset rp. Safe on NULL and when called twice. */
void replay_free(Replay *rp);

#endif /* REPLAY_H */
