/*
 * test_playlive.c - tests for playlive.c (item 25b, stage 2). Expected
 * frames are worked out by hand, not copied from the program.
 *
 * SMALL: a 2 x 4 map, one robot starting at (0,0), one task at (1,2). The
 * robot walks (0,0) (0,1) (0,2) (1,2), picks the task at tick 3, steps to
 * (1,3) at tick 4. Terminal 79 x 23 usable: play_layout gives k = 1, cells 2
 * characters wide, panel at the right (79 - 8 >= 50). Plain mode draws floor
 * as ". ", a robot as its number and a space, T waiting, x picked, o trail.
 *   tick 0: robot 1 on the start cell, T waiting, no trail;
 *   tick 3: robot on the task cell (the robot hides the T), trail = the
 *           3 cells it left: (0,2) (0,1) (0,0); load 4/10, battery 3/20;
 *   tick 4: robot at (1,3); trail (1,2) (0,2) (0,1), but (1,2) shows x
 *           (a picked task beats a trail); header says DONE.
 *
 * SCALED: a 4 x 4 map "R.## / ..## / ..T. / ...C", one robot walking
 * (0,0) (1,0) (2,0) (2,1) (2,2) and picking the task at (2,2) at tick 4,
 * drawn with k = 2 (each drawn cell is a 2 x 2 block), cells 2 wide, panel
 * below. Blocks: top-left holds the robot; top-right is 4 of 4 blocked = #;
 * bottom-left is floor; bottom-right holds a waiting task and a charger and
 * the task wins. At tick 4 the trail cells (2,1) (2,0) (1,0) fall in the
 * bottom-left block (rank 0 kept) and the top-left block (rank 2).
 *
 * NOBODY: a 1 x 3 map with an unpicked task: no robot takes it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "playlive.h"
#include "playrun.h"
#include "replay.h"
#include "tests.h"

#define FILE_NAME "test_playlive_input.replay"

static const char *SMALL =
    "REPLAY 1\nGRID 2 4\nR...\n..T.\n"
    "ROBOT 1 10 20 0 0\nTASK 1 1 2 4 9 1\nPLAN A\n"
    "STEP 0 1 0 0\nSTEP 1 1 0 1\nSTEP 2 1 0 2\nSTEP 3 1 1 2\nPICK 3 1 1\n"
    "STEP 4 1 1 3\nEND\n";

static const char *SCALED =
    "REPLAY 1\nGRID 4 4\nR.##\n..##\n..T.\n...C\n"
    "ROBOT 1 10 20 0 0\nTASK 1 2 2 4 9 1\nPLAN A\n"
    "STEP 0 1 0 0\nSTEP 1 1 1 0\nSTEP 2 1 2 0\nSTEP 3 1 2 1\nSTEP 4 1 2 2\n"
    "PICK 4 1 1\nEND\n";

static const char *NOBODY =
    "REPLAY 1\nGRID 1 3\nR.T\nROBOT 1 10 20 0 0\nTASK 1 0 2 4 9 1\nPLAN A\n"
    "STEP 0 1 0 0\nEND\n";

#define TERM_W 79
#define TERM_H 23

static int load(const char *text, Replay *rp)
{
    char err[200];
    FILE *f = fopen(FILE_NAME, "w");

    if (f == NULL) {
        return -99;
    }
    fputs(text, f);
    fclose(f);
    return replay_read(FILE_NAME, rp, err, sizeof err);
}

static int count_char(const char *s, char ch)
{
    int n = 0;

    for (; *s != '\0'; s++) {
        n += *s == ch;
    }
    return n;
}

static int count_str(const char *s, const char *needle)
{
    int n = 0;
    size_t len = strlen(needle);

    for (; (s = strstr(s, needle)) != NULL; s += len) {
        n++;
    }
    return n;
}

/* Frame in plain mode with the layout play_layout chooses. */
static int plain(const Replay *rp, int tick, int paused, char *buf, size_t size)
{
    PlayLayout lo;

    if (play_layout(rp->rows, rp->cols, TERM_W, TERM_H, rp->n_robots, &lo) != 0) {
        return -77;
    }
    return playlive_frame(rp, 0, tick, &lo, TERM_W, TERM_H, 0, 4, paused, buf, size);
}

