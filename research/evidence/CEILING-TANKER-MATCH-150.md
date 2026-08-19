# CEILING-TANKER-MATCH-150

Date: 2026-08-13
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/CEILING-TANKER-MATCH-150.csv`
SHA256: `3120848D5D5642E44467B20903694FBBBA7F89AAA168F0844F8E7251C1B0ECA4`

This is the first valid-config successor for the independent full-match tanker
question. Before freeze, the configuration was checked against every published
invariant: four days, 16 steps/day for an 8x8 map, three agents, three spots,
distinct non-spot starts, positive fuel and no road-information leakage.

The research oracle is the complete joint step/full-match DP already designed
for 148/149. It carries positions, fuels, terminal refuel, lifetime brands and
official accumulated score across days, resets only daily stock/visited state,
and reconstructs a whole-match witness for exact simulator plus independent
validator agreement. Production source remains unchanged.

## Development result

Seed `3200000` was started alone with `--details`. It emitted no completed-case
record before the 900-second research-wrapper limit. The wrapper timed out and
left the still-responsive oracle as PID `2616`; the exact PID was verified and
terminated. No score, witness, invalidity or HEAD comparison was produced.

Verdict: infrastructure-inconclusive. This is not a tie or loss. The fixture
must not be weakened and the holdout must not be opened. A rerun is admissible
only after a mathematically exact, semantics-preserving state-quotient or
equivalent oracle optimization is proved; otherwise close this direction as
proof-infeasible.

The exact audit then introduced only two proof-preserving quotients: canonical
ordering of the interchangeable patrol pair with witness remapping back to
physical identities, and a `(positions, fuels)` daily-transition cache after an
explicit assertion that all four days have identical 16-step roadless/stock
dynamics. No action or state frontier was capped.

The admissible rerun completed:

`rendezvous-chain,low,3200000`: tie; HEAD `3/12/12`, oracle `3/12/12`;
step frontier `69608`; match frontier `893`; settled `87733918`; zero
invalid/incomplete; result hash `df8373f677ed51ae`.

`split-duty,low,3200100`: oracle win at tier 3 by `+1`; HEAD `3/12/15`,
oracle `3/12/16`; step frontier `136549`; match frontier `1437`; settled
`426359927`; zero invalid/incomplete; result hash `45ada1153ec4107b`.

Development stopped immediately at exact-vs-HEAD `1/1/0`. The remaining ten
development seeds and sealed holdout stay unopened. This disproves convergence
for the full-match tanker/refuel suffix domain. Attribute the consumed witness
before any production candidate.

## Consumed-witness attribution

The exact rerun reproduced result hash `45ada1153ec4107b` and emitted per-day
plans, cumulative scores, terminal states and HEAD audit membership. The first
divergence is day 1: oracle `3/3/4`, HEAD `3/3/3`. Days 2--4 preserve that one
serving gap while both collect all three daily brands, ending `3/12/16` versus
`3/12/15`.

The oracle day-1 stable plan is absent from all 16 HEAD candidates. Thus the
earliest causal gap is before master/certification: the joint patrol--tanker
candidate portfolio never generates the immediately lexicographically better
all-stock route. Suffix evaluation is not the cause of this counterexample.

## Holdout result

Sealed.
