/*
 * playview.h - the PURE decisions of the live player (item 25b).
 *
 * Nothing here prints, reads the console or allocates memory, so every
 * function can be tested with hand-worked tables. The live player (play.c)
 * only asks these functions what to draw.
 *
 * 1. LAYOUT  play_layout(map_rows, map_cols, term_w, term_h, n_robots, out)
 *    Chooses how the map is drawn so that it ALWAYS fits the measured
 *    terminal, whatever its size. Candidates are tried best first:
 *       k = 1, cell 2 characters wide   exact map, cells look square
 *       k = 1, cell 1 character wide    exact map, narrower
 *       k = 2, 3, 4, ... cell 2 wide    every k x k block of map cells is
 *                                       drawn as ONE cell (scaled mode)
 *    For one candidate the map is  block_rows = ceil(rows / k)  lines of
 *    block_cols = ceil(cols / k) cells,  map_w = block_cols * cell_w
 *    characters wide. The text panel (header, one line per robot, events,
 *    legend, prompt) goes
 *       to the RIGHT of the map  if term_w - map_w >= PLAY_SIDE_PANEL_W; the
 *                                the screen then needs PLAY_SIDE_LINES +
 *                                max(map lines, robot lines + 1) lines;
 *       BELOW the map            otherwise; the map may use
 *                                term_h - play_panel_lines(n_robots) lines.
 *    The first candidate whose map fits (side first, then below) wins.
 *    A terminal that is too small even for k = max(rows, cols) is reported
 *    (-2); the caller then prints a one-line summary and keeps running.
 *
 * 2. BLOCK  play_block_class(...)  what one drawn cell shows when it covers
 *    several map cells. Priority: robot > waiting task > picked task >
 *    charger/dock > wall (more than half of the block's cells blocked) >
 *    floor.
 *
 * 3. TRAIL  play_trail(...)  the last few DIFFERENT cells a robot stood on
 *    (most recent first), never the cell it stands on now, never a cell
 *    twice. A robot that waits adds no dot.
 *
 * 4. SPEED  play_default_tps(n_ticks)  ticks per second so a whole run takes
 *    about PLAY_TARGET_SECONDS, limited to PLAY_MIN_TPS .. PLAY_MAX_TPS.
 *    play_ticks_per_frame(tps) = how many ticks one drawn frame advances so
 *    that at most PLAY_MAX_FPS frames per second are drawn.
 *
 * Complexity: play_layout O(max(rows, cols)); the others O(1) except
 * play_trail, O(tick) in the worst case.
 */
#ifndef PLAYVIEW_H
#define PLAYVIEW_H

#define PLAY_SIDE_PANEL_W   50   /* width the side panel needs              */
#define PLAY_SIDE_LINES      3   /* lines used by header, legend, prompt    */
#define PLAY_PANEL_ROBOTS    8   /* robot lines shown (rest: "+k more")     */
#define PLAY_TARGET_SECONDS 18
#define PLAY_MIN_TPS         4
#define PLAY_MAX_TPS        60
#define PLAY_MAX_FPS        30

typedef struct {
    int k;             /* map cells per drawn cell, in each direction (>= 1) */
    int cell_w;        /* characters per drawn cell: 1 or 2                  */
    int block_rows;    /* drawn lines of the map = ceil(rows / k)            */
    int block_cols;    /* drawn cells per line   = ceil(cols / k)            */
    int map_w;         /* characters per map line = block_cols * cell_w      */
    int panel_side;    /* 1 = panel right of the map, 0 = below the map      */
} PlayLayout;

/* Lines the panel needs below the map: header, min(n_robots, 8) robot lines,
 * a "+k more" line when there are more than 8, events, legend, prompt. */
int play_panel_lines(int n_robots);

/*
 * Choose the layout. Returns 0 and fills *out, -1 for NULL out or any
 * argument below 1, -2 if nothing fits (out is then zeroed).
 */
int play_layout(int map_rows, int map_cols, int term_w, int term_h,
                int n_robots, PlayLayout *out);

/* What a drawn cell shows. */
enum {
    BLOCK_FLOOR = 0,
    BLOCK_WALL,
    BLOCK_SPECIAL,       /* charger or dock          */
    BLOCK_TASK_DONE,
    BLOCK_TASK_WAIT,
    BLOCK_ROBOT
};

/*
 * has_* are 0 or 1 (does the block contain a robot, a waiting task, a picked
 * task, a charger or dock); blocked = number of obstacle or shelf cells in
 * the block, cells = number of map cells in the block (edge blocks can be
 * smaller than k * k). Returns one of the BLOCK_* values, or -1 if cells < 1
 * or blocked is outside 0..cells.
 */
int play_block_class(int has_robot, int has_task_wait, int has_task_done,
                     int has_special, int blocked, int cells);

/*
 * Fill out_row / out_col with up to max_dots trail cells of one robot at
 * tick: walking back from tick (clamped to len - 1) through its stored
 * positions row[0..len-1], col[0..len-1], collect each cell that is neither
 * the current cell nor already collected. Most recent first. Returns the
 * number of dots (0..max_dots), or -1 for NULL arrays, len < 1, tick < 0 or
 * max_dots < 0.
 */
int play_trail(const int *row, const int *col, int len, int tick,
               int max_dots, int *out_row, int *out_col);

/* Ticks per second for a run of n_ticks ticks (n_ticks < 0 counts as 0). */
int play_default_tps(int n_ticks);

/* Delay between frames for a rate: 1000 / tps milliseconds (tps >= 1, else 1). */
int play_delay_ms(int tps);

/* Ticks advanced per drawn frame: 1 up to PLAY_MAX_FPS, else ceil(tps / 30). */
int play_ticks_per_frame(int tps);

#endif /* PLAYVIEW_H */
