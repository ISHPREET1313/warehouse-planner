/*
 * playrun.h - the live, real-time player loop (item 25b, stage 3).
 *
 * This is the only part of the player that touches the console. It does
 * four things in a loop and nothing else:
 *   1. measure the console window (every frame, so resizing works);
 *   2. ask play_layout / play_layout_k (playview.h) how to draw the map;
 *   3. build the screen with playlive_frame (playlive.h) and write it with
 *      ONE fwrite;
 *   4. wait for one frame time, reading keys without Enter (_kbhit/_getch),
 *      and give every key to play_apply_key / play_advance (playview.h).
 * All decisions live in those tested pure functions. This file cannot be
 * tested by a program: colours, flicker, key reading and the window size are
 * checked by looking at the real console.
 *
 * Keys: space pause/play, + / - speed, z / x zoom, 0 automatic zoom, n / b
 * one tick forward / back, p next plan, r restart, q or Esc quit.
 *
 * Needs a Windows console with colour codes (Windows 10 or later, Windows
 * Terminal). If the output or the input is not a console (a pipe or a file),
 * or the console cannot show colour codes, or the system is not Windows,
 * nothing is drawn and PLAYRUN_UNAVAILABLE is returned: the caller then uses
 * the line-based player play_run (play.h).
 */
#ifndef PLAYRUN_H
#define PLAYRUN_H

#include "replay.h"

#define PLAYRUN_UNAVAILABLE (-4)

/*
 * Play plan plan_index of rp until the user quits. Returns 0 after a normal
 * quit, -1 for NULL arguments, a plan index outside the replay, a replay
 * without robots or out of memory, PLAYRUN_UNAVAILABLE if live mode cannot
 * run (see above; nothing was drawn).
 */
int playrun_live(const Replay *rp, int plan_index);

#endif /* PLAYRUN_H */
