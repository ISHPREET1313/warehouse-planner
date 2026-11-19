/*
 * io.c - parser and validator for the plain-text input file. See io.h for
 * the file format and the list of checks.
 *
 * Design: the text is read line by line. Three steps, each its own function:
 *   parse_header   find "MAP <rows> <cols>" and allocate the grid
 *   parse_map      read exactly <rows> rows, check symbols, note R and T cells
 *   parse_records  read ROBOT and TASK lines and fill in the numbers
 * Then validate_problem re-checks the finished Problem as a whole.
 *
 * No sscanf and no snprintf: numbers are read by a small hand-written
 * function so every rule (no sign surprises, no trailing junk) is visible.
 */
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE        1024              /* longest line we accept          */
#define MAX_FILE_BYTES  (4 * 1024 * 1024) /* biggest input file we accept    */

/* ------------------------------------------------------------------ */
/* Error messages                                                      */
/* ------------------------------------------------------------------ */

/* Append text to dst without ever writing past size-1 characters. */
static void put_text(char *dst, size_t size, size_t *n, const char *s)
{
    while (*s != '\0' && *n + 1 < size) {
        dst[*n] = *s;
        (*n)++;
        s++;
    }
    dst[*n] = '\0';
}

/* Append a (small) integer written in decimal. */
static void put_number(char *dst, size_t size, size_t *n, int value)
{
    char digits[12];
    int  len = 0;
    char one[2];

    if (value < 0) {
        put_text(dst, size, n, "-");
        value = -value;
    }
    if (value == 0) {
        digits[len++] = '0';
    }
    while (value > 0 && len < 11) {
        digits[len++] = (char)('0' + value % 10);
        value /= 10;
    }
    one[1] = '\0';
    while (len > 0) {
        len--;
        one[0] = digits[len];
        put_text(dst, size, n, one);
    }
}

/* Message "line 7: text". line <= 0 means there is no line number to show. */
static void set_error(char *err, size_t size, int line, const char *msg)
{
    size_t n = 0;

    if (err == NULL || size == 0) {
        return;
    }
    err[0] = '\0';
    if (line > 0) {
        put_text(err, size, &n, "line ");
        put_number(err, size, &n, line);
        put_text(err, size, &n, ": ");
    }
    put_text(err, size, &n, msg);
}

/* Message "robot 2: text" (index is shown 1-based). */
static void set_item_error(char *err, size_t size, const char *what,
                           int index, const char *msg)
{
    size_t n = 0;

    if (err == NULL || size == 0) {
        return;
    }
    err[0] = '\0';
    put_text(err, size, &n, what);
    put_text(err, size, &n, " ");
    put_number(err, size, &n, index + 1);
    put_text(err, size, &n, ": ");
    put_text(err, size, &n, msg);
}

/* Fill the message and hand the status back, so callers can write
 * "return fail(...)" in one line. */
static ParseStatus fail(char *err, size_t size, ParseStatus status,
                        int line, const char *msg)
{
    set_error(err, size, line, msg);
    return status;
}

/* ------------------------------------------------------------------ */
/* Reading lines and numbers                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *pos;      /* next unread character of the text          */
    int         line_no;  /* number of the line most recently read (1..) */
} Reader;

/*
 * Copy the next line into buf without its \n or \r\n.
 * Returns 1 = got a line, 0 = end of text, -1 = line longer than size-1.
 */
static int read_line(Reader *rd, char *buf, size_t size)
{
    size_t n = 0;

    if (*rd->pos == '\0') {
        return 0;
    }
    rd->line_no++;
    while (*rd->pos != '\0' && *rd->pos != '\n') {
        if (n + 1 >= size) {
            return -1;
        }
        buf[n] = *rd->pos;
        n++;
        rd->pos++;
    }
    if (*rd->pos == '\n') {
        rd->pos++;
    }
    if (n > 0 && buf[n - 1] == '\r') {
        n--;                      /* Windows line ending */
    }
    buf[n] = '\0';
    return 1;
}

