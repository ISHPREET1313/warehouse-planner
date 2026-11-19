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
`project`, `process`, `environment`, `graphify`, `metrics`, `parser`, `graph`, `heap`, `dijkstra`, `comparison`, `bellman-ford`, `knapsack`, `tsp`, `planner`, `benchmark`, `testing`, `complexity`, `viva`

## Registry
### project
- `20261119-project-overview` — **Project overview** — `topics/project/project-overview.md` — What the planner is, who it is for, how it is assessed.
- `20261119-build-order` — **Build order and progress** — `topics/project/build-order.md` — The demo core first (items 9 to 14), the rest of the 23 items on hold; which are done.
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

### dijkstra
- `20261119-dijkstra-design-and-complexity` — **Dijkstra with heap design and complexity** — `topics/dijkstra/dijkstra-design-and-complexity.md` — Loop, lazy deletion, counters, best/average/worst, hand example.
- `20261119-dijkstra-viva-questions` — **Dijkstra explain-back and viva questions** — `topics/dijkstra/dijkstra-viva-questions.md` — 5-line summary and likely evaluator questions.

### comparison
- `20261119-comparison-design-and-complexity` — **Dijkstra array scan and Bellman-Ford design and complexity** — `topics/comparison/comparison-design-and-complexity.md` — How each works, counters, best/average/worst, when to use which.
- `20261119-comparison-viva-questions` — **Scan and Bellman-Ford explain-back and viva questions** — `topics/comparison/comparison-viva-questions.md` — 5-line summary and likely evaluator questions.

### graph
- `20261119-graph-design-and-complexity` — **Grid to graph design and complexity** — `topics/graph/graph-design-and-complexity.md` — Nodes, arcs, CSR storage, BFS reachability, time and space.
- `20261119-graph-viva-questions` — **Graph explain-back and viva questions** — `topics/graph/graph-viva-questions.md` — 5-line summary and likely evaluator questions.

### knapsack
- `20261119-knapsack-design-and-complexity` — **0/1 knapsack design and complexity** — `topics/knapsack/knapsack-design-and-complexity.md` — DP vs greedy, counters, the case where greedy fails.
- `20261119-knapsack-viva-questions` — **Knapsack explain-back and viva questions** — `topics/knapsack/knapsack-viva-questions.md` — 5-line summary and likely evaluator questions.

### tsp
- `20261119-tsp-design-and-complexity` — **Pickup order (TSP) design and complexity** — `topics/tsp/tsp-design-and-complexity.md` — Nearest neighbour vs Held-Karp, counters, worked example.
- `20261119-tsp-viva-questions` — **Pickup order explain-back and viva questions** — `topics/tsp/tsp-viva-questions.md` — 5-line summary and likely evaluator questions.

### planner
- `20261119-planner-design-and-complexity` — **Planner pipeline design and complexity** — `topics/planner/planner-design-and-complexity.md` — Five steps, rounds, battery trimming, worked example.
- `20261119-planner-viva-questions` — **Planner explain-back and viva questions** — `topics/planner/planner-viva-questions.md` — 5-line summary and likely evaluator questions.

### benchmark
- `20261119-benchmark-design-and-results` — **Benchmark design and results** — `topics/benchmark/benchmark-design-and-results.md` — What the table measures and what the saved numbers show.
- `20261119-benchmark-viva-questions` — **Benchmark explain-back and viva questions** — `topics/benchmark/benchmark-viva-questions.md` — 5-line summary and likely evaluator questions.
