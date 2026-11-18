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
