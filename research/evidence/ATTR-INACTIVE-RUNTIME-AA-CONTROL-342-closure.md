# 342 complete: identical-binary runtime is not exactly repeatable

Completed2026-09-07 20:35:54+07, about12m05s after launch. All32 atomic sides,
8 fixture markers,16 same-binary comparisons,128 valid ACK and96 reconciled
transitions completed. Runner7088 exited, stderr0, no remaining compute child.
All1470 dependencies and all side/fixture/complete hashes verified. Frozen
summary independently recomputed in memory and exactly equals the saved JSON.

- Manifest:375A1991CA74F6907827A649C754A57D8E8B588E067E89C9E8192BD0FF1D37DC
- Completion:35236C5A09CDE21BBCF55B336A85B01024687453F2DBE3EBF5727C1F598FF6D4
- Summary:73C4F8877A3805C949FBD16DBE7C133AEF9BD1138361E8AC09EFDF51406288FB
- Parent:F97F168FE76FEF1B6226D2C2CFDAF954F116D39F427B6F157594151C461E1275
- Candidate:C8603EB32E33B44EF1E3A2BE56DF2CC8A1C8E1001E11A5E28248814E028BE2A4

## Verdict and scope

Exact repeatability is falsified for BOTH frozen binaries on these fresh inputs.
Parent has3/8 score/plan/state/ledger/road differences and2/8 role differences.
Candidate has3/8 score/plan/state/ledger/road differences and1/8 role difference.
Each binary's other5/8 trajectories are exact. These are A/A repeats, NOT a
candidate-versus-parent score comparison or statistical variance estimate.
All candidate supported/work/improvement counters are zero, as required for
road-present controls. Unsupported pricing call counts total120 across16 sides.

Parent B-vs-A W/T/L3/5/0, delta0/+1/+14; first tier2 once, tier3 twice.
Candidate B-vs-A1/5/2, delta0/-1/-14; first tier2 once, tier3 twice.
These directions are descriptive arbitrary repeat labels, not evidence one build
is better. No safety failure. Maxmain parent3379ms/candidate3381ms; maxresponse
4007ms/3991ms inside5000ms synthetic windows. No official BTC latency authority.

## Every changed pair

| Seed | Binary | A -> B official score | First divergence | Role change |
|---|---|---|---|---|
|202609073420019|parent|7/23/28 ->7/24/29|pre-match|0010 ->0100|
|202609073420033|parent|6/24/173 ->6/24/184|day1|none|
|202609073420041|parent|6/24/145 ->6/24/147|pre-match|00001000 ->00100000|
|202609073420033|candidate|6/24/177 ->6/24/170|day3|none|
|202609073420034|candidate|6/24/97 ->6/23/87|day3|none|
|202609073420041|candidate|6/24/145 ->6/24/148|pre-match|00001000 ->00100000|

The negative-control pool starts202609073419998 because seeds are aligned modulo6
for generator family identity; this is the frozen342 seed, not a341 reused input.
All8 gameplay identities were checked against all consumed341 splits before run.

## Every registered stratum

Counts below are score/trajectory differences (equal in this experiment), with
role differences in parentheses. Full JSON preserves each exact score, plan
hash, first tier, delta, safety day and all per-stratum W/T/L and component nets.

| Stratum | A/A pairs per binary | Parent differences(roles) | Candidate differences(roles) |
|---|---:|---:|---:|
|map8|4|1(1)|0(0)|
|map32|4|2(1)|3(1)|
|fixed|4|1(0)|1(0)|
|native|4|2(2)|2(1)|
|low fuel|4|0(0)|1(0)|
|default fuel|4|3(2)|2(1)|
|8teams|3|1(1)|1(0)|
|9teams|3|1(1)|1(1)|
|10teams|2|1(0)|1(0)|
|balanced|2|0(0)|1(0)|
|rare-brand|2|1(1)|1(1)|
|threshold-corridor|1|0(0)|0(0)|
|fuel-tight|1|1(1)|0(0)|
|high-stock|1|0(0)|0(0)|
|overnight|1|1(0)|1(0)|

## Source accounting, not an excuse for341

The canonical timed role selector uses15% scan,35% probe,85% rollout boundaries,
and divides remaining wall time among remaining assignments. Its generator and
master also accept wall-clock deadlines. The frozen candidate does not call
pricing in this role path. Same-binary role changes therefore show that an
independent-build A/B role mismatch is not uniquely attributable to pricing.
Day search and certification also depend on wall-clock completion. This is a
confirmed control observation, not a newly proven cause of every341 difference
and not evidence that user background applications caused the variation.

Separate confirmed coupling remains in341: setup state/ledger copies before the
support guard, and supported pricing consumes the same deadline as subsequent
scenarios. Canonical `decision.cpp:5153-5179` allocates each remaining candidate
a share of the remaining certification interval. Rollback of a witness does not
restore elapsed time, original candidate allocation or generator/cache work.
342 cannot quantify the supported/exhausted cost: it deliberately never ran it.

341 stays rejected under its frozen gate. Its narrow holdout benefit remains
scoped evidence; neither wins nor losses in protected341 are erased or used to
tune a successor. Do not replace every timed search with a smaller operation cap,
force fixed roles, drop scenarios or current-floor admission, or simply relax the
inactive gate. All of these would require their own semantics/score proof.

Next: inspect a single general capability-preserving boundary for W1 pricing
that leaves the complete original certification pool intact before refinement.
Before any successor code, determine whether same-invocation original/priced
profiles and remaining budget can be compared causally without rerunning main
solve, stealing validation/transport reserve, or removing a designed consumer.
This is source/design work under the current follow-up, not an already-qualified
optimization. Any implementation or new runtime measurement needs separate
registration and fresh inputs.

No production source/binary changed; nothing removed, disabled, deferred, reduced
or deleted. No commit, promotion or competition signature. All evidence retained.
