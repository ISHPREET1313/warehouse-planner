/*
 * playview.c - the pure decisions of the live player. See playview.h.
 * No printing, no console calls, no allocation.
 */
#include <stddef.h>
#include <string.h>

#include "playview.h"

static int ceil_div(int a, int b)
{
    return (a + b - 1) / b;
}

int play_panel_lines(int n_robots)
{
    int shown = n_robots < 0 ? 0 : n_robots;

    if (shown > PLAY_PANEL_ROBOTS) {
        shown = PLAY_PANEL_ROBOTS + 1;          /* robot lines + "+k more" */
    }
    return 1 + shown + 3;                       /* header, robots, events, legend, prompt */
}

/* Lines of robot text and events that stand beside the map in a side panel. */
static int side_text_lines(int n_robots)
{
    int shown = n_robots > PLAY_PANEL_ROBOTS ? PLAY_PANEL_ROBOTS + 1 : n_robots;

    return shown + 1;                           /* robots (+ "+k more"), events */
}

/* Does the map drawn with k and cell width w fit? If so fill *o. */
static int try_layout(int rows, int cols, int k, int w, int term_w, int term_h,
                      int n_robots, PlayLayout *o)
{
    int br = ceil_div(rows, k);
    int bc = ceil_div(cols, k);
    int mw = bc * w;
    int side_lines, side_h;

    if (mw > term_w) {
        return 0;
    }
    side_h = side_text_lines(n_robots);
    side_lines = PLAY_SIDE_LINES + (br > side_h ? br : side_h);
    if (term_w - mw >= PLAY_SIDE_PANEL_W && side_lines <= term_h) {
        o->panel_side = 1;
    } else if (br + play_panel_lines(n_robots) <= term_h) {
        o->panel_side = 0;
    } else {
        return 0;
    }
    o->k = k;
    o->cell_w = w;
    o->block_rows = br;
    o->block_cols = bc;
    o->map_w = mw;
    return 1;
}

int play_layout(int map_rows, int map_cols, int term_w, int term_h,
                int n_robots, PlayLayout *out)
{
    int k, biggest;

    if (out == NULL) {
        return -1;
    }
    out->k = out->cell_w = out->block_rows = out->block_cols = 0;
    out->map_w = out->panel_side = 0;
    if (map_rows < 1 || map_cols < 1 || term_w < 1 || term_h < 1 ||
        n_robots < 1) {
        return -1;
    }
    biggest = map_rows > map_cols ? map_rows : map_cols;
    for (k = 1; k <= biggest; k++) {
        if (try_layout(map_rows, map_cols, k, 2, term_w, term_h, n_robots, out)) {
            return 0;
        }
        if (k == 1 &&
            try_layout(map_rows, map_cols, 1, 1, term_w, term_h, n_robots, out)) {
            return 0;
        }
    }
    out->k = out->cell_w = out->block_rows = out->block_cols = 0;
    out->map_w = out->panel_side = 0;
    return -2;
}

int play_layout_k(int map_rows, int map_cols, int term_w, int term_h,
                  int n_robots, int k, PlayLayout *out)
{
    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof *out);
    if (map_rows < 1 || map_cols < 1 || term_w < 1 || term_h < 1 ||
        n_robots < 1 || k < 1) {
        return -1;
    }
    if (try_layout(map_rows, map_cols, k, 2, term_w, term_h, n_robots, out) ||
        (k == 1 &&
         try_layout(map_rows, map_cols, 1, 1, term_w, term_h, n_robots, out))) {
        return 0;
    }
    memset(out, 0, sizeof *out);
    return -2;
}

int play_block_class(int has_robot, int has_task_wait, int has_task_done,
                     int has_special, int blocked, int cells)
{
    if (cells < 1 || blocked < 0 || blocked > cells) {
        return -1;
    }
    if (has_robot) {
        return BLOCK_ROBOT;
    }
    if (has_task_wait) {
        return BLOCK_TASK_WAIT;
    }
    if (has_task_done) {
        return BLOCK_TASK_DONE;
    }
    if (has_special) {
        return BLOCK_SPECIAL;
    }
    if (blocked * 2 > cells) {
        return BLOCK_WALL;
    }
    return BLOCK_FLOOR;
}

