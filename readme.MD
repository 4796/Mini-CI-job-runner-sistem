## Quick start

```bash
./build
./build --run
./build --clean
./build --release
```

## Structure

- `src/` - source code
- `include/` - header files
- `out/` - build output
- `tests/` - tests

A minimal, from-scratch CI/CD job runner written in C, built to demonstrate core concurrent programming concepts: parallel task execution, dependency-graph scheduling, safe access to shared state, and correct worker shutdown, modeled loosely on the dependency (`needs`) semantics of GitHub Actions and GitLab CI.
This is **not** a replacement for Jenkins, GitHub Actions, or GitLab CI. It is a small, self-contained system built to explore and demonstrate concurrency primitives (mutexes, condition variables, thread pools) in the context of a realistic scheduling problem.

## Overview

Mini CI loads a pipeline definition from a JSON file describing a set of jobs (e.g. `build`, `test`, `package`, `deploy`), each with a shell command and a list of dependencies (`needs`). The pipeline is modeled as a **directed acyclic graph (DAG)**: nodes are jobs, edges are dependencies. A pool of worker threads executes jobs concurrently, respecting the constraint that a job may only start once all of its dependencies have completed successfully.

Key guarantees the system provides:

- A job **never** starts before all of its dependencies have finished.
- A job is **never** executed more than once.
- Circular dependencies are detected and rejected **before** any thread starts running.
- Workers **never busy-wait**, they block on a condition variable when no work is available.
- The system shuts down cleanly once every job has reached a terminal state.

## Pipeline Configuration Format

Pipelines are defined in JSON:

```json
{
  "jobs": [
    { "name": "build",   "command": "make build",               "needs": [] },
    { "name": "test",    "command": "make test",                 "needs": ["build"] },
    { "name": "package", "command": "tar czf app.tar.gz app",     "needs": ["build"] },
    { "name": "deploy",  "command": "docker run -d myapp:latest", "needs": ["test", "package"] }
  ]
}
```

- Jobs with no shared dependencies run **in parallel** (here: `test` and `package`).
- `deploy` only runs once **both** `test` and `package` succeed.
- If `test` fails, `deploy` is marked `SKIPPED`, not `FAILED` — it never runs.

## Job Lifecycle

```
PENDING ──▶ RUNNING ──▶ SUCCESS
                   └───▶ FAILED ──▶ (propagates) ──▶ SKIPPED (dependents)
```

- **PENDING** - waiting on one or more unfinished dependencies.
- **RUNNING** - currently being executed by a worker.
- **SUCCESS** - completed with exit code `0`.
- **FAILED** - completed with a non-zero exit code.
- **SKIPPED** - never ran, because a dependency failed or was itself skipped.

