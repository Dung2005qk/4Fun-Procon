# UDON-SHIELD Research State

Updated: 2026-08-01

## Current phase

Minimal governance bootstrap followed by forward research from frozen parent
`6f84a06`. Re-running or rebuilding every historical checkpoint is not a research
objective; old checkpoints remain lane champions for targeted A/B only.

## Authoritative budget evidence

- The rules allow response time to vary per match.
- Standard team-vs-team match `m-1042` displayed `60000 ms` per day. This is the
  outer server window, not permission to increase the internal compute budget.
- BTC preflight `m-1181` used `5000 ms`; current HEAD passed connection, assignment,
  valid-plan and deadline checks, with slowest response `279 ms` and final rank `#1`.
- User operating decision on 2026-07-31: `5000 ms` remains the primary hard cap for
  solver and role selection. No candidate may be promoted using work that exceeds it.
- Existing `500/1200/2500 ms` local lanes are diagnostic degradation lanes. They are
  not production-selection gates unless BTC announces a matching deadline.

## Frozen checkpoints

- `6132f41` — baseline-btc
- `02df79d` — exact-highfuel general-score plateau
- `6f84a06` — current correctness/proof checkpoint

The complete immutable mapping is `old/CHECKPOINTS.csv`.

## Active gap

Promotion semantics were corrected by the user on 2026-08-01. A comprehensive
win means a sufficiently large, broadly distributed paired advantage with bounded
downside; it does not require zero losses on every map and it is not a fixed W/L
ratio. Exact dominance remains preferred. Invalid/emergency output and exceeding
the 5000 ms internal cap remain hard blocks. Every match is still compared by the
official lexicographic vector; the global verdict reports W/T/L, first differing
tier, gain/loss magnitude and downside distribution without a weighted sum.

`SCORE-ROLE-018` and the first measurement definition in `SCORE-ROLE-019` are
closed. `SCORE-ROLE-020` passed its scoped low-fuel screen without causal downside,
but the same evidence exposed long-horizon timed role instability outside low fuel.
`SCORE-ROLE-021` passed its frozen local score screen. Fresh BTC high-fuel match
`m-1257` then exposed a hard lifecycle/runtime block, and performance-only
`PERF-DEADLINE-002` repaired that block without changing solver, role, comparator
or action semantics. The combined candidate passed the preregistered fresh
high-fuel BTC target in `m-1258`; it is now frozen for final scoped-diff review and
promotion/commit gating. No further role or submission-floor tuning is permitted
against these opened fixtures.

The read-only semantics attribution `SEM-REFUEL-001` is accepted. On
`m-1258`, transitions to days 9 and 10 reported a patrol at the simulator-predicted
terminal position but with full fuel `300` instead of predicted `185` and `194`.
In both cases the patrol first arrived on the tanker cell at step 100; the local
trace did not count a completed full co-location step. A scan of all 37 archived
live replays found no second match with this exact terminal-arrival/full-refuel
pattern. Therefore neither the documented sustained-docking rule nor a server
day-boundary refuel rule is yet established. No simulator or planner change is
authorized until a preregistered BTC conformance match isolates this boundary.
That match, `m-1261`, did so: assignment `[tanker, patrol, patrol]` was accepted;
one patrol first reached the stationary tanker at terminal step 20 and another
arrived at step 19 then waited one step. Both began day 2 on the tanker cell with
full fuel 20. The terminal-arrival patrol would have retained fuel 14 under both
local engines. The active candidate is now `SEM-REFUEL-002`, which may change only
the next-day terminal fuel transition after all current-day semantics are complete.
It must not grant fuel to an action within the day or alter score/traffic/claims.

`SEM-REFUEL-002` is accepted. Both engines implement the boundary independently;
unit tests pass, and rescanning all 37 live replays improves `m-1258` transition
agreement from 7/9 to 9/9 without introducing any mismatch. Its frozen paired
score matrix is 8 wins, 11 ties and 5 losses with zero invalid/emergency. Every
loss is tier 3 and at most five servings; the candidate has one tier-2 win and
low-fuel tier-3 gains of 86, 24 and 1. Fresh explicit-advanced BTC `m-1264`
selected mask128, submitted ten HTTP-2xx valid plans, reconciled all 9/9 state
transitions and ranked first at `6/60/261` versus `6/60/202` next. Maximum solver
time was 2845 ms, replayed role selection 3956 ms, and exact deadline overrun,
submission skip, server WAIT and emergency were all zero. Replay SHA256 is
`2811EC26C075829D63F61B7949394470B568CFDA72F6B30A51838CF9ACD4540A`.

