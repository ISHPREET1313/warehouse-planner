/*
 * playrun.c - the live player loop. See playrun.h. Console code only; every
 * decision comes from playview.c and the drawing from playlive.c.
 */
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN      /* skip printing headers: they define MAX_PRIORITY too */
#include <windows.h>
#include <conio.h>
#endif

#include "playlive.h"
#include "playrun.h"
#include "playview.h"

#ifndef _WIN32

int playrun_live(const Replay *rp, int plan_index)
{
    if (rp == NULL || plan_index < 0 || plan_index >= rp->n_plans ||
        rp->n_robots < 1) {
        return -1;
    }
    return PLAYRUN_UNAVAILABLE;
}

#else

/* 1 if both input and output are a real console and the output understands
 * colour codes. 0x0004 is ENABLE_VIRTUAL_TERMINAL_PROCESSING, written out
 * because old MinGW headers do not define it. */
static int console_ready(void)
{
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (in == INVALID_HANDLE_VALUE || out == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (!GetConsoleMode(in, &mode)) {
        return 0;                          /* input is a pipe or a file  */
    }
    if (!GetConsoleMode(out, &mode)) {
        return 0;                          /* output is a pipe or a file */
    }
    return SetConsoleMode(out, mode | 0x0004) != 0;
}

/* The visible window (not the scroll-back buffer). Returns 1 if measured. */
static int measure(int *w, int *h)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(out, &info)) {
        return 0;
    }
    *w = info.srWindow.Right - info.srWindow.Left + 1;
    *h = info.srWindow.Bottom - info.srWindow.Top + 1;
    return *w > 0 && *h > 0;
}

/* Choose the layout: the forced zoom if the user set one and it fits (if it
 * does not, go back to automatic), otherwise the automatic layout. */
static int choose_layout(const Replay *rp, PlayState *st, int uw, int uh,
                         PlayLayout *lo)
{
    int rc = -2;

    if (st->zoom_k > 0) {
        rc = play_layout_k(rp->rows, rp->cols, uw, uh, rp->n_robots,
                           st->zoom_k, lo);
        if (rc != 0) {
            st->zoom_k = 0;
        }
    }
    if (st->zoom_k == 0) {
        rc = play_layout(rp->rows, rp->cols, uw, uh, rp->n_robots, lo);
    }
    return rc;
}

/*
 * Wait for one frame. Returns when a key is pressed (*key = its code, 0 for
 * a function or arrow key), when the window is resized (*key = -1), or, only
 * while playing, when the frame time is over (*key = -1, *timed_out = 1).
 */
static void wait_frame(const PlayState *st, int w, int h, int *key,
                       int *timed_out)
{
    int fps = st->tps / play_ticks_per_frame(st->tps);
    int w2, h2;
    DWORD t0 = GetTickCount();

    if (fps < 1) {
        fps = 1;
    }
    *key = -1;
    *timed_out = 0;
    for (;;) {
        if (_kbhit()) {
            *key = _getch();
            if (*key == 0 || *key == 224) {   /* arrow or function key: two codes */
                (void)_getch();
                *key = 0;
            }
            return;
        }
        if (measure(&w2, &h2) && (w2 != w || h2 != h)) {
            return;
        }
        if (st->playing && GetTickCount() - t0 >= (DWORD)(1000 / fps)) {
            *timed_out = 1;
            return;
        }
        Sleep(5);
    }
}

int playrun_live(const Replay *rp, int plan_index)
{
    PlayState st;
    PlayLayout lo;
    int nt[REPLAY_MAX_PLANS];
    char *buf = NULL, *bigger;
    size_t cap = 0, need;
    int w = 80, h = 24, last_w = -1, last_h = -1, uw, uh, auto_k = 1;
    int i, n, key, timed_out, rc = 0;

    if (rp == NULL || plan_index < 0 || plan_index >= rp->n_plans ||
        rp->n_robots < 1) {
        return -1;
    }
    if (!console_ready()) {
        return PLAYRUN_UNAVAILABLE;
    }
    for (i = 0; i < rp->n_plans; i++) {
        nt[i] = rp->plan[i].n_ticks;
    }
    play_state_init(&st, nt[plan_index]);
    st.plan = plan_index;

    fputs("\033[2J\033[?25l", stdout);          /* clear once, hide the cursor */
    while (!st.quit && rc == 0) {
        if (!measure(&w, &h)) {
            w = 80;
            h = 24;
        }
        if (w != last_w || h != last_h) {
            fputs("\033[2J", stdout);           /* new size: clear once */
            last_w = w;
            last_h = h;
        }
        uw = w > 1 ? w - 1 : 1;                 /* keep the last column and */
        uh = h > 1 ? h - 1 : 1;                 /* line free: no scrolling  */

        if (choose_layout(rp, &st, uw, uh, &lo) == 0) {
            auto_k = lo.k;
            need = playlive_frame_size(rp, &lo, uw, uh);
            if (need > cap) {
                bigger = (char *)realloc(buf, need);
                if (bigger == NULL) {
                    rc = -1;
                    break;
                }
                buf = bigger;
                cap = need;
            }
            n = playlive_frame(rp, st.plan, st.tick, &lo, uw, uh, 1, st.tps,
                               !st.playing && st.tick < nt[st.plan], buf, cap);
            if (n > 0) {
                fwrite(buf, 1, (size_t)n, stdout);
            }
        } else {
            fputs("\033[HThe window is too small for this map. Enlarge it, or press q to quit.\033[K\033[J",
                  stdout);
        }
        fflush(stdout);

        wait_frame(&st, w, h, &key, &timed_out);
        if (key >= 0) {
            play_apply_key(&st, key, rp->n_plans, nt, auto_k);
        } else if (timed_out) {
            play_advance(&st, nt[st.plan]);
        }
    }
    free(buf);
    fputs("\033[0m\033[?25h\n", stdout);        /* normal colours, cursor back */
    fflush(stdout);
    return rc;
}

#endif /* _WIN32 */
