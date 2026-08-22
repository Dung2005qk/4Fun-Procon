# DEADLINE-DOMINANCE-ANYTIME-187

Date: 2026-08-22

Parent: `0f01d69`

Frozen manifest:
`research/holdouts/DEADLINE-DOMINANCE-ANYTIME-187.csv`

SHA256:
`6598C60159E1397803B50FE8211BFA7B098CB1FF078FD01DE2032FDECB7583F6`

## Gap

The competition artifact clamps all compute at 5000 ms. Experiment 166 proves
that longer search can improve 32x32 score, while 184 proves that direct or
sampled-scenario takeovers can regress the closed loop. Experiment 186 proves a
sound forward-simulation admission relation but finds zero qualifying candidate
in 634 ordinary Long-pool alternatives. The untested capability is a generator
that searches inside the relation by construction.

## Mechanism under test

1. Freeze and exact-validate the canonical 5-second incumbent.
2. On the terminal day, continue additive search and select only a strictly
   better dual-validated official score.
3. On earlier days, retain protected-terminal exact routes and team candidates
   that satisfy the complete 175/178/186 simulation relation.
4. Return the exact 5-second incumbent on incomplete search or failed dominance.
5. Measure whether the same protected incumbent strengthens exact-root ordering
   for 185; do not reduce its action or opponent space.

## Gates

- Consumed 166/184 large-map states establish witness existence and reproduce
  the known regression as a protected no-regression control.
- Completed exact-178 anchors establish agreement with the exact boundary
  relation.
- Fresh stratified development remains sealed until at least one natural strict
  terminal or liftable nonterminal witness exists.
- Holdout and BTC remain sealed until fresh development passes.

## Functionality preservation

No designed functionality is deleted, disabled, deferred or reduced. Nothing is
deleted, so no canonical-equivalent deletion proof is required.

## Results

### Consumed existence gate

The terminal-only continuation was exact-valid but produced no strict takeover
on consumed seeds `4310000`, `4310100`, `4310200`, and `4411000`.  The known
Long-search gap is therefore nonterminal on these states.

The state-preserving WAIT-detour generator found a strict witness on consumed
seed `4411000` (32x32, 10 days, 8 agents, high-stock family).  In the paired
virtual-parent run:

- virtual 5-second continuation: `6/60/466`;
- protected actual continuation: `6/60/467`;
- W/T/L against its in-run virtual parent: `1/0/0` at the final score;
- invalid/emergency: `0/0`;
- takeover: day 6, cumulative `6/36/281 -> 6/36/282`;
- witness: patrol 0, WAIT 28 at cell 660, 12-step off-road round trip through
  spot 756, final patrol fuel `125 -> 197`;
- the exact simulator and independent validator agreed, raw road footprint was
  equal, terminal cells/kinds were equal, and protected state/ledger assertions
  held after every later day.

Consumed low-fuel `4310100` and high-fuel `4310200` were exact ties with zero
takeovers: respectively `6/60/433` and `6/60/527` for both actual and virtual
parent.  Absence of a witness leaves the canonical parent unchanged.

### Fresh stratified development

The pre-registered development matrix opened only after the consumed witness.
It covered 14/20/26/32 maps, 5/7/8/10 days, 4/4/6/8 agents, all six map
families, low/default/high fuel, and fixed/native role selection.  Every run
contained its own 5-second virtual parent, so cutoff noise between separate
executions cannot create a paired win or hide a loss.

| tier | fixed W/T/L | native W/T/L | total gain/loss |
|---|---:|---:|---:|
| 14x14 easy | 5/4/0 | 2/7/0 | +15/0 servings |
| 20x20 medium | 5/4/0 | 2/7/0 | +8/0 servings |
| 26x26 hard | 2/7/0 | 1/8/0 | +4/0 servings |
| 32x32 very-hard | 2/7/0 | 1/8/0 | +4/0 servings |
| **total** | **14/22/0** | **6/30/0** | **+31/0 servings** |

Across 72 paired fixtures the result was `20/52/0`, with zero invalid,
emergency, protected-state failure, protected-ledger failure or protected-phase
deadline.  Lifetime and cumulative daily distinct never decreased; all strict
gains were servings gains that persisted to the final match score.

Terminal continuation produced `0` takeovers over four consumed and eighteen
fresh easy/medium fixtures and is closed as inert for this candidate.  The
nonterminal WAIT-detour mechanism passes development and opens only the frozen
holdout.  No threshold or routing rule was tuned from development.

The attempt to allocate a separate GCP Spot VM for holdout failed before
creation because the project-wide 12-vCPU quota was fully consumed.  Holdout
therefore runs sequentially; local elapsed time has no verdict authority.

### Frozen holdout

The sealed holdout was opened only after development closed. It used 18 fresh
seeds per tier and both fixed and native role selection (144 in-run paired
comparisons). The generator, dominance relation and 30-second protected-phase
budget were frozen before the first holdout result.