BTC `m-1228` (hard, 3 bots, 32x32, 10 days, 100 steps/day, 8 agents,
12 spots, 6 brands, fuel 200, response 5000 ms) is the authoritative new
counterexample. SCORE-ROLE-016 selected mask `2`, but day 1 column generation
spent `7342 ms` in exact orienteering (`2111440` settled states, seven complete
patrol agents) against a `4353 ms` total solver profile, so the runtime skipped
submission. Day 2 later received a stale-day `E_STEP_OVERFLOW`; replay-check
shows the plan is exact-valid against the state it was built from, so the first
confirmed fault is the unbounded exact-orienteering stage/lifecycle deadline, not
route semantics. Final live result was rank 4 at `6/48/217` versus rank-1
`6/60/281`. Invalid/deadline behavior is a hard block: preserve SCORE-ROLE-016
as a frozen patch/binary, restore source to `6f84a06`, and fix the performance-only
deadline path before resuming role-score comparison.

`PERF-DEADLINE-001` is the only active implementation axis. Attribution on the
frozen parent path first showed that exact route enumeration accepted the search
deadline while team coordination/finalization could continue after it. A first
cooperative-cancellation candidate returned promptly but lost the exact bundle and
regressed `m-1228` day 1 from `6/36` to `6/16`, so that intermediate form was
rejected. Deeper telemetry then isolated two exact performance defects without
changing the search space: deadline polling used `settledStates`, so stale queue
entries could run without checking the clock, and the team feasibility DFS visited
`3,000,181` nodes even after the local incumbent had reached the proven absolute
upper bound over all positive-stock brands and `sum(min(stock, patrolCount))`.

The current candidate polls by processed queue/bucket entries, enumerates independent
high-fuel patrol starts on at most four workers, caches only exact hex distances,
and skips feasibility only when the incumbent equals that mathematical upper bound.
Team beam/local/pairwise work remains cooperative and retains only fully constructed
incumbents. On the frozen `m-1228` day-1 replay at the live-observed `4353 ms`
available window, candidate and parent are byte-identical for all eight agents and
both score `6/36`; candidate exact attribution was `187 ms` total (`137 ms`
enumeration, `50 ms` finalization), zero deadline overrun and `0` feasibility nodes,
with total solve `2729 ms`, leaving `1624 ms` for the `1600 ms` HTTP floor. Local
targeted equivalence/expired-deadline tests pass. BTC target-host `m-1235` then ran
the candidate on the preregistered explicit advanced configuration: hard, three
bots, 10 days, 32x32, 100 steps/day, 5000 ms/day, eight agents, 12 spots, six
brands and medium fuel 2x (`200`). All 10 submissions were HTTP 200 and valid,
with zero emergency, zero exact deadline overrun and a maximum response of
`2647 ms`. The independent replay validator agreed on every day and reconstructed
the final official score `6/60/337`; the candidate ranked first over bots at
`6/60/293`, `6/60/261` and `6/60/255`. Replay SHA256 is
`73F36180715F6E9ADF1E2FAB6199118461DABECC758689DA962D8B7361F879CD`.
This clears the fresh BTC runtime hard block for the medium-fuel lane, but does
not grant commit authority until the replay joins the frozen semantic equivalence
suite and the protected comparison is complete. No route cap, score comparator,
search width, role logic or simulator semantics changed.

`SCORE-ROLE-001` was rejected. BTC replay `m-1143` is a 32x32, 3-agent,
13-brand, 4-day map.
The live `6f84a06` candidate selected all patrols and finished `13/26/29`; after
day 2 its patrol fuel was `1/0/5`, limiting days 3-4 to `3/3` each. On the same
authoritative replay, the frozen binary with tanker mask `4`, a 5000 ms cap, the
exact simulator and the independent validator produced `13/29/29`. This is a
development counterexample, not a promotion result, because later replay traffic
contains the original team's footprint.

