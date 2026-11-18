/*
 * test_parser.c - tests for problem.c and io.c (item 2).
 *
 * Groups: normal, boundary, invalid input, error messages, files,
 * validate_problem, helpers.  Most tests feed a small text to the parser
 * and compare the returned status NAME with the expected one.
 *
 * Run from the project root (make test does this) so the data/ paths work.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "io.h"
#include "problem.h"
#include "tests.h"

/* A small valid problem: 2 rows x 3 cols, one robot R at (0,0), one task T
 * at (0,2), an obstacle at (1,1). MAP2 is just the map part. */
#define MAP2  "MAP 2 3\nR.T\n.#.\n"
#define GOOD  MAP2 "ROBOT 5 10\nTASK 2 7 3\n"

static char g_err[200];

static ParseStatus parse(const char *text, Problem *p)
{
    return parse_problem_text(text, p, g_err, sizeof g_err);
}

/* Compare status names so the table prints PARSE_ERR_MAP, not a number. */
static void check_status(const char *name, ParseStatus want, ParseStatus got)
{
    check_str(name, parse_status_name(want), parse_status_name(got));
}

/* Parse text, compare the status, free whatever was built. */
static void expect_status(const char *name, const char *text, ParseStatus want)
{
    Problem p;
    ParseStatus got = parse(text, &p);

    check_status(name, want, got);
    problem_free(&p);
}

/* 1 if needle occurs inside hay. */
static int has_text(const char *hay, const char *needle)
{
    return strstr(hay, needle) != NULL;
}

/* Text of a rows x cols map of '.' with one R at (0,0) and ROBOT 1 1.
 * The caller frees the result. */
static char *make_map_text(int rows, int cols)
{
    size_t size = (size_t)rows * (size_t)(cols + 1) + 64;
    char *text = (char *)malloc(size);
    char *w;
    int r, c;

    if (text == NULL) {
        return NULL;
    }
    w = text;
    w += sprintf(w, "MAP %d %d\n", rows, cols);
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            *w++ = (r == 0 && c == 0) ? 'R' : '.';
        }
        *w++ = '\n';
    }
    w += sprintf(w, "ROBOT 1 1\n");
    *w = '\0';
    return text;
}

/* Text with n robots in one row: "MAP 1 n", n times R, n ROBOT lines. */
static char *make_robot_row(int n)
{
    char *text = (char *)malloc((size_t)n * 11 + 64);
    char *w;
    int i;

    if (text == NULL) {
        return NULL;
    }
    w = text;
    w += sprintf(w, "MAP 1 %d\n", n);
    for (i = 0; i < n; i++) {
        *w++ = 'R';
    }
    *w++ = '\n';
    for (i = 0; i < n; i++) {
        w += sprintf(w, "ROBOT 1 1\n");
    }
    *w = '\0';
    return text;
}

/* ---------------------------------------------------------------- */

