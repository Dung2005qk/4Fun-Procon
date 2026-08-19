# CEILING-TANKER-MATCH-149

Date: 2026-08-13
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/CEILING-TANKER-MATCH-149.csv`
SHA256: `CCEA64E1D4CE491937BC88B635C63CDFC8A69BACE238DD3AC439B0D75E4A01B8`

Experiment 148 ended before score because a 2x2 component cannot satisfy three
distinct non-spot starts plus three spots. This successor freezes new seeds on
a 2x3 roadless component and keeps the same complete three-day joint step DP,
cross-day position/fuel/refuel/lifetime state and official lexicographic score.
Every reconstructed match must agree between the exact simulator and independent
validator. Production source remains unchanged.

## Development result

Rejected at preflight with no score result. Official config validation rejected
horizon three because a match must have 4--10 days. Source-level invariant
audit also found the proposed six steps below the minimum 16 for an 8x8 map.
Neither value is relaxed: 149 closes and a fresh horizon-four, 16-step successor
is required.

## Holdout result

Unopened.