The earliest demonstrated mismatch remains pre-match role evaluation. `replay-roles`
ranks all-patrol first at `13/25/27` and mask `4` fourth at `13/20/21`, while the
day solver counterfactual reverses them. The role rollout uses a narrower portfolio
than production: in the timed path at most 4 columns and 6 targets per agent, no
production seed plan, shallow harvest extension, and 256 master combinations. The open
axis is to improve role-rollout fidelity within the same 5000 ms cap, not to route
by map identity or to prefer tankers heuristically. Replacing the narrow rollout
with a wider one was not monotonic: on `m-1138` it changed the selected mask from
`16` to all-patrol and exact replay score collapsed from `6/60/326` to `6/45/140`.
The wider search made coordinated tanker roles harder to solve before their shared
deadline, so its incomplete score cannot overwrite a stronger narrow-rollout witness.

`m-1138` is now a development counterexample. The previous six-replay holdout was
opened for role ranking during the rejected experiment and cannot be reused to tune
its successor. `SCORE-ROLE-002` preserved the development lanes and tied the first
six-replay exact holdout, but was rejected on the unchanged secondary holdout:
`m-0929` switched tanker mask `8` to `32` and fell from `6/60/367` to `6/60/357`.
Repeated role scans also changed the top mask on the same binary, confirming that
wall-clock cutoffs are influencing role rank. The next role candidate must be
operation-bounded or deterministic before score tuning continues.

A deterministic central-one-tanker policy was falsified before implementation.
It retained the known `m-1143` gain (`13/29/29`) and scored `6/60/338` on
`m-1138`, but on `m-0929` central mask `128` scored `6/60/362`, below parent
mask `8` at `6/60/367`. Therefore centrality may seed an operation-bounded beam
but cannot select the production role by itself.

`SCORE-ROLE-008` combined a complete fixed-operation role pass with additive
post-refuel harvest extensions. It completed the `m-1143` role gate in `544 ms`
without a deadline flag, but still ranked all-patrol `13/27/27` above mask `4`
`13/23/23` and mask `2` `13/21/21`. The candidate therefore failed the first
pre-registered development gate and was reverted without opening another map.
This rules out the unsupported claim that merely enabling post-refuel extension
inside a complete pass repairs tanker valuation. The next step is attribution of
the exact stage where a valid refuel-enabled witness disappears or loses rank,
not another parameter or portfolio-width change.

`SCORE-ROLE-009` instrumented that path without changing its intended ordering.
One full pass showed mask `4` with `14` eligible post-refuel sources, `3` generated,
`3` retained and `0` selected. When atomic companion retention was added, two runs
fail-closed to the day-1 structural fallback (`1616 ms` and `1565 ms`); the day-1
mask-4 trace was `2/1/1/1/1`, proving an extension and its tanker companion can
survive and be selected. The full pass is therefore not genuinely operation-bounded:
fixed combination caps still coexist with internal wall-clock deadlines. No later
score attribution from a partial/fallback pass is admissible.

`SCORE-ROLE-010` removed every internal wall-clock cutoff from the ranked role
pass. Two runs were identical (`465 ms`, `403 ms`): all-patrol `13/27/27`, mask
`4` `13/23/23`, mask `2` `13/21/21`. For mask `4`, `14` eligible sources produced
`3` extensions, all `3` survived patrol pruning, `2` retained their tanker
companion, but none was selected. In contrast, the fixed narrow day-1 probe had
selected its atomic extension. The measured successor gap is bounded-master
portfolio non-monotonicity: widening the per-agent portfolio can hide a complete
narrow witness even when its route columns remain present. Timing noise and raw
route absence are no longer admissible explanations for this counterexample.

`SCORE-ROLE-011` evaluated the complete narrow and wide fixed-operation
portfolios independently and advanced their exact lexicographic maximum on every
role/day. It still ranked all-patrol `13/27/27` above mask `4` `13/23/23` and mask
`2` `13/21/21` at `1946 ms`; mask `4` chose the narrow portfolio on one of four
days but received no full-horizon gain. The candidate was fully reverted. The
earliest quantified mismatch is now day 1 itself: the fixed role probe produces
only `4/4` for mask `4`, whereas the frozen production counterfactual produces
`7/8` on the same authoritative initial state. Therefore another portfolio merge,
post-refuel extension or bounded-master width change is not an admissible next
hypothesis. The open gap is production-quality day-1 seed fidelity.

