# SCORE-ROADLESS-SPOT-DIVERSITY-090

Date: 2026-08-12

Parent: `f574d4e`.

The candidate changed one production condition: the existing spot-diversity
lane in `prune_columns` was enabled on roadless maps in addition to its existing
fuel-at-least-three-day-budgets domain. It did not change column, master, F0,
W1, ALNS, role or deadline caps. Road-containing behavior remained the parent
path.

On the frozen 18-case CEILING-BRANCH-PATROL-089 development split, direct
candidate-vs-parent W/T/L was `2/15/1`, zero invalid. It closed terminal-fork
low from `5/20/28` to exact `5/20/30` (+2 tier 3) and improved fuel-split low
from `6/24/25` to `6/24/27` (+2 tier 3). However, branched-duplicate low
regressed from `5/20/25` to `5/19/29`, a tier-2 loss of one daily distinct.
Every other case tied parent. Candidate oracle-gap result hash was
`28a37d0058be368e` versus parent `33080960367d1631`.

Verdict: rejected. A tier-2 loss cannot be exchanged for two tier-3 serving
gains. The frozen 089 holdout was not opened. The production condition was
fully reverted. Do not tune spot-diversity ranking, slot count or thresholds on
the consumed 089 development cohort; reopen only from a new independent
counterexample that isolates a lexicographically safe retention rule.
