/*
 * test_replay.c - tests for replay.c (item 24). Expected text and numbers
 * are worked out by hand.
 *
 * Corridor "R.T.T.T", order T1 T2 T3, plan label A. The file must read
 *   0 REPLAY 1            7 PLAN A            13 STEP 4 1 0 4
 *   1 GRID 1 7            8 STEP 0 1 0 0      14 PICK 4 1 2
 *   2 R.T.T.T             9 STEP 1 1 0 1      15 STEP 5 1 0 5
 *   3 ROBOT 1 100 100 0 0 10 STEP 2 1 0 2     16 STEP 6 1 0 6
 *   4 TASK 1 0 2 10 5 1   11 PICK 2 1 1       17 PICK 6 1 3
 *   5 TASK 2 0 4 10 5 1   12 STEP 3 1 0 3     18 END
 *   6 TASK 3 0 6 10 5 1
 * (line numbers 0-based here, 19 lines in all). Two plans in one file:
 * 7 header lines + 2 * 11 plan lines + END = 30 lines.
 * Error tests use a 1 x 3 map "R.T" with one robot and one task; the five
 * header lines come first, so the first body line is line 6.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "harness.h"
#include "io.h"
#include "planner.h"
#include "problem.h"
#include "replay.h"
#include "route.h"
#include "tests.h"

#define TMP "test_replay_tmp.txt"

static void line_of(const char *path, int n, char *buf, size_t size)
{
    FILE *f = fopen(path, "r");
    int i = 0;

    strcpy(buf, "(missing)");
    if (f == NULL) {
        return;
    }
    while (fgets(buf, (int)size, f) != NULL) {
        if (i == n) {
            buf[strcspn(buf, "\r\n")] = '\0';
            fclose(f);
            return;
        }
        i++;
    }
    strcpy(buf, "(missing)");
    fclose(f);
}

static int count_lines(const char *path)
{
    FILE *f = fopen(path, "r");
    char buf[200];
    int n = 0;

    if (f == NULL) {
        return -1;
    }
    while (fgets(buf, sizeof buf, f) != NULL) {
        n++;
    }
    fclose(f);
    return n;
}

static void write_text(const char *text)
{
    FILE *f = fopen(TMP, "wb");

    if (f != NULL) {
        fputs(text, f);
        fclose(f);
    }
}

/* Write text to TMP and read it. Returns the status; message in err. */
static int read_text(const char *text, Replay *rp, char *err, size_t size)
{
    write_text(text);
    return replay_read(TMP, rp, err, size);
}

#define HEAD "REPLAY 1\nGRID 1 3\nR.T\nROBOT 1 10 40 0 0\nTASK 1 0 2 3 5 1\n"

/* Body (after HEAD) must fail with status -2 and exactly this message. */
static void expect_bad(const char *name, const char *text, const char *message)
{
    static Replay rp;
    char err[200];

    check_long(name, -2, read_text(text, &rp, err, sizeof err));
    check_str(name, message, err);
    check_long("  failed read leaves grid NULL", 1, rp.grid == NULL);
    check_long("  failed read leaves no plans", 0, rp.n_plans);
    replay_free(&rp);
}