static const char *skip_blanks(const char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/* Blank line or a line whose first non-blank character is '#'. */
static int is_blank_or_comment(const char *line)
{
    const char *s = skip_blanks(line);
    return *s == '\0' || *s == '#';
}

/* 1 if nothing but spaces/tabs is left in s. */
static int only_blanks_left(const char *s)
{
    return *skip_blanks(s) == '\0';
}

/*
 * Does the line start with the word kw followed by a space, tab or the end?
 * (so "ROBOT 5 9" matches "ROBOT" but "ROBOTS 5 9" does not).
 * On a match, *rest points just after the word.
 */
static int keyword_is(const char *line, const char *kw, const char **rest)
{
    const char *s = skip_blanks(line);
    size_t len = strlen(kw);

    if (strncmp(s, kw, len) != 0) {
        return 0;
    }
    if (s[len] != '\0' && s[len] != ' ' && s[len] != '\t') {
        return 0;
    }
    *rest = s + len;
    return 1;
}

/*
 * Read one whole number (optional leading '-') after skipping blanks.
 * Returns 1 and moves *s past it on success; 0 if there is no number or
 * it is glued to other characters (like "12abc").
 * A huge number stops growing at about 10^9, so it can never overflow an
 * int; it is still far above every allowed maximum, so the range check
 * rejects it later.
 */
static int read_int(const char **s, int *out)
{
    const char *p = skip_blanks(*s);
    int negative = 0;
    int value = 0;

    if (*p == '-') {
        negative = 1;
        p++;
    }
    if (*p < '0' || *p > '9') {
        return 0;
    }
    while (*p >= '0' && *p <= '9') {
        if (value < 100000000) {
            value = value * 10 + (*p - '0');
        }
        p++;
    }
    if (*p != '\0' && *p != ' ' && *p != '\t') {
        return 0;
    }
    *out = negative ? -value : value;
    *s = p;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Range checks shared by the parser and validate_problem              */
/* ------------------------------------------------------------------ */

/* NULL if the robot's numbers are fine, otherwise a message. */
static const char *robot_problem(const Robot *r)
{
    /* The numbers in the text match MAX_NUMBER in problem.h. */
    if (r->capacity < 1 || r->capacity > MAX_NUMBER) {
        return "capacity must be between 1 and 1000000";
    }
    if (r->battery < 1 || r->battery > MAX_NUMBER) {
        return "battery must be between 1 and 1000000";
    }
    return NULL;
}

static const char *task_problem(const Task *t)
{
    if (t->weight < 1 || t->weight > MAX_NUMBER) {
        return "weight must be between 1 and 1000000";
    }
    if (t->value < 0 || t->value > MAX_NUMBER) {
        return "value must be between 0 and 1000000";
    }
    if (t->priority < 1 || t->priority > MAX_PRIORITY) {
        return "priority must be between 1 and 5";
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* The three parsing steps                                             */
/* ------------------------------------------------------------------ */

/* Step 1: skip comments/blank lines, read "MAP <rows> <cols>", allocate. */
static ParseStatus parse_header(Reader *rd, Problem *p,
                                char *err, size_t err_size)
{
    char line[MAX_LINE];
    const char *rest = NULL;
    int rows = 0, cols = 0;
    int got;

    for (;;) {
        got = read_line(rd, line, sizeof line);
        if (got < 0) {
            return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                        "line is too long");
        }
        if (got == 0) {
            return fail(err, err_size, PARSE_ERR_SYNTAX, 0,
                        "missing MAP header");
        }
        if (!is_blank_or_comment(line)) {
            break;
        }
    }

    if (!keyword_is(line, "MAP", &rest)) {
        return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                    "expected the MAP header first: MAP <rows> <cols>");
    }
    if (!read_int(&rest, &rows) || !read_int(&rest, &cols)
        || !only_blanks_left(rest)) {
        return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                    "MAP needs exactly two whole numbers: MAP <rows> <cols>");
    }
    /* The numbers in the text match MAX_DIM in problem.h. */
    if (rows < 1 || rows > MAX_DIM || cols < 1 || cols > MAX_DIM) {
        return fail(err, err_size, PARSE_ERR_MAP, rd->line_no,
                    "rows and cols must each be between 1 and 500");
    }

    p->rows = rows;
    p->cols = cols;
    p->cells = (char *)malloc((size_t)rows * (size_t)cols);
    if (p->cells == NULL) {
        return fail(err, err_size, PARSE_ERR_FILE, 0, "out of memory");
    }
    return PARSE_OK;
}

/* Step 2: read exactly p->rows map lines. Record every R and T cell. */
static ParseStatus parse_map(Reader *rd, Problem *p,
                             char *err, size_t err_size)
{
    char line[MAX_LINE];
    int r, c, got;

    for (r = 0; r < p->rows; r++) {
        got = read_line(rd, line, sizeof line);
        if (got < 0) {
            return fail(err, err_size, PARSE_ERR_MAP, rd->line_no,
                        "map row is too long");
        }
        if (got == 0) {
            return fail(err, err_size, PARSE_ERR_MAP, 0,
                        "the text ends before all map rows were read");
        }
        if ((int)strlen(line) != p->cols) {
            return fail(err, err_size, PARSE_ERR_MAP, rd->line_no,
                        "map row length does not match the MAP header");
        }
        for (c = 0; c < p->cols; c++) {
            if (!problem_is_symbol(line[c])) {
                char msg[96];
                size_t n = 0;

                msg[0] = '\0';
                put_text(msg, sizeof msg, &n, "invalid map symbol at column ");
                put_number(msg, sizeof msg, &n, c + 1);
                put_text(msg, sizeof msg, &n, " (allowed: . # S C D R T)");
                return fail(err, err_size, PARSE_ERR_MAP, rd->line_no, msg);
            }
            p->cells[r * p->cols + c] = line[c];

            if (line[c] == CELL_ROBOT) {
                if (p->n_robots >= MAX_ROBOTS) {
                    return fail(err, err_size, PARSE_ERR_COUNT, rd->line_no,
                                "too many R cells (maximum is 32)");
                }
                p->robots[p->n_robots].row = r;
                p->robots[p->n_robots].col = c;
                p->n_robots++;
            } else if (line[c] == CELL_TASK) {
                if (p->n_tasks >= MAX_TASKS) {
                    return fail(err, err_size, PARSE_ERR_COUNT, rd->line_no,
                                "too many T cells (maximum is 256)");
                }
                p->tasks[p->n_tasks].row = r;
                p->tasks[p->n_tasks].col = c;
                p->n_tasks++;
            }
        }
    }
    return PARSE_OK;
}

/* Step 3: ROBOT and TASK lines until the end of the text. */
static ParseStatus parse_records(Reader *rd, Problem *p,
                                 char *err, size_t err_size)
{
    char line[MAX_LINE];
    const char *rest = NULL;
    const char *problem_msg;
    int robot_lines = 0, task_lines = 0;
    int a = 0, b = 0, c = 0, got;

    for (;;) {
        got = read_line(rd, line, sizeof line);
        if (got < 0) {
            return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                        "line is too long");
        }
        if (got == 0) {
            break;
        }
        if (is_blank_or_comment(line)) {
            continue;
        }

        if (keyword_is(line, "ROBOT", &rest)) {
            if (robot_lines >= p->n_robots) {
                return fail(err, err_size, PARSE_ERR_COUNT, rd->line_no,
                            "more ROBOT lines than R cells on the map");
            }
            if (!read_int(&rest, &a) || !read_int(&rest, &b)
                || !only_blanks_left(rest)) {
                return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                            "ROBOT needs two whole numbers: "
                            "ROBOT <capacity> <battery>");
            }
            p->robots[robot_lines].capacity = a;
            p->robots[robot_lines].battery  = b;
            problem_msg = robot_problem(&p->robots[robot_lines]);
            if (problem_msg != NULL) {
                return fail(err, err_size, PARSE_ERR_VALUE, rd->line_no,
                            problem_msg);
            }
            robot_lines++;

        } else if (keyword_is(line, "TASK", &rest)) {
            if (task_lines >= p->n_tasks) {
                return fail(err, err_size, PARSE_ERR_COUNT, rd->line_no,
                            "more TASK lines than T cells on the map");
            }
            if (!read_int(&rest, &a) || !read_int(&rest, &b)
                || !read_int(&rest, &c) || !only_blanks_left(rest)) {
                return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                            "TASK needs three whole numbers: "
                            "TASK <weight> <value> <priority>");
            }
            p->tasks[task_lines].weight   = a;
            p->tasks[task_lines].value    = b;
            p->tasks[task_lines].priority = c;
            problem_msg = task_problem(&p->tasks[task_lines]);
            if (problem_msg != NULL) {
                return fail(err, err_size, PARSE_ERR_VALUE, rd->line_no,
                            problem_msg);
            }
            task_lines++;

        } else {
            return fail(err, err_size, PARSE_ERR_SYNTAX, rd->line_no,
                        "unknown keyword (expected ROBOT or TASK)");
        }
    }

    if (p->n_robots == 0) {
        return fail(err, err_size, PARSE_ERR_COUNT, 0,
                    "the map has no R cell: at least one robot is needed");
    }
    if (robot_lines < p->n_robots) {
        return fail(err, err_size, PARSE_ERR_COUNT, 0,
                    "fewer ROBOT lines than R cells on the map");
    }
    if (task_lines < p->n_tasks) {
        return fail(err, err_size, PARSE_ERR_COUNT, 0,
                    "fewer TASK lines than T cells on the map");
    }
    return PARSE_OK;
}