`SCORE-ROLE-012` closed that day-1 gap without tuning after observation. Across
two stable runs (`698 ms`, `798 ms`), mask `4` produced the frozen production
day-1 score `7/7/8`. However, continuing from its exact terminal state with the
fixed cheap evaluator ended at only `12/21/22`, while the no-seed whole-horizon
floor retained all brands at `13/20/20`; all-patrol remained first at `13/24/27`.
The source was fully reverted. Production-quality day-1 route generation is no
longer the earliest open cause. The new counterexample is a certified tier-1
continuation failure from the improved terminal state.

`SCORE-ROLE-013` added a fixed `64`-rollout MacroMCTS candidate on later days
while lifetime brands remained missing. Two stable runs (`884 ms`, `835 ms`)
restored mask `4` to all `13` lifetime brands but reached only `13/22/24`, below
all-patrol `13/24/27`. Its exact daily sequence was `7/8, 5/5, 6/7, 4/4`, versus
the frozen production counterfactual `7/8, 6/6, 8/8, 6/6`. The lifetime-only
trigger stopped enhanced continuation after day 3, although tier 2 was still
open. This is the architecture's documented tier-2-witness failure in executable
form: completing lifetime coverage does not authorize a cheap continuation.

`SCORE-ROLE-014` kept the production-width fixed pass active on every day. It
completed in `4250 ms`, but mask `4` reached only `13/25/26` with daily sequence
`7/8, 5/5, 7/7, 6/6`, below the frozen `13/27/28`; mask `2` ranked first at
`13/27/27`. The candidate was rejected without repeat or another map. Increasing
the same operation caps is now prohibited. Existing production replay telemetry
shows the independent planner produced an exact-valid candidate on every day, but
does not expose its score or whether the final incumbent came from it. That stage
boundary is the next read-only attribution target.

## Invariants

- Official lexicographic score only; no weighted-sum promotion.
- Exact simulator and independent validator must agree; invalid rate remains zero.
- Performance changes preserve semantics.
- Logic changes must show a sufficiently large paired global advantage with bounded
  downside across protected official-budget strata; small sparse regressions are
  admissible, systematic or catastrophic regressions are not.
- Same fixture, role policy, traffic history and budget for every checkpoint pair.
- Solver and role-selection wall clock must not exceed 5000 ms on primary lanes.
- Evidence produced with an internal budget above 5000 ms has no promotion authority.
- Local output is a development filter only. BTC target-host is authoritative for
  latency, cutoff behavior, runtime safety and the final candidate verdict.
- A candidate cannot be promoted or committed while BTC connectivity/evidence is
  unavailable, even if all local checks pass.
- Commit only after a comprehensive global win against parent and relevant lane
  champions: zero invalid/emergency, no hard-cap violation, broadly distributed
  paired benefit whose magnitude exceeds bounded downside, and a passing BTC gate
  at the same 5000 ms cap. No fixed win ratio and no weighted tier aggregation.

## Protected lanes

1. Primary competition: outer server window `60000 ms`, internal hard cap `5000 ms`,
   fixed and deadline-bounded native roles, low/default/high fuel, 8x8 generated and
   BTC-like 32x32, horizons 4/5/10 days.
2. BTC preflight: hard cap `5000 ms`, protocol validity, runtime safety and score gate.
3. Degradation diagnostics: `500/1200/2500 ms`, never the sole production verdict.

## Next actions

1. Freeze the composed `PERF-DEADLINE-001 + SCORE-ROLE-021 +
   PERF-DEADLINE-002` candidate. Do not tune it on any opened seed or BTC match.
2. Complete a file-by-file diff audit, remove only nonessential research scaffolding,
   rerun the exact/equivalence tests and verify no token or research switch can enter
   the staged scope.
3. If the scoped candidate remains canonical and all gates stay clean, commit it as
   the new champion with explicit files only; never stage `.gitignore`, temporary
   artifacts, generated probe strategies or the broad untracked `old/` tree.
