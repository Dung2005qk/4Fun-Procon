# Historical checkpoint tournament

This directory contains a version-neutral tournament harness. Every checkpoint
is exported from Git into `old/snapshots`, built independently, and linked
against the exact same `old/harness/historical_tournament.cpp`.

The harness freezes all non-solver semantics:

- identical generated maps and seed windows;
- identical endogenous two-day own-traffic feedback;
- identical exogenous opponent footprints;
- native role selection through each checkpoint's `UdonShieldEngine`;
- official lexicographic score;
- exact simulator plus independent validator;
- invalid plans replaced by exact WAIT only after being counted.

`CHECKPOINTS.csv` lists architecture milestones rather than every adjacent
commit. Build directories, exported snapshots, and raw tournament output are
generated artifacts and are not production source.

The durable evidence surface is:

- `harness/historical_tournament.cpp`: shared C++ fixture and execution path;
- `CHECKPOINTS.csv`: immutable commit-to-label mapping;
- `results/raw/*.txt`: per-fixture measurements;
- `summarize_tournament.py`: deterministic report generator;
- `results/HISTORICAL_TOURNAMENT.md`: final comparison and verdict.

The executable protected matrix is defined in `research/MATRIX.csv` and launched
by `research/run_checkpoint_matrix.ps1`. Every forward research lane uses the
`5000 ms` internal solver/role hard cap. A match may advertise an outer window
such as `60000 ms`, but that window is lifecycle/network evidence only and never
authorizes more than 5000 ms of compute. Shorter budgets remain degradation
diagnostics only. The shared forward harness explicitly uses production harvest
mode `7`; historical reports generated before `EVAL-PARITY-047` remain mode-6
artifacts and are not silently relabeled.

Regenerate the report after raw files are present:

```powershell
python old/summarize_tournament.py
```
