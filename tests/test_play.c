/*
 * test_play.c - tests for play.c (item 25).
 *
 * One hand-written replay: a 2 x 4 map, one robot starting at (0,0) with
 * capacity 10 and battery 20, one task at (1,2) (weight 4, value 9). The
 * robot walks (0,0) (0,1) (0,2) (1,2), picks the task at tick 3, then steps
 * to (1,3) at tick 4. The expected frames below are worked out by hand:
 *   tick 0: robot on the start cell, task waiting (T);
 *   tick 2: robot at (0,2), above the task;
 *   tick 3: robot stands on the task cell (the robot hides the T), the pick
 *           event appears and load is 4/10, 3 steps used;
 *   tick 4: robot at (1,3), the task cell shows x, and the "done" line.
 * play_run is driven with commands written to a file instead of a keyboard.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "play.h"
#include "replay.h"
#include "tests.h"

#define REPLAY_FILE "test_play_input.replay"
#define CMD_FILE    "test_play_cmds.txt"
#define OUT_FILE    "test_play_out.txt"

static const char *SMALL =
    "REPLAY 1\nGRID 2 4\nR...\n..T.\n"
    "ROBOT 1 10 20 0 0\nTASK 1 1 2 4 9 1\nPLAN A\n"
    "STEP 0 1 0 0\nSTEP 1 1 0 1\nSTEP 2 1 0 2\nSTEP 3 1 1 2\nPICK 3 1 1\n"
    "STEP 4 1 1 3\nEND\n";

static int write_text(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");

    if (f == NULL) {
        return -1;
    }
    fputs(text, f);
    return fclose(f);
}

/* Read a whole small file into buf; "" if missing. */
static void slurp(const char *path, char *buf, size_t size)
{
    FILE *f = fopen(path, "r");
    size_t n = 0;

    buf[0] = '\0';
    if (f == NULL) {
        return;
    }
    n = fread(buf, 1, size - 1, f);
    buf[n] = '\0';
    fclose(f);
}

/* Run play_run on the commands in cmds; the output ends in OUT_FILE. */
static int run_with(const Replay *rp, const char *cmds, int use_clear)
{
    FILE *in, *out;
    int rc;

    if (write_text(CMD_FILE, cmds) != 0) {
        return -99;
    }
    in = fopen(CMD_FILE, "r");
    out = fopen(OUT_FILE, "w");
    if (in == NULL || out == NULL) {
        if (in) fclose(in);
        if (out) fclose(out);
        return -99;
    }
    rc = play_run(rp, 0, in, out, 0, use_clear);
    fclose(in);
    fclose(out);
    return rc;
}