4. After freezing the champion, open exactly one new gap from authoritative BTC
   telemetry. The first candidate is the conservative replay/server fuel-state
   divergence observed on later `m-1258` days; trace API and simulator semantics
   read-only before proposing any logic change.
5. Continue development locally but require a new frozen holdout and fresh BTC
   target-host gate for every subsequent promotion. The internal cap remains
   5000 ms even when the outer server window is 60000 ms.

The frozen `SCORE-ROLE-016` archive holdout was opened only after its 22-row
manifest was written and hashed as
`16F8E3A820D33BF42FA0791D64E125D635962C6E926298213AEDCD8B19C78312`.
The selector kept the parent role mask on 17/22 replay setups. Exact validated
counterfactuals on the five changed masks produced 3 wins and 2 losses, hence the
full role verdict is `3/17/2`. Lifetime was unchanged throughout. Daily-distinct
gains/losses were `+15/-10` (net `+5`), and servings gains/losses were
`+178/-30` (net `+148`). The exact changed-mask pairs were: `m-0854`
`6/60/308 -> 6/60/373`, `m-0865` `6/60/225 -> 6/60/205`, `m-0878`
`6/52/135 -> 6/60/240`, `m-1139` `12/23/23 -> 12/30/31`, and `m-1140`
`13/34/34 -> 13/24/24`. All counterfactuals passed simulator/independent-validator
agreement. This is a broadly positive but non-monotonic holdout, not commit
authority: the protected matrix and a final combined BTC lane still decide whether
the `m-1140` tier-2 loss is acceptable globally.

The combined protected screen tied parent on all 6/6 general fixed and 6/6 general
native seeds with zero invalid/emergency. The first fixed-role BTC-scale pass also
had zero invalid/emergency and preserved lifetime/daily, but showed tier-3 deltas
of `-3`, `-13` and `-7` for default/low/high. Observational day telemetry then
repeated the same fixtures without changing solver logic: default remained a small
negative (`-2`), low flipped from `-13` to `+14`, and high changed from `-7` to a
tie. All runs kept `6/60`; every day was deadline-limited, action hashes diverged
under timed search, and all outputs independently validated. This directly confirms
the documented BTC-scale local cutoff/load instability. The local tier-3 sample is
therefore inconclusive, not a production regression and not a promotion win.
`PERF-DEADLINE-001` remains provisional on its target-host `m-1235` pass, and the
next authoritative gate is a fresh explicit low-fuel BTC match with the frozen
role candidate stacked unchanged.

That gate passed on BTC `m-1241`, created with every field explicit: hard, three
bots, 10 days, 32x32, 100 steps/day, 5000 ms/day, eight agents, 12 spots, six
brands and low fuel 1x (`100`). The combined candidate finished rank 1 at
`6/60/340`; the three bots reached only `6/58/181`, `6/58/171` and `6/57/185`.
All 10 submissions were HTTP 200 and valid, max response was `3224 ms`, exact
deadline overrun and emergency were zero, and replay-check independently rebuilt
`6/60/340`. Replay SHA256 is
`88DB2191111AAFB039CCB5477CFB2624EF12354CA48E1857CA00EB1621918DFA`.
The live assignment was mask `32`; parent and candidate replay-role scans both
select mask `32`, so this match is authoritative combined runtime/safety evidence
but not causal evidence that the role patch improved this particular setup.

The full 60-map `competition-general-native` matrix produced `0/56/4`, with zero
invalid/emergency. The four losses were `310013` (`6/30/49 -> 6/29/47`, mask
`8 -> 1`), `310033` (`7/28/36 -> 7/28/33`, mask `8 -> 1`), `310044`
(`6/24/39 -> 6/24/37`, mask `1 -> 8`) and `310055` (`6/30/46 -> 6/30/42`,
mask `4 -> 8`). A runtime-path audit on 2026-08-01 found that this lane used
`role_mode=exhaustive`, which calls `select_roles(3)`, while SCORE-ROLE-016 changes
only `select_roles_until(...)`. The compared binary also included PERF-DEADLINE-001.
Therefore these four losses are combined-build/timed-day evidence, not causal
evidence against the wide timed-role mechanism. The lane is retained as an
exhaustive-role control and cannot accept or reject SCORE-ROLE-016/017/018.