static void group_normal(void)
{
    Problem p;
    ParseStatus st;

    harness_suite("parser: normal input");

    st = parse(GOOD, &p);
    check_status("minimal valid problem", PARSE_OK, st);
    check_long("rows", 2, p.rows);
    check_long("cols", 3, p.cols);
    check_long("n_robots", 1, p.n_robots);
    check_long("n_tasks", 1, p.n_tasks);
    check_long("robot 1 row", 0, p.robots[0].row);
    check_long("robot 1 col", 0, p.robots[0].col);
    check_long("robot 1 capacity", 5, p.robots[0].capacity);
    check_long("robot 1 battery", 10, p.robots[0].battery);
    check_long("task 1 col", 2, p.tasks[0].col);
    check_long("task 1 weight", 2, p.tasks[0].weight);
    check_long("task 1 value", 7, p.tasks[0].value);
    check_long("task 1 priority", 3, p.tasks[0].priority);
    check_long("cell (0,0) is R", 1, problem_cell(&p, 0, 0) == 'R');
    check_long("cell (1,1) is #", 1, problem_cell(&p, 1, 1) == '#');
    problem_free(&p);

    expect_status("CRLF line endings",
                  "MAP 2 3\r\nR.T\r\n.#.\r\nROBOT 5 10\r\nTASK 2 7 3\r\n",
                  PARSE_OK);
    expect_status("comments and blank lines, TASK before ROBOT",
                  "# top\n\nMAP 2 3\nR.T\n.#.\n# mid\n\nTASK 2 7 3\n"
                  "ROBOT 5 10\n   # indented comment\n",
                  PARSE_OK);
    expect_status("no newline at end of file",
                  "MAP 2 3\nR.T\n.#.\nROBOT 5 10\nTASK 2 7 3", PARSE_OK);
    expect_status("tabs and extra spaces",
                  MAP2 "ROBOT \t 5\t10  \nTASK\t2 7 3\n", PARSE_OK);
    expect_status("# at start of a map row is an obstacle",
                  "MAP 2 3\nR##\n#.T\nROBOT 5 10\nTASK 2 7 3\n", PARSE_OK);

    st = parse("MAP 2 3\nR##\n#.T\nROBOT 5 10\nTASK 2 7 3\n", &p);
    check_long("  that obstacle is stored", 1, problem_cell(&p, 1, 0) == '#');
    problem_free(&p);
}

static void group_boundary(void)
{
    char *big;
    char *robots;

    harness_suite("parser: boundary input");

    expect_status("1x1 map with one robot", "MAP 1 1\nR\nROBOT 1 1\n", PARSE_OK);
    expect_status("zero tasks is allowed", "MAP 1 2\nR.\nROBOT 1 1\n", PARSE_OK);
    expect_status("task value 0 is allowed", MAP2 "ROBOT 5 10\nTASK 2 0 3\n",
                  PARSE_OK);
    expect_status("priority 5 is allowed", MAP2 "ROBOT 5 10\nTASK 2 7 5\n",
                  PARSE_OK);
    expect_status("capacity and battery 1000000",
                  MAP2 "ROBOT 1000000 1000000\nTASK 2 7 3\n", PARSE_OK);

    big = make_map_text(500, 500);
    if (big != NULL) {
        expect_status("largest map 500x500", big, PARSE_OK);
        free(big);
    }
    expect_status("MAP 501 1 is too big", "MAP 501 1\nR\nROBOT 1 1\n",
                  PARSE_ERR_MAP);
    expect_status("MAP 1 501 is too big", "MAP 1 501\nR\nROBOT 1 1\n",
                  PARSE_ERR_MAP);
    expect_status("MAP 0 5 is too small", "MAP 0 5\nROBOT 1 1\n", PARSE_ERR_MAP);
    expect_status("MAP -1 3 is rejected", "MAP -1 3\nROBOT 1 1\n", PARSE_ERR_MAP);

    robots = make_robot_row(32);
    if (robots != NULL) {
        expect_status("32 robots (the maximum)", robots, PARSE_OK);
        free(robots);
    }
    robots = make_robot_row(33);
    if (robots != NULL) {
        expect_status("33 robots is too many", robots, PARSE_ERR_COUNT);
        free(robots);
    }

    expect_status("capacity 1000001 too big", MAP2 "ROBOT 1000001 10\nTASK 2 7 3\n",
                  PARSE_ERR_VALUE);
    expect_status("number far too big (no overflow)",
                  MAP2 "ROBOT 99999999999 10\nTASK 2 7 3\n", PARSE_ERR_VALUE);
    expect_status("capacity 0", MAP2 "ROBOT 0 10\nTASK 2 7 3\n", PARSE_ERR_VALUE);
    expect_status("battery 0", MAP2 "ROBOT 5 0\nTASK 2 7 3\n", PARSE_ERR_VALUE);
    expect_status("negative capacity", MAP2 "ROBOT -5 10\nTASK 2 7 3\n",
                  PARSE_ERR_VALUE);
    expect_status("task weight 0", MAP2 "ROBOT 5 10\nTASK 0 7 3\n",
                  PARSE_ERR_VALUE);
    expect_status("task value -1", MAP2 "ROBOT 5 10\nTASK 2 -1 3\n",
                  PARSE_ERR_VALUE);
    expect_status("priority 0", MAP2 "ROBOT 5 10\nTASK 2 7 0\n",
                  PARSE_ERR_VALUE);
    expect_status("priority 6", MAP2 "ROBOT 5 10\nTASK 2 7 6\n",
                  PARSE_ERR_VALUE);
}

