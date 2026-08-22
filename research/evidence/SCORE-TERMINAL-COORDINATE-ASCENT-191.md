# SCORE-TERMINAL-COORDINATE-ASCENT-191

## Registered scope

- Parent: `994c33a`.
- Frozen manifest:
  `research/holdouts/SCORE-TERMINAL-COORDINATE-ASCENT-191.csv`.
- SHA256: `0B6B9F89B4B3605A3F70E5BF7BEE60716B88EE3F941E4B40E806C07EE3ABAC35`.
- Official comparison: lifetime distinct, cumulative daily distinct, servings.
- Internal compute cap: 5000 ms. Local elapsed is not performance evidence.

## Consumed BTC counterexample

Experiment 190 submitted `m-3896` at `6/60/407` after raising its virtual
parent from `6/60/402`. Reapplying the unchanged final-day sidecar to that
already submitted plan yields `6/60/410`: 191 of 191 generated plans are exact-
simulated and independently validated, with two strict improvements and no
deadline or failure.

The best exact-valid one-agent exchange on each immutable replay state is:

`36->39, 79->82, 122->125, 166->169, 210->212, 252->254, 288->291,
326->329, 365->367, 407->410`.

The current transition/footprint-preserving subset improves only days 2 and 6.
The repeated final-day gain therefore identifies a one-round combination limit,
not a failure to generate sparse routes and not evidence from the bot rank.

The implemented round-frozen coordinate ascent passes both consumed gates:

- `m-3896`: baseline `6/60/407`, protected round 1 `6/60/410`, fixed point
  `6/60/413`; 3 accepted rounds, 768 route evaluations, 665/665 generated plans
  valid, 5 strict record improvements, no deadline.
- `m-3877`: baseline `6/60/458`, protected round 1 `6/60/460`, fixed point
  `6/60/464`; 5 accepted rounds, 1,344 route evaluations, 1,174/1,174 generated
  plans valid, 6 strict record improvements, no deadline.

Round 1 exactly reproduces experiment 190 on both independent BTC replays. The
candidate therefore passes the consumed causal and parent-preservation gate and
may enter fresh development.

## Proposed invariant

Round 1 remains exactly the accepted experiment-190 result. Later rounds reuse
the same route set. A round freezes its incumbent, checks every one-agent
replacement against that same base, and applies only the best strict official-
score improvement after simulator/validator agreement. The best certified plan
survives timeout, failure and no-gain exits. Final day and all existing eligibility
guards remain unchanged.

## Functionality-preservation answers

1. The candidate removes, disables, defers or reduces no designed function.
2. Nothing is deleted; the experiment-190 result is a protected incumbent.

## Fresh development gate

The frozen development split completed all 54 registered fixtures. Comparing
the protected experiment-190 first round directly with the coordinate-ascent
fixed point produced `27/27/0` wins/ties/losses and `+251` aggregate servings,
with no lifetime- or daily-distinct change. The largest per-fixture gain was
`+22` servings.

The gains are distributed across both fuels (`13/14/0` default and `14/13/0`
high), both role modes (`13/14/0` fixed and `14/13/0` native), and every
generated family. By map tier the results are easy `0/12/0`, medium `4/8/0`,
hard `11/4/0`, and very hard `12/3/0`; the easy dense-feasible controls remain
exact ties.

All 25,154 generated plans were independently valid. There were zero invalid,
emergency, sidecar-failure and sidecar-deadline cases. The development log
SHA256 is
`2DE9C68AFD203C7109410CF758BB2738E16CD6939AF83D4EB987716720FF4F18`.

This clears the preregistered development gate without changing the mechanism.
The frozen holdout may now open. Experiment 185 is independent and unchanged.

## Frozen holdout gate

The untouched holdout completed all 108 registered fixtures. Relative to the
protected experiment-190 first round, coordinate ascent produced `49/59/0`
wins/ties/losses and `+412` aggregate servings, with zero lifetime- or
daily-distinct change. The largest per-fixture gain was `+26` servings.

Results by tier were easy `0/24/0`, medium `7/17/0`, hard `22/8/0`, and
very hard `20/10/0`. Both fuels (`24/30/0` default and `25/29/0` high), both
role modes (`23/30/0` fixed and `26/29/0` native), and all six traffic/map
families contain wins. The 12-spot dense-feasible lane tied `34/34`; every
sparser unsupported spot lane from 14 through 30 contains wins.

All 39,653 generated plans were independently valid. There were zero invalid,
emergency and sidecar-failure cases. Twelve fixtures exhausted the local
sidecar slice only after preserving the best certified incumbent; none lost to
round 1. Their local elapsed time has no performance authority and is carried
to the required BTC 5000-ms gate rather than hidden or used to tune the code.

The holdout log SHA256 is
`185A5986FD99B2505E043986A1FE12D3A297CC625C3DAA078E583A9B2B8AE086`;
the stdout copy is byte-identical and stderr is empty. The frozen holdout gate
is cleared without source or threshold changes. BTC target-host validity,
lifecycle and telemetry remain the final gate.

## BTC target-host gate and verdict

Authenticated fresh match `m-3897` used hard difficulty, one bot, ten days, a
32x32 map, 100 steps/day, 5000-ms response, eight agents, 30 spots, six brands
and high fuel. All ten submissions received HTTP 200 valid acknowledgements;
the maximum reported response was 2,888 ms. Replay-check accepted all ten plans
and the independent validator agreed on every day, with nine authoritative
state reconciliations and no invalid action.

On day 10 the protected experiment-190 first round reached cumulative
`6/60/432`. Coordinate ascent evaluated 311/311 valid generated plans from 376
sparse routes, recorded four strict improvements, accepted three rounds and
submitted cumulative `6/60/435`. The sidecar reached its local slice only after
locking that stronger certified incumbent; the valid submission remained well
inside the 5000-ms hard cap. This is a target-host multi-round takeover, not a
rank inference. The final rank and bot score are excluded from promotion.

Replay SHA256 is
`9A528FD0E5AC81B0A7A30E26AC5B7AB25AF76CA640EDC5D97ABC1A868176E9A8`.

Experiment 191 is accepted. It strictly generalizes experiment 190's terminal
sidecar, protects the complete first-round result, produces no paired loss on
development or holdout, and demonstrates an incremental target-host gain under
the authoritative cap without adding a route search or dispatcher. The accepted
implementation commit is `c3ee753`.
