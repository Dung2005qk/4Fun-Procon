# PERF-ROLE-DIAGNOSTIC-ISOLATION-227

Date: 2026-08-26. Parent: `e8bf766`.

## Registered gap

ATTR-224 stored per-day rollout diagnostics in `RoleAssignment`. Although no
decision reads the field, every timed production role rollout clears and appends
to the vector, and every beam copy carries the larger object. Deadline-bounded
role selection makes that diagnostic code score-affecting. SCORE-225 therefore
proved its flag delta under a shared instrumented binary but did not establish
the final combined candidate against the uninstrumented direct parent.

## Candidate and invariants

- Restore the direct-parent `RoleAssignment` layout.
- Return the same per-day rollout trace through a separate diagnostics result.
- Compile separate trace/no-trace selection implementations; production callers
  use no-trace and research callers use trace.
- Preserve the exact rollout, sort, fallback, simulator, validator and official
  lexicographic logic.
- Preserve replay-roles and historical `--role-details` output end-to-end.
- Do not use local elapsed time as performance evidence.

No designed functionality is removed or reduced. The active equivalent for the
only removed storage location is the opt-in diagnostics result consumed by both
existing research callers.

## Frozen evidence

Manifest: `research/holdouts/PERF-ROLE-DIAGNOSTIC-ISOLATION-227.csv`

SHA256: `9D5795D00CF9557C9EC7830F967B9974E0BFC269D5F6DC272CCDD2F41C6C0895`

## Development result

Frozen binaries:

- candidate historical harness:
  `1CA7F739AD977D3BD0395761B5FE9F571E7602429D0CE241F66268E81D828D9B`
- `e8bf766` historical harness:
  `145D9D0CA225E4AE4F18C5AABE39DC29B85044EFA425840C3F794F0095EC7ABA`
- `baebad8` historical harness:
  `3D82CB2021D7E472534A75DAFB1EA6B5156D7DFC24CC5DF35F98B9EBF5D9C1DE`
- candidate BTC binary:
  `DC01FE16131F26CF8FDFFDE24262F17A6AE9BE360FB85436079CB6933AD7223A`
- frozen source-diff object:
  `F99E9FD08DA43E370964203D98C343A71EECE0EE`

Across 32 fresh development cases, candidate versus `e8bf766` is
`9/18/5`, net `+40` servings, zero tier-1/tier-2 loss and one tier-2 win.
Candidate versus true parent `baebad8` is `11/12/9`, net `+85`; the only
tier differences are two low-fuel wins of `+4` and `+3` daily. No invalid or
emergency result occurred.

An exhaustive one-case diagnostic comparison retained identical ranked role
masks and identical daily traces for all three rows (`6,6,6,6,6`). Aggregate
rollout servings differed because the independent rollout planner retains its
pre-existing `60 ms` inner budget; that field was never the invariant under
test. Diagnostic capability and trace semantics are preserved.

Development gate passed. The one-time 40-case holdout is authorized on these
unchanged frozen binaries.

## Holdout environment correction

The candidate side of the original 40-case holdout was accidentally executed
on the local workstation. No scores were inspected or aggregated; only per-suite
completion counts were observed. Because role selection is cutoff-sensitive,
that environment is not authoritative. Those 40 seeds are consumed and must
never be rerun or used for promotion; neither baseline side was started.

A fresh replacement manifest was frozen before any source change:
`research/holdouts/PERF-ROLE-DIAGNOSTIC-ISOLATION-227-v2.csv`. It contains 40
new seeds and must run candidate, `e8bf766`, and `baebad8` sequentially on one
quiet Spot VM. Only this replacement holdout has promotion authority.
Frozen SHA256:
`D526D86C5A9D9762B1FE873A2421BD5A51146EDC627F3B30E775128ABDDF1A8B`.

## Authoritative VM holdout result

The replacement holdout ran strictly sequentially on Spot VM
`udon-role-227-0826`: candidate, `e8bf766`, then `baebad8`, 40 cases per
side. All 120 atomic result files and all completion markers were present;
runner stderr was empty. The preserved evidence archive is
`PERF-ROLE-DIAGNOSTIC-ISOLATION-227-vm-holdout.tar.gz`, SHA256
`E1007CF6055D27D3C6B8338A7CA94433F6658EBA54793EF198F24C0809ED6FF9`.

Candidate versus `e8bf766` was `1/36/3`, serving net `-4`. All four first
differences were tier 3: the win was `+1`; the losses were `-1`, `-1`, and
`-3`. There was no tier-1 or tier-2 loss, no invalid result and no emergency.
This is bounded cutoff displacement against the shared instrumented HEAD, not
a loss of the accepted 225 mechanism.

Candidate versus the true clean 225 parent `baebad8` was `6/33/1`, serving
net `+229`. Five wins were tier 2 for a total `+22` daily distinct; the
remaining win was tier 3 `+2`; the sole loss was tier 3 `-1`. The gains crossed
both easy and live-like brand-8 suites. General was `0/8/0`, medium was
`0/7/1`, and all safety counters were zero.

## Verdict

## BTC target-host gate

The frozen Windows candidate binary
`DC01FE16131F26CF8FDFFDE24262F17A6AE9BE360FB85436079CB6933AD7223A`
was connected to fresh advanced match `m-4276` (24x24, five days, 60
steps/day, four agents, 18 spots, six brands, fuel 60). Role selection produced
the required low-fuel composition `PPPT`, so the 225/227 live composition path
activated correctly.

The first action was generated and dual-valid, but the workstation's current
network was not suitable for an authoritative lifecycle gate: only about
1165 ms remained before the published deadline when the POST began, while the
observed POST-to-response interval was about 1587 ms. BTC therefore returned a
valid ACK for day 2 to the client's day-1 submission, and the client correctly
stopped on the stale-day invariant. The eight-line replay is retained at
`artifacts/btc/m-4276.jsonl` as operational evidence and must not be resumed or
used for score promotion.

This is inconclusive transport evidence, not a score regression of 227. Per the
user's explicit decision, the canonical 5000-ms solve and accepted 1100-ms
protected-refinement reserve are unchanged; no reserve is retuned to a weak
temporary network.

The debt was paid on a stable connection with the current integrated Windows
binary in fresh advanced match `m-4290`: 24x24, seven days, 100 steps/day, four
agents, 18 spots, six brands and low fuel. Role selection produced `PTPP`
(three Patrol and one Tanker), activating the intended 225/227 target
composition. All seven submissions were ACKed for their correct day with zero
transport retry, zero deadline skip and zero recovery wait. End-to-end
day-state-to-action-result durations were `3340, 3203, 3017, 3088, 3074, 3085,
2162 ms` (maximum `3340 ms`, mean `2995.6 ms`); maximum reported decision solve
time was `2683 ms`. Exact replay reconciliation returned `6/42/159` with seven
valid actions and six reconciled transitions. Final BTC standing was rank 1 at
`6/42/159`; rank is recorded only as operational context, not promotion
evidence. The replay is `artifacts/btc/m-4290.jsonl`, SHA256
`47BC89693F014D8FD2ABF83F5E4599C3619DB9A4FBC3426B22374AD76A61C9D6`.

The score/semantic and BTC target-host gates pass. The candidate preserves the accepted
225 improvement against its clean direct parent with large tier-2 benefit and
bounded downside, while removing decision-dead diagnostic work from the
production cutoff path without removing the diagnostic capability. It is not
claimed to dominate the instrumented `e8bf766` on every tier-3 case.