/* ------------------------------------------------------------------ */
/* Public functions                                                    */
/* ------------------------------------------------------------------ */

ParseStatus validate_problem(const Problem *p, char *err, size_t err_size)
{
    const char *msg;
    int i;

    if (p == NULL || p->cells == NULL) {
        return fail(err, err_size, PARSE_ERR_MAP, 0, "there is no grid");
    }
    if (p->rows < 1 || p->rows > MAX_DIM || p->cols < 1 || p->cols > MAX_DIM) {
        return fail(err, err_size, PARSE_ERR_MAP, 0,
                    "rows and cols must each be between 1 and 500");
    }
    if (p->n_robots < 1 || p->n_robots > MAX_ROBOTS) {
        return fail(err, err_size, PARSE_ERR_COUNT, 0,
                    "robot count must be between 1 and 32");
    }
    if (p->n_tasks < 0 || p->n_tasks > MAX_TASKS) {
        return fail(err, err_size, PARSE_ERR_COUNT, 0,
                    "task count must be between 0 and 256");
    }

    for (i = 0; i < p->n_robots; i++) {
        if (problem_cell(p, p->robots[i].row, p->robots[i].col) != CELL_ROBOT) {
            set_item_error(err, err_size, "robot", i,
                           "start position is not an R cell");
            return PARSE_ERR_MAP;
        }
        msg = robot_problem(&p->robots[i]);
        if (msg != NULL) {
            set_item_error(err, err_size, "robot", i, msg);
            return PARSE_ERR_VALUE;
        }
    }
    for (i = 0; i < p->n_tasks; i++) {
        if (problem_cell(p, p->tasks[i].row, p->tasks[i].col) != CELL_TASK) {
            set_item_error(err, err_size, "task", i,
                           "pickup position is not a T cell");
            return PARSE_ERR_MAP;
        }
        msg = task_problem(&p->tasks[i]);
        if (msg != NULL) {
            set_item_error(err, err_size, "task", i, msg);
            return PARSE_ERR_VALUE;
        }
    }
    return PARSE_OK;
}

