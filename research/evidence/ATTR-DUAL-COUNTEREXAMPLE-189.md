# ATTR-DUAL-COUNTEREXAMPLE-189

Date: 2026-08-22

Parent: `5c3aa7a`

Frozen consumed manifest:
`research/holdouts/ATTR-DUAL-COUNTEREXAMPLE-189.csv`

SHA256:
`1A73B1403AFEE15D37E8E3FF1B981BE22D03195DDE4CC7C7F4063B1910727C2F`

## Exact lane: seed 1721100

The completed experiment-185 slice exposes four day-1 root actions whose exact
robust closed-loop score is `5/17/18`. Current production already chooses root
action index 48, plan `3.2.2.2.0.0.2.-3`, which is one of those four exact root
actions. Current maximum-dwell and status-toggle continuations nevertheless end
at `5/17/17`; minimum-dwell ends at `5/16/17`.

The missing serving is therefore not caused by absent day-1 route generation or
root selection. It is lost after the selected root, in later closed-loop
replanning and realization. Experiment 187 is inert on this fixture and does not
cause the loss.

## BTC lane: m-3877

The immutable replay has 24 spots, 8 agents with one tanker, high fuel, 10 days
and 100 steps per day. All ten submitted plans are dual-valid and acknowledged.
Current production ends at `6/60/458`; the BTC bot ends at `6/60/512`. Rank is
not used as evidence, while the 54-serving deficit remains a counterexample
signal.

Every day reports an incomplete, deadline-reached bounded master. Exact
orienteering reports zero supported and zero complete agents on every day, and
the protected-slack refiner generates zero candidates. Code attribution shows
that both exact high-fuel and exact resource enumeration reject configurations
with more than 16 spots; on 32x32, the independent `mask x cell` state-count cap
is already exceeded above 13 spots.

A sound step/fuel reachability pass over every authoritative replay state found
all 24 spots individually reachable by every patrol on every day. Filtering
unreachable spots therefore cannot reduce this fixture to the exact-feasible
domain without changing semantics. Raising the dense threshold is also
infeasible: `2^24 x 1024` states is outside the bounded runtime and memory class.

## Relationship and verdict

The two gaps are real but not the same causal capability. Seed 1721100 already
has an exact-feasible six-spot frontier and loses value after a correct root.
BTC m-3877 is a dense-frontier capability boundary before exact route guidance
can start. They remain separate research lanes.

Experiment 189 closes `accepted-attribution-unrelated-gaps`. Experiment 185
continues streaming exact root certificates. The BTC lane opens a separate
research-only sparse-frontier causal probe; it must not alter the 1721100 oracle
space, tune m-3877, or claim that the BTC deficit is recoverable until an exact-
valid route exchange produces a strict official-score gain.

## Functionality preservation

No production function or call path changed. No designed logic was deleted,
disabled, deferred or reduced.
