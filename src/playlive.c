/*
 * playlive.c - build one screen of the live player. See playlive.h.
 * No printing, no console calls, no sleeping.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "playlive.h"

#define PAL_N       8
#define BG_FLOOR    234
#define BG_WALL     244
#define BG_SHELF    94
#define FG_BLACK    16
#define PANEL_MAX   (PLAY_PANEL_ROBOTS + 2)   /* robots, "+k more", events */
#define TRAIL_DOTS  3

/* One colour per robot (256-colour codes); the robot number is always
 * printed too, so robots 9 and up and colour-blind viewers stay readable. */
static const int PAL[PAL_N] = {208, 46, 39, 201, 226, 51, 141, 250};   /* no red: red = unserved */

/* ---------- output buffer ---------- */

typedef struct {
    char  *buf;
    size_t size, len;
    int    err, color;
    char   cur[64];          /* style currently active ("" = default) */
} Out;

static void put(Out *o, const char *s)
{
    size_t n = strlen(s);

    if (o->err || o->len + n + 1 > o->size) {
        o->err = 1;
        return;
    }
    memcpy(o->buf + o->len, s, n + 1);
    o->len += n;
}

/* Write at most max characters of s. */
static void put_cut(Out *o, const char *s, int max)
{
    char tmp[256];
    size_t n;

    if (max <= 0) {
        return;
    }
    n = strlen(s);
    if (n > (size_t)max) {
        n = (size_t)max;
    }
    if (n > sizeof tmp - 1) {
        n = sizeof tmp - 1;
    }
    memcpy(tmp, s, n);
    tmp[n] = '\0';
    put(o, tmp);
}

static void clear_style(Out *o)
{
    if (o->color && o->cur[0] != '\0') {
        put(o, "\033[0m");
        o->cur[0] = '\0';
    }
}

/* Switch to style st (emits codes only when it differs from the current). */
static void set_style(Out *o, const char *st)
{
    if (!o->color || strcmp(o->cur, st) == 0) {
        return;
    }
    put(o, "\033[0m");
    put(o, st);
    strcpy(o->cur, st);
}

static void eol(Out *o, int last)
{
    clear_style(o);
    if (o->color) {
        put(o, "\033[K");
        if (!last) {
            put(o, "\n");
        }
    } else {
        put(o, "\n");
    }
}

static char robot_char(int r)
{
    int id = r + 1;

    return id <= 9 ? (char)('0' + id) : (char)('a' + (id - 10));
}

static int pos_index(const ReplayPlan *pl, int r, int tick)
{
    return tick < pl->len[r] ? tick : pl->len[r] - 1;
}

/* ---------- the panel text ---------- */

typedef struct {
    int  chip;               /* robot index for a colour chip, or -1 */
    char text[200];
} PanelLine;

static int make_panel(const Replay *rp, const ReplayPlan *pl, int tick,
                      PanelLine *out)
{
    int n = 0, shown, r, i, load, picked, mine, idx, found, any;
    char next[80];
    char piece[80];

    shown = rp->n_robots < PLAY_PANEL_ROBOTS ? rp->n_robots : PLAY_PANEL_ROBOTS;
    for (r = 0; r < shown; r++) {
        load = picked = mine = 0;
        found = -1;
        for (i = 0; i < pl->n_picks; i++) {
            if (pl->pick[i].robot != r + 1) {
                continue;
            }
            mine++;
            if (pl->pick[i].tick <= tick) {
                picked++;
                load += rp->task[pl->pick[i].task - 1].weight;
            } else if (found < 0) {
                found = i;               /* picks are in tick order */
            }
        }
        if (found < 0) {
            strcpy(next, "done");
        } else {
            const ReplayTask *t = &rp->task[pl->pick[found].task - 1];

            sprintf(next, "T%d at (%d,%d)", pl->pick[found].task, t->row, t->col);
        }
        idx = pos_index(pl, r, tick);
        out[n].chip = r;
        sprintf(out[n].text, "load %d/%d  battery %d/%d  next %s", load,
                rp->robot[r].capacity, idx, rp->robot[r].battery, next);
        n++;
    }
    if (rp->n_robots > shown) {
        out[n].chip = -1;
        sprintf(out[n].text, "+%d more robots", rp->n_robots - shown);
        n++;
    }
    out[n].chip = -1;
    strcpy(out[n].text, "events:");
    any = 0;
    for (i = 0; i < pl->n_picks; i++) {
        if (pl->pick[i].tick == tick) {
            sprintf(piece, " robot %c picked T%d",
                    robot_char(pl->pick[i].robot - 1), pl->pick[i].task);
            if (strlen(out[n].text) + strlen(piece) < sizeof out[n].text) {
                strcat(out[n].text, piece);
            }
            any = 1;
        }
    }
    if (!any) {
        strcat(out[n].text, " none");
    }
    n++;
    return n;
}