| tier | combined W/T/L | gain/loss |
|---|---:|---:|
| 14x14 easy | 11/25/0 | +16/0 servings |
| 20x20 medium | 10/26/0 | +14/0 servings |
| 26x26 hard | 6/30/0 | +9/0 servings |
| 32x32 very-hard | 9/27/0 | +16/0 servings |
| **total** | **36/108/0** | **+55/0 servings** |

Fixed roles were `25/47/0` with +39 servings; native roles were `11/61/0`
with +16. Strict gains occurred in low, default and high fuel and across all
six map families. No fixture lost lifetime distinct, daily distinct or
servings; invalid, emergency, state-relation failure, ledger-relation failure
and protected-phase deadline counts were all zero. The largest observed final
fixture gain was +4 servings.

The logic candidate passes its frozen paired-score holdout.

### Canonical runtime integration

Production uses one delayed submission, not a same-day resend. The canonical
5-second decision is kept as a virtual parent. Before serialization,
`ProtectedSlackRefiner` may replace one patrol WAIT interval by an off-road
round trip that returns to the same cell at the same time. Admission requires:

- exact simulator and independent-validator agreement;
- the same ordered terminal agent kinds and cells;
- no lower patrol fuel;
- the exact same road footprint;
- lifetime-brand superset and componentwise no-lower cumulative daily distinct
  and servings, with a strict daily-distinct or servings gain.

After a protected takeover, the bounded solver continues from the virtual
parent state. Its plan is exact-simulated and independently validated on the
authoritative richer state before submission. If the authoritative state no
longer forward-simulates the virtual parent, runtime resets to the authoritative
state instead of continuing the protected lane.

The first integration retained the virtual parent even before any takeover.
BTC `m-3876` exposed why that was wrong: the server returned a richer fuel state
on the final transition although the refiner had never activated. Runtime now
tracks `protectedDivergenceActive`; authoritative state is used directly until
an improved protected plan is acknowledged. Match `m-3880` then demonstrated
the corrected state machine: inactive on days 1--6, one accepted takeover on
day 6, active on days 7--10, and no state or ledger failure.

The additive refiner is charged to the unused part of the same 5000-ms
competition compute budget. Its deadline is the minimum of the local 5000-ms
deadline and the authoritative action deadline, minus the unchanged network
reserve. A 15/45/60-second outer server window never increases compute.

### BTC target-host gate

All six matches used the authenticated HTTP lifecycle. Bot rank is excluded
from promotion evidence; these rows have authority only for deadline, transport,
validity, transition and active-path attribution.

| match | configuration axis | accepted | parent/actual final score | takeover | max response |
|---|---|---:|---:|---:|---:|
| `m-3876` | default fuel, 12 spots, 15 s outer | 10/10 | `6/60/354 -> 6/60/354` | 0 | 3646 ms |
| `m-3877` | high fuel, 24 spots, 15 s outer | 10/10 | `6/60/458 -> 6/60/458` | 0 | 3599 ms |
| `m-3878` | low fuel, 24 spots, 15 s outer | 10/10 | `6/60/294 -> 6/60/296` | 2 | 3556 ms |
| `m-3879` | low fuel, 24 spots, 15 s outer | 10/10 | `6/60/277 -> 6/60/278` | 1 | 3633 ms |
| `m-3880` | low fuel, 24 spots, 15 s outer | 10/10 | `6/60/362 -> 6/60/363` | 1 | 3639 ms |
| `m-3881` | default fuel, 12 spots, 5 s outer | 10/10 | `6/60/347 -> 6/60/348` | 1 | 2466 ms |

Replay-check dual-validated all 60 accepted day plans. Five of the six replays
reconciled every transition; `m-3876` reconciled 8/9 because the authoritative
final fuel was richer than the predicted state, the mismatch that produced the
state-machine repair. On every non-takeover day in the corrected `m-3880` and
`m-3881` runs, serialized actions were byte-identical to the virtual-parent
decision. Only the registered takeover day differed.

`m-3881` is the decisive hard-cap activation gate. The state arrived with only
3821--4013 ms left in the authoritative window. Day 8 used a 57-step WAIT at
cell 973 for a 30-step off-road round trip through spot 840, retained fuel 200,
and improved the day from 6/28 to 6/29. Total response was 2439 ms on that day;
the maximum over the match was 2466 ms. The final actual score retained the one
serving gain over its in-run virtual parent.

`m-3877` ranked second, but the refiner generated no takeover and every submitted
action was byte-identical to the parent decision. It is therefore a new dense
high-fuel incumbent counterexample for a separate score experiment, not a
regression caused by experiment 187.

The final MSVC build and unit suite passed. Frozen BTC executable SHA256:
`AE761118A7DD086B211FF9BB3CD99EBD0DBD3FEED332E9511E23742F25B8F356`.

## Verdict

Accepted. The candidate is a strict additive improvement over the protected
parent: fresh development `20/52/0`, frozen holdout `36/108/0`, BTC active-path
gain under the 5000-ms hard cap, zero invalid/emergency, and no action change
without an exact componentwise-dominant witness. Reopen this mechanism only for
a broader protected neighborhood registered as a new experiment; do not tune
the consumed WAIT-detour fixtures.
