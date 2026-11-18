/* problem.c - small helpers for the Problem structure. See problem.h. */
#include "problem.h"

#include <stdlib.h>
#include <string.h>

void problem_init(Problem *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof *p);   /* all ints 0, cells NULL */
}

void problem_free(Problem *p)
{
    if (p == NULL) {
        return;
    }
    free(p->cells);            /* free(NULL) is allowed */
    memset(p, 0, sizeof *p);   /* so a second problem_free does nothing */
}

char problem_cell(const Problem *p, int row, int col)
{
    if (p == NULL || p->cells == NULL) {
        return '\0';
    }
    if (row < 0 || row >= p->rows || col < 0 || col >= p->cols) {
        return '\0';
    }
    return p->cells[row * p->cols + col];
}

int problem_is_symbol(char c)
{
    /* strchr also "finds" the terminating '\0', so rule that out first. */
    if (c == '\0') {
        return 0;
    }
    return strchr(".#SCDRT", c) != NULL;
}