static void emit_panel(Out *o, const PanelLine *p, int width)
{
    char st[64], chip[3];

    if (p->chip >= 0) {
        chip[0] = robot_char(p->chip);
        chip[1] = ' ';
        chip[2] = '\0';
        sprintf(st, "\033[48;5;%dm\033[38;5;%dm\033[1m", PAL[p->chip % PAL_N],
                FG_BLACK);
        set_style(o, st);
        put_cut(o, chip, width);
        sprintf(st, "\033[38;5;%dm\033[1m", PAL[p->chip % PAL_N]);
        set_style(o, st);
        put_cut(o, p->text, width - 2);
    } else {
        set_style(o, "");
        put_cut(o, p->text, width);
    }
    clear_style(o);
}

/* ---------- the map ---------- */

typedef struct {
    int *robot_at, *wait_has, *wait_owner, *done_has, *trail_own, *trail_rank;
    int *obst, *shelf, *cells, *chg, *dock;
} Blocks;

static void draw_block(Out *o, const Blocks *b, int i, int cell_w, int rank_of_robot)
{
    int cls, special, blocked, rid;
    char st[64], t[3];

    (void)rank_of_robot;
    special = b->chg[i] + b->dock[i] > 0;
    blocked = b->obst[i] + b->shelf[i];
    cls = play_block_class(b->robot_at[i] > 0, b->wait_has[i], b->done_has[i],
                           special, blocked, b->cells[i]);
    if (cls < 0) {
        cls = BLOCK_FLOOR;
    }
    st[0] = '\0';
    t[0] = ' ';
    t[1] = ' ';
    t[2] = '\0';

    if (b->trail_own[i] > 0 && (cls == BLOCK_FLOOR || cls == BLOCK_SPECIAL)) {
        const char *weight[TRAIL_DOTS] = {"\033[1m", "", "\033[2m"};

        rid = b->trail_own[i] - 1;
        t[0] = 'o';
        sprintf(st, "\033[48;5;%dm\033[38;5;%dm%s", BG_FLOOR, PAL[rid % PAL_N],
                weight[b->trail_rank[i]]);
    } else {
        switch (cls) {
        case BLOCK_ROBOT:
            rid = b->robot_at[i] - 1;
            t[0] = robot_char(rid);
            sprintf(st, "\033[48;5;%dm\033[38;5;%dm\033[1m", PAL[rid % PAL_N],
                    FG_BLACK);
            break;
        case BLOCK_TASK_WAIT:
            if (b->wait_owner[i] > 0) {          /* a robot will pick it */
                t[0] = 'T';
                sprintf(st, "\033[48;5;%dm\033[38;5;%dm\033[1m", BG_FLOOR,
                        PAL[(b->wait_owner[i] - 1) % PAL_N]);
            } else {                             /* no robot takes it */
                t[0] = '?';
                sprintf(st, "\033[48;5;88m\033[38;5;231m\033[1m");
            }
            break;
        case BLOCK_TASK_DONE:
            t[0] = 'x';
            sprintf(st, "\033[48;5;%dm\033[38;5;240m", BG_FLOOR);
            break;
        case BLOCK_SPECIAL:
            t[0] = b->chg[i] > 0 ? 'C' : 'D';
            sprintf(st, "\033[48;5;%dm\033[38;5;%dm\033[1m", BG_FLOOR,
                    b->chg[i] > 0 ? 51 : 46);
            break;
        case BLOCK_WALL:
            if (b->shelf[i] >= b->obst[i]) {
                t[0] = t[1] = '=';
                sprintf(st, "\033[48;5;%dm", BG_SHELF);
            } else {
                t[0] = t[1] = '#';
                sprintf(st, "\033[48;5;%dm", BG_WALL);
            }
            break;
        default:
            t[0] = '.';
            sprintf(st, "\033[48;5;%dm", BG_FLOOR);
            break;
        }
    }
    if (o->color && (cls == BLOCK_WALL || cls == BLOCK_FLOOR) &&
        t[0] != 'o') {
        t[0] = t[1] = ' ';              /* colour alone shows walls and floor */
    }
    set_style(o, st);
    t[cell_w] = '\0';
    put(o, t);
}

