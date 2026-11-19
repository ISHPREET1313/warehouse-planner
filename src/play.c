/*
 * play.c - the ASCII replay player (item 25). See play.h for the symbols,
 * the frame layout and the commands. Reads a Replay, draws it, never plans.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN      /* skip printing headers: they define MAX_PRIORITY too */
#include <windows.h>
#endif

#include "play.h"

#define SCROLL_STEP 10           /* rows moved by one u / d command */

/* ---------- frame text ---------- */

/* Append s to buf (size bytes, *len used). Returns 0, or -1 if it won't fit. */
static int put(char *buf, size_t size, size_t *len, const char *s)
{
    size_t n = strlen(s);

    if (*len + n + 1 > size) {
        return -1;
    }
    memcpy(buf + *len, s, n + 1);
    *len += n;
    return 0;
}

/* Robot r (0-based) is drawn as 1..9, then a, b, c, ... (never C, D, T, x). */
static char robot_char(int r)
{
    int id = r + 1;

    return id <= 9 ? (char)('0' + id) : (char)('a' + (id - 10));
}

/* How a map symbol is drawn: start cells and pickup cells are free floor
 * (the robots and tasks are drawn separately), a shelf S becomes =. */
static char glyph(char ch)
{
    if (ch == 'R' || ch == 'T') {
        return '.';
    }
    return ch == 'S' ? '=' : ch;
}

/* Index into a robot's stored positions at this tick (it stays put after
 * its last stored tick). */
static int pos_index(const ReplayPlan *pl, int r, int tick)
{
    return tick < pl->len[r] ? tick : pl->len[r] - 1;
}

size_t play_frame_size(const Replay *rp)
{
    size_t most = 0, n;
    int k;

    if (rp == NULL) {
        return 0;
    }
    for (k = 0; k < rp->n_plans; k++) {
        n = (size_t)rp->plan[k].n_picks;
        if (n > most) {
            most = n;
        }
    }
    return (size_t)rp->rows * ((size_t)rp->cols + 1)   /* the map            */
         + 200                                          /* header, done line  */
         + (size_t)rp->n_robots * 120                   /* robot lines        */
         + 60 + most * 40;                              /* events line        */
}

