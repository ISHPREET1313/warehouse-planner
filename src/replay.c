/* replay.c - write and read the replay file. See replay.h. */
#include "replay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"

#define REPLAY_MAX_BYTES (64L * 1024L * 1024L)

/* ---------------------------------------------------------------- write */

static int label_ok(const char *s)
{
    size_t n = strlen(s);

    if (n == 0 || n >= REPLAY_LABEL_MAX) {
        return 0;
    }
    for (; *s != '\0'; s++) {
        if (*s <= ' ' || *s > '~') {
            return 0;          /* no spaces, controls or non-ASCII */
        }
    }
    return 1;
}

static int write_plan(FILE *f, const Problem *p, const char *label,
                      const Plan *plan, const RouteSet *rs)
{
    int last = 0, t, r, i, node;

    if (fprintf(f, "PLAN %s\n", label) < 0) {
        return -1;
    }
    for (r = 0; r < rs->n_robots; r++) {
        if (rs->robot[r].n_steps > last) {
            last = rs->robot[r].n_steps;
        }
    }
    for (t = 0; t <= last; t++) {
        for (r = 0; r < rs->n_robots; r++) {
            const RobotRoute *rt = &rs->robot[r];

            if (t > rt->n_steps) {
                continue;               /* this robot has finished */
            }
            node = rt->node[t];
            if (fprintf(f, "STEP %d %d %d %d\n", t, r + 1, node / p->cols,
                        node % p->cols) < 0) {
                return -1;
            }
            for (i = 0; i < plan->robot[r].n_stops; i++) {
                if (rt->pick_tick[i] == t &&
                    fprintf(f, "PICK %d %d %d\n", t, r + 1,
                            plan->robot[r].stop[i] + 1) < 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

/* 1 if the route set fits plan and problem, otherwise 0. */
static int route_fits(const Problem *p, const Plan *plan, const RouteSet *rs)
{
    int r, i, cells = p->rows * p->cols;

    if (plan->n_robots != p->n_robots || rs->n_robots != p->n_robots) {
        return 0;
    }
    for (r = 0; r < rs->n_robots; r++) {
        const RobotRoute *rt = &rs->robot[r];

        if (rt->node == NULL || rt->n_steps < 0 ||
            rt->n_picks != plan->robot[r].n_stops) {
            return 0;
        }
        for (i = 0; i <= rt->n_steps; i++) {
            if (rt->node[i] < 0 || rt->node[i] >= cells) {
                return 0;
            }
        }
        for (i = 0; i < rt->n_picks; i++) {
            if (rt->pick_tick[i] < 0 || rt->pick_tick[i] > rt->n_steps ||
                plan->robot[r].stop[i] < 0 ||
                plan->robot[r].stop[i] >= p->n_tasks) {
                return 0;
            }
        }
    }
    return 1;
}

int replay_write(const char *path, const Problem *p, int n_plans,
                 const char *const label[], const Plan plan[],
                 const RouteSet route[])
{
    FILE *f;
    int k, r, rc = 0;

    if (path == NULL || p == NULL || p->cells == NULL || label == NULL ||
        plan == NULL || route == NULL || n_plans < 1 ||
        n_plans > REPLAY_MAX_PLANS) {
        return -1;
    }
    for (k = 0; k < n_plans; k++) {
        if (label[k] == NULL || !label_ok(label[k]) ||
            !route_fits(p, &plan[k], &route[k])) {
            return -1;
        }
    }
    f = fopen(path, "w");
    if (f == NULL) {
        return -1;
    }
    if (fprintf(f, "REPLAY 1\nGRID %d %d\n", p->rows, p->cols) < 0) {
        rc = -1;
    }
    for (r = 0; r < p->rows && rc == 0; r++) {
        if (fwrite(p->cells + (size_t)r * (size_t)p->cols, 1,
                   (size_t)p->cols, f) != (size_t)p->cols ||
            fputc('\n', f) == EOF) {
            rc = -1;
        }
    }
    for (r = 0; r < p->n_robots && rc == 0; r++) {
        if (fprintf(f, "ROBOT %d %d %d %d %d\n", r + 1, p->robots[r].capacity,
                    p->robots[r].battery, p->robots[r].row,
                    p->robots[r].col) < 0) {
            rc = -1;
        }
    }
    for (r = 0; r < p->n_tasks && rc == 0; r++) {
        if (fprintf(f, "TASK %d %d %d %d %d %d\n", r + 1, p->tasks[r].row,
                    p->tasks[r].col, p->tasks[r].weight, p->tasks[r].value,
                    p->tasks[r].priority) < 0) {
            rc = -1;
        }
    }
    for (k = 0; k < n_plans && rc == 0; k++) {
        rc = write_plan(f, p, label[k], &plan[k], &route[k]);
    }
    if (rc == 0 && fprintf(f, "END\n") < 0) {
        rc = -1;
    }
    if (fclose(f) != 0) {
        rc = -1;
    }
    return rc;
}

/* ----------------------------------------------------------------- read */

void replay_free(Replay *rp)
{
    int k, r;

    if (rp == NULL) {
        return;
    }
    free(rp->grid);
    for (k = 0; k < REPLAY_MAX_PLANS; k++) {
        for (r = 0; r < MAX_ROBOTS; r++) {
            free(rp->plan[k].row[r]);
            free(rp->plan[k].col[r]);
        }
        free(rp->plan[k].pick);
    }
    memset(rp, 0, sizeof *rp);
}

/* Bounded text building for messages (no snprintf, same idea as io.c). */
static size_t put_text(char *buf, size_t size, size_t pos, const char *text)
{
    if (buf == NULL || size == 0) {
        return pos;
    }
    while (*text != '\0' && pos + 1 < size) {
        buf[pos++] = *text++;
    }
    buf[pos < size ? pos : size - 1] = '\0';
    return pos;
}

static size_t put_number(char *buf, size_t size, size_t pos, long v)
{
    char tmp[24];
    int n = 0;

    if (buf == NULL || size == 0) {
        return pos;
    }
    if (v == 0) {
        tmp[n++] = '0';
    }
    while (v > 0 && n < 22) {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    }
    while (n > 0 && pos + 1 < size) {
        buf[pos++] = tmp[--n];
    }
    buf[pos < size ? pos : size - 1] = '\0';
    return pos;
}

/* Message "line N: text" into err, return -2 (invalid content). */
static int bad(char *err, size_t size, int line, const char *text)
{
    size_t pos = 0;

    if (err != NULL && size > 0) {
        pos = put_text(err, size, pos, "line ");
        pos = put_number(err, size, pos, line);
        pos = put_text(err, size, pos, ": ");
        put_text(err, size, pos, text);
    }
    return -2;
}

/* Read the whole file into a malloc'd, NUL-terminated buffer. */
static char *slurp(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    char *buf = NULL, *bigger;
    size_t len = 0, cap = 0, got;

    if (f == NULL) {
        return NULL;
    }
    for (;;) {
        if (len + 4097 > cap) {
            cap = cap == 0 ? 8192 : cap * 2;
            bigger = (char *)realloc(buf, cap);
            if (bigger == NULL) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = bigger;
        }
        got = fread(buf + len, 1, 4096, f);
        len += got;
        if (got < 4096 || (long)len > REPLAY_MAX_BYTES) {
            break;
        }
    }
    if (ferror(f) || (long)len > REPLAY_MAX_BYTES) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    buf[len] = '\0';
    *out_len = len;
    return buf;
}

static char *skip_blank(char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/* Next whole number in lo..hi. Returns 0 and moves *s, or -1. */
static int next_int(char **s, int lo, int hi, int *out)
{
    char *q = skip_blank(*s);
    long v = 0;

    if (*q < '0' || *q > '9') {
        return -1;
    }
    while (*q >= '0' && *q <= '9') {
        v = v * 10 + (*q - '0');
        if (v > hi) {
            return -1;
        }
        q++;
    }
    if (*q != '\0' && *q != ' ' && *q != '\t') {
        return -1;                      /* "12abc" is not a number */
    }
    if (v < lo) {
        return -1;
    }
    *out = (int)v;
    *s = q;
    return 0;
}

/* Next word (no blanks) into word[size]. Returns 0, or -1 if none/too long. */
static int next_word(char **s, char *word, size_t size)
{
    char *q = skip_blank(*s);
    size_t n = 0;

    while (*q != '\0' && *q != ' ' && *q != '\t') {
        if (n + 1 >= size) {
            return -1;
        }
        word[n++] = *q++;
    }
    word[n] = '\0';
    *s = q;
    return n > 0 ? 0 : -1;
}

static int at_end(char *s)
{
    return *skip_blank(s) == '\0';
}

/* Append a position for robot r of plan pl. Returns 0 or -1 (memory). */
static int add_position(ReplayPlan *pl, int r, int row, int col)
{
    int *a, *b, cap;

    if (pl->len[r] == pl->cap[r]) {
        cap = pl->cap[r] == 0 ? 16 : pl->cap[r] * 2;
        a = (int *)realloc(pl->row[r], (size_t)cap * sizeof(int));
        if (a == NULL) {
            return -1;
        }
        pl->row[r] = a;
        b = (int *)realloc(pl->col[r], (size_t)cap * sizeof(int));
        if (b == NULL) {
            return -1;
        }
        pl->col[r] = b;
        pl->cap[r] = cap;
    }
    pl->row[r][pl->len[r]] = row;
    pl->col[r][pl->len[r]] = col;
    pl->len[r]++;
    return 0;
}

static int add_pick(ReplayPlan *pl, int tick, int robot, int task)
{
    ReplayPick *a;
    int cap;

    if (pl->n_picks == pl->pick_cap) {
        cap = pl->pick_cap == 0 ? 16 : pl->pick_cap * 2;
        a = (ReplayPick *)realloc(pl->pick, (size_t)cap * sizeof *a);
        if (a == NULL) {
            return -1;
        }
        pl->pick = a;
        pl->pick_cap = cap;
    }
    pl->pick[pl->n_picks].tick = tick;
    pl->pick[pl->n_picks].robot = robot;
    pl->pick[pl->n_picks].task = task;
    pl->n_picks++;
    return 0;
}

/* Parser state that lives across lines. */
typedef struct {
    int state;          /* 0 REPLAY, 1 GRID, 2 map rows, 3 body, 4 after END */
    int rows_read;
    int last_tick;      /* latest tick seen in the current plan              */
    char picked[MAX_TASKS];   /* tasks already picked in the current plan   */
} Parse;

static int cell_ok(const Replay *rp, int row, int col)
{
    return row >= 0 && row < rp->rows && col >= 0 && col < rp->cols;
}

/* One map row (state 2). */
static int do_map_row(Replay *rp, Parse *ps, char *line, int no,
                      char *err, size_t es)
{
    int c;

    if ((int)strlen(line) != rp->cols) {
        return bad(err, es, no, "map row does not have exactly <cols> characters");
    }
    for (c = 0; c < rp->cols; c++) {
        if (!problem_is_symbol(line[c])) {
            return bad(err, es, no, "invalid map symbol");
        }
        rp->grid[(size_t)ps->rows_read * (size_t)rp->cols + (size_t)c] = line[c];
    }
    ps->rows_read++;
    if (ps->rows_read == rp->rows) {
        ps->state = 3;
    }
    return 0;
}

/* 1 if every robot has at least its tick-0 STEP in plan pl. */
static int plan_complete(const Replay *rp, const ReplayPlan *pl)
{
    int r;

    for (r = 0; r < rp->n_robots; r++) {
        if (pl->len[r] < 1) {
            return 0;
        }
    }
    return 1;
}

static int do_step(Replay *rp, Parse *ps, char *s, int no, char *err, size_t es)
{
    ReplayPlan *pl;
    int t, r, row, col, k;

    if (rp->n_plans == 0) {
        return bad(err, es, no, "STEP before PLAN");
    }
    pl = &rp->plan[rp->n_plans - 1];
    if (next_int(&s, 0, 100000000, &t) || next_int(&s, 1, rp->n_robots, &r) ||
        next_int(&s, 0, MAX_DIM, &row) || next_int(&s, 0, MAX_DIM, &col) ||
        !at_end(s)) {
        return bad(err, es, no, "STEP needs: tick robot row col (robot must exist)");
    }
    r--;
    if (!cell_ok(rp, row, col) ||
        !cell_is_walkable(rp->grid[row * rp->cols + col])) {
        return bad(err, es, no, "STEP is not on a walkable cell of the map");
    }
    if (t < ps->last_tick) {
        return bad(err, es, no, "records go back in time");
    }
    if (t != pl->len[r]) {
        return bad(err, es, no, "STEP ticks of a robot must be 0, 1, 2, ... in order");
    }
    if (t == 0) {
        if (row != rp->robot[r].row || col != rp->robot[r].col) {
            return bad(err, es, no, "tick 0 must be the robot's start cell");
        }
    } else {
        k = (row - pl->row[r][t - 1]) * (row - pl->row[r][t - 1]) +
            (col - pl->col[r][t - 1]) * (col - pl->col[r][t - 1]);
        if (k > 1) {
            return bad(err, es, no, "STEP moves more than one cell");
        }
    }
    if (add_position(pl, r, row, col) != 0) {
        return -1;
    }
    ps->last_tick = t;
    if (t > pl->n_ticks) {
        pl->n_ticks = t;
    }
    return 0;
}

static int do_pick(Replay *rp, Parse *ps, char *s, int no, char *err, size_t es)
{
    ReplayPlan *pl;
    int t, r, k;

    if (rp->n_plans == 0) {
        return bad(err, es, no, "PICK before PLAN");
    }
    pl = &rp->plan[rp->n_plans - 1];
    if (next_int(&s, 0, 100000000, &t) || next_int(&s, 1, rp->n_robots, &r) ||
        next_int(&s, 1, rp->n_tasks, &k) || !at_end(s)) {
        return bad(err, es, no, "PICK needs: tick robot task (both must exist)");
    }
    r--;
    k--;
    if (t < ps->last_tick) {
        return bad(err, es, no, "records go back in time");
    }
    if (t >= pl->len[r] || pl->row[r][t] != rp->task[k].row ||
        pl->col[r][t] != rp->task[k].col) {
        return bad(err, es, no, "PICK: the robot is not on the task's cell at that tick");
    }
    if (ps->picked[k]) {
        return bad(err, es, no, "PICK: task already picked in this plan");
    }
    ps->picked[k] = 1;
    ps->last_tick = t;
    return add_pick(pl, t, r + 1, k + 1) == 0 ? 0 : -1;
}

/* One body line (state 3). Returns 0, -1 or -2. */
static int do_body(Replay *rp, Parse *ps, char *s, int no, char *err, size_t es)
{
    char word[16];
    int id, a, b, c, d, e;

    if (next_word(&s, word, sizeof word) != 0) {
        return bad(err, es, no, "unknown record");
    }
    if (strcmp(word, "ROBOT") == 0) {
        if (rp->n_plans > 0 || rp->n_robots >= MAX_ROBOTS ||
            next_int(&s, 1, MAX_ROBOTS, &id) || id != rp->n_robots + 1 ||
            next_int(&s, 1, MAX_NUMBER, &a) || next_int(&s, 1, MAX_NUMBER, &b) ||
            next_int(&s, 0, MAX_DIM, &c) || next_int(&s, 0, MAX_DIM, &d) ||
            !at_end(s) || !cell_ok(rp, c, d)) {
            return bad(err, es, no, "bad ROBOT record (before PLAN, ids 1, 2, ... in order)");
        }
        rp->robot[rp->n_robots].capacity = a;
        rp->robot[rp->n_robots].battery = b;
        rp->robot[rp->n_robots].row = c;
        rp->robot[rp->n_robots].col = d;
        rp->n_robots++;
        return 0;
    }
    if (strcmp(word, "TASK") == 0) {
        if (rp->n_plans > 0 || rp->n_tasks >= MAX_TASKS ||
            next_int(&s, 1, MAX_TASKS, &id) || id != rp->n_tasks + 1 ||
            next_int(&s, 0, MAX_DIM, &a) || next_int(&s, 0, MAX_DIM, &b) ||
            next_int(&s, 1, MAX_NUMBER, &c) || next_int(&s, 0, MAX_NUMBER, &d) ||
            next_int(&s, 1, MAX_PRIORITY, &e) || !at_end(s) ||
            !cell_ok(rp, a, b)) {
            return bad(err, es, no, "bad TASK record (before PLAN, ids 1, 2, ... in order)");
        }
        rp->task[rp->n_tasks].row = a;
        rp->task[rp->n_tasks].col = b;
        rp->task[rp->n_tasks].weight = c;
        rp->task[rp->n_tasks].value = d;
        rp->task[rp->n_tasks].priority = e;
        rp->n_tasks++;
        return 0;
    }
    if (strcmp(word, "PLAN") == 0) {
        if (rp->n_plans > 0 && !plan_complete(rp, &rp->plan[rp->n_plans - 1])) {
            return bad(err, es, no, "every robot needs STEP records from tick 0");
        }
        if (rp->n_plans >= REPLAY_MAX_PLANS || rp->n_robots == 0 ||
            next_word(&s, rp->plan[rp->n_plans].label, REPLAY_LABEL_MAX) != 0 ||
            !at_end(s)) {
            return bad(err, es, no, "bad PLAN record (one label word, robots defined first)");
        }
        rp->n_plans++;
        ps->last_tick = 0;
        memset(ps->picked, 0, sizeof ps->picked);
        return 0;
    }
    if (strcmp(word, "STEP") == 0) {
        return do_step(rp, ps, s, no, err, es);
    }
    if (strcmp(word, "PICK") == 0) {
        return do_pick(rp, ps, s, no, err, es);
    }
    if (strcmp(word, "END") == 0) {
        if (!at_end(s) || rp->n_plans == 0) {
            return bad(err, es, no, "END needs at least one PLAN before it");
        }
        if (!plan_complete(rp, &rp->plan[rp->n_plans - 1])) {
            return bad(err, es, no, "every robot needs STEP records from tick 0");
        }
        ps->state = 4;
        return 0;
    }
    return bad(err, es, no, "unknown record");
}

static int do_header(Replay *rp, Parse *ps, char *s, int no, char *err, size_t es)
{
    char word[16];
    int a, b;

    if (next_word(&s, word, sizeof word) != 0) {
        return bad(err, es, no, "unknown record");
    }
    if (ps->state == 0) {
        if (strcmp(word, "REPLAY") != 0 || next_int(&s, 1, 1, &a) ||
            !at_end(s)) {
            return bad(err, es, no, "first record must be: REPLAY 1");
        }
        ps->state = 1;
        return 0;
    }
    if (strcmp(word, "GRID") != 0 || next_int(&s, 1, MAX_DIM, &a) ||
        next_int(&s, 1, MAX_DIM, &b) || !at_end(s)) {
        return bad(err, es, no, "second record must be: GRID <rows> <cols> (1..500)");
    }
    rp->rows = a;
    rp->cols = b;
    rp->grid = (char *)malloc((size_t)a * (size_t)b);
    if (rp->grid == NULL) {
        return -1;
    }
    ps->state = 2;
    return 0;
}

int replay_read(const char *path, Replay *rp, char *err, size_t err_size)
{
    Parse ps;
    char *buf, *line, *nl, *end;
    size_t len = 0, n;
    int no = 0, rc = 0;

    if (err != NULL && err_size > 0) {
        err[0] = '\0';
    }
    if (rp == NULL || path == NULL) {
        return -1;
    }
    memset(rp, 0, sizeof *rp);
    memset(&ps, 0, sizeof ps);
    buf = slurp(path, &len);
    if (buf == NULL) {
        put_text(err, err_size, 0, "cannot read the replay file");
        return -1;
    }
    end = buf + len;
    for (line = buf; rc == 0 && line < end; line = nl) {
        nl = strchr(line, '\n');
        if (nl != NULL) {
            *nl++ = '\0';
        } else {
            nl = end;                            /* last line, no newline */
        }
        no++;
        n = strlen(line);
        if (n > 0 && line[n - 1] == '\r') {
            line[n - 1] = '\0';                  /* \r\n line end */
        }
        if (ps.state == 2) {
            rc = do_map_row(rp, &ps, line, no, err, err_size);
            continue;
        }
        if (*skip_blank(line) == '\0' || *skip_blank(line) == '#') {
            continue;
        }
        if (ps.state == 4) {
            rc = bad(err, err_size, no, "text after END");
        } else if (ps.state < 2) {
            rc = do_header(rp, &ps, line, no, err, err_size);
        } else {
            rc = do_body(rp, &ps, line, no, err, err_size);
        }
    }
    if (rc == 0 && ps.state != 4) {
        rc = bad(err, err_size, no, "file ended before END");
    }
    free(buf);
    if (rc != 0) {
        if (rc == -1) {
            put_text(err, err_size, 0, "out of memory");
        }
        replay_free(rp);
    }
    return rc;
}
