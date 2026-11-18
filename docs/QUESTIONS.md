# QUESTIONS

Open questions for the admin (you). Claude does not stop to ask in chat: it states an assumption, continues, and logs it here. Discuss later; mark answered ones with the answer.

| # | Date | Question | Assumption taken | Status |
|---|------|----------|------------------|--------|
| Q1 | 2026-11-19 | Should blank lines be allowed inside the map block? | No. A blank line there is a map error (row length 0). Strict keeps the format simple to explain. | open |
| Q2 | 2026-11-19 | Are the limits right (map 500x500, 32 robots, 256 tasks, numbers up to 1,000,000, file up to 4 MB)? | Yes. Chosen so sums never overflow a 32-bit int and the O(V^2) array-scan comparison still runs. Change in `src/problem.h`. | open |
| Q3 | 2026-11-19 | Should `PROMPT.md` be git-ignored? `.gitignore` had a broken UTF-16 `PROMPT.md` line, so it was never ignored. | Not ignored (matches the old effective behaviour). The project memory is then committed and pushed. | open |
| Q4 | 2026-11-19 | May a task with value 0 exist? | Yes (value 0..1000000). Weight must be at least 1. | open |
| Q5 | 2026-11-19 | Is a map with zero T cells valid? | Yes. Zero tasks is allowed; at least one robot is required. | open |
| Q6 | 2026-11-19 | Is it OK that Graphify only sees pushed code? | Assumed yes: commit and push after each finished item. Free plan indexes only the README from documents. | open |
| Q7 | 2026-11-19 | Should the knowledge base live in `knowledge-base/` (skill layout) rather than `docs/`? | Yes, `knowledge-base/INDEX.md` is the entry point; `docs/` keeps the four handoff files. | open |
| Q8 | 2026-11-19 | Comments after numbers on a ROBOT/TASK line (`ROBOT 5 10 # fast`)? | Rejected as a syntax error. Comments must be on their own line. | open |
| Q9 | 2026-11-19 | Should a shelf (S) be walkable? | No. A robot cannot drive through a shelf; tasks (T) sit in the aisles. Change `cell_is_walkable` in `src/graph.c` if shelves should be enterable. | open |
| Q10 | 2026-11-19 | Should unreachable tasks be a parse error? | No. The parser accepts them; `graph_unreachable_tasks` reports them and later planners will skip them. A map with a sealed task is still a legal input. | open |
| Q11 | 2026-11-19 | Do chargers (C) and docks (D) need special rules in the graph? | Not yet. They are ordinary walkable cells. Battery and recharging logic is for the planner step, not for the graph. | open |
| Q12 | 2026-11-19 | Admin said "I will review and you will ask". Does that mean Claude should quiz the admin? | Yes, but not in chat (rule 6.7): the review/viva questions live in each module's `*-viva-questions.md` note and in this file; the admin answers them here when ready. Claude does not wait for answers before moving on. | open |
| Q13 | 2026-11-19 | Should the heap support decrease-key instead of duplicate pushes? | No. Lazy deletion is simpler to explain. The heap just needs capacity n_arcs + 1 for Dijkstra. Revisit only if a benchmark shows the larger heap hurts. | open |
| Q14 | 2026-11-19 | Do equal-key items need a stable order (first in, first out)? | No. Dijkstra only needs the smallest distance; ties may come out in any order. | open |
| Q15 | 2026-11-19 | Viva answers for items 1 to 4 are still unanswered. Review prompts: why count operations as well as time (item 1)? Why `volatile sink` in test_metrics.c (item 1)? Why must ROBOT lines equal R cells (item 2)? Why keep # and S cells as nodes (item 3)? Why no decrease-key in the heap (item 4)? | Model answers are in the knowledge-base viva notes. Admin answers here. | open |