static void round_trip_tests(void)
{
    static Plan plan;
    static RouteSet rs;
    static Replay rp;
    Problem p;
    Graph g;
    char err[200], buf[200];
    const char *labels[2] = {"A", "B"};
    Plan two[2];
    RouteSet rss[2];
    int i;

    if (parse_problem_text("MAP 1 7\nR.T.T.T\nROBOT 100 100\nTASK 10 5 1\n"
                           "TASK 10 5 1\nTASK 10 5 1\n", &p, err, sizeof err) != PARSE_OK ||
        graph_build(&p, &g) != 0) {
        check_long("corridor: loaded", 0, 1);
        return;
    }
    memset(&plan, 0, sizeof plan);
    plan.n_robots = 1;
    plan.n_tasks = 3;
    plan.robot[0].n_stops = 3;
    plan.robot[0].stop[1] = 1;
    plan.robot[0].stop[2] = 2;
    plan.robot[0].cost = 6;
    check_long("corridor: route_build", 0, route_build(&p, &g, &plan, &rs));

    check_long("write: code", 0, replay_write(TMP, &p, 1, labels, &plan, &rs));
    check_long("write: line count", 19, count_lines(TMP));
    line_of(TMP, 0, buf, sizeof buf);  check_str("write line 0", "REPLAY 1", buf);
    line_of(TMP, 1, buf, sizeof buf);  check_str("write line 1", "GRID 1 7", buf);
    line_of(TMP, 2, buf, sizeof buf);  check_str("write line 2", "R.T.T.T", buf);
    line_of(TMP, 3, buf, sizeof buf);  check_str("write line 3", "ROBOT 1 100 100 0 0", buf);
    line_of(TMP, 4, buf, sizeof buf);  check_str("write line 4", "TASK 1 0 2 10 5 1", buf);
    line_of(TMP, 6, buf, sizeof buf);  check_str("write line 6", "TASK 3 0 6 10 5 1", buf);
    line_of(TMP, 7, buf, sizeof buf);  check_str("write line 7", "PLAN A", buf);
    line_of(TMP, 8, buf, sizeof buf);  check_str("write line 8", "STEP 0 1 0 0", buf);
    line_of(TMP, 10, buf, sizeof buf); check_str("write line 10", "STEP 2 1 0 2", buf);
    line_of(TMP, 11, buf, sizeof buf); check_str("write line 11", "PICK 2 1 1", buf);
    line_of(TMP, 14, buf, sizeof buf); check_str("write line 14", "PICK 4 1 2", buf);
    line_of(TMP, 17, buf, sizeof buf); check_str("write line 17", "PICK 6 1 3", buf);
    line_of(TMP, 18, buf, sizeof buf); check_str("write line 18", "END", buf);

    check_long("read: code", 0, replay_read(TMP, &rp, err, sizeof err));
    check_long("read: rows", 1, rp.rows);
    check_long("read: cols", 7, rp.cols);
    check_long("read: grid symbol (0,2)", 'T', rp.grid[2]);
    check_long("read: robots", 1, rp.n_robots);
    check_long("read: robot capacity", 100, rp.robot[0].capacity);
    check_long("read: robot battery", 100, rp.robot[0].battery);
    check_long("read: tasks", 3, rp.n_tasks);
    check_long("read: task 2 col", 4, rp.task[1].col);
    check_long("read: task 3 value", 5, rp.task[2].value);
    check_long("read: plans", 1, rp.n_plans);
    check_str("read: label", "A", rp.plan[0].label);
    check_long("read: n_ticks", 6, rp.plan[0].n_ticks);
    check_long("read: positions of robot 1", 7, rp.plan[0].len[0]);
    for (i = 0; i <= 6; i++) {
        check_long("read: col at tick i is i", i, rp.plan[0].col[0][i]);
    }
    check_long("read: picks", 3, rp.plan[0].n_picks);
    check_long("read: pick 1 tick", 2, rp.plan[0].pick[0].tick);
    check_long("read: pick 1 robot", 1, rp.plan[0].pick[0].robot);
    check_long("read: pick 1 task", 1, rp.plan[0].pick[0].task);
    check_long("read: pick 3 tick", 6, rp.plan[0].pick[2].tick);
    check_long("read: pick 3 task", 3, rp.plan[0].pick[2].task);
    replay_free(&rp);
    check_long("free: grid reset", 1, rp.grid == NULL);
    replay_free(&rp);                      /* second call is harmless */
    replay_free(NULL);

    /* two plans in one file */
    two[0] = plan;
    two[1] = plan;
    memset(rss, 0, sizeof rss);
    check_long("two: route set 1", 0, route_build(&p, &g, &two[0], &rss[0]));
    check_long("two: route set 2", 0, route_build(&p, &g, &two[1], &rss[1]));
    check_long("two: write", 0, replay_write(TMP, &p, 2, labels, two, rss));
    check_long("two: line count", 30, count_lines(TMP));   /* 7 header + 2 * 11 + END = 30 */
    check_long("two: read", 0, replay_read(TMP, &rp, err, sizeof err));
    check_long("two: plans", 2, rp.n_plans);
    check_str("two: label 2", "B", rp.plan[1].label);
    check_long("two: plan 2 picks", 3, rp.plan[1].n_picks);
    replay_free(&rp);

    /* invalid arguments to the writer */
    check_long("write invalid: NULL path", -1, replay_write(NULL, &p, 1, labels, &plan, &rs));
    check_long("write invalid: zero plans", -1, replay_write(TMP, &p, 0, labels, &plan, &rs));
    check_long("write invalid: too many plans", -1,
               replay_write(TMP, &p, REPLAY_MAX_PLANS + 1, labels, &plan, &rs));
    check_long("write invalid: NULL problem", -1, replay_write(TMP, NULL, 1, labels, &plan, &rs));
    check_long("write invalid: NULL route", -1, replay_write(TMP, &p, 1, labels, &plan, NULL));
    {
        const char *spaced[1] = {"A B"};
        const char *empty[1] = {""};
        const char *nul[1] = {NULL};
        check_long("write invalid: label with space", -1,
                   replay_write(TMP, &p, 1, spaced, &plan, &rs));
        check_long("write invalid: empty label", -1,
                   replay_write(TMP, &p, 1, empty, &plan, &rs));
        check_long("write invalid: NULL label", -1,
                   replay_write(TMP, &p, 1, nul, &plan, &rs));
    }
    check_long("write invalid: folder does not exist", -1,
               replay_write("no_such_folder/r.txt", &p, 1, labels, &plan, &rs));
    rs.n_robots = 0;                       /* route set not built for this plan */
    check_long("write invalid: route set does not fit", -1,
               replay_write(TMP, &p, 1, labels, &plan, &rs));
    route_free(&rs);
    route_free(&rss[0]);
    route_free(&rss[1]);
    graph_free(&g);
    problem_free(&p);
}