`SCORE-ROLE-017` kept the frozen
`SCORE-ROLE-016` mechanism unchanged when `spot_count > 8`, because only then can
the parent narrow rollout's `maximumTargetSpots = 8` truncate configured targets.
When `spot_count <= 8`, it skips the duplicate wide pass entirely and therefore
retains the parent role path. The dispatcher is based on an observable semantic
capacity boundary, not seed, match id or family. The 60-map native matrix and the
opened 22-replay archive are development evidence only; a new generated
default/low/high BTC-scale native-role holdout must be frozen before testing.

The first independent guard screen exposed a deeper timing confound: on general
seed `320003`, base scored `6/30/39` with mask `4`, PERF-only scored `6/30/34`
with mask `8`, and the guarded role build again scored `6/30/39` with mask `4`.
Because `spot_count <= 8`, PERF-only and the guarded role build execute the same
narrow role logic; their different result is compile/layout wall-clock sensitivity,
not evidence that the guard itself fixes ranking. `SCORE-ROLE-017` is therefore
inconclusive and its opened screen cannot validate a successor.

`SCORE-ROLE-018` is now the only active role candidate. It enforces global
additivity in two phases: first every parent narrow beam member receives the
original shared rollout window ending at 85% of the role budget; only after all
narrow members finish may production-width rollouts consume genuine unused slack
remaining before that same 85% boundary. Wide results can replace only their
same-assignment narrow result by the official comparator. The `spot_count > 8`
semantic guard remains. The prewarm deadline stays at 92%, and prewarm still
retains at least the 85-92% opportunity. However, parent prewarm can begin before
85% when narrow work finishes early, so wide work may replace some of that earlier
cache-warming headroom. This is an explicit performance trade-off requiring
target-host telemetry, not an equivalent-semantics claim. Wide work cannot consume
time required to construct the parent narrow beam. A completely new generated
deadline-role holdout is frozen
before score; the earlier exhaustive manifest was corrected before opening because
it did not execute the candidate runtime path. The corrected manifest hash is
`A4688DA19C5B6A6611125DF7C3E13659C5EA5727C7FDCE2431CEA9D996590726`.

The first corrected deadline-role general screen still compared two separately
compiled binaries. Although `spot_count <= 8` made the wide stage unreachable,
role masks changed on seeds `330001` and `330004`; the apparent `330004` serving
gain is therefore compile-layout/wall-clock noise and has no candidate authority.
No BTC-like seed from that manifest was opened. A research-only same-binary switch
now disables only the wide slack stage while preserving identical code layout and
the PERF parent. The replacement causal manifest was frozen before score at
`research/holdouts/SCORE-ROLE-018-CAUSAL.csv`, SHA256
`2626EED2A950B53CBF7915DFD9316695D5EA0C0195CAD4DCF6171A4334573F15`.

## Reopen conditions

- Short-deadline quality: only if BTC publishes such a deadline or live telemetry
  demonstrates an equivalent authoritative window.
- High fuel, roles, ALNS, viability or master: only after a new official-budget
  counterexample identifies that subsystem as the earliest score-limiting cause.

## Current BTC role counterexample and SCORE-ROLE-020

Fresh explicit-advanced BTC match `m-1252` used hard difficulty, three bots,
10 days, a 32x32 map, 100 steps/day, 5000 ms response, eight agents, 12 spots,
six brands and low fuel 1x (`100`). The SCORE-ROLE-018 working binary returned
mask `0` (eight patrols) after about 4266 ms of role selection. All 10 day
submissions were HTTP 200 and exact-valid with no emergency, but the team finished
rank 4 at `6/52/138`; the bots, each using one tanker, finished `6/60/182`,
`6/60/186` and `6/60/201`. Replay SHA256 is
`B443AB6C3AC0A73EE6FB623E559684AF0A590CE1C2B8DCA3FD6F5ABCA45B87A4`.

This is an attributed role failure, not a day-planner or protocol failure. The
same frozen setup replayed with exact validation under the current binary reaches
`6/60/315` with central one-tanker mask `64` and `6/60/284` with one-tanker mask
`4`, versus the live all-patrol `6/52/138`. Offline role replay on the same binary
also ranks a one-tanker assignment first, proving the live mask `0` came from
cutoff/load-dependent incomplete evidence rather than a stable score preference.
The full-horizon timed loop allocates slices sequentially; a timed-out assignment
retains its one-day probe score, so the final comparator can mix evidence depths.