ParseStatus parse_problem_text(const char *text, Problem *p,
                               char *err, size_t err_size)
{
    Reader rd;
    ParseStatus st;

    if (err != NULL && err_size > 0) {
        err[0] = '\0';
    }
    if (p == NULL) {
        return fail(err, err_size, PARSE_ERR_SYNTAX, 0, "no Problem to fill");
    }
    problem_init(p);
    if (text == NULL) {
        return fail(err, err_size, PARSE_ERR_SYNTAX, 0, "no input text");
    }

    rd.pos = text;
    rd.line_no = 0;

    st = parse_header(&rd, p, err, err_size);
    if (st == PARSE_OK) {
        st = parse_map(&rd, p, err, err_size);
    }
    if (st == PARSE_OK) {
        st = parse_records(&rd, p, err, err_size);
    }
    if (st == PARSE_OK) {
        st = validate_problem(p, err, err_size);
    }
    if (st != PARSE_OK) {
        problem_free(p);          /* leave the Problem empty on any failure */
    }
    return st;
}

ParseStatus parse_problem_file(const char *path, Problem *p,
                               char *err, size_t err_size)
{
    FILE *f;
    char *text;
    long size;
    size_t got;
    ParseStatus st;

    if (p != NULL) {
        problem_init(p);
    }
    if (path == NULL) {
        return fail(err, err_size, PARSE_ERR_FILE, 0, "no file name given");
    }
    f = fopen(path, "rb");
    if (f == NULL) {
        return fail(err, err_size, PARSE_ERR_FILE, 0, "cannot open the file");
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return fail(err, err_size, PARSE_ERR_FILE, 0, "cannot read the file");
    }
    size = ftell(f);
    if (size < 0 || size > MAX_FILE_BYTES) {
        fclose(f);
        return fail(err, err_size, PARSE_ERR_FILE, 0,
                    "file is larger than 4 MB or its size is unknown");
    }
    rewind(f);

    text = (char *)malloc((size_t)size + 1);
    if (text == NULL) {
        fclose(f);
        return fail(err, err_size, PARSE_ERR_FILE, 0, "out of memory");
    }
    got = fread(text, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) {
        free(text);
        return fail(err, err_size, PARSE_ERR_FILE, 0, "cannot read the file");
    }
    text[size] = '\0';

    st = parse_problem_text(text, p, err, err_size);
    free(text);
    return st;
}