static void sample1_tests(void)
{
    static Plan plan;
    static RouteSet rs;
    static Replay rp;
    const char *labels[1] = {"A"};
    Problem p;
    Graph g;
    char err[200];

    if (parse_problem_file("data/small/sample1.txt", &p, err, sizeof err) != PARSE_OK ||
        graph_build(&p, &g) != 0) {
        check_long("sample1: loaded", 0, 1);
        return;
    }
    memset(&plan, 0, sizeof plan);
    plan.n_robots = 2;
    plan.n_tasks = 3;
    plan.robot[0].n_stops = 2;
    plan.robot[0].stop[0] = 1;
    plan.robot[0].stop[1] = 0;
    plan.robot[0].cost = 13;
    plan.robot[1].n_stops = 1;
    plan.robot[1].stop[0] = 2;
    plan.robot[1].cost = 2;
    check_long("sample1: route_build", 0, route_build(&p, &g, &plan, &rs));
    check_long("sample1: write", 0, replay_write(TMP, &p, 1, labels, &plan, &rs));
    check_long("sample1: read", 0, replay_read(TMP, &rp, err, sizeof err));
    check_long("sample1: robots", 2, rp.n_robots);
    check_long("sample1: tasks", 3, rp.n_tasks);
    check_long("sample1: n_ticks is robot 1's last tick", 13, rp.plan[0].n_ticks);
    check_long("sample1: robot 1 positions", 14, rp.plan[0].len[0]);
    check_long("sample1: robot 2 stops after tick 2 (3 positions)", 3, rp.plan[0].len[1]);
    check_long("sample1: three PICK records", 3, rp.plan[0].n_picks);
    check_long("sample1: robot 1 start row", 2, rp.plan[0].row[0][0]);
    check_long("sample1: robot 1 start col", 1, rp.plan[0].col[0][0]);
    check_long("sample1: robot 1 end row", 2, rp.plan[0].row[0][13]);
    check_long("sample1: robot 1 end col", 6, rp.plan[0].col[0][13]);
    check_long("sample1: picks are in tick order", 1,
               rp.plan[0].pick[0].tick <= rp.plan[0].pick[1].tick &&
               rp.plan[0].pick[1].tick <= rp.plan[0].pick[2].tick);
    replay_free(&rp);
    route_free(&rs);
    graph_free(&g);
    problem_free(&p);
}