`SCORE-ROLE-019` removed the SCORE-ROLE-018 wide-rollout mechanism and initially
treated `rolloutValid` as complete evidence. The first six generated general
screens showed that this measured plan validity, not equal-depth search evidence.
Before opening any BTC-like seed, attribution separated `rolloutValid` from a new
`rolloutComplete` flag that remains true only when the independent planner, column
generation and master all finish without hitting their internal deadlines. This
changes the candidate mechanism, so the old SCORE-ROLE-019 manifest cannot be
used as a clean promotion holdout.

The corrected mechanism is `SCORE-ROLE-020`. Only when any narrow-beam assignment
lacks deadline-complete evidence and the public fuel/day-step ratio is low does it
prohibit all-patrol from winning the cutoff, promoting the best already-enumerated
exactly-one-tanker member by the existing official role comparator. Default/high
fuel paths are controls. The unit gate directly verifies incomplete-low-fuel,
complete-evidence and default-fuel branches. A research-only same-binary switch
disables the fallback and must be removed before any promotion.

The now-development-only BTC-like low-fuel seeds `760000..760003` produced a
conservative causal result of 1 win, 3 ties and 0 losses: on seed `760001`, control
selected all patrols and scored `6/60/160`, while SCORE-ROLE-020 selected mask 32
and scored `6/60/422`, a tier-3 gain of 262 with lifetime/daily preserved. Seeds
760000 and 760002 tied exactly; seed 760003 retained the same mask and its local
tier-3 delta is not causal authority. All runs had zero invalid/emergency.

Fresh explicit-advanced BTC match `m-1255` then used the same hard/3-bot/10-day/
32x32/100-step/5000-ms/8-agent/12-spot/6-brand/low-fuel configuration. Candidate
selected mask 64 in 3911 ms, received 10/10 HTTP 200 valid acknowledgements, had
zero emergency and exact deadline overrun, and the independent replay validator
agreed on all 10 days. It ranked first at `6/60/247` over `6/60/222`, `6/60/208`
and `6/58/202`; maximum day solver time was 2564 ms. Replay SHA256 is
`E3A467251223CE545680DE2731C336805FA6A9788A8CB48D1F71A425AAADC3B0`.

This is strong development and BTC target-host evidence, but not promotion proof
because the mechanism was refined after the old manifest opened. The replacement
holdout was frozen before any new score at `research/holdouts/SCORE-ROLE-020.csv`,
SHA256 `12193CD454ACBF9EA5248C9CBB1A24DE46FA0F67BD280519929099834B4081D8`.

The SCORE-ROLE-020 frozen screen produced no causal regression: six general,
four BTC-low, two BTC-default and two BTC-high fixtures all tied once changes with
the same selected role were separated from local day-planner cutoff noise. Every
run kept zero invalid/emergency. General seed `360001` and default seed `771001`
were repeated without code changes; candidate/control masks and scores crossed
when run order changed, proving the apparent tier-2 deltas were timed-selector
nondeterminism rather than fallback effects. On BTC-low, all four candidate/control
pairs selected the same one-tanker masks; on default/high, the low-fuel guard was
mathematically disabled. SCORE-ROLE-020 is therefore a safe scoped mechanism with
strong development/live benefit, but it does not address the broader instability
and remains uncommitted.

The new counterexample is default-fuel, 10-day seed `771001`: with the fallback
disabled by construction, repeated runs oscillated between all-patrol scores
`6/56/199` and `6/50/209` and one-tanker scores `6/60/322` and `6/60/320`.
Candidate and control exchanged those outcomes solely with run order. A 10-day
high-fuel control also changed daily score by three with the same all-patrol mask,
confirming local solver noise, but the default role flip exposes a real selection
hazard: unequal-depth timed evidence can lose tier 2 outside the original low-fuel
guard.

