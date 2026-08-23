# DEADLINE-STRATIFIED-ANYTIME-184

Parent: `828ea78` (competition 1.0 source checkpoint)

Frozen manifest: `research/holdouts/DEADLINE-STRATIFIED-ANYTIME-184.csv`

SHA256: `53C1A08A0686D6D5D716EAAE7FC306D47414BA508BDFB07906447BAC72F1D136`

## Question

Experiment 166 showed that replacing the 5-second plan with a standalone Long
plan could improve some 32x32 fixtures, but it also regressed smaller fixtures.
Experiments 167, 168 and 183 protected the 5-second incumbent, yet their strict
lower-versus-upper proof never authorized a takeover. Experiment 184 tested
whether public response slack could be used safely without weakening the
official lexicographic objective or routing by seed, family, fuel or opponent.

## Public configuration boundary

The authenticated PTIT practice form exposes maps up to `32x32` and a response
window from `200` through `15000 ms`. The user-supplied UET table also stops at
`32x32`, although it lists `45000/60000 ms` windows. UET is an external
reference and is not PTIT authority. Consequently `32x32` is the maximum
competition-relevant map side for this experiment; extrapolation beyond it
would not support a current deployment decision.

## Development gates

### Strict protected proof

The 5-second incumbent was recorded before a 10-second refinement. On consumed
seed `4310000`, the refinement completed three greedy and three guided wide
trajectories but produced no strict lower-witness improvement and no certified
takeover. On high-stock seed `4411000`, the sound spatiotemporal upper tightened
all ten day states and the refinement completed the registered work, but again
produced no takeover. This preserved the negative conclusions of 168 and 183.

### Deferred comparator without a transition invariant

Allowing the ordinary comparator to replace the incumbent before one delayed
submission selected a different plan on seven of ten days on `4310000`, but the
closed-loop score moved from an adjacent 5-second `6/60/355` to `6/60/353`.
Further attribution found that running a rejected second solve on the same
engine also mutated internal belief/cache state. This construction was invalid
as a protected parent comparison and was discarded.

### Transition-preserving slack gate

Two independent instances of the same canonical solver were advanced with the
same selected history. One produced the 5-second incumbent; the other used the
remaining 10 seconds. A challenger could replace the incumbent only when its
exact score was better and its final agent states and road footprint were
identical.

- `4310000`: 9 different challengers, 9 transition rejections, 0 selections.
- `4411000`: 9 different challengers, 9 transition rejections, 0 selections.

The proposed post-search slack harvester therefore had no usable candidate on
the two strongest consumed large-map gates.

### Paired scenario Actual-vs-Actual gate

The final development candidate compared both certified profiles on the same
scenario IDs, classes and weights. It accepted a challenger only if every
scenario score was lexicographically no worse and at least one was better.

- `4310000`: 8 different challengers, 1 paired takeover.
- `4411000`: 8 different challengers, 5 paired takeovers.
- `4411000` candidate repeated exactly at `6/60/454` with five takeovers.
- Adjacent 5-second controls were `6/60/461` and `6/60/462`.

The 7--8 serving regression reproduced with zero invalid or emergency plans.
The paired manifest therefore does not cover the full endogenous closed-loop
consequence of a same-day replacement. Actual-vs-Actual is less conservative
than the valid-upper proof, but it is not a monotonic safety certificate.

Local elapsed and cutoff-dependent scores are not BTC performance evidence;
they are admissible here only as development falsification. Because the
candidate was falsified before promotion, all sealed holdouts remained closed
and no BTC match was created for it.

## Verdict

`rejected-closed-loop-regression`.

Extra response time is not activated in the competition artifact. The canonical
5-second cap remains correct. Direct Long has isolated search value, but every
tested safe integration is either inert or admits a reproducible closed-loop
regression. A 45/60-second UET window cannot override the authenticated PTIT
maximum of 15 seconds and cannot justify a speculative production branch.

Reopen only if PTIT publishes an authoritative window above 5 seconds and a
complete closed-loop certificate or exact evaluator first eliminates the
`4411000` regression on consumed evidence, then shows distributed paired upside
on fresh development. Do not reopen by weakening the scenario gate, tuning a
public dispatcher to these seeds, or researching maps larger than 32 without a
new authoritative competition configuration.

All experiment-only planner, deadline, comparator, harness and bound source was
removed after rejection. Production source is content-identical to HEAD.

## Functionality-preservation check

1. No designed production functionality was removed, disabled, deferred or
   reduced. The 5-second competition path was the protected parent throughout.
2. No canonical implementation was deleted. Only rejected experiment-only
   instrumentation was removed after its development gap was closed.
