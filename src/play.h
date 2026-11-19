/*
 * play.h - the ASCII replay player (item 25).
 *
 * It only PLAYS a recording (replay.h): it reads positions and picks from
 * a Replay and draws them. It never plans and never changes anything, so
 * deleting this module leaves the planner output identical.
 *
 * SYMBOLS (the player prints this legend under every frame)
 *   1-9, a-w  robot 1-9, then robots 10, 11, ... (lowercase, so they never
 *             clash with the map letters C and D)
 *   T         task still waiting          x  task already picked up
 *   .         free floor                  #  obstacle (wall)
 *   =         shelf (the input's S)       C  charger        D  dock
 * On a real console the symbols are also coloured (robots by number, tasks
 * yellow, picked tasks dim, walls grey, shelves magenta).
 *
 * A frame at tick t (play_render / play_render_view):
 *   line 1      "tick <t>/<last>   plan <label>"  (a window adds
 *               "   map rows a-b of N")
 *   the map     as above; robots are drawn on top of everything else
 *   one line per robot:
 *               "robot <id>: load <w>/<capacity>, battery used <u>/<battery>,
 *                picked <k> of <n>"  (load = weight of the tasks picked up to
 *               tick t; battery used = steps walked so far; n = tasks this
 *               robot picks in this plan). In a window the line also ends
 *               with " at (<row>,<col>)" so you know where to scroll to.
 *   events      "events: robot <id> picked T<n>" for each pick at exactly this
 *               tick, or "events: none"
 *   last tick   "done: <k> of <N> tasks picked, value <v>"
 *
 * LARGE MAPS: a map with more than PLAY_VIEW_ROWS rows is shown through a
 * window of that many rows; u / d scroll it by 10 rows. Columns are never cut,
 * so a map wider than PLAY_MAX_COLS is refused (-3).
 *
 * play_run is the interactive loop, one command per line typed:
 *   (Enter) one tick forward   b  one tick back   a  play to the end
 *   p  switch to the next plan in the file   u / d  scroll the map up / down
 *   + / -  auto mode faster / slower   q  quit
 * "a" waits delay_ms between frames.
 *
 * Complexity: one frame costs O(rows * cols + robots * picks).
 */
#ifndef PLAY_H
#define PLAY_H

#include <stddef.h>
#include <stdio.h>

#include "replay.h"

#define PLAY_VIEW_ROWS 20    /* map rows shown at once (fits a 30-line console) */
#define PLAY_MAX_COLS  100   /* wider maps do not fit a console line       */

/*
 * Write the frame of plan plan_index at tick, showing map rows top ..
 * top + n_rows - 1, into buf (zero terminated). Returns the text length, or -1
 * for NULL arguments, a plan index or tick out of range (tick must be
 * 0..n_ticks of that plan), a window outside the map or a buffer that is too
 * small. A buffer of play_frame_size(rp) bytes is always enough.
 */
int play_render_view(const Replay *rp, int plan_index, int tick, int top,
                     int n_rows, char *buf, size_t size);

/* The whole map: play_render_view(rp, plan_index, tick, 0, rp->rows, ...). */
int play_render(const Replay *rp, int plan_index, int tick,
                char *buf, size_t size);

/* A buffer size that is large enough for any frame of rp. */
size_t play_frame_size(const Replay *rp);

/*
 * Interactive loop reading commands from in and drawing to out. use_clear = 1
 * clears the screen, colours the map and prints before each frame (the real
 * console); 0 gives plain text for tests and pipes. Returns 0 when the user
 * quits or the input ends, -1 for bad arguments or out of memory, -3 if the
 * map is wider than PLAY_MAX_COLS.
 */
int play_run(const Replay *rp, int plan_index, FILE *in, FILE *out,
             int delay_ms, int use_clear);

#endif /* PLAY_H */
