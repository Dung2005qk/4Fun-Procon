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
by `research/run_checkpoint_matrix.ps1`. The primary production-selection lane
uses the observed standard competition budget of `60000 ms`; the BTC preflight
lane uses `5000 ms`. Shorter budgets remain degradation diagnostics only.

Regenerate the report after raw files are present:

```powershell
python old/summarize_tournament.py
```