`SCORE-ROLE-021` replaces the fuel predicate with the public horizon boundary:
short protected horizons of 4/5 days keep the parent selector, while longer
horizons with any incomplete role evidence fail closed to the best existing
one-tanker assignment. This is not a new search or seed-tuned route; it is an
objective dispatcher over day count and evidence completeness. Its fresh manifest
was frozen before code or score at `research/holdouts/SCORE-ROLE-021.csv`, SHA256
`31D9677FC73E4189CD2FC6604979427E349808E776827020359430CF7F229325`.

The frozen SCORE-ROLE-021 screen completed at 3 wins, 15 causal ties and zero
losses across six short-horizon general fixtures and four each BTC-like low,
default and high fuel. All runs had zero invalid/emergency. General fixtures were
six exact same-mask/score ties. Low and default fixtures selected the same tanker
in each pair and preserved `6/60`. High-fuel produced three causal gains and one
tie: seed `782001` improved all-patrol `6/58/360` to mask32 `6/60/449`; seed
`782002` improved all-patrol `6/57/326` to mask64 `6/60/435`; seed `782003`
preserved `6/60` and improved servings `260` to `327`; seed `782000` tied
`6/60/508`. A reversed-order reproduction on `782002` again selected all-patrol
for control at `6/52/333` and mask64 for candidate at `6/60/435`.

Fresh explicit-advanced high-fuel BTC match `m-1257` then used hard difficulty,
three bots, 10 days, 32x32, 100 steps/day, 5000 ms, eight agents, 12 spots,
six brands and fuel 3x (`300`). SCORE-ROLE-021 selected mask8 in 4423 ms and all
submitted plans were exact-valid, but the lifecycle sent only six of ten days.
Days 4, 6, 7 and 8 were replaced by server WAIT with reason
`insufficient-authoritative-day-window`; final result was rank4 `6/52/254` versus
three bots at `6/60/311..326`. Replay SHA256 is
`FBD04D7F2170FF22D4AB19B7156A77C1237D069F2ED5F7FFE683DADFA0E90CF3`.

This is not a role-score refutation. The decision engine produced exact-valid
one-tanker plans, but the final HTTP gate required 1500 ms remaining even though
the solver already reserved 1600 ms network time. The four skipped decisions had
1129, 1352, 989 and 1498 ms of their authoritative local window remaining. A
read-only scan paired `actions` with `action_result` over 366 BTC replay files and
794 successful POSTs: p50 13 ms, p95 81 ms, p99 594 ms, maximum 4773 ms; eight
samples exceeded 500 ms and four exceeded 1000 ms. On m-1257 the six observed
POST RTTs were 8-12 ms except one 443 ms sample.

`PERF-DEADLINE-002` therefore keeps the 1600 ms solver network reserve and all
search/actions unchanged, but lowers only the final go/no-go submission floor to
800 ms, equal to archive p99 plus 206 ms margin. `post_until_deadline` still uses
the authoritative server deadline and the exact WAIT fallback remains below the
floor. This accepts a quantified rare-tail trade-off instead of guaranteeing four
known misses. The frozen evidence/target manifest is
`research/holdouts/PERF-DEADLINE-002.csv`, SHA256
`2A52D220E342B2780063DE65D3F0DE4FE4DF75B931F3B508E4C355A403144A57`.

The preregistered fresh target `m-1258` used the same explicit high-fuel advanced
configuration as `m-1257`. The candidate selected one-tanker mask16 in 4445 ms,
submitted and received HTTP 200 for all ten days, and finished rank1 at
`6/60/415` versus bots at `6/60/363`, `6/60/360` and `6/60/351`. There were zero
submission skips, server WAITs, emergencies and exact deadline overruns; maximum
solver time was 3130 ms and observed POST RTT was 7-49 ms. The independent replay
validator accepted every submitted plan. Replay SHA256 is
`6BA3E14F97259CB0259DEB4F014C6FF86DBC28EE71ED6BC58143052C2C499D1B`.
This clears the `m-1257` lifecycle blocker and supplies target-host evidence for
the composed role/runtime candidate. Seven of nine replay state transitions were
byte-exact; the remaining two were conservative authoritative-state reconciliations
where the server reported more patrol fuel after tanker co-location than the local
simulator predicted. They caused no invalid action or score loss and are recorded
as the next read-only semantic-attribution gap, not silently treated as equivalence.
