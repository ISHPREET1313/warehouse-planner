# HANDOFF

Overwritten at the end of every session.

## Session 2 (2026-11-19)
- Read PROMPT.md and docs. The previous chat's parser code never reached the folder, so it was rewritten for MinGW GCC 6.3 / C11.
- Admin change: questions go to docs/QUESTIONS.md, not chat. PROMPT.md rule 6.7 and change log updated.
- Wrote item 2: src/problem.[ch], src/io.[ch], tests/test_parser.c, data/small/sample1.txt, data/adversarial/bad_count.txt. Edited Makefile, tests/tests.h, tests/run_tests.c, tests/harness.[ch] (check_str), src/main.c, .gitignore.
- Built knowledge-base/ (INDEX.md + 10 notes). Graphify is connected but only indexes pushed GitHub commits (2 nodes now).
- NOTHING WAS COMPILED OR RUN. Item 2 is unverified.

## Next step
1. User runs: make clean && make && make test && ./planner.exe data/small/sample1.txt, and pastes the real output. Fix any warning or FAIL first.
2. User commits and pushes (so Graphify indexes the code).
3. User answers the viva questions for items 1 and 2.
4. Then item 3: grid to graph. Start with a design note.

## Suggested commit (after tests pass)
git add -A
git commit -m "Item 2: parser and validator, tests, sample data, knowledge base"
git push
