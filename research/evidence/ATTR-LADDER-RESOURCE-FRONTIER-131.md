# ATTR-LADDER-RESOURCE-FRONTIER-131

Date: 2026-08-13  
Parent: `5dccb0f`  
Fixture: consumed seed `1800000`, exact day-2 state

The complete dual-valid DayOutcome frontiers contain 55 routes for agent 0 and
44 for agent 1. Served-terminal Pareto counts are:

- agent 0: `19:11 | 20:7 | 21:3 | 26:3 | 27:7 | 28:6`;
- agent 1: `19:4 | 20:6 | 21:7 | 26:3 | 27:6 | 28:4`.

The maximum per-terminal fronts are 11 and 7. Composing all pairs yields 2420
dual-valid candidate plans. Research augmentation retained every unique route,
giving widths `56|45|1` with 95 novel routes; no K or rank was chosen.

The exact outcome was absent under both existing master regimes: W1 visited 64
nodes and retained neither plan nor outcome; normal 40000/32/8 visited 532 and
also retained neither. The standard attribution lattice 48, 64, 96, 128, 192
and 256 never retained it (`first_cap=-1`). Thus complete route availability
alone does not fix the gap. Retaining the exact class would additionally require
an upper-aware supplemental population/solver before current-score pruning.

Verdict: accepted negative attribution. A full frontier is not naturally small,
and combining it with a new upper-aware master would duplicate solver and value
paths. Do not implement or tune that compound mechanism from this witness.
Reopen only if a bounded existing production representation independently emits
the same cross-terminal class or a general upper-reservoir proof is available
without a second route search.