int play_render_view(const Replay *rp, int plan_index, int tick, int top,
                     int n_rows, char *buf, size_t size)
{
    const ReplayPlan *pl;
    char tmp[200];
    size_t len = 0, map_start, need, stride;
    int r, c, i, k, load, n_mine, used, row, col, idx, any, total_value, clipped;

    if (rp == NULL || buf == NULL || size == 0 || plan_index < 0 ||
        plan_index >= rp->n_plans) {
        return -1;
    }
    pl = &rp->plan[plan_index];
    if (tick < 0 || tick > pl->n_ticks || top < 0 || n_rows < 1 ||
        top + n_rows > rp->rows) {
        return -1;
    }
    clipped = n_rows < rp->rows;
    stride = (size_t)rp->cols + 1;
    buf[0] = '\0';

    if (clipped) {
        sprintf(tmp, "tick %d/%d   plan %s   map rows %d-%d of %d\n", tick,
                pl->n_ticks, pl->label, top + 1, top + n_rows, rp->rows);
    } else {
        sprintf(tmp, "tick %d/%d   plan %s\n", tick, pl->n_ticks, pl->label);
    }
    if (put(buf, size, &len, tmp) != 0) {
        return -1;
    }

    /* the map window */
    need = (size_t)n_rows * stride;
    if (len + need + 1 > size) {
        return -1;
    }
    map_start = len;
    for (r = top; r < top + n_rows; r++) {
        for (c = 0; c < rp->cols; c++) {
            buf[len++] = glyph(rp->grid[r * rp->cols + c]);
        }
        buf[len++] = '\n';
    }
    buf[len] = '\0';

    for (i = 0; i < rp->n_tasks; i++) {          /* T waiting, x picked */
        int picked = 0;

        for (k = 0; k < pl->n_picks; k++) {
            if (pl->pick[k].task == i + 1 && pl->pick[k].tick <= tick) {
                picked = 1;
            }
        }
        row = rp->task[i].row;
        col = rp->task[i].col;
        if (row >= top && row < top + n_rows && col >= 0 && col < rp->cols) {
            buf[map_start + (size_t)(row - top) * stride + (size_t)col] =
                picked ? 'x' : 'T';
        }
    }
    for (r = 0; r < rp->n_robots; r++) {         /* robots on top */
        idx = pos_index(pl, r, tick);
        row = pl->row[r][idx];
        col = pl->col[r][idx];
        if (row >= top && row < top + n_rows && col >= 0 && col < rp->cols) {
            buf[map_start + (size_t)(row - top) * stride + (size_t)col] =
                robot_char(r);
        }
    }

    /* one line per robot */
    for (r = 0; r < rp->n_robots; r++) {
        load = 0;
        k = 0;
        n_mine = 0;
        for (i = 0; i < pl->n_picks; i++) {
            if (pl->pick[i].robot == r + 1) {
                n_mine++;
                if (pl->pick[i].tick <= tick) {
                    k++;
                    load += rp->task[pl->pick[i].task - 1].weight;
                }
            }
        }
        idx = pos_index(pl, r, tick);
        used = idx;
        sprintf(tmp, "robot %c: load %d/%d, battery used %d/%d, picked %d of %d",
                robot_char(r), load, rp->robot[r].capacity, used,
                rp->robot[r].battery, k, n_mine);
        if (clipped) {
            sprintf(tmp + strlen(tmp), " at (%d,%d)", pl->row[r][idx],
                    pl->col[r][idx]);
        }
        if (put(buf, size, &len, tmp) != 0 || put(buf, size, &len, "\n") != 0) {
            return -1;
        }
    }

    /* events at exactly this tick */
    if (put(buf, size, &len, "events:") != 0) {
        return -1;
    }
    any = 0;
    for (i = 0; i < pl->n_picks; i++) {
        if (pl->pick[i].tick == tick) {
            sprintf(tmp, " robot %c picked T%d", robot_char(pl->pick[i].robot - 1),
                    pl->pick[i].task);
            if (put(buf, size, &len, tmp) != 0) {
                return -1;
            }
            any = 1;
        }
    }
    if ((!any && put(buf, size, &len, " none") != 0) ||
        put(buf, size, &len, "\n") != 0) {
        return -1;
    }

    if (tick == pl->n_ticks) {
        k = 0;
        total_value = 0;
        for (i = 0; i < pl->n_picks; i++) {
            k++;
            total_value += rp->task[pl->pick[i].task - 1].value;
        }
        sprintf(tmp, "done: %d of %d tasks picked, value %d\n", k, rp->n_tasks,
                total_value);
        if (put(buf, size, &len, tmp) != 0) {
            return -1;
        }
    }
    return (int)len;
}

int play_render(const Replay *rp, int plan_index, int tick,
                char *buf, size_t size)
{
    if (rp == NULL) {
        return -1;
    }
    return play_render_view(rp, plan_index, tick, 0, rp->rows, buf, size);
}

/* ---------- screen, colour and timing (the system-specific code) ---------- */

#ifdef _WIN32
/* Turn on ANSI escape codes in the console. 0x0004 is
 * ENABLE_VIRTUAL_TERMINAL_PROCESSING, defined by hand: old MinGW headers
 * do not have it. Returns 1 if it worked. */
static int enable_ansi(void)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (h == INVALID_HANDLE_VALUE || !GetConsoleMode(h, &mode)) {
        return 0;
    }
    return SetConsoleMode(h, mode | 0x0004) != 0;
}
#endif

/* 1 if the console understands ANSI codes (checked once). */
static int ansi_ok(void)
{
    static int v = -1;

    if (v < 0) {
#ifdef _WIN32
        v = enable_ansi();
#else
        v = 1;
#endif
    }
    return v;
}

static void clear_screen(FILE *out)
{
    if (ansi_ok()) {
        fputs("\033[2J\033[H", out);
    } else {
#ifdef _WIN32
        fflush(out);
        system("cls");
#endif
    }
}

static void pause_ms(int ms)
{
    if (ms <= 0) {
        return;
    }
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    {
        clock_t end = clock() + (clock_t)ms * CLOCKS_PER_SEC / 1000;

        while (clock() < end) {
        }
    }
#endif
}

/* Colour code of a map symbol, or NULL for plain floor. */
static const char *color_of(char ch)
{
    static const char *robot[6] = {
        "\033[1;31m", "\033[1;32m", "\033[1;34m",
        "\033[1;35m", "\033[1;36m", "\033[1;37m"
    };

    if (ch >= '1' && ch <= '9') {
        return robot[(ch - '1') % 6];
    }
    if (ch >= 'a' && ch <= 'w') {
        return robot[(ch - 'a' + 9) % 6];
    }
    switch (ch) {
    case '#': return "\033[90m";
    case '=': return "\033[35m";
    case 'T': return "\033[1;33m";
    case 'x': return "\033[2m";
    case 'C': return "\033[36m";
    case 'D': return "\033[32m";
    default:  return NULL;
    }
}