int play_trail(const int *row, const int *col, int len, int tick,
               int max_dots, int *out_row, int *out_col)
{
    int cur, i, j, n = 0, dup;

    if (row == NULL || col == NULL || out_row == NULL || out_col == NULL ||
        len < 1 || tick < 0 || max_dots < 0) {
        return -1;
    }
    cur = tick < len ? tick : len - 1;
    for (i = cur - 1; i >= 0 && n < max_dots; i--) {
        if (row[i] == row[cur] && col[i] == col[cur]) {
            continue;                           /* the robot stands here now */
        }
        dup = 0;
        for (j = 0; j < n; j++) {
            if (out_row[j] == row[i] && out_col[j] == col[i]) {
                dup = 1;
                break;
            }
        }
        if (!dup) {
            out_row[n] = row[i];
            out_col[n] = col[i];
            n++;
        }
    }
    return n;
}

int play_default_tps(int n_ticks)
{
    int t = n_ticks < 0 ? 0 : n_ticks;
    int tps = t / PLAY_TARGET_SECONDS;

    if (tps < PLAY_MIN_TPS) {
        tps = PLAY_MIN_TPS;
    }
    if (tps > PLAY_MAX_TPS) {
        tps = PLAY_MAX_TPS;
    }
    return tps;
}

int play_delay_ms(int tps)
{
    return tps < 1 ? 1000 : 1000 / tps;
}

int play_ticks_per_frame(int tps)
{
    if (tps <= PLAY_MAX_FPS) {
        return 1;
    }
    return ceil_div(tps, PLAY_MAX_FPS);
}

void play_state_init(PlayState *s, int n_ticks)
{
    if (s == NULL) {
        return;
    }
    s->tick = 0;
    s->plan = 0;
    s->tps = play_default_tps(n_ticks);
    s->playing = 1;
    s->zoom_k = 0;
    s->quit = 0;
}

int play_apply_key(PlayState *s, int key, int n_plans,
                   const int *n_ticks_of_plan, int auto_k)
{
    int last;

    if (s == NULL || n_ticks_of_plan == NULL || n_plans < 1 ||
        s->plan < 0 || s->plan >= n_plans || auto_k < 1) {
        return -1;
    }
    last = n_ticks_of_plan[s->plan];
    if (key >= 'A' && key <= 'Z') {
        key = key - 'A' + 'a';
    }
    switch (key) {
    case ' ':
        if (s->tick >= last) {
            s->tick = 0;                    /* finished: play again */
            s->playing = 1;
        } else {
            s->playing = !s->playing;
        }
        break;
    case '+':
        s->tps = s->tps * 2 > PLAY_USER_MAX_TPS ? PLAY_USER_MAX_TPS : s->tps * 2;
        break;
    case '-':
        s->tps = s->tps / 2 < 1 ? 1 : s->tps / 2;
        break;
    case 'z':
        s->zoom_k = (s->zoom_k == 0 ? auto_k : s->zoom_k) - 1;
        if (s->zoom_k < 1) {
            s->zoom_k = 1;
        }
        break;
    case 'x':
        s->zoom_k = (s->zoom_k == 0 ? auto_k : s->zoom_k) + 1;
        if (s->zoom_k > PLAY_MAX_ZOOM) {
            s->zoom_k = PLAY_MAX_ZOOM;
        }
        break;
    case '0':
        s->zoom_k = 0;
        break;
    case 'n':
        if (s->tick < last) {
            s->tick++;
        }
        s->playing = 0;
        break;
    case 'b':
        if (s->tick > 0) {
            s->tick--;
        }
        s->playing = 0;
        break;
    case 'p':
        s->plan = (s->plan + 1) % n_plans;
        s->tick = 0;
        s->playing = 1;
        s->tps = play_default_tps(n_ticks_of_plan[s->plan]);
        break;
    case 'r':
        s->tick = 0;
        s->playing = 1;
        break;
    case 'q':
    case 27:
        s->quit = 1;
        break;
    default:
        break;
    }
    return 0;
}

int play_advance(PlayState *s, int n_ticks)
{
    if (s == NULL || n_ticks < 0) {
        return -1;
    }
    if (!s->playing) {
        return 0;
    }
    s->tick += play_ticks_per_frame(s->tps);
    if (s->tick >= n_ticks) {
        s->tick = n_ticks;
        s->playing = 0;
    }
    return 0;
}
