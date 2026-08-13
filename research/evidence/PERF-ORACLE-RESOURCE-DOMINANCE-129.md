# PERF-ORACLE-RESOURCE-DOMINANCE-129

Date: 2026-08-13  
Production parent: `5dccb0f` (unchanged)  
Scope: research full-match oracle only

## Exact quotient

At identical canonical unordered patrol terminal positions, state `R` dominates
state `L` only if both patrols retain at least as much fuel, lifetime brands are
a superset, accumulated daily distinct and servings are no worse, retained own
traffic footprint and resulting road status are componentwise no worse, and at
least one field is strict. `R` can replay every future action sequence feasible
from `L`: positions are identical, greater fuel cannot invalidate an action,
no-worse traffic cannot increase movement cost, and score/lifetime objectives
are monotone. The exact witness of `R` is retained for reconstruction.

No production file, action space, transition, score, fixture, fuel, horizon,
topology or validator semantics changed.

## Frozen equivalence

The quotient binary was compared with the pre-129 probe on every completed
development oracle score:

- CEILING-MULTI-PATROL-085: 18/18 exact scores equal;
- CEILING-BRANCH-PATROL-089: 18/18 exact scores equal;
- CEILING-CYCLE-PATROL-095: 18/18 exact scores equal.

For 095, the four frozen exact-win scores matched the recorded evidence. The
other fourteen were frozen oracle=HEAD ties; a detached `f77c101` reference
using only the 095 fixture adapter and a head-only path reproduced all fourteen
HEAD scores, all valid. Thus all 54 prior exact scores are preserved.

Two whole-suite pre/post orchestration attempts timed out because buffered old
probe output was lost; they supplied no result. Child processes were identified
by exact executable path and stopped. They are not part of the equivalence
evidence.

## Ladder result and verdict

The only authorized ladder fixture, balanced/low seed `1800000`, completed with
oracle `6/24/26` versus HEAD `6/23/25`: a dual-valid tier-2 exact gain of one,
maximum exact frontier 3976 and 222 cached day enumerations. The result hash is
`94088825f11f84c5`. The 101 holdout and remaining development rows stay sealed.

Accepted research infrastructure. The quotient removes only proof states with
a mathematically dominating exact continuation and changes no production path.
Local elapsed is research-infrastructure telemetry only, never BTC/competition
performance evidence.
