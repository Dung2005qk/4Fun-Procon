# ATTR-TRAFFIC-MARKOV-115

Parent: `5dccb0f`

Frozen source manifest SHA256:
`6027B078DA6BB80D9B613FB57210C55B10DCB012B67185BDD9C5F62F044B1263`.

This attribution reopens the infrastructure-inconclusive 097 oracle under its
only allowed condition: a mathematically exact state quotient. At the start of
day `d`, `F[d-2]` has already been consumed into the authoritative current road
status `R[d]`. It cannot affect any remaining transition except through `R[d]`.
The exact Markov state therefore retains physical agent state, lifetime brands,
accumulated score, `F[d-1]`, and `R[d]`. After an exact day plan, the transition
computes `F[d]` from the exact step footprint and derives `R[d+1]` from
`F[d] + F[d-1]` plus the frozen opponent footprints. Witness reconstruction
recomputes and requires equality of every next status vector.

The first permitted development row completed with dual-valid oracle and HEAD
witnesses:

`seed=1700000, traffic-balanced/low, oracle=6/22/22, HEAD=6/14/19,
tier=2, gain=8, max_frontier=36382, day_enumerations=1873,
result_hash=5d2f3c73d5d99dbf`.

The first divergence is day 1. Oracle deliberately accepts current score
`5/5/5`, ending at agents `34@10|19@8|0@12`, while HEAD takes `6/6/11` and ends
at `33@1|17@3|0@12`. The oracle path then reaches cumulative `6/11/11`,
`6/17/17`, and `6/22/22`; HEAD reaches `6/10/15`, `6/12/17`, and `6/14/19`.
The oracle day-1 plan matches zero of HEAD's 16 audited candidates.

Focused attribution locates the first causal gap before final selection. The
unchanged 16-column portfolio retains only one of the three oracle agent plans;
64 columns retain all three. Exact additive augmentation also retains all three,
but the unchanged master does not retain the joint oracle candidate until the
existing diversity allocation reaches 24 slots (or total candidate cap 48 with
its proportional diversity allocation). With a sufficiently wide augmented
candidate set, the exact joint candidate and outcome are both present. Its
unchanged candidate-specific FastViability upper is `6/23/29`, above the selected
day-1 path's `6/16/21`; the evaluator therefore exposes useful future signal once
the candidate exists. This is evidence of a bounded capability-wiring/retention
gap, not evidence for globally raising portfolio or master caps.

Local elapsed time and memory are research-instrument observations only, not
performance evidence. The 097 holdout remains sealed. Verdict:
`accepted-attribution`. Any production candidate must use a fresh score split,
preserve all existing routes and master candidates, expose the missing class by
a public structural rule rather than a fixture-specific width, and keep the
5000-ms internal hard cap. No threshold, seed, family, opponent or replay tuning
is authorized.
