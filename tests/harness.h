/*
 * harness.h - a tiny test harness that prints an expected-vs-actual table.
 *
 * Usage inside a test file:
 *     harness_suite("metrics");
 *     check_long("normal: 2 + 2", 4, 2 + 2);
 * and in main:
 *     return harness_summary();     (0 = all passed, 1 = something failed)
 */
#ifndef HARNESS_H
#define HARNESS_H

/* Print a suite title and the table header. */
void harness_suite(const char *name);

/* Compare two integers, print one table row, remember pass/fail. */
void check_long(const char *test_name, long expected, long actual);

/* Compare two strings (NULL counts as "(null)"), same table row. */
void check_str(const char *test_name, const char *expected, const char *actual);

/* Print totals. Returns 0 if every check passed, otherwise 1. */
int harness_summary(void);

#endif /* HARNESS_H */