const char *parse_status_name(ParseStatus s)
{
    switch (s) {
    case PARSE_OK:         return "PARSE_OK";
    case PARSE_ERR_FILE:   return "PARSE_ERR_FILE";
    case PARSE_ERR_SYNTAX: return "PARSE_ERR_SYNTAX";
    case PARSE_ERR_MAP:    return "PARSE_ERR_MAP";
    case PARSE_ERR_COUNT:  return "PARSE_ERR_COUNT";
    case PARSE_ERR_VALUE:  return "PARSE_ERR_VALUE";
    }
    return "PARSE_UNKNOWN";
}

void problem_print(const Problem *p)
{
    int r, c, i;

    if (p == NULL || p->cells == NULL) {
        printf("(empty problem)\n");
        return;
    }
    printf("GRID %d %d\n", p->rows, p->cols);
    for (r = 0; r < p->rows; r++) {
        for (c = 0; c < p->cols; c++) {
            putchar(p->cells[r * p->cols + c]);
        }
        putchar('\n');
    }
    for (i = 0; i < p->n_robots; i++) {
        printf("ROBOT %d at (%d,%d) capacity=%d battery=%d\n", i + 1,
               p->robots[i].row, p->robots[i].col,
               p->robots[i].capacity, p->robots[i].battery);
    }
    for (i = 0; i < p->n_tasks; i++) {
        printf("TASK %d at (%d,%d) weight=%d value=%d priority=%d\n", i + 1,
               p->tasks[i].row, p->tasks[i].col, p->tasks[i].weight,
               p->tasks[i].value, p->tasks[i].priority);
    }
}