static void reader_error_tests(void)
{
    static Replay rp;
    char err[200];

    check_long("missing file: code", -1, replay_read("no_such_replay_file.txt", &rp, err, sizeof err));
    check_str("missing file: message", "cannot read the replay file", err);
    check_long("empty file: code", -2, read_text("", &rp, err, sizeof err));
    check_str("empty file: message", "line 0: file ended before END", err);
    check_long("NULL err: code", -2, read_text("REPLAY 2\n", &rp, NULL, 0));

    expect_bad("header: wrong version", "REPLAY 2\n", "line 1: first record must be: REPLAY 1");
    expect_bad("header: not REPLAY", "GRID 1 3\n", "line 1: first record must be: REPLAY 1");
    expect_bad("grid: zero rows", "REPLAY 1\nGRID 0 5\n",
               "line 2: second record must be: GRID <rows> <cols> (1..500)");
    expect_bad("grid: 501 columns", "REPLAY 1\nGRID 1 501\n",
               "line 2: second record must be: GRID <rows> <cols> (1..500)");
    expect_bad("grid: row too short", "REPLAY 1\nGRID 1 3\nR.\n",
               "line 3: map row does not have exactly <cols> characters");
    expect_bad("grid: bad symbol", "REPLAY 1\nGRID 1 3\nR.x\n", "line 3: invalid map symbol");
    expect_bad("robot: wrong id", "REPLAY 1\nGRID 1 3\nR.T\nROBOT 2 10 40 0 0\n",
               "line 4: bad ROBOT record (before PLAN, ids 1, 2, ... in order)");
    expect_bad("task: priority 9", "REPLAY 1\nGRID 1 3\nR.T\nROBOT 1 10 40 0 0\nTASK 1 0 2 3 5 9\n",
               "line 5: bad TASK record (before PLAN, ids 1, 2, ... in order)");

    expect_bad("step before plan", HEAD "STEP 0 1 0 0\nEND\n", "line 6: STEP before PLAN");
    expect_bad("end without plan", HEAD "END\n", "line 6: END needs at least one PLAN before it");
    expect_bad("unknown record", HEAD "PLAN A\nHELLO 1\nEND\n", "line 7: unknown record");
    expect_bad("step: missing number", HEAD "PLAN A\nSTEP 0 1 0\nEND\n",
               "line 7: STEP needs: tick robot row col (robot must exist)");
    expect_bad("step: extra text", HEAD "PLAN A\nSTEP 0 1 0 0 9\nEND\n",
               "line 7: STEP needs: tick robot row col (robot must exist)");
    expect_bad("step: robot 2 does not exist", HEAD "PLAN A\nSTEP 0 2 0 0\nEND\n",
               "line 7: STEP needs: tick robot row col (robot must exist)");
    expect_bad("step: tick 0 not at the start", HEAD "PLAN A\nSTEP 0 1 0 1\nEND\n",
               "line 7: tick 0 must be the robot's start cell");
    expect_bad("step: jump of two cells", HEAD "PLAN A\nSTEP 0 1 0 0\nSTEP 1 1 0 2\nEND\n",
               "line 8: STEP moves more than one cell");
    expect_bad("step: skipped tick", HEAD "PLAN A\nSTEP 0 1 0 0\nSTEP 2 1 0 1\nEND\n",
               "line 8: STEP ticks of a robot must be 0, 1, 2, ... in order");
    expect_bad("step: outside the map", HEAD "PLAN A\nSTEP 0 1 1 0\nEND\n",
               "line 7: STEP is not on a walkable cell of the map");
    expect_bad("pick: not on the task cell", HEAD "PLAN A\nSTEP 0 1 0 0\nPICK 0 1 1\nEND\n",
               "line 8: PICK: the robot is not on the task's cell at that tick");
    expect_bad("pick: task does not exist", HEAD "PLAN A\nSTEP 0 1 0 0\nPICK 0 1 2\nEND\n",
               "line 8: PICK needs: tick robot task (both must exist)");
    expect_bad("pick: twice", HEAD "PLAN A\nSTEP 0 1 0 0\nSTEP 1 1 0 1\nSTEP 2 1 0 2\n"
                               "PICK 2 1 1\nPICK 2 1 1\nEND\n",
               "line 11: PICK: task already picked in this plan");
    expect_bad("pick: back in time", HEAD "PLAN A\nSTEP 0 1 0 0\nSTEP 1 1 0 1\nSTEP 2 1 0 2\n"
                                         "PICK 1 1 1\nEND\n",
               "line 10: records go back in time");
    expect_bad("missing END", HEAD "PLAN A\nSTEP 0 1 0 0\n", "line 7: file ended before END");
    expect_bad("text after END", HEAD "PLAN A\nSTEP 0 1 0 0\nEND\nPLAN B\n",
               "line 9: text after END");
    expect_bad("robot without steps",
               "REPLAY 1\nGRID 1 3\nR.T\nROBOT 1 10 40 0 0\nROBOT 2 10 40 0 1\n"
               "TASK 1 0 2 3 5 1\nPLAN A\nSTEP 0 1 0 0\nEND\n",
               "line 9: every robot needs STEP records from tick 0");
    {
        static Replay r2;
        char e2[200];

        /* an obstacle map: STEP onto '#' is rejected */
        check_long("step on obstacle: code", -2,
                   read_text("REPLAY 1\nGRID 1 3\nR#T\nROBOT 1 10 40 0 0\nTASK 1 0 2 3 5 1\n"
                             "PLAN A\nSTEP 0 1 0 0\nSTEP 1 1 0 1\nEND\n", &r2, e2, sizeof e2));
        check_str("step on obstacle: message",
                  "line 8: STEP is not on a walkable cell of the map", e2);
        replay_free(&r2);
    }
}

