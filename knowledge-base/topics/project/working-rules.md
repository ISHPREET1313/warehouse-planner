---
id: 20261119-working-rules
title: Working rules
tags: [project, process]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-project-overview, 20261119-build-order]
summary: How the student and Claude work: authorship split, one module at a time, questions go to a file, handoff docs.
---

# Working rules

> Summary: How the student and Claude work: authorship split, one module at a time, questions go to a file, handoff docs.

## Context
Instructor approved AI assistance. The student must be able to explain, trace, debug and modify every line, so code stays simple and each piece is taught.

## Details
- The student writes the min-heap and Dijkstra. Claude gives pseudocode, hints, review and quizzes for those two only. Everything else Claude may write, then teach.
- One module at a time. Each gets: design note, code, build/test commands, tests (normal, boundary, invalid, adversarial), time and space note, a 5-line explain-back and two viva questions.
- Count operations, not just time.
- No clarifying questions in chat (admin change 2026-11-19): state the assumption, continue, log it in `docs/QUESTIONS.md`.
- Memory between chats is only the files: `docs/STATE.md`, `DECISIONS.md`, `TODO.md`, `HANDOFF.md` (overwritten each session), `QUESTIONS.md`, and this knowledge base.
- Never edit outside the project folder. Never delete files without asking.
- Claude cannot run code in the user's folder; the user pastes real build and test output.

## Related
- [Project overview](project-overview.md) — prerequisite.
- [Build order](build-order.md) — see-also: what comes next.
