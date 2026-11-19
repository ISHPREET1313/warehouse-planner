/*
 * test_playview.c - tests for playview.c (item 25b), all expected values
 * worked out by hand from the rules in playview.h.
 *
 * LAYOUT, worked by hand (panel_lines(n) = 1 + min(n,8) [+1 if n > 8] + 3,
 * side panel needs 50 columns and 3 + max(block_rows, robots + events) lines):
 *  A  6x8,   144x38, 2 robots: k=1 cell 2 wide, map 8x2 = 16 wide, side panel.
 *  B  62x80, 144x38, 4 robots: k=1 is 160 wide (no); 1-wide cells need 62 rows
 *     (side 65 > 38, below 70 > 38: no); k=2: 31 lines x 40 cells x 2 = 80
 *     wide, side panel needs 3 + 31 = 34 <= 38: yes.
 *  C  62x80, 80x24, 4 robots: k=3 gives 21 lines (below needs 21 + 8 = 29 > 24),
 *     k=4 gives 16 x 20 cells x 2 = 40 wide, below needs 16 + 8 = 24 <= 24: yes.
 *  D  500x500, 80x24, 4 robots: below needs ceil(500/k) <= 16, so k = 32
 *     (500/32 = 15.6 -> 16), 16 cells x 2 = 32 wide (side would leave 48 < 50).
 *  E  1x1, 80x24, 1 robot: 1 line x 1 cell x 2 = 2 wide, side panel.
 *  F  6x8, 10x5, 4 robots: even one drawn cell needs 1 + 8 = 9 lines: -2.
 *  G  10x30, 100x30, 2 robots: 60 wide, 100 - 60 = 40 < 50 so below, 10 + 6 <= 30.
 *  H  20x50, 60x40, 2 robots: 2-wide cells would be 100 wide, 1-wide are 50.
 *  I  10x10, 40x20, 12 robots: panel_lines(12) = 13; k=1 needs 23 > 20;
 *     k=2: 5 + 13 = 18 <= 20: yes.
 */
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "playview.h"
#include "tests.h"

static void check_layout(const char *name, int rows, int cols, int tw, int th,
                         int n, int rc, int k, int w, int br, int bc, int mw,
                         int side)
{
    PlayLayout lo;
    char label[96];

    memset(&lo, 99, sizeof lo);
    sprintf(label, "layout %s: return code", name);
    check_long(label, rc, play_layout(rows, cols, tw, th, n, &lo));
    sprintf(label, "layout %s: k", name);
    check_long(label, k, lo.k);
    sprintf(label, "layout %s: cell width", name);
    check_long(label, w, lo.cell_w);
    sprintf(label, "layout %s: drawn lines", name);
    check_long(label, br, lo.block_rows);
    sprintf(label, "layout %s: drawn cells per line", name);
    check_long(label, bc, lo.block_cols);
    sprintf(label, "layout %s: map width", name);
    check_long(label, mw, lo.map_w);
    sprintf(label, "layout %s: panel at the side", name);
    check_long(label, side, lo.panel_side);
}

/* Trail of a path as "r,c;r,c;..." ("" if none), or "error". */
static const char *trail_text(const int *row, const int *col, int len, int tick,
                              int max_dots)
{
    static char text[200];
    int out_r[8], out_c[8], n, i;
    char piece[24];

    n = play_trail(row, col, len, tick, max_dots, out_r, out_c);
    if (n < 0) {
        return "error";
    }
    text[0] = '\0';
    for (i = 0; i < n; i++) {
        sprintf(piece, "%s%d,%d", i > 0 ? ";" : "", out_r[i], out_c[i]);
        strcat(text, piece);
    }
    return text;
}

