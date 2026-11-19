---
id: 20261119-working-rules
title: Working rules
tags: [project, process]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-project-overview, 20261119-build-order]
summary: How the student and Claude work: Claude writes the code and the admin reviews, one module at a time, questions go to a file, handoff docs.
---

# Working rules

> Summary: How the student and Claude work: Claude writes the code and the admin reviews, one module at a time, questions go to a file, handoff docs.

## Context
Instructor approved AI assistance. The student must be able to explain, trace, debug and modify every line, so code stays simple and each piece is taught.

## Details
- Claude writes all the code, including the min-heap and Dijkstra (admin change 2026-11-19; before that the student wrote those two). The admin reviews, and Claude teaches each module: explain-back, viva questions, complexity note.
- One module at a time. Each gets: design note, code, build/test commands, tests (normal, boundary, invalid, adversarial), time and space note, a 5-line explain-back and two viva questions.
- Count operations, not just time.
- No clarifying questions in chat (admin change 2026-11-19): state the assumption, continue, log it in `docs/QUESTIONS.md`.
- Memory between chats is only the files: `docs/STATE.md`, `DECISIONS.md`, `TODO.md`, `HANDOFF.md` (overwritten each session), `QUESTIONS.md`, and this knowledge base.
- Never edit outside the project folder. Never delete files without asking.
- Claude runs builds, tests and the program on the user's laptop through Remote Desktop Commander when it is connected (otherwise the user pastes the real output). Commits and pushes wait for the admin's word.
- Quality loop (PROMPT.md section 6b): plan with hand-worked expected numbers, code, review, then log each mistake in `docs/LESSONS.md`.

## Related
- [Project overview](project-overview.md) — prerequisite.
- [Build order](build-order.md) — see-also: what comes next.