/* Print a frame, colouring only the n_rows map lines that follow line 1. */
static void print_colored(const char *frame, int n_rows, FILE *out)
{
    const char *p, *code;
    int line = 0;

    for (p = frame; *p != '\0'; p++) {
        if (*p == '\n') {
            line++;
            fputc('\n', out);
            continue;
        }
        code = (line >= 1 && line <= n_rows) ? color_of(*p) : NULL;
        if (code != NULL) {
            fputs(code, out);
            fputc(*p, out);
            fputs("\033[0m", out);
        } else {
            fputc(*p, out);
        }
    }
}

/* ---------- the interactive loop ---------- */

static void show(const Replay *rp, int plan, int tick, int top, int n_rows,
                 char *frame, size_t size, FILE *out, int use_clear)
{
    if (play_render_view(rp, plan, tick, top, n_rows, frame, size) < 0) {
        fputs("(frame could not be drawn)\n", out);
        return;
    }
    if (use_clear) {
        clear_screen(out);
    }
    if (use_clear && ansi_ok()) {
        print_colored(frame, n_rows, out);
    } else {
        fputs(frame, out);
    }
    fputs("legend: 1-9 robot  T task  x picked  . floor  # wall  = shelf  "
          "C charger  D dock\n", out);
    fflush(out);
}

int play_run(const Replay *rp, int plan_index, FILE *in, FILE *out,
             int delay_ms, int use_clear)
{
    char cmd[64], *frame;
    size_t size;
    int plan = plan_index, tick = 0, last, need_draw = 1, quit = 0;
    int view = 0, top = 0, max_top = 0;

    if (rp == NULL || in == NULL || out == NULL || plan_index < 0 ||
        plan_index >= rp->n_plans) {
        return -1;
    }
    if (rp->cols > PLAY_MAX_COLS) {
        fprintf(out, "map too wide for the terminal player (limit %d columns)\n",
                PLAY_MAX_COLS);
        return -3;
    }
    view = rp->rows > PLAY_VIEW_ROWS ? PLAY_VIEW_ROWS : rp->rows;
    max_top = rp->rows - view;
    size = play_frame_size(rp);
    frame = (char *)malloc(size);
    if (frame == NULL) {
        return -1;
    }

    while (!quit) {
        last = rp->plan[plan].n_ticks;
        if (need_draw) {
            show(rp, plan, tick, top, view, frame, size, out, use_clear);
        }
        need_draw = 1;
        fputs(max_top > 0
                  ? "[Enter] step  b back  a auto  +/- speed  p plan  u/d scroll  q quit > "
                  : "[Enter] step  b back  a auto  +/- speed  p plan  q quit > ",
              out);
        fflush(out);
        if (fgets(cmd, (int)sizeof cmd, in) == NULL) {
            break;
        }
        switch (cmd[0]) {
        case 'q':
            quit = 1;
            break;
        case 'b':
            if (tick > 0) {
                tick--;
            }
            break;
        case 'p':
            plan = (plan + 1) % rp->n_plans;
            if (tick > rp->plan[plan].n_ticks) {
                tick = rp->plan[plan].n_ticks;
            }
            break;
        case 'u':
            top = top - SCROLL_STEP < 0 ? 0 : top - SCROLL_STEP;
            break;
        case 'd':
            top = top + SCROLL_STEP > max_top ? max_top : top + SCROLL_STEP;
            break;
        case '+':
            delay_ms /= 2;
            break;
        case '-':
            delay_ms = delay_ms <= 0 ? 50 : (delay_ms >= 2500 ? 5000 : delay_ms * 2);
            break;
        case 'a':
            while (tick < last) {
                tick++;
                show(rp, plan, tick, top, view, frame, size, out, use_clear);
                if (tick < last) {
                    pause_ms(delay_ms);
                }
            }
            need_draw = 0;
            break;
        case '\n':
        case '\r':
        case 'n':
            if (tick < last) {
                tick++;
            }
            break;
        default:
            break;
        }
    }
    free(frame);
    fputs("\n", out);
    return 0;
}