static void group_invalid(void)
{
    char *long_line;

    harness_suite("parser: invalid input");

    /* header problems */
    expect_status("empty text", "", PARSE_ERR_SYNTAX);
    expect_status("only comments", "# a\n# b\n\n", PARSE_ERR_SYNTAX);
    expect_status("NULL text", NULL, PARSE_ERR_SYNTAX);
    expect_status("map rows before any MAP header", "R.T\n.#.\n",
                  PARSE_ERR_SYNTAX);
    expect_status("MAP with one number", "MAP 2\nR.\n", PARSE_ERR_SYNTAX);
    expect_status("MAP with a letter", "MAP 2 x\nR.\n", PARSE_ERR_SYNTAX);
    expect_status("MAP with three numbers", "MAP 2 3 4\nR.T\n", PARSE_ERR_SYNTAX);
    expect_status("MAP 2.5 3 (not whole)", "MAP 2.5 3\nR.T\n", PARSE_ERR_SYNTAX);
    expect_status("MAPS is not MAP", "MAPS 2 3\nR.T\n.#.\n", PARSE_ERR_SYNTAX);

    long_line = (char *)malloc(2100);
    if (long_line != NULL) {
        memset(long_line, 'A', 2000);
        long_line[0] = '#';
        long_line[2000] = '\n';
        long_line[2001] = '\0';
        expect_status("line of 2000 characters", long_line, PARSE_ERR_SYNTAX);
        free(long_line);
    }

    /* map problems */
    expect_status("text ends inside the map", "MAP 3 3\nR.T\n.#.\n",
                  PARSE_ERR_MAP);
    expect_status("map row too short", "MAP 2 3\nR.\n.#.\nROBOT 5 10\n",
                  PARSE_ERR_MAP);
    expect_status("map row too long", "MAP 2 3\nR.T.\n.#.\nROBOT 5 10\n",
                  PARSE_ERR_MAP);
    expect_status("unknown symbol X", "MAP 2 3\nR.T\n.X.\nROBOT 5 10\n",
                  PARSE_ERR_MAP);
    expect_status("lower-case r is not a symbol", "MAP 2 3\nr.T\n.#.\n",
                  PARSE_ERR_MAP);
    expect_status("blank line inside the map", "MAP 2 3\nR.T\n\n.#.\n",
                  PARSE_ERR_MAP);

    /* ROBOT / TASK line syntax */
    expect_status("ROBOT with one number", MAP2 "ROBOT 5\nTASK 2 7 3\n",
                  PARSE_ERR_SYNTAX);
    expect_status("ROBOT with three numbers", MAP2 "ROBOT 5 10 3\nTASK 2 7 3\n",
                  PARSE_ERR_SYNTAX);
    expect_status("ROBOT 5x 10 (glued letter)", MAP2 "ROBOT 5x 10\nTASK 2 7 3\n",
                  PARSE_ERR_SYNTAX);
    expect_status("TASK with two numbers", MAP2 "ROBOT 5 10\nTASK 2 7\n",
                  PARSE_ERR_SYNTAX);
    expect_status("unknown keyword WALL", MAP2 "ROBOT 5 10\nWALL 1\n",
                  PARSE_ERR_SYNTAX);
    expect_status("lower-case robot keyword", MAP2 "robot 5 10\nTASK 2 7 3\n",
                  PARSE_ERR_SYNTAX);
    expect_status("ROBOTS is not ROBOT", MAP2 "ROBOTS 5 10\nTASK 2 7 3\n",
                  PARSE_ERR_SYNTAX);
    expect_status("comment after numbers on a line",
                  MAP2 "ROBOT 5 10 # fast\nTASK 2 7 3\n", PARSE_ERR_SYNTAX);

    /* counts */
    expect_status("fewer ROBOT lines than R", MAP2 "TASK 2 7 3\n",
                  PARSE_ERR_COUNT);
    expect_status("more ROBOT lines than R",
                  MAP2 "ROBOT 5 10\nROBOT 5 10\nTASK 2 7 3\n", PARSE_ERR_COUNT);
    expect_status("fewer TASK lines than T", MAP2 "ROBOT 5 10\n",
                  PARSE_ERR_COUNT);
    expect_status("more TASK lines than T",
                  MAP2 "ROBOT 5 10\nTASK 2 7 3\nTASK 1 1 1\n", PARSE_ERR_COUNT);
    expect_status("no R on the map", "MAP 1 2\n..\n", PARSE_ERR_COUNT);
}