/* The legend with the real colours (colour mode). Every symbol is drawn the
 * way it appears on the map, so nothing has to be explained by a colour
 * name. Cut at width characters like every other line. */
typedef struct {
    const char *style;       /* escape codes, "" = default */
    const char *text;
} LegendPart;

static void emit_legend(Out *o, int width)
{
    static const LegendPart part[] = {
        {"\033[48;5;234m\033[38;5;208m", "o"},          {"", " trail  "},
        {"\033[48;5;234m\033[38;5;39m\033[1m", "T"},    {"", " task  "},
        {"\033[48;5;88m\033[38;5;231m\033[1m", "?"},    {"", " unserved  "},
        {"\033[48;5;234m\033[38;5;240m", "x"},          {"", " picked  "},
        {"\033[48;5;234m\033[38;5;51m\033[1m", "C"},    {"", " charger  "},
        {"\033[48;5;234m\033[38;5;46m\033[1m", "D"},    {"", " dock  "},
        {"\033[48;5;94m", "  "},                        {"", " shelf  "},
        {"\033[48;5;244m", "  "},                       {"", " wall"}
    };
    size_t i;
    int left = width, n;

    for (i = 0; i < sizeof part / sizeof part[0] && left > 0; i++) {
        set_style(o, part[i].style);
        n = (int)strlen(part[i].text);
        put_cut(o, part[i].text, left);
        left -= n;
    }
}

static int ceil_div(int a, int b)
{
    return (a + b - 1) / b;
}

size_t playlive_frame_size(const Replay *rp, const PlayLayout *lo, int term_w,
                           int term_h)
{
    if (rp == NULL || lo == NULL || term_w < 1 || term_h < 1) {
        return 0;
    }
    return ((size_t)lo->block_rows + 20) *
               ((size_t)lo->block_cols * 40 + (size_t)term_w + 200) + 1024;
}

