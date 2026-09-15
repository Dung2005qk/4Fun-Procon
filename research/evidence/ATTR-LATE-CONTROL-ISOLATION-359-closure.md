# 359 complete: treatment blindness must extend through submission and ACK

2026-09-09. Source/existing-byte inspection only; zero solver runs, builds or VM
actions. No production or frozen source change, no function reduction, no deletion,
no commit. Result F1B21E5DD29EDA4FF209C9CD275F38685CCCF3F54B9F9D9310910DB904566024.
All 281 frozen execution dependencies verified. The adjacent script has a read-only
`--check` mode. Its 16-row guard model passes with five mutation controls.

## What the frozen executable establishes

The Linux 357 binary stores run_http's RuntimeOptions pointer at rbp-0x4580.
resourceMarginal357 is its first field. Two direct first-byte comparisons remain:

- 0x3eec5 tests treatment, with its zero branch skipping the subsequent public
  authorization comparison. This lies in the richer-checkpoint region; the
  telemetry witness below suffices even when this region is not reached.
- 0x3f069 tests treatment before telemetry. Treatment=false jumps from 0x3f06c
  straight to 0x3d2ae. Treatment=true additionally executes the authorization
  comparison at 0x3f072 and its branch at 0x3f079; inactive authorization then joins
  the same 0x3d2ae. Therefore the instruction paths are not identical at this point.

Exact instructions and tool/binary hashes are in the result. This is a local
control-flow witness, not a whole-program taint proof or a cycle measurement.
It does NOT prove that these few instructions explain the observed serving loss,
nor that delaying the flag will make independently timed A/A trajectories equal.
357 remains inconclusive. Its sealed54/protected108 remain unopened.

The stronger distinction is useful: a no-optimizer-entry assertion did not cover
the measurement-control path itself. Removing the assertion would hide this fact;
changing the control boundary can instead make the required property testable.

## Source and persistent-state accounting

Frozen portable btc_main.cpp: startup flag read 147-148; complete checkpoint saved
2267-2268; old public refinement 2316-2396; optimizer guard 2397; telemetry guard
2445. The new result's storage is also unconditionally constructed after checkpoint
at 2270. Neither new consumer is in main selection, but telemetry is before the
serialized POST and response timing observation.

ACK calls MatchSession::acknowledge_submitted -> engine.record_submitted. The
engine stores response duration, last profile/candidate, certified contingencies,
own-road history and expected agents. Its remaining post-ACK compute allowance is
5000 minus MAIN decision timing, not response duration. Do not invent an adaptive
reserve adjustment from responseTime. The host's time-dependent polling/precompute/
proof cadence can still change which persistent artifacts exist next day.

Existing sparse enumeration uses four workers, atomic task assignment and clock
checks inside resource enumeration and ascent. Equal outer budgets or route counts
do not imply equal per-worker progress. The 358 same-budget difference is consistent
with this dependency, but no exact worker schedule was recorded, so its cause is
still not uniquely identified.

## Decision: late treatment observation, not less functionality

A contract-only integration is justified. Restore canonical RuntimeOptions layout
and argument parsing. Observe the research control only inside the already
authorized nonterminal public region AFTER the full original public refiner has
returned. Observe it once there and retain both consumers: optimizer only if the
original deadline condition permits; telemetry even if that condition prevents
optimizer entry. The 16-case model proves these Boolean outputs match 357; it does
not prove compilation, runtime equivalence or promotion.

No designed solver/telemetry responsibility is removed, reduced or deferred. All
original work executes at its original point. The experimental control's equivalent
late reader must be wired to BOTH consumers before the redundant startup storage
is removed in the isolated contract. Production is not changed by this design.

### Mandatory compiled-contract obligations before any SCORE successor

1. Pin canonical source, exact unchanged library, compiler/link command, control
   integration source, executable and probe. No optimization or budget change.
2. Verify no treatment read in startup/roles/main/checkpoint/inactive submission/
   ACK/post-ACK. Put an interposed treatment-reader counter on the real compiled
   call, not a hand-written alternate solver or only the Boolean model.
3. Exercise full original process-RPC lifecycle in inactive 5000 and mixed 5/10-second
   contract days, both treatment values. Assert zero inactive reads and at most one
   authorized nonterminal read per day; preserve telemetry on no-entry/deadline.
4. Inspect actual compiled control-flow domination and all source reads. A finite
   test supports but cannot replace the source/machine argument. Include no-gain,
   expired window, terminal and resumed persistent state in the obligation map.
5. State the theorem correctly: given equal normalized starting process/session
   state and the SAME exogenous clock, I/O, allocation and worker-scheduling inputs,
   treatment cannot change an inactive trace if its bytes are never observed.
   This is not a promise that two independent wall-clock runs finish identically.
   Exclude startup environment-layout differences by equal-length control values;
   distinguish that test control from a universal OS-level proof.
6. Prove that removing the test interposer and hard-enabling a qualified suffix
   would not invalidate the control boundary. No hidden test-only planner path.

Only after those obligations have evidence may a separate prospective SCORE
protocol define its inactive equivalence evidence, fresh measured controls and
unchanged official-score/breadth/downside/safety gates. Keep all measured differences
in its tables; do not discard losses as noise. This is not permission to pass 357,
retune on its closed evidence or reduce the role/search/transport functions.