static void group_messages(void)
{
    Problem p;
    char small[16];
    ParseStatus st;

    harness_suite("parser: error messages and cleanup");

    st = parse("MAP 2 3\nR.T\n.X.\nROBOT 5 10\nTASK 2 7 3\n", &p);
    check_status("bad symbol status", PARSE_ERR_MAP, st);
    check_long("message names line 3", 1, has_text(g_err, "line 3:"));
    check_long("failed parse: cells is NULL", 1, p.cells == NULL);
    check_long("failed parse: rows is 0", 0, p.rows);
    check_long("failed parse: n_robots is 0", 0, p.n_robots);
    check_long("failed parse: n_tasks is 0", 0, p.n_tasks);
    problem_free(&p);

    st = parse(MAP2 "ROBOT 5\nTASK 2 7 3\n", &p);
    check_status("bad ROBOT line status", PARSE_ERR_SYNTAX, st);
    check_long("message names line 4", 1, has_text(g_err, "line 4:"));
    problem_free(&p);

    st = parse(MAP2 "TASK 2 7 3\n", &p);
    check_status("count error status", PARSE_ERR_COUNT, st);
    check_long("count message says fewer ROBOT", 1,
               has_text(g_err, "fewer ROBOT"));
    problem_free(&p);

    check_long("after a failure, a good parse clears err", 1,
               parse(GOOD, &p) == PARSE_OK && g_err[0] == '\0');
    problem_free(&p);

    /* A 7-character buffer must be respected: no overflow. */
    memset(small, 'Z', sizeof small);
    st = parse_problem_text("MAP 2 3\nR.T\n.X.\n", &p, small, 8);
    check_status("tiny error buffer: status", PARSE_ERR_MAP, st);
    check_long("tiny error buffer: length <= 7", 1, strlen(small) <= 7);
    check_long("tiny error buffer: no overflow", 'Z', small[8]);
    check_long("tiny error buffer: no overflow 2", 'Z', small[15]);
    problem_free(&p);

    st = parse_problem_text("MAP 2 3\nR.T\n.X.\n", &p, NULL, 0);
    check_status("err == NULL is allowed", PARSE_ERR_MAP, st);
    problem_free(&p);

    problem_free(&p);
    check_long("problem_free twice is safe", 1, p.cells == NULL);
}