int playlive_frame(const Replay *rp, int plan_index, int tick,
                   const PlayLayout *lo, int term_w, int term_h, int color,
                   int tps, int paused, char *buf, size_t size)
{
    const ReplayPlan *pl;
    PanelLine panel[PANEL_MAX];
    Blocks b;
    Out o;
    int *mem;
    int nb, n_panel, r, c, i, k, idx, bi, rank, n_dots, body, line, pw;
    int tr[TRAIL_DOTS], tc[TRAIL_DOTS], picked, owner, done_k, done_v;
    char head[240], tmp[80];

    (void)term_h;
    if (rp == NULL || lo == NULL || buf == NULL || size == 0 || term_w < 1 ||
        plan_index < 0 || plan_index >= rp->n_plans) {
        return -1;
    }
    pl = &rp->plan[plan_index];
    if (tick < 0 || tick > pl->n_ticks || lo->k < 1 ||
        (lo->cell_w != 1 && lo->cell_w != 2) ||
        lo->block_rows != ceil_div(rp->rows, lo->k) ||
        lo->block_cols != ceil_div(rp->cols, lo->k) ||
        lo->map_w != lo->block_cols * lo->cell_w) {
        return -1;
    }

    nb = lo->block_rows * lo->block_cols;
    mem = (int *)calloc((size_t)nb * 11, sizeof(int));
    if (mem == NULL) {
        return -1;
    }
    b.robot_at = mem;
    b.wait_has = mem + nb;
    b.wait_owner = mem + 2 * nb;
    b.done_has = mem + 3 * nb;
    b.trail_own = mem + 4 * nb;
    b.trail_rank = mem + 5 * nb;
    b.obst = mem + 6 * nb;
    b.shelf = mem + 7 * nb;
    b.cells = mem + 8 * nb;
    b.chg = mem + 9 * nb;
    b.dock = mem + 10 * nb;

    /* terrain: count the map cells of every drawn cell */
    for (r = 0; r < rp->rows; r++) {
        for (c = 0; c < rp->cols; c++) {
            char ch = rp->grid[r * rp->cols + c];

            bi = (r / lo->k) * lo->block_cols + c / lo->k;
            b.cells[bi]++;
            if (ch == '#') {
                b.obst[bi]++;
            } else if (ch == 'S') {
                b.shelf[bi]++;
            } else if (ch == 'C') {
                b.chg[bi]++;
            } else if (ch == 'D') {
                b.dock[bi]++;
            }
        }
    }
    /* tasks: waiting (coloured by the robot that picks it) or picked */
    for (i = 0; i < rp->n_tasks; i++) {
        picked = 0;
        owner = 0;
        for (k = 0; k < pl->n_picks; k++) {
            if (pl->pick[k].task == i + 1) {
                owner = pl->pick[k].robot;
                if (pl->pick[k].tick <= tick) {
                    picked = 1;
                }
            }
        }
        bi = (rp->task[i].row / lo->k) * lo->block_cols + rp->task[i].col / lo->k;
        if (picked) {
            b.done_has[bi] = 1;
        } else {
            b.wait_has[bi] = 1;
            if (b.wait_owner[bi] == 0) {
                b.wait_owner[bi] = owner;      /* 0 = nobody takes it */
            }
        }
    }
    /* trails, then robots (the lowest robot number wins a shared cell) */
    for (r = 0; r < rp->n_robots; r++) {
        if (pl->len[r] < 1) {
            continue;
        }
        n_dots = play_trail(pl->row[r], pl->col[r], pl->len[r], tick,
                            TRAIL_DOTS, tr, tc);
        for (rank = 0; rank < n_dots; rank++) {
            bi = (tr[rank] / lo->k) * lo->block_cols + tc[rank] / lo->k;
            if (b.trail_own[bi] == 0 || rank < b.trail_rank[bi]) {
                b.trail_own[bi] = r + 1;
                b.trail_rank[bi] = rank;
            }
        }
    }
    for (r = rp->n_robots - 1; r >= 0; r--) {
        idx = pos_index(pl, r, tick);
        bi = (pl->row[r][idx] / lo->k) * lo->block_cols + pl->col[r][idx] / lo->k;
        b.robot_at[bi] = r + 1;
    }

    n_panel = make_panel(rp, pl, tick, panel);

    o.buf = buf;
    o.size = size;
    o.len = 0;
    o.err = 0;
    o.color = color;
    o.cur[0] = '\0';
    buf[0] = '\0';
    if (color) {
        put(&o, "\033[H");
    }

    /* header */
    /* most important first: a narrow terminal cuts the end */
    sprintf(head, "plan %s  tick %d/%d", pl->label, tick, pl->n_ticks);
    if (paused) {
        strcat(head, "  PAUSED");
    }
    if (tick == pl->n_ticks) {
        done_k = 0;
        done_v = 0;
        for (i = 0; i < pl->n_picks; i++) {
            done_k++;
            done_v += rp->task[pl->pick[i].task - 1].value;
        }
        sprintf(tmp, "  DONE: %d of %d tasks, value %d", done_k, rp->n_tasks,
                done_v);
        strcat(head, tmp);
    }
    if (lo->k == 1) {
        strcat(head, "  full map");
    } else {
        sprintf(tmp, "  1 square = %dx%d cells", lo->k, lo->k);
        strcat(head, tmp);
    }
    sprintf(tmp, "  %d ticks/s", tps);
    strcat(head, tmp);
    set_style(&o, "\033[1m");
    put_cut(&o, head, term_w);
    eol(&o, 0);

    /* body */
    if (lo->panel_side) {
        body = lo->block_rows > n_panel ? lo->block_rows : n_panel;
        pw = term_w - lo->map_w - 2;
        for (line = 0; line < body; line++) {
            if (line < lo->block_rows) {
                for (c = 0; c < lo->block_cols; c++) {
                    draw_block(&o, &b, line * lo->block_cols + c, lo->cell_w, 0);
                }
            } else {
                set_style(&o, "");
                for (c = 0; c < lo->map_w; c++) {
                    put(&o, " ");
                }
            }
            set_style(&o, "");
            put(&o, "  ");
            if (line < n_panel) {
                emit_panel(&o, &panel[line], pw);
            }
            eol(&o, 0);
        }
    } else {
        for (line = 0; line < lo->block_rows; line++) {
            for (c = 0; c < lo->block_cols; c++) {
                draw_block(&o, &b, line * lo->block_cols + c, lo->cell_w, 0);
            }
            eol(&o, 0);
        }
        for (line = 0; line < n_panel; line++) {
            emit_panel(&o, &panel[line], term_w);
            eol(&o, 0);
        }
    }

    /* legend and keys */
    set_style(&o, "");
    if (color) {
        emit_legend(&o, term_w);
    } else {
        put_cut(&o, PLAYLIVE_LEGEND, term_w);
    }
    eol(&o, 0);
    put_cut(&o, PLAYLIVE_KEYS, term_w);
    eol(&o, 1);
    if (color) {
        put(&o, "\033[J");
    }

    free(mem);
    if (o.err) {
        buf[0] = '\0';
        return -1;
    }
    return (int)o.len;
}
