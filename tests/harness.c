/* harness.c - see harness.h */
#include "harness.h"

#include <stdio.h>
#include <string.h>

static int g_run    = 0;   /* checks executed so far */
static int g_failed = 0;   /* checks that did not match */

void harness_suite(const char *name)
{
    printf("\n== %s ==\n", name);
    printf("%-44s %18s %18s   %s\n", "TEST", "EXPECTED", "ACTUAL", "RESULT");
}

void check_long(const char *test_name, long expected, long actual)
{
    int ok = (expected == actual);

    g_run++;
    if (!ok) {
        g_failed++;
    }
    printf("%-44s %18ld %18ld   %s\n",
           test_name, expected, actual, ok ? "PASS" : "FAIL");
}

void check_str(const char *test_name, const char *expected, const char *actual)
{
    const char *e = (expected != NULL) ? expected : "(null)";
    const char *a = (actual != NULL) ? actual : "(null)";
    int ok = (strcmp(e, a) == 0);

    g_run++;
    if (!ok) {
        g_failed++;
    }
    printf("%-44s %18s %18s   %s\n", test_name, e, a, ok ? "PASS" : "FAIL");
}

int harness_summary(void)
{
    printf("\nSUMMARY: %d tests, %d passed, %d failed\n",
           g_run, g_run - g_failed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}