static void group_files(void)
{
    Problem p;
    ParseStatus st;

    harness_suite("parser: files");

    st = parse_problem_file("data/small/sample1.txt", &p, g_err, sizeof g_err);
    check_status("data/small/sample1.txt", PARSE_OK, st);
    check_long("sample1 rows", 6, p.rows);
    check_long("sample1 cols", 8, p.cols);
    check_long("sample1 robots", 2, p.n_robots);
    check_long("sample1 tasks", 3, p.n_tasks);
    check_long("sample1 robot 1 row", 2, p.robots[0].row);
    check_long("sample1 robot 1 col", 1, p.robots[0].col);
    check_long("sample1 robot 1 capacity", 10, p.robots[0].capacity);
    check_long("sample1 robot 2 row", 4, p.robots[1].row);
    check_long("sample1 robot 2 col", 5, p.robots[1].col);
    check_long("sample1 robot 2 battery", 30, p.robots[1].battery);
    check_long("sample1 task 1 col", 6, p.tasks[0].col);
    check_long("sample1 task 1 value", 20, p.tasks[0].value);
    check_long("sample1 task 2 row", 4, p.tasks[1].row);
    check_long("sample1 task 2 priority", 5, p.tasks[1].priority);
    check_long("sample1 task 3 row", 5, p.tasks[2].row);
    check_long("sample1 task 3 weight", 5, p.tasks[2].weight);
    check_long("sample1 cell (0,0) is D", 1, problem_cell(&p, 0, 0) == 'D');
    check_long("sample1 cell (0,7) is C", 1, problem_cell(&p, 0, 7) == 'C');
    problem_free(&p);

    st = parse_problem_file("data/small/does_not_exist.txt", &p, g_err,
                            sizeof g_err);
    check_status("missing file", PARSE_ERR_FILE, st);
    problem_free(&p);

    st = parse_problem_file(NULL, &p, g_err, sizeof g_err);
    check_status("NULL path", PARSE_ERR_FILE, st);
    problem_free(&p);

    st = parse_problem_file("data/adversarial/bad_count.txt", &p, g_err,
                            sizeof g_err);
    check_status("data/adversarial/bad_count.txt", PARSE_ERR_COUNT, st);
    problem_free(&p);
}

static void group_validate(void)
{
    Problem p;

    harness_suite("validate_problem and helpers");

    parse(GOOD, &p);
    check_status("validate a good problem", PARSE_OK,
                 validate_problem(&p, g_err, sizeof g_err));

    p.robots[0].row = 1;                       /* cell (1,0) is '.', not R */
    check_status("robot not on an R cell", PARSE_ERR_MAP,
                 validate_problem(&p, g_err, sizeof g_err));
    p.robots[0].row = 0;

    p.robots[0].capacity = 0;
    check_status("robot capacity 0", PARSE_ERR_VALUE,
                 validate_problem(&p, g_err, sizeof g_err));
    check_long("message starts with 'robot 1:'", 1, has_text(g_err, "robot 1:"));
    p.robots[0].capacity = 5;

    p.tasks[0].priority = 9;
    check_status("task priority 9", PARSE_ERR_VALUE,
                 validate_problem(&p, g_err, sizeof g_err));
    p.tasks[0].priority = 3;

    p.n_robots = 0;
    check_status("zero robots", PARSE_ERR_COUNT,
                 validate_problem(&p, g_err, sizeof g_err));
    p.n_robots = 1;
    check_status("restored problem is valid again", PARSE_OK,
                 validate_problem(&p, g_err, sizeof g_err));

    check_long("cell outside grid is 0", 0, problem_cell(&p, 5, 5));
    check_long("negative index is 0", 0, problem_cell(&p, -1, 0));
    problem_free(&p);

    check_status("validate NULL problem", PARSE_ERR_MAP,
                 validate_problem(NULL, g_err, sizeof g_err));
    check_long("problem_cell on NULL", 0, problem_cell(NULL, 0, 0));

    check_long("problem_is_symbol('.')", 1, problem_is_symbol('.'));
    check_long("problem_is_symbol('T')", 1, problem_is_symbol('T'));
    check_long("problem_is_symbol('x')", 0, problem_is_symbol('x'));
    check_long("problem_is_symbol(' ')", 0, problem_is_symbol(' '));
    check_long("problem_is_symbol('\\0')", 0, problem_is_symbol('\0'));
    check_str("parse_status_name(PARSE_ERR_VALUE)", "PARSE_ERR_VALUE",
              parse_status_name(PARSE_ERR_VALUE));
}

void test_parser(void)
{
    group_normal();
    group_boundary();
    group_invalid();
    group_messages();
    group_files();
    group_validate();
}