void test_playview(void)
{
    /* path: (0,0) (0,1) (0,1) (0,2) (1,2): the robot waits at tick 2 */
    static const int pr[5] = {0, 0, 0, 0, 1};
    static const int pc[5] = {0, 1, 1, 2, 2};
    /* path A B A: (0,0) (0,1) (0,0) */
    static const int ar[3] = {0, 0, 0};
    static const int ac[3] = {0, 1, 0};
    static const int one_r[1] = {4};
    static const int one_c[1] = {7};
    int dummy_r[2], dummy_c[2];
    PlayLayout lo;

    harness_suite("playview");

    /* panel lines */
    check_long("panel lines: 0 robots", 4, play_panel_lines(0));
    check_long("panel lines: 1 robot", 5, play_panel_lines(1));
    check_long("panel lines: 4 robots", 8, play_panel_lines(4));
    check_long("panel lines: 8 robots", 12, play_panel_lines(8));
    check_long("panel lines: 9 robots (adds a +k more line)", 13, play_panel_lines(9));
    check_long("panel lines: 32 robots", 13, play_panel_lines(32));

    /* layout */
    check_layout("A small map", 6, 8, 144, 38, 2, 0, 1, 2, 6, 8, 16, 1);
    check_layout("B 62x80 in 144x38", 62, 80, 144, 38, 4, 0, 2, 2, 31, 40, 80, 1);
    check_layout("C 62x80 in 80x24", 62, 80, 80, 24, 4, 0, 4, 2, 16, 20, 40, 0);
    check_layout("D 500x500 in 80x24", 500, 500, 80, 24, 4, 0, 32, 2, 16, 16, 32, 0);
    check_layout("E 1x1", 1, 1, 80, 24, 1, 0, 1, 2, 1, 1, 2, 1);
    check_layout("F terminal too small", 6, 8, 10, 5, 4, -2, 0, 0, 0, 0, 0, 0);
    check_layout("G panel below", 10, 30, 100, 30, 2, 0, 1, 2, 10, 30, 60, 0);
    check_layout("H one-wide cells", 20, 50, 60, 40, 2, 0, 1, 1, 20, 50, 50, 0);
    check_layout("I many robots", 10, 10, 40, 20, 12, 0, 2, 2, 5, 5, 10, 0);

    check_long("layout invalid: NULL out", -1, play_layout(6, 8, 80, 24, 2, NULL));
    check_long("layout invalid: 0 rows", -1, play_layout(0, 8, 80, 24, 2, &lo));
    check_long("layout invalid: 0 columns", -1, play_layout(6, 0, 80, 24, 2, &lo));
    check_long("layout invalid: terminal width 0", -1, play_layout(6, 8, 0, 24, 2, &lo));
    check_long("layout invalid: terminal height 0", -1, play_layout(6, 8, 80, 0, 2, &lo));
    check_long("layout invalid: 0 robots", -1, play_layout(6, 8, 80, 24, 0, &lo));

    /* block class */
    check_long("block: robot beats everything", BLOCK_ROBOT, play_block_class(1, 1, 1, 1, 0, 9));
    check_long("block: waiting task beats the rest", BLOCK_TASK_WAIT, play_block_class(0, 1, 1, 1, 0, 9));
    check_long("block: picked task", BLOCK_TASK_DONE, play_block_class(0, 0, 1, 1, 0, 9));
    check_long("block: charger beats walls", BLOCK_SPECIAL, play_block_class(0, 0, 0, 1, 8, 9));
    check_long("block: 5 of 9 blocked is a wall", BLOCK_WALL, play_block_class(0, 0, 0, 0, 5, 9));
    check_long("block: 4 of 9 blocked is floor", BLOCK_FLOOR, play_block_class(0, 0, 0, 0, 4, 9));
    check_long("block: 1 of 1 blocked is a wall", BLOCK_WALL, play_block_class(0, 0, 0, 0, 1, 1));
    check_long("block: 0 of 1 blocked is floor", BLOCK_FLOOR, play_block_class(0, 0, 0, 0, 0, 1));
    check_long("block: 1 of 2 blocked is floor (not more than half)", BLOCK_FLOOR, play_block_class(0, 0, 0, 0, 1, 2));
    check_long("block: 3 of 4 blocked is a wall", BLOCK_WALL, play_block_class(0, 0, 0, 0, 3, 4));
    check_long("block invalid: 0 cells", -1, play_block_class(0, 0, 0, 0, 0, 0));
    check_long("block invalid: more blocked than cells", -1, play_block_class(0, 0, 0, 0, 10, 9));
    check_long("block invalid: negative blocked", -1, play_block_class(0, 0, 0, 0, -1, 9));

    /* trail */
    check_str("trail: tick 4, 3 dots", "0,2;0,1;0,0", trail_text(pr, pc, 5, 4, 3));
    check_str("trail: tick 4, 2 dots", "0,2;0,1", trail_text(pr, pc, 5, 4, 2));
    check_str("trail: tick 3 (the wait adds no dot)", "0,1;0,0", trail_text(pr, pc, 5, 3, 3));
    check_str("trail: tick 2 (waiting, same cell as tick 1)", "0,0", trail_text(pr, pc, 5, 2, 3));
    check_str("trail: tick 0 has none", "", trail_text(pr, pc, 5, 0, 3));
    check_str("trail: tick past the end counts as the last", "0,2;0,1;0,0", trail_text(pr, pc, 5, 99, 3));
    check_str("trail: 0 dots wanted", "", trail_text(pr, pc, 5, 4, 0));
    check_str("trail: back on the start cell shows only the other cell", "0,1", trail_text(ar, ac, 3, 2, 3));
    check_str("trail: robot that never moved", "", trail_text(one_r, one_c, 1, 5, 3));
    check_str("trail invalid: length 0", "error", trail_text(pr, pc, 0, 4, 3));
    check_str("trail invalid: negative tick", "error", trail_text(pr, pc, 5, -1, 3));
    check_str("trail invalid: negative max", "error", trail_text(pr, pc, 5, 4, -1));
    check_long("trail invalid: NULL rows", -1, play_trail(NULL, pc, 5, 4, 2, dummy_r, dummy_c));
    check_long("trail invalid: NULL output", -1, play_trail(pr, pc, 5, 4, 2, NULL, dummy_c));

    /* speed */
    check_long("tps: 0 ticks gives the minimum", 4, play_default_tps(0));
    check_long("tps: 14 ticks gives the minimum", 4, play_default_tps(14));
    check_long("tps: 71 ticks is 3, raised to 4", 4, play_default_tps(71));
    check_long("tps: 72 ticks is exactly 4", 4, play_default_tps(72));
    check_long("tps: 90 ticks", 5, play_default_tps(90));
    check_long("tps: 360 ticks", 20, play_default_tps(360));
    check_long("tps: 1080 ticks is exactly 60", 60, play_default_tps(1080));
    check_long("tps: 1134 ticks is 63, cut to 60", 60, play_default_tps(1134));
    check_long("tps: 5000 ticks gives the maximum", 60, play_default_tps(5000));
    check_long("tps: negative ticks count as 0", 4, play_default_tps(-5));
    check_long("delay: 4 per second", 250, play_delay_ms(4));
    check_long("delay: 20 per second", 50, play_delay_ms(20));
    check_long("delay: 60 per second", 16, play_delay_ms(60));
    check_long("delay: 30 per second", 33, play_delay_ms(30));
    check_long("delay: invalid rate 0 gives 1000", 1000, play_delay_ms(0));
    check_long("frame: 4 per second draws every tick", 1, play_ticks_per_frame(4));
    check_long("frame: 30 per second draws every tick", 1, play_ticks_per_frame(30));
    check_long("frame: 31 per second skips one", 2, play_ticks_per_frame(31));
    check_long("frame: 60 per second", 2, play_ticks_per_frame(60));
    check_long("frame: 61 per second", 3, play_ticks_per_frame(61));
}
