# Knowledge Base — Index

Project: Warehouse Robot Path and Task Planner (CSEG3053). Rules live in `../PROMPT.md`; live status in `../docs/`.

## How to use this KB (for agents)
1. Read this index first. Pick notes by their descriptions; open only those.
2. Prefer 1–3 specific notes over scanning everything.
3. Cite the note `id` when you use information from it.

## How to update this KB (for agents)
- New idea -> new atomic note. Same idea changed -> edit in place and bump `updated`.
- Any create, rename or delete MUST update this registry and the topic's `_topic.md` in the same change.
- Use only tags from the list below; add a tag here before using it.
- Add notes for each finished build item (what, why, complexity, viva questions).

## Controlled tags
`project`, `process`, `environment`, `graphify`, `metrics`, `parser`, `graph`, `heap`, `testing`, `complexity`, `viva`

## Registry
### project
- `20261119-project-overview` — **Project overview** — `topics/project/project-overview.md` — What the planner is, who it is for, how it is assessed.
- `20261119-build-order` — **Build order and progress** — `topics/project/build-order.md` — The 18 items in tier order and which are done.
- `20261119-working-rules` — **Working rules** — `topics/project/working-rules.md` — Authorship split, one module at a time, questions go to a file.

### environment
- `20261119-toolchain` — **Toolchain on the student's machine** — `topics/environment/toolchain.md` — MinGW GCC 6.3, Make 3.81, C11, functions to avoid.
- `20261119-graphify-setup` — **Graphify code index** — `topics/environment/graphify-setup.md` — Indexes the GitHub repo on push; commit and push first.

### metrics
- `20261119-operation-counters` — **Operation counters** — `topics/metrics/operation-counters.md` — Fixed meaning of each counter and the timer.

### testing
- `20261119-test-harness` — **Test harness** — `topics/testing/test-harness.md` — Expected vs actual table, check functions, categories.

### parser
- `20261119-parser-input-format` — **Parser input file format** — `topics/parser/parser-input-format.md` — Exact text format with a worked example.
- `20261119-parser-validation-and-errors` — **Parser validation rules and error codes** — `topics/parser/parser-validation-and-errors.md` — Every check, five error codes, message format.
- `20261119-parser-design-and-complexity` — **Parser design and complexity** — `topics/parser/parser-design-and-complexity.md` — Structure and best/average/worst time and space.
- `20261119-parser-viva-questions` — **Parser explain-back and viva questions** — `topics/parser/parser-viva-questions.md` — 5-line summary and likely evaluator questions.

### heap
- `20261119-heap-design-and-complexity` — **Min-heap design and complexity** — `topics/heap/heap-design-and-complexity.md` — Array layout, push/pop, lazy deletion, best/average/worst.
- `20261119-heap-viva-questions` — **Min-heap explain-back and viva questions** — `topics/heap/heap-viva-questions.md` — 5-line summary and likely evaluator questions.

### graph
- `20261119-graph-design-and-complexity` — **Grid to graph design and complexity** — `topics/graph/graph-design-and-complexity.md` — Nodes, arcs, CSR storage, BFS reachability, time and space.
- `20261119-graph-viva-questions` — **Graph explain-back and viva questions** — `topics/graph/graph-viva-questions.md` — 5-line summary and likely evaluator questions.
