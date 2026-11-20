/*
 * playlive.h - build ONE screen of the live player (item 25b, stage 2).
 *
 * playlive_frame writes the whole screen for plan_index at tick into a
 * buffer. It does not print, read the console or sleep: the caller writes
 * the buffer with one fwrite. The layout (scale k, cell width, panel at the
 * side or below) comes from play_layout (playview.h), so the drawing always
 * fits the measured terminal.
 *
 * WHAT IS ON THE SCREEN (top to bottom)
 *   header   "plan A  tick 17/48  full map  12 ticks/s" ("1 square = 3x3
 *            cells" instead of "full map" when k > 1). "  PAUSED" follows the
 *            tick when paused and, on the last tick, "  DONE: <k> of <N>
 *            tasks, value <v>"; both come before the scale and the speed, so
 *            a narrow terminal cuts the least important part.
 *   map      block_rows lines of block_cols cells, each cell_w characters:
 *              robot      its number (1-9, then a, b, ...), on its own colour
 *              T          task waiting, in the colour of the robot that will
 *                         pick it; white on red = no robot takes it
 *              x          task already picked, dim
 *              o          trail: the last 3 different cells a robot stood on
 *                         (bold, normal, dim); shown on floor and chargers
 *              C / D      charger / dock
 *              wall, shelf, floor  coloured blank cells
 *            When k > 1 one drawn cell covers k x k map cells and shows the
 *            most important thing inside (robot, waiting task, picked task,
 *            trail, charger or dock, wall, floor).
 *   panel    one line per robot (up to PLAY_PANEL_ROBOTS, then "+k more
 *            robots"): its colour chip, load, battery used, next stop and the
 *            cell of that stop; then "events: ..." for the picks at exactly
 *            this tick. At the right of the map when layout.panel_side is 1,
 *            below it otherwise.
 *   legend, key help
 * Text lines are cut to the terminal width so nothing wraps.
 *
 * color = 1: ANSI 256-colour codes. The frame starts with ESC[H (cursor
 *            home, no screen clear, so no flicker), every line ends with
 *            ESC[K (erase to end of line), the last line has no newline and
 *            ESC[J follows (erase below).
 * color = 0: plain text, no escape codes, "\n" after every line, walls as
 *            "##", shelves as "==", floor as ". " (used by the tests and by
 *            the fallback when the console cannot show colours).
 *
 * Complexity: O(rows * cols + robots * trail + tasks * picks + screen size).
 * It allocates one temporary array of 11 ints per drawn cell.
 */
#ifndef PLAYLIVE_H
#define PLAYLIVE_H

#include <stddef.h>

#include "playview.h"
#include "replay.h"

#define PLAYLIVE_LEGEND \
    "o trail  T task  x picked  # wall  = shelf  C charger  D dock  red = unserved"
#define PLAYLIVE_KEYS \
    "space pause  +/- speed  z/x zoom  0 auto  n/b step  p plan  q quit"

/* A buffer size that is always enough for one frame of this layout. */
size_t playlive_frame_size(const Replay *rp, const PlayLayout *lo, int term_w,
                           int term_h);

/*
 * Write the frame. term_w and term_h are the usable width and height in
 * characters (the caller leaves one column and one line spare); lo must
 * come from play_layout for the same terminal and map. tps is the speed
 * shown in the header. Returns the text length, or -1 for NULL arguments,
 * a plan index or tick out of range (tick 0..n_ticks), an invalid layout
 * (k < 1, cell_w not 1 or 2, sizes not matching the map) or a buffer that
 * is too small.
 */
int playlive_frame(const Replay *rp, int plan_index, int tick,
                   const PlayLayout *lo, int term_w, int term_h, int color,
                   int tps, int paused, char *buf, size_t size);

#endif /* PLAYLIVE_H */