static void tolerance_tests(void)
{
    static Replay rp;
    char err[200];

    check_long("CRLF line ends accepted", 0,
               read_text("REPLAY 1\r\nGRID 1 3\r\nR.T\r\nROBOT 1 10 40 0 0\r\n"
                         "TASK 1 0 2 3 5 1\r\nPLAN A\r\nSTEP 0 1 0 0\r\nEND\r\n",
                         &rp, err, sizeof err));
    check_long("CRLF: map row read without the CR", 'T', rp.grid[2]);
    replay_free(&rp);
    check_long("comments and blank lines accepted", 0,
               read_text("# a recording\n\nREPLAY 1\nGRID 1 3\nR.T\n\n# robots\n"
                         "ROBOT 1 10 40 0 0\nTASK 1 0 2 3 5 1\n\nPLAN A\nSTEP 0 1 0 0\nEND\n",
                         &rp, err, sizeof err));
    check_long("blank-line file: one plan", 1, rp.n_plans);
    replay_free(&rp);
    check_long("no final newline after END accepted", 0,
               read_text(HEAD "PLAN A\nSTEP 0 1 0 0\nEND", &rp, err, sizeof err));
    replay_free(&rp);
    check_long("a replay with no tasks and a stationary robot", 0,
               read_text("REPLAY 1\nGRID 1 1\nR\nROBOT 1 1 1 0 0\nPLAN A\nSTEP 0 1 0 0\nEND\n",
                         &rp, err, sizeof err));
    check_long("stationary robot: ticks", 0, rp.plan[0].n_ticks);
    replay_free(&rp);
}

void test_replay(void)
{
    harness_suite("replay");
    round_trip_tests();
    sample1_tests();
    reader_error_tests();
    tolerance_tests();
    remove(TMP);
}
