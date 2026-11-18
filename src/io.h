/*
 * io.h - parser and validator for the plain-text input file.
 *
 * INPUT FILE FORMAT (version 1)
 * -----------------------------
 *   # a comment line (only outside the map block)
 *   MAP <rows> <cols>
 *   <rows lines, each exactly <cols> characters from  . # S C D R T >
 *   ROBOT <capacity> <battery>        one line per R on the map
 *   TASK  <weight> <value> <priority> one line per T on the map
 *
 * The N-th ROBOT line belongs to the N-th R found when reading the map
 * top-to-bottom, left-to-right; TASK lines work the same way with T.
 * Blank lines and '#' comment lines are allowed before MAP and after the
 * map block.  Inside the map block a '#' is an obstacle, not a comment.
 * Both \n and \r\n line endings are accepted.  Keywords are upper case.
 * A comment must be on its own line (no comment after a ROBOT/TASK line).
 *
 * VALIDATION (every failure gives a status code and a message, with the
 * line number where there is one, and leaves the Problem empty):
 *   - MAP header present and well formed, 1 <= rows, cols <= MAX_DIM
 *   - every map row has exactly <cols> characters, all valid symbols
 *   - number of ROBOT lines == number of R cells (same for TASK and T)
 *   - at least one robot; at most MAX_ROBOTS robots and MAX_TASKS tasks
 *   - capacity, battery, weight in 1..MAX_NUMBER; value in 0..MAX_NUMBER;
 *     priority in 1..MAX_PRIORITY
 *   - no unknown keywords, no missing or extra numbers on a line
 * Whether a task can actually be REACHED is checked later, when the grid
 * is turned into a graph (item 3).
 *
 * Time O(size of input), auxiliary space O(rows*cols) for the grid.
 */
#ifndef IO_H
#define IO_H

#include <stddef.h>

#include "problem.h"

typedef enum {
    PARSE_OK = 0,
    PARSE_ERR_FILE,     /* cannot open or read the file                  */
    PARSE_ERR_SYNTAX,   /* a line is malformed or has an unknown keyword */
    PARSE_ERR_MAP,      /* bad map dimensions, row length or symbol      */
    PARSE_ERR_COUNT,    /* R/T cells and ROBOT/TASK lines do not match,
                           or too many / no robots                       */
    PARSE_ERR_VALUE     /* a number is outside its allowed range         */
} ParseStatus;

/*
 * Parse a problem from a string. p must be empty or uninitialised (the
 * parser calls problem_init on it). On success p owns the grid: call
 * problem_free(p) when done. On failure p is left empty and err (if not
 * NULL) holds a one-line message.
 */
ParseStatus parse_problem_text(const char *text, Problem *p,
                               char *err, size_t err_size);

/* Same, but reads the text from a file first. */
ParseStatus parse_problem_file(const char *path, Problem *p,
                               char *err, size_t err_size);

/* Check a Problem that already exists in memory (the parser calls this
 * at the end; the generator can use it too). */
ParseStatus validate_problem(const Problem *p, char *err, size_t err_size);

/* "PARSE_OK", "PARSE_ERR_SYNTAX", ... for printing. */
const char *parse_status_name(ParseStatus s);

/* Print the grid, the robots and the tasks to stdout. */
void problem_print(const Problem *p);

#endif /* IO_H */