void test_play(void)
{
    Replay rp;
    char err[200], frame[1024], big[8192];
    char tall[4096];
    const char *after;
    int i;

    harness_suite("play");

    check_long("setup: write replay", 0, write_text(REPLAY_FILE, SMALL));
    check_long("setup: read replay", 0, replay_read(REPLAY_FILE, &rp, err, sizeof err));
    check_long("frame size is positive", 1, play_frame_size(&rp) > 0);

    /* frames */
    check_long("tick 0: length is 18 + 5 + 5 + 53 + 13", 94,
               play_render(&rp, 0, 0, frame, sizeof frame));
    play_render(&rp, 0, 0, frame, sizeof frame);
    check_str("tick 0 frame",
              "tick 0/4   plan A\n"
              "1...\n"
              "..T.\n"
              "robot 1: load 0/10, battery used 0/20, picked 0 of 1\n"
              "events: none\n",
              frame);
    play_render(&rp, 0, 2, frame, sizeof frame);
    check_str("tick 2 frame",
              "tick 2/4   plan A\n"
              "..1.\n"
              "..T.\n"
              "robot 1: load 0/10, battery used 2/20, picked 0 of 1\n"
              "events: none\n",
              frame);
    play_render(&rp, 0, 3, frame, sizeof frame);
    check_str("tick 3 frame (pick)",
              "tick 3/4   plan A\n"
              "....\n"
              "..1.\n"
              "robot 1: load 4/10, battery used 3/20, picked 1 of 1\n"
              "events: robot 1 picked T1\n",
              frame);
    play_render(&rp, 0, 4, frame, sizeof frame);
    check_str("tick 4 frame (done)",
              "tick 4/4   plan A\n"
              "....\n"
              "..x1\n"
              "robot 1: load 4/10, battery used 4/20, picked 1 of 1\n"
              "events: none\n"
              "done: 1 of 1 tasks picked, value 9\n",
              frame);

    /* invalid arguments */
    check_long("render: tick -1", -1, play_render(&rp, 0, -1, frame, sizeof frame));
    check_long("render: tick past the end", -1, play_render(&rp, 0, 5, frame, sizeof frame));
    check_long("render: plan 1 does not exist", -1, play_render(&rp, 1, 0, frame, sizeof frame));
    check_long("render: NULL replay", -1, play_render(NULL, 0, 0, frame, sizeof frame));
    check_long("render: NULL buffer", -1, play_render(&rp, 0, 0, NULL, 10));
    check_long("render: buffer too small", -1, play_render(&rp, 0, 0, frame, 20));
    check_long("render: buffer one byte short", -1, play_render(&rp, 0, 0, frame, 94));
    check_long("render: buffer exactly big enough", 94, play_render(&rp, 0, 0, frame, 95));

    /* the interactive loop */
    check_long("run: two steps then quit", 0, run_with(&rp, "\n\nq\n", 0));
    slurp(OUT_FILE, big, sizeof big);
    check_long("run: shows tick 0", 1, strstr(big, "tick 0/4") != NULL);
    check_long("run: shows tick 2", 1, strstr(big, "tick 2/4") != NULL);
    check_long("run: never reaches tick 3", 1, strstr(big, "tick 3/4") == NULL);
    check_long("run: no screen clear in test mode", 1, strstr(big, "\033[2J") == NULL);
    check_long("run: the legend is printed under the frame", 1,
               strstr(big, "legend: 1-9 robot  T task  x picked  . floor  # wall  = shelf") != NULL);
    check_long("run: speed commands are accepted", 0, run_with(&rp, "+\n-\nq\n", 0));

    check_long("run: auto to the end", 0, run_with(&rp, "a\nq\n", 0));
    slurp(OUT_FILE, big, sizeof big);
    check_long("auto: shows the pick tick", 1, strstr(big, "tick 3/4") != NULL);
    check_long("auto: ends with the done line", 1,
               strstr(big, "done: 1 of 1 tasks picked, value 9") != NULL);

    check_long("run: back goes down one tick", 0, run_with(&rp, "\n\nb\nq\n", 0));
    slurp(OUT_FILE, big, sizeof big);
    after = strstr(big, "tick 2/4");
    check_long("back: tick 1 appears after tick 2", 1,
               after != NULL && strstr(after, "tick 1/4") != NULL);

    check_long("run: back at tick 0 stays at 0", 0, run_with(&rp, "b\nq\n", 0));
    check_long("run: end of input ends the loop", 0, run_with(&rp, "\n", 0));
    check_long("run: stepping past the last tick is harmless", 0,
               run_with(&rp, "\n\n\n\n\n\n\nq\n", 0));
    slurp(OUT_FILE, big, sizeof big);
    check_long("past the end: never prints tick 5", 1, strstr(big, "tick 5/4") == NULL);
    check_long("run: clear mode runs", 0, run_with(&rp, "q\n", 1));
    check_long("run: NULL replay", -1, play_run(NULL, 0, stdin, stdout, 0, 0));
    check_long("run: bad plan index", -1, play_run(&rp, 3, stdin, stdout, 0, 0));
    replay_free(&rp);

    /* symbols: robot, wall, shelf, free floor, charger, dock, waiting task */
    check_long("symbols: write replay", 0,
               write_text(REPLAY_FILE,
                          "REPLAY 1\nGRID 1 7\nR#S.CDT\nROBOT 1 5 9 0 0\n"
                          "TASK 1 0 6 1 1 1\nPLAN A\nSTEP 0 1 0 0\nEND\n"));
    check_long("symbols: read replay", 0, replay_read(REPLAY_FILE, &rp, err, sizeof err));
    check_long("symbols: render ok", 1, play_render(&rp, 0, 0, frame, sizeof frame) > 0);
    check_long("symbols: robot 1, wall #, shelf =, floor ., C, D, task T", 1,
               strstr(frame, "\n1#=.CDT\n") != NULL);
    replay_free(&rp);

    /* a map taller than the window: 30 rows, robot at row 0, task at row 29 */
    strcpy(tall, "REPLAY 1\nGRID 30 3\nR..\n");
    for (i = 1; i < 29; i++) {
        strcat(tall, "...\n");
    }
    strcat(tall, "..T\nROBOT 1 5 99 0 0\nTASK 1 29 2 1 1 1\nPLAN A\nSTEP 0 1 0 0\nEND\n");
    check_long("window: write replay", 0, write_text(REPLAY_FILE, tall));
    check_long("window: read replay", 0, replay_read(REPLAY_FILE, &rp, err, sizeof err));
    check_long("window: render rows 11-30", 1,
               play_render_view(&rp, 0, 0, 10, 20, frame, sizeof frame) > 0);
    check_long("window: header names the rows", 1,
               strstr(frame, "tick 0/0   plan A   map rows 11-30 of 30\n") != NULL);
    check_long("window: the task is on the last map line", 1,
               strstr(frame, "\n..T\nrobot 1:") != NULL);
    check_long("window: robot line gives its position (this robot has no picks)", 1,
               strstr(frame, "picked 0 of 0 at (0,0)\n") != NULL);
    check_long("window: top row robot is out of view", 1, strstr(frame, "\n1..\n") == NULL);
    play_render_view(&rp, 0, 0, 0, 20, frame, sizeof frame);
    check_long("window: top rows show the robot", 1, strstr(frame, "\n1..\n") != NULL);
    check_long("window: past the map end", -1,
               play_render_view(&rp, 0, 0, 11, 20, frame, sizeof frame));
    check_long("window: negative top", -1,
               play_render_view(&rp, 0, 0, -1, 20, frame, sizeof frame));
    check_long("window: zero rows", -1,
               play_render_view(&rp, 0, 0, 0, 0, frame, sizeof frame));
    check_long("window: run scrolls down and up", 0, run_with(&rp, "d\nd\nu\nq\n", 0));
    slurp(OUT_FILE, big, sizeof big);
    check_long("window: first view is rows 1-20", 1, strstr(big, "map rows 1-20 of 30") != NULL);
    check_long("window: d shows rows 11-30", 1, strstr(big, "map rows 11-30 of 30") != NULL);
    check_long("window: the scroll hint is shown", 1, strstr(big, "u/d scroll") != NULL);
    check_long("window: u does not pass the top", 0, run_with(&rp, "u\nu\nq\n", 0));
    replay_free(&rp);

    /* a map wider than the console is refused */
    strcpy(tall, "REPLAY 1\nGRID 1 101\nRT");
    for (i = 2; i < 101; i++) {
        strcat(tall, ".");
    }
    strcat(tall, "\nROBOT 1 10 20 0 0\nTASK 1 0 1 1 1 1\nPLAN A\nSTEP 0 1 0 0\nEND\n");
    check_long("wide: write replay", 0, write_text(REPLAY_FILE, tall));
    check_long("wide: read replay", 0, replay_read(REPLAY_FILE, &rp, err, sizeof err));
    check_long("wide: player refuses the map", -3, run_with(&rp, "q\n", 0));
    slurp(OUT_FILE, big, sizeof big);
    check_long("wide: says why", 1, strstr(big, "too wide") != NULL);
    replay_free(&rp);

    remove(REPLAY_FILE);
    remove(CMD_FILE);
    remove(OUT_FILE);
}
