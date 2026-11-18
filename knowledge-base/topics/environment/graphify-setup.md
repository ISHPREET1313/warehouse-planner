---
id: 20261119-graphify-setup
title: Graphify code index
tags: [environment, graphify]
created: 2026-11-19
updated: 2026-11-19
related: [20261119-toolchain]
summary: Graphify indexes the GitHub repo on push, so commit and push before asking it about the code.
---

# Graphify code index

> Summary: Graphify indexes the GitHub repo on push, so commit and push before asking it about the code.

## Context
Graphify is connected to Claude. It answers questions about symbols, callers, dependencies and tests from a code graph.

## Details
- Repository: `ISHPREET1313/warehouse-planner` (GitHub, default branch `main`), status ready.
- It reads only what is pushed. Checked 2026-11-19: 2 nodes indexed (README only), searching "parse" returned nothing. Local, uncommitted code is invisible to it.
- Pushes re-index automatically; results reflect the last indexed commit.
- Free plan: only README.md is indexed from documents. Code files are indexed as code. This knowledge base and `docs/` are NOT indexed by Graphify, so Claude reads them through the filesystem.
- Useful tools once indexed: `graphify_find` (where is a symbol defined), `graphify_callers`, `graphify_file_neighbors` (includes and dependents), `graphify_tests_for`, `graphify_impact` (what a change touches), `query_graph`.
- Plan for use: after each finished item, commit and push; then ask Graphify for callers and tests of the new module as a cross-check against the explain-back.

## Related
- [Toolchain](toolchain.md) — see-also: how the code is built locally.