void test_playlive(void)
{
    Replay rp;
    PlayLayout lo, bad;
    char frame[4096];
    int n;

    harness_suite("playlive");

    /* ---- SMALL, plain ---- */
    check_long("small: read replay", 0, load(SMALL, &rp));
    check_long("small: layout is side panel", 0,
               play_layout(rp.rows, rp.cols, TERM_W, TERM_H, rp.n_robots, &lo));
    check_long("small: layout k", 1, lo.k);
    check_long("small: layout cell width", 2, lo.cell_w);
    check_long("small: layout panel at the side", 1, lo.panel_side);
    check_long("small: frame size is enough", 1,
               playlive_frame_size(&rp, &lo, TERM_W, TERM_H) > 400);

    n = plain(&rp, 0, 0, frame, sizeof frame);
    check_long("tick 0: length equals strlen", (long)strlen(frame), n);
    check_str("tick 0 frame",
              "plan A  tick 0/4  full map  4 ticks/s\n"
              "1 . . .   1 load 0/10  battery 0/20  next T1 at (1,2)\n"
              ". . T .   events: none\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);
    plain(&rp, 3, 0, frame, sizeof frame);
    check_str("tick 3 frame (trail of 3, pick event)",
              "plan A  tick 3/4  full map  4 ticks/s\n"
              "o o o .   1 load 4/10  battery 3/20  next done\n"
              ". . 1 .   events: robot 1 picked T1\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);
    plain(&rp, 4, 0, frame, sizeof frame);
    check_str("tick 4 frame (x beats trail, DONE)",
              "plan A  tick 4/4  DONE: 1 of 1 tasks, value 9  full map  4 ticks/s\n"
              ". o o .   1 load 4/10  battery 4/20  next done\n"
              ". . x 1   events: none\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);
    plain(&rp, 0, 1, frame, sizeof frame);
    check_str("paused header",
              "plan A  tick 0/4  PAUSED  full map  4 ticks/s\n"
              "1 . . .   1 load 0/10  battery 0/20  next T1 at (1,2)\n"
              ". . T .   events: none\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);

    /* ---- SMALL, colour ---- */
    playlive_frame(&rp, 0, 3, &lo, TERM_W, TERM_H, 1, 4, 0, frame, sizeof frame);
    check_long("colour: starts with cursor home", 0, strncmp(frame, "\033[H", 3));
    check_long("colour: ends with erase below", 0,
               strcmp(frame + strlen(frame) - 3, "\033[J"));
    check_long("colour: robot 1 uses its colour", 1,
               strstr(frame, "\033[48;5;196m") != NULL);
    check_long("colour: no full screen clear", 0, strstr(frame, "\033[2J") != NULL);
    check_long("colour: each of the 5 lines ends with erase-to-end", 5,
               count_str(frame, "\033[K"));
    check_long("colour: 4 newlines (last line has none)", 4, count_char(frame, '\n'));
    check_long("colour: fits the size estimate", 1,
               strlen(frame) < playlive_frame_size(&rp, &lo, TERM_W, TERM_H));
    plain(&rp, 3, 0, frame, sizeof frame);
    check_long("plain: 5 newlines", 5, count_char(frame, '\n'));
    check_long("plain: no escape codes", 0, strchr(frame, '\033') != NULL);

    /* ---- invalid arguments ---- */
    check_long("invalid: NULL replay", -1,
               playlive_frame(NULL, 0, 0, &lo, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    check_long("invalid: NULL layout", -1,
               playlive_frame(&rp, 0, 0, NULL, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    check_long("invalid: NULL buffer", -1,
               playlive_frame(&rp, 0, 0, &lo, TERM_W, TERM_H, 0, 4, 0, NULL, sizeof frame));
    check_long("invalid: plan 1 does not exist", -1,
               playlive_frame(&rp, 1, 0, &lo, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    check_long("invalid: tick -1", -1,
               playlive_frame(&rp, 0, -1, &lo, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    check_long("invalid: tick past the end", -1,
               playlive_frame(&rp, 0, 5, &lo, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    check_long("invalid: buffer too small", -1,
               playlive_frame(&rp, 0, 0, &lo, TERM_W, TERM_H, 0, 4, 0, frame, 10));
    bad = lo;
    bad.k = 0;
    check_long("invalid: k = 0", -1,
               playlive_frame(&rp, 0, 0, &bad, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    bad = lo;
    bad.cell_w = 3;
    check_long("invalid: cell width 3", -1,
               playlive_frame(&rp, 0, 0, &bad, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    bad = lo;
    bad.block_rows = 3;
    check_long("invalid: layout does not match the map", -1,
               playlive_frame(&rp, 0, 0, &bad, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame));
    /* playrun_live validates its arguments BEFORE it looks at the console, so
     * these calls never draw anything (a real run is checked by eye). */
    check_long("live invalid: NULL replay", -1, playrun_live(NULL, 0));
    check_long("live invalid: plan -1", -1, playrun_live(&rp, -1));
    check_long("live invalid: plan 1 does not exist", -1, playrun_live(&rp, 1));
    replay_free(&rp);

    /* ---- SCALED ---- */
    check_long("scaled: read replay", 0, load(SCALED, &rp));
    lo.k = 2;
    lo.cell_w = 2;
    lo.block_rows = 2;
    lo.block_cols = 2;
    lo.map_w = 4;
    lo.panel_side = 0;
    playlive_frame(&rp, 0, 0, &lo, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame);
    check_str("scaled tick 0 frame (panel below)",
              "plan A  tick 0/4  1 square = 2x2 cells  4 ticks/s\n"
              "1 ##\n"
              ". T \n"
              "1 load 0/10  battery 0/20  next T1 at (2,2)\n"
              "events: none\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);
    playlive_frame(&rp, 0, 4, &lo, TERM_W, TERM_H, 0, 4, 0, frame, sizeof frame);
    check_str("scaled tick 4 frame (trail shares blocks)",
              "plan A  tick 4/4  DONE: 1 of 1 tasks, value 9  1 square = 2x2 cells  4 ticks/s\n"
              "o ##\n"
              "o 1 \n"
              "1 load 4/10  battery 4/20  next done\n"
              "events: robot 1 picked T1\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);
    replay_free(&rp);

    /* ---- NOBODY takes the task ---- */
    check_long("nobody: read replay", 0, load(NOBODY, &rp));
    n = plain(&rp, 0, 0, frame, sizeof frame);
    check_long("nobody: frame built", 1, n > 0);
    check_str("nobody frame (empty map line is padded)",
              "plan A  tick 0/0  DONE: 0 of 1 tasks, value 0  full map  4 ticks/s\n"
              "1 . T   1 load 0/10  battery 0/20  next done\n"
              "        events: none\n"
              PLAYLIVE_LEGEND "\n" PLAYLIVE_KEYS "\n",
              frame);
    play_layout(rp.rows, rp.cols, TERM_W, TERM_H, rp.n_robots, &lo);
    playlive_frame(&rp, 0, 0, &lo, TERM_W, TERM_H, 1, 4, 0, frame, sizeof frame);
    check_long("nobody: unserved task is white on red", 1,
               strstr(frame, "\033[48;5;88m") != NULL);
    replay_free(&rp);
    remove(FILE_NAME);
}
