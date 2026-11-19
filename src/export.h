/*
 * export.h - write a Plan to two CSV files (item 13).
 *
 * tasks file, one row per task (header first):
 *   strategy,robot,order,task,row,col,weight,value,priority
 *     robot = 1-based robot number, order = 1-based visiting order of that
 *     robot. A task nobody serves has robot "none" and order 0. Served rows
 *     come first (robot by robot, in visiting order), then unserved tasks in
 *     task order. task is 1-based, row and col are 0-based map coordinates.
 * robots file, one row per robot (header first):
 *   strategy,robot,stops,load,capacity,value,cost,battery
 *
 * label names the strategy set ("A", "B", "custom"), so several plans can
 * share the same files: append = 0 starts new files with a header, append = 1
 * adds rows to existing files without a header.
 *
 * Returns 0 on success, -1 for NULL arguments or a file that cannot be
 * opened or written. No algorithm lives here; time O(robots + tasks).
 */
#ifndef EXPORT_H
#define EXPORT_H

#include "planner.h"
#include "problem.h"

int export_plan_csv(const char *tasks_path, const char *robots_path,
                    const char *label, const Problem *p, const Plan *plan,
                    int append);

#endif /* EXPORT_H */
