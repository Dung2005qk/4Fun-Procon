# UDON-SHIELD Research State

Updated: 2026-08-27

## Current phase — competition checkpoint `18ecdd3`; no active source candidate

The final-day acknowledgement mismatch found during experiment 232 is closed in
commit `18ecdd3` (`fix: align final-day ACK with official score`). A shared
ledger relation now preserves componentwise dominance on every nonterminal day
and uses the official lexicographic relation on the terminal day. The production
pre-submission path is action/score-equivalent to its parent because it already
used the same terminal lexicographic relation; the repair removes only the stale
post-ACK rejection after the server has accepted a valid terminal action. The
historical closed-loop harness now applies the identical contract. No designed
functionality is removed, disabled, deferred or reduced.

The authoritative build and tests passed before the commit. Target-host match
`m-4476` used hard 24x24, seven days, 100 steps/day, four agents, 18 spots,
four brands, low fuel and an authoritative 15000-ms response window. The exact
Windows binary SHA256 was
`212EE9A4D58C8C26B842AFBD54BA8E2D8261F87163DAA7D26D8C105EB165C097`.
All seven action responses were HTTP 200 and valid; all six observable
transitions reconciled; replay-check returned `4/28/123`; invalid, emergency,
checkpoint, mid-day, sparse and public-continuation failure counters were zero.
Canonical main solve was 1615--3376 ms, the complete checkpoint retained its
5000-ms cap, and end-to-end action responses were 2947--6258 ms inside the
15000-ms authoritative deadline. Public continuation ran on all seven days,
generated 1601 plans with 1588 dual-valid in aggregate and correctly kept the
checkpoint because it found zero certified takeover. Replay SHA256:
`1DB2F2DFAF95A2B3D0F872F536752F593EB030EDC241E0051491C609AB8897B3`.
Rank is deliberately not used as promotion evidence.

The convergence audit closes the current research queue at a practical
competition stopping point, not as a mathematical proof of the global optimum.
Experiment 209's exact position-then-sweep gap has two parts. The expressible
deep-sweep part was exploited and promoted by 210 and 215. The residual
restrained-positioning part necessarily changes the nonterminal state and loses
the monotone transition certificate. Independent attempts 164, 167, 168, 183
and 184 showed that the available safe certificates are inert within the
authoritative window, while weaker actual-vs-actual scenario comparison admits
a reproduced closed-loop regression. Experiments 229--232 independently closed
post-ACK diversification, frozen-proof injection, authoritative-ACK rebasing
and protected proof-consumption without finding causal global benefit. An exact
oracle that requires hours and billions of transitions is evidence of a
theoretical gap, not a deployable response-time mechanism.

There is therefore no active production source candidate and no registered
experiment. Reopen research only on new evidence: a fresh strong opponent or
replay counterexample, an authoritative deadline/capability change, or a new
sound state-coupled certificate that can express restrained positioning and
prove its full remaining-horizon dominance inside the public deadline. Do not
reopen by weakening the comparator, reusing consumed holdouts, increasing caps,
or retrying the rejected post-ACK consumers.

## Closed predecessor — SCORE-PROOF-PROTECTED-CONTINUATION-232-v2 rejected

Accepted checkpoints are now isolated as commits `9af342b` (227) and
`1f30e69` (228). Rejected experiments 229--231 remain research-only evidence;
their source changes are absent from production.

The remaining post-ACK gap is not permission to retry direct W0 injection.
Experiment 230 proved that inserting the frozen-scenario proof argmax into the
next canonical search is unsafe: its only two activated score differences were
losses. Experiment 231 proved that rebasing all background work onto the richer
ACK transition is nearly neutral, loses proof coverage and has a systematic
high-fuel downside. Both mechanisms remain closed.

SCORE-PROOF-PROTECTED-CONTINUATION-232 tested a narrower invariant-derived
consumer. A complete strong proof retains its exact argmax plan sequence on
both causal sides. The canonical main solve, 5000-ms checkpoint, W0 cache,
candidate generation and all accepted protected refiners remain identical.
Only after the accepted public-window continuation reaches its fixed point may
the enabled side exact-evaluate a proof witness for the current authoritative
day. A nonterminal takeover requires the existing strict protected certificate:
equal road footprint, identical ordered terminal cells, no patrol-fuel loss,
brand monotonicity and strict official day gain. On the terminal day, exact
dual-engine validity and strict official lexicographic gain are authoritative.
Failure, deadline, invalidity, mismatch or non-dominance keeps the incumbent.

Revision v1 was stopped after 15/24 control-off cases and before any partial
score was inspected. Source audit found that the takeover gate correctly
removed terminal-state equality on the final day, but two later closed-loop
checks still required transition dominance plus componentwise ledger dominance;
the older non-propagating probe had the same stale terminal requirement. This
could reject an exact-valid final-day official gain solely because its terminal
cell or lower-priority score component changed. V1 is invalid pre-audit
operational evidence: its root `/home/LMC/udon232-0827` must never be resumed,
scored or mixed with v2.

V2 makes the historical harness match the already-authoritative production
certificate: nonterminal days retain transition plus ledger dominance, while
the final day requires only exact dual-engine validity and official
lexicographic non-regression/strict gain. A direct unit fixture now proves that
a final-day proof witness with a different terminal state and a strict official
gain is accepted. Windows and Linux unit suites pass.

The fresh replacement manifest was frozen before any v2 score at
`research/holdouts/SCORE-PROOF-PROTECTED-CONTINUATION-232-v2.csv`, SHA256
`6EF6FFA72F5CADA334D5A0C22C845DED03C1522FC743AFCD416F44B404C40513`.
Development completed
24 exact-5000-ms controls plus 60 fresh paired 15000-ms public-window cases;
the one-time holdout has 108 different fresh cases. Both sides use one binary
and both retain proof paths, isolating only the protected consumer. The sealed
holdout remained unopened because development failed both the exact-control and
causal-activation gates.

The v2 resumable runner is frozen at SHA256
`28354D9FBE2CFD82B95E2C5A9A70E87828F25478FCF26640B8DD41523165D46B`.
The paired official-score/activation summarizer is frozen at SHA256
`C535F66D605D410AD4ED72DB2D6708B948F2AF0F8B07961CBE00F90B4212EA2F`.
The corrected candidate source archive is frozen at SHA256
`B4985EE90EF5D0B6CE2F6919C7D60DACB29EF871E0452D3BEC5D2A461D68F1CF`;
the Linux historical binary built from it is SHA256
`705B524677D2A639AE7FD7776F00607410BF307AE345DA1961AD93B5A3B23701`.
The hash-verifying sequential v2 launcher is
SHA256
`5F26D63A335CBE980661B5D796CD2C68C2C90F47B552C8C813BC299D4D4080CF`.
V2 completed all four sides on the quiet Spot VM with matching frozen hashes,
empty stderr and zero invalid, emergency or certificate failures. The 5000-ms
control was `2/22/0`, net `+6` servings, but both differences occurred with
zero mechanism activation, so exact equivalence was not established. The
15000-ms development lane was `4/50/6`, net `-23` servings. The enabled side
found 51 eligible and 15 exact-valid proof witnesses but accepted none; all ten
score differences occurred without activation. The candidate was therefore
safe but inert and every score difference was background cutoff variance, not
a causal benefit. The sealed holdout remains unopened. Frozen log hashes and
the complete verdict are in
`research/evidence/SCORE-PROOF-PROTECTED-CONTINUATION-232-v2.md`.

Candidate-only proof-path persistence, checkpoint schema changes, protected
consumer, harness flag and tests must be removed. This does not delete a
designed production function: no proof argmax consumer existed at parent, and
the rejected experiment failed to establish a sound active equivalent. Strong
proof score/upper-bound capability remains intact.

The integration audit also found a pre-existing final-day lifecycle mismatch
after server acknowledgement. Pre-POST continuation certification already uses
official lexicographic final-day non-regression, but the later
`checkpointLedger -> actual ledger` acknowledgement invariant still calls
componentwise `protected_slack_ledger_dominates` unconditionally. It must use
the same official final-day relation (and retain componentwise dominance on all
nonterminal days), with a direct ACK test. This correctness repair was kept out
of the frozen score binary and cannot be used to reinterpret v2 results. It is
now the immediate non-research task, followed by the final protected-matrix,
BTC target-host and convergence audit.

## Current phase — SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231 rejected; commit boundary recovery next

Revision v1 remains invalid operational evidence and must never be resumed or
mixed with revision v2. Revision v2 completed both frozen development sides
`60/60 + 60/60` on the Spot VM with empty stderr, matching frozen hashes and
zero invalid, emergency, checkpoint, midday or terminal failures.

Candidate versus parent was `7/47/6`; all thirteen differences were tier 3 and
the aggregate delta was only `0/0/+1`. The mechanism rebased in `56/60` cases
and twelve score differences were activated, but high fuel was `2/11/3` with
net `-13` servings and a `-15` tail. Completed-proof coverage fell from `55` to
`54`, and strong-proof records fell from `468` to `467`. This violates the
preregistered preserved-proof-coverage gate and does not establish clear global
benefit with bounded nonsystematic downside. The sealed 108-case holdout remains
unopened and consumed no evidence.

Frozen development log SHA256 values are off
`6B1A833698CA929215600F3D36358C314FEEB0FF7DCF60FCEE23AB39AADA6826`
and on
`8DE0CDE21721C47192943CE77338A03D9C790A4C1EAC09AF6707A1A95E90A557`.
The rejected ACK anchor is removed from production, runtime, harness and tests.
It does not reopen rejected experiment 230: direct proof-witness injection had
no causal gain and remains closed. A future successor requires a new invariant-
derived consumer, fresh unopened seeds, preserved proof coverage and no
systematic fuel-regime downside.

The immediate task is durable commit-boundary recovery: commit accepted 227 as
the production diagnostic-isolation change and accepted 228 separately as the
historical-harness lifecycle correction. Rejected 229-231 remain research-only
evidence and must not enter either production commit.

## Current phase — SCORE-POSTACK-PROOF-WITNESS-230 rejected; authoritative-anchor attribution next

SCORE-229 is closed and its production changes are removed. The next registered
gap is the unwired result of `prove_remaining_horizon`: a complete proof stores
the exact best score for its frozen scenario portfolio but discards the argmax
route sequence, so it cannot affect W0 on the next authoritative day.

SCORE-230 is frozen before source change from HEAD `e8bf766` plus accepted
227/228 production-source diff SHA256
`97B9D096B1BFFA10432EB4EE476C651531D0BA389FE441DFDE2658C7D34720CD`.
Under a default-off flag it will retain the exact argmax path only for a complete
feasible strong proof, deduplicate its first plan into the existing next-day
cache, and rely on the unchanged W0 exact evaluator and independent validator
against the next authoritative state. A complete suffix may be retained only on
no-road maps; predicted future traffic is never promoted as authoritative.
The proof score and upper bound remain scoped to the frozen generated portfolio
and may not prune or dominate the global action space.

Frozen manifest
`research/holdouts/SCORE-POSTACK-PROOF-WITNESS-230.csv`, SHA256
`741944ED1C48FFF5BEBB3E3EF7D33BA82E77959136CC25D5592F97639544D8B3`.
Frozen runner SHA256
`9A30964B3D76637584E23654EC1FC9332FB9859422D7595D7B81176FBEB7B1B0`;
frozen summarizer SHA256
`C89CFBBED7CC2A80D0D986BC8E2583D6B74D806C26577B23CC9E01F5355D711A`.
Frozen VM source archive SHA256
`240F9F6B1008E7916345318C95531BA56655C66750DF0FBF255203682FECB733`;
frozen Linux tournament binary SHA256
`DC1D79F7B80D50989E0155348D3C04E90BF7179AF2720DB4DA6CF94F536479BB`.
Windows and Linux unit suites passed before development began.
Hash-verifying sequential VM launcher SHA256
`04C9FDFF1BD7553719ED4D6F8DE9C7F5126064E73C9C10CF5CA8A58C68282FA6`.
Development completed 60+60 with unchanged hashes and empty stderr. Candidate
versus parent was `5/45/10`, all 15 differences tier 3, aggregate `0/0/-17`,
and both sides had zero invalid/emergency/checkpoint/midday/terminal failures.
Aggregate proof coverage was equal at 49 completed proofs and 465 records.
Candidate produced 47 proof witnesses in 20 cases; 21 were exact-valid and
reused while 26 were rejected on authoritative state. Fourteen cases reused a
witness. Only two score differences coincided with witness production/reuse,
and both were losses: seed `5321000` lost 2 servings and seed `5323023` lost 22.
There was no causal gain. Logs SHA256: off
`3C6A66B0E8C1B155F2FCA87A1DFBCC612D204D123C2B4F8933F169978F3F1C17`,
on `1B9A2416DF9BF436FDAB0FB782DBDA630C72251E6A0F34B719E3C11FD10FBE2A`.
SCORE-230 is rejected before holdout and its sealed 108-case split remains
unopened. Direct W0 injection is closed: a frozen-scenario proof argmax is not a
safe canonical-search consumer after authoritative-state drift.

The next permitted work is read-only attribution of the 26/47 rejected proof
witnesses and the exact ACK/submitted-transition anchor across production HTTP,
runtime and the historical harness. No successor may be registered until this
trace distinguishes a real production anchor mismatch from expected traffic
counterfactual invalidation. Strong proof remains designed capability and is not
deleted merely because this consumer failed.

## Current phase — SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229 closed; proof-consumer attribution next

ATTR-228 restored faithful cache survival and classified the remaining tail as
a generation/diversification gap: the losing next day had no valid full-daily
cache to adopt. SCORE-229 is frozen before source change from production-source
diff hash `d56b9d3c4cd8894137a328ee42765902792cc2b6` over HEAD `e8bf766`.
It preserves the current narrow precompute as a stateful first phase across idle
slices, then uses only a parent-equivalent confirmation slice for a nested
production-width portfolio given the same-scenario cache in canonical insertion
order; the existing production-width `maximumSeedPlans = 2` cap remains
unchanged. Prior cache entries are never removed; new entries must exact-validate
and deduplicate. The first candidate incorrectly allowed diversification to
remain pending across the entire background budget. Source audit caught that
this could starve the existing `prove_remaining_horizon` phase, while its harness
did not execute proof work at all. That binary is development-only invalid
operational evidence; the sealed holdout remains unopened. Revision v2 must
transition to proof no later than the parent: broad generation may consume only
the residual of a narrow-completion slice and, only when that slice added a new
narrow plan, the single zero-add confirmation slice the parent would otherwise
run. A deadline-interrupted narrow attempt must not mark its scenario complete.
If that slice added an exact-valid plan, the parent-equivalent confirmation call
retries the scenario; if it added nothing, control still passes to strong proof
exactly as in the parent, so no extra retry slice is purchased.
The paired harness must execute the same precompute-to-proof state machine and
report proof completion/records before v2 can be judged.
Main solve, 5000-ms checkpoint, official comparator and submission path remain
unchanged. Frozen manifest
`research/holdouts/SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229.csv`, SHA256
`9D28F7FE1088AC6D46CB11C075D26663A7387C4497E408A6E816946C7C65CE33`.
Run 60 fresh paired revision-v2 development cases before any one-time 108-case holdout.
m-4195 is consumed attribution-only and cannot promote or tune 229.

The first Linux v2 run was invalidated after 16 causal-off results and before
causal-on started when source audit found that an interrupted narrow attempt
still advanced its scenario cursor. No score from that run has authority; its
archive SHA256 is
`ECA3A8BB3A6520D46EDAF04E1F800D7BC27FCD0E55D4FA98FF0D4394373F0F63`.
Corrected development ran from fresh root `/home/LMC/udon229-v2c-0826` with
Linux binary SHA256
`5F1C818C22BECC3C29C1A9F8F264D8532F5F0688F648463215DC73D36829C654`
and runner SHA256
`96E822D153211BE2A552A4305092AED156E5617537CC74FACB0CCD44060395BA`.
The hash-verifying resumable launcher is SHA256
`5E2985179AFC8E8144D76EAD156E70D929FA3968403EB9F5EA88B9FEC1DCAEAD`.
The aggregation script, including activation and proof-coverage strata, was
frozen before seeing paired development at SHA256
`35C79E33D26527252BAFDE54EB047022BBADC799B3092CFE2FD59BB4B04ABA59`.
The corrected 60+60 development completed with empty stderr and unchanged
frozen hashes. Candidate versus parent was `5/49/6`, all 11 first differences
were tier 3, and aggregate delta was `0/0/-68`. Both sides had zero
invalid/emergency/checkpoint/midday/terminal failures and identical proof
coverage: 38 completed proofs and 458 strong-proof records. Candidate telemetry
reported 3021 diversified contingencies across 22 activated cases, but all 22
activated pairs tied; every score difference occurred in a zero-activation pair.
The mechanism therefore showed no causal score benefit. Development log SHA256:
off `B7369E35B51CCC722DAA896D7337B3F26A7F62F9680845D30396A997461A22CB`,
on `0D2DC5A58045F77040C9D41D072518805A8300BF21C2BA193B182B58E55B26E5`.
SCORE-229 is rejected before holdout; the sealed 108-case split remains unopened.
Its production changes are removed without touching accepted 227/228.

SCORE-229 tests diversification on the existing production virtual-parent
post-ACK anchor; it does not change acknowledgement semantics. Protected
refinement can make the actual submitted terminal state richer, so every cache
still revalidates on the next authoritative state. If 229 is rejected and
development shows generation-rich but systematically rejected caches due to
that state drift, the next permissible attribution is actual-ACK-transition
anchoring on fresh seeds. A 229 rejection alone is not a post-ACK ceiling.

Queued after 229, read-only attribution first: `ResponseLedger::strongProofs`
is currently produced by `prove_remaining_horizon`, serialized, restored and
reported, but a complete call-site scan found no decision consumer. This is an
unwired designed capability, not deletable telemetry. After 229 closes, trace
the exact certificate domain (`remaining-horizon-persistent-frozen-scenario-
route-portfolios-v1`) and determine the sound consumer boundary before any
source change. A frozen-portfolio proof must never be misused as a global action-
space upper bound. No SCORE experiment may open until attribution proves a
general decision path where the certificate can safely tighten or preserve a
witness.

## Current phase — ATTR-CONTINGENCY-HARNESS-PARITY-228 closed; generation gap open

The audit after 227 found that accepted attribution 226 did not actually feed
its generated post-ACK contingencies into the following replay day. In
`replay-counterfactual --post-ack-ms`, the harness calls `record_submitted`,
generates contingencies, then calls `record_applied_transition`; that last call
clears `cachedContingencies` and `strongProofs`. `record_submitted` already owns
the accepted-transition belief, footprint, expected-agent and response-artifact
lifecycle, so the second transition call is both redundant and destructive.
The reported m-4195 tail change cannot be attributed to production's live
contingency net until this parity defect is corrected.

ATTR-CONTINGENCY-HARNESS-PARITY-228 is registered before source change from the
frozen 227 working tree. It changes only the replay-counterfactual post-ACK
lifecycle and preserves `record_applied_transition` for its intended external
or fallback callers. Frozen manifest:
`research/holdouts/ATTR-CONTINGENCY-HARNESS-PARITY-228.csv`, SHA256
`A914D1C9674EA839667EBAAE4826E255C43393D1C41AB0B963726959FAE3C425`.
On one quiet VM, run round-robin no-ACK control 12 times, the buggy 226
instrument 12 times and the cache-faithful candidate 24 times on consumed
`m-4195` mask 4. This is attribution only. The run completed 48/48 with empty
stderr and unchanged hashes. The no-ACK and buggy-226 lanes each produced
12/12 `8/40/130` and zero cache previews. The cache-faithful lane exposed 596
previews and scored `8/40/130` in 23/24 runs, but one run still fell to
`8/37/117`. In that tail, the eight day-4 previews contained six invalid plans
and valid plans of only `1/3` and `7/11`; no valid full-eight-brand plan existed
for adoption. This is the preregistered generation/diversification gap, not an
adoption gap. Experiment 226's live-cache causal claim is superseded because
its artifacts were cleared before use. Raw evidence archive SHA256
`069DCE99186433C2C05A999311F3840CBDF9081CD9C60EF9539FDC1D2DB512B6`.
No SCORE successor may reuse this replay for promotion or tuning.

## PERF-ROLE-DIAGNOSTIC-ISOLATION-227 accepted; BTC debt paid

The post-compact audit found that ATTR-224's diagnostic
`rolloutDailyTrace` is structurally dead to every decision but is not
operation-inert: it enlarges every `RoleAssignment` copied by the timed role
beam and performs a vector clear/push on every simulated rollout day. Because
short-role selection is deadline-bounded and already known to be bistable near
the cutoff, accepted SCORE-225 was causally isolated flag-off/on but its final
binary was not cleanly compared with the true direct parent `baebad8` without
this shared instrumentation.

PERF-ROLE-DIAGNOSTIC-ISOLATION-227 is registered before source change from
parent `e8bf766`. The candidate restores the direct-parent `RoleAssignment`
layout and moves the full per-day trace to a separate
`RoleSelectionDiagnostics` result. Production `select_roles_until` and the
diagnostic selection are compile-time-specialized: HTTP/sandbox/main take the
no-trace instantiation, while `replay-roles` and historical harness
`--role-details` take the trace instantiation. This preserves every intended
diagnostic caller end-to-end without a runtime branch, allocation or write in
the production hot loop.

Frozen manifest:
`research/holdouts/PERF-ROLE-DIAGNOSTIC-ISOLATION-227.csv`, SHA256
`9D5795D00CF9557C9EC7830F967B9974E0BFC269D5F6DC272CCDD2F41C6C0895`.
Development covers 32 fresh short-low-fuel/live-like and medium/general
controls, including the canonical `5000 ms` role window; the 40-case sealed
holdout remains unopened until development passes. Candidate must retain
diagnostic trace alignment, exact replay gates and accepted 225 low-fuel gains,
with zero attributable tier-1/tier-2 regression. BTC target-host remains the
final authority.

Development passed on the frozen candidate/e8bf766/baebad8 binaries. Candidate
vs e8bf766 is `9/18/5`, net `+40` servings, with zero tier-1/tier-2 loss and one
tier-2 win. Candidate vs true 225 parent baebad8 is `11/12/9`, net `+85`; both
tier differences are low-fuel wins (`+4`, `+3` daily), with zero tier loss.
The exhaustive diagnostic probe preserved every ranked role and all three
per-day traces exactly (`6,6,6,6,6`); the rollout-serving component remains
subject to its pre-existing inner `60 ms` cutoff. The one-time 40-case holdout
is authorized on the unchanged frozen binaries.

Operational correction before any score inspection: the candidate side of the
first holdout manifest completed locally, which is not an authoritative
environment for a cutoff-sensitive role experiment. Only completion counts were
observed; no result score was read or aggregated. Nevertheless all 40 original
holdout seeds are consumed and will never be rerun or used for promotion. The
baseline sides were not started. A fresh replacement manifest was frozen before
any further source change at
`research/holdouts/PERF-ROLE-DIAGNOSTIC-ISOLATION-227-v2.csv`; it will run
candidate/e8bf766/baebad8 sequentially on a quiet Spot VM and is the only
holdout with promotion authority for 227. Frozen SHA256:
`D526D86C5A9D9762B1FE873A2421BD5A51146EDC627F3B30E775128ABDDF1A8B`.

The authoritative quiet-VM holdout completed all 120 atomic cases with empty
stderr and zero invalid/emergency results. Candidate versus instrumented HEAD
`e8bf766` was `1/36/3`, net `-4`, with only bounded tier-3 differences
(`+1`; `-1,-1,-3`) and zero tier-1/tier-2 loss. Candidate versus the true
clean 225 parent `baebad8` was `6/33/1`, net `+229`: five tier-2 wins totaling
`+22` daily distinct, one tier-3 win `+2`, and one tier-3 loss `-1`. This
passes the preregistered global-benefit/bounded-downside score gate and retains
the accepted 225 mechanism. Evidence archive SHA256:
`E1007CF6055D27D3C6B8338A7CA94433F6658EBA54793EF198F24C0809ED6FF9`.
The score/semantic gate passed. The integrated candidate then paid the BTC
target-host lifecycle, composition and latency debt on a stable connection.

The first BTC attempt used the frozen Windows candidate binary
`DC01FE16131F26CF8FDFFDE24262F17A6AE9BE360FB85436079CB6933AD7223A` on
fresh advanced match `m-4276`. The assignment frame was the required `PPPT`,
confirming the short low-fuel production composition. The current workstation
network then took about 1587 ms from recorded action to response while only
about 1165 ms remained before `endsAt`; BTC acknowledged the request as day 2
and the stale-day invariant stopped the client. This is an inconclusive
transport sample, not 227 score evidence. The replay is retained and must not
be resumed. By explicit user decision, the canonical solver and accepted
reserve were not retuned to this temporary weak network.

Fresh advanced match `m-4290` used the current integrated Windows binary on a
24x24, seven-day, 100-step, four-agent, 18-spot, six-brand low-fuel
configuration. Role selection produced `PTPP` (three Patrol and one Tanker).
All seven actions were ACKed on the correct day with zero retry, deadline skip
or recovery wait. End-to-end response durations were `3340, 3203, 3017, 3088,
3074, 3085, 2162 ms` (maximum `3340 ms`, mean `2995.6 ms`); maximum reported
decision solve time was `2683 ms`. Exact replay reconciliation returned
`6/42/159`, seven valid actions and six reconciled transitions. The final rank-1
standing is operational context only. Replay SHA256:
`47BC89693F014D8FD2ABF83F5E4599C3619DB9A4FBC3426B22374AD76A61C9D6`.
Experiment 227 is accepted; its remaining source integration is part of the
current 229 working tree and must not be reverted with a failed successor.

## 226 superseded by harness-parity audit 228

ATTR-CONTINGENCY-FAITHFUL-TAIL-226 is superseded by 228. Its harness generated
post-ACK plans and then immediately cleared them through the external-transition
path, so its claimed 33% -> ~12% live-cache reduction was not causal evidence.
The faithful 228 replay proves the cache survives but still lacks a valid
full-daily day-4 plan in the observed tail. The low-risk successor entry point
is therefore post-ACK-only generation/diversification. It must preserve the
canonical solve, existing cache candidates and next-day exact validation; it
must not touch the main comparator or reshuffle protected budgets. The old
226 binary and logs remain consumed attribution evidence only and have no
production or promotion authority.

## 225 accepted production (low-fuel all-patrol floor)

SCORE-ROLE-LOWFUEL-FLOOR-225 closed accepted-production: under the same
production flag, short (<=5-day) all-patrol fronts at or below the existing
fuelLimit > maximumDaySteps boundary now take the single-tanker floor.
Attribution 224 proved the truncated rollout blind to multi-day fuel
exhaustion (per-day traces dip equally in collapse and healthy fixtures)
while the structural boundary classifies 10/10 observed cases. Dev easy
6/16/1 net +132 and sealed holdout easy 4/10/1 net +131 with EVERY tier-2
difference a WIN on fresh low-fuel seeds; frozen-replay gates identical to
accepted v3 everywhere. Production binary SHA256
AAF73A3ADCD2E47B7C52A70E0A11B6767B37CB9C574BA7891E67399596FE08A7.
BTC debt: fresh short practice matches (a short low-fuel config expects
>=1 tanker in the assignment frame).

Also closed today: ATTR-SHARED-WINDOW-RESIDUAL-223 (the 216 scheduling gap
measures ZERO protected residual on 23 post-219 live days — the 217-invited
shared-retention successor is moot; the remaining +2..+6/day global
residual is certificate-blocked, not time-blocked) and
ATTR-ROLE-COLLAPSE-DISCRIMINATOR-224 (rollout traces carry no signal; the
structural fuel boundary does).

## 222 closed; the 216-invited shared-traversal successor was evaluated

221 BTC debt PAID 2026-08-26: m-4208 (the exact 12x12 five-day 8-brand loss
regime) selected 3 patrols + 1 tanker and finished RANK 1 at 8/40/120 vs
109/105/103; m-4209 (new 24x24 five-day 5-agent regime) kept its all-patrol
front and finished RANK 1 at 7/35/187 vs 178/175/173.

ATTR-TERMINAL-SUFFIX-POISON-222 closed accepted-attribution: the m-4195
117-tail is next-day SEARCH VARIANCE from clustered day terminals (a run10
witness proves an 8-brand day-4 from the same cluster exists), the
TerminalSlack oracle already measures the hazard but sits below uncertified
quantile estimates in better_evaluation, and the offline counterfactual
overstates the live tail because it never runs the post-ACK contingency
net — live m-4208/m-4209 held full daily every day. No SCORE opened
(main-comparator risk class vs zero live occurrences); successor
certificate design and reopen conditions recorded in the ledger row and
research/evidence/ATTR-TERMINAL-SUFFIX-POISON-222.md.

## 221 accepted production (short-horizon role fallback)

SCORE-ROLE-SHORT-HORIZON-221 is closed **accepted-production** (final
mechanism v3 after two pre-registered revisions; full lineage in the ledger
row and research/evidence/SCORE-ROLE-SHORT-HORIZON-221.md). Production now
extends the accepted single-tanker fallback to `day_count() <= 5`, acting
ONLY when the incumbent role-beam front is tanker-heavy
(patrolCount <= agent_count-2) — the exact class ATTR-220 proved broken with
full-engine counterfactuals (live losses m-4195/m-4196 at 100/94 vs winners
114/113; 3-patrol counterfactuals 130/130). All-patrol and single-tanker
parents are never displaced. Long-horizon behavior byte-identical; flag-off
byte-inert; dev (4 suites) and a fresh sealed holdout show zero
flag-attributable regression under the interleaved off/on attribution
standard. Production binary SHA256
`4EB926039A50D28F2202BFBE840866D770FD1928119183441C0034377BAA2FE4`.
BTC debt: fresh short-horizon practice matches to confirm live composition
(>=3 patrols) and rank.

Successor axes registered (NOT covered by 221): (1) bistable role selection
under wall-clock rollout truncation — same-flag composition coin flips worth
±36 servings, including a fuel-tight class where an unlucky all-patrol draw
loses tier-2 daily (seeds 7300017/8340005/8400006); (2) synthetic harness
maps cannot reproduce the live 2-tanker front pathology — live-replay gates
are the activation instrument; (3) single-tanker agent-placement jitter; (4) mid-match terminal-position suffix poisoning - localized on m-4195 mask4: a day-3 solve that parks all four agents on one cell (full fuel, day 3 unharmed) collapses day 4 to 5 brands (~4/9 of runs, 117 vs 130); the day profile cannot see it. 

## Superseded in-development notes (220/221)

Two fresh target-host losses (m-4195, m-4196: 12x12 five-day 60-step fuel-180
8-brand practice regime, both rank 4 at 8/40/100 and 8/40/94 against winners
114/113) are fully attributed by ATTR-ROLE-SHORT-HORIZON-220
(accepted-attribution) to the day-0 role composition: live selection chose
2 patrols + 2 tankers because `select_roles_until` for `day_count() <= 5`
skips the accepted long-horizon incomplete-rollout fallback (decision.cpp
gate from 8caea45) and trusts truncated-rollout daily-distinct noise
(37-38 vs 36, rank 1 alternating across identical reruns). Same-binary
`replay-counterfactual` scores 8/40/130 with 3 patrols + 1 tanker and
8/40/128 / 8/40/118 with all patrols on the two losses — rank 1 either way —
while single-tanker placement can lose tier-2 (mask4 on m-4195: 8/37/117),
so the successor prefers the all-patrol row only under threshold-free
lexicographic rollout dominance and otherwise keeps the single-tanker floor.
Not machine speed (deadlineReached=0, checkpoint predictions matched realized
exactly); not the protected-lane stack (0 acceptances, inert in this regime).
Evidence: research/evidence/ATTR-ROLE-SHORT-HORIZON-220.md.

SCORE-ROLE-SHORT-HORIZON-221 (open, registered before source change):
default-off `includeShortHorizon` parameter on
`apply_incomplete_long_horizon_role_fallback` + engine/session setter +
harness/btc `--short-role-fallback`. Frozen-replay gates all passed on the
new binary FDAAD8BBFCA795A9998C52B18253E9C86B148CD0876E9E3DF1F561E4BAA66269:
flag-off replay-check byte-reproduces m-4043/44/45 (6/42/127, 6/60/364,
6/60/144); flag-on beam[0] becomes PPPP on m-4195/m-4196 (220-matrix
counterfactual 128/118 >= 118), stays PPPP on m-4149/m-4155, stays PPP on
m-3810/m-3907 (complete rollouts), and long-horizon beam[0] is
flag-independent (m-4043 single-tanker agent choice jitters run-to-run under
BOTH flag values — pre-existing production nondeterminism, verified 6 reruns).
Unit tests extended (5 new fallback cases; all tests pass). Dev A/B running
on frozen splits research/holdouts/SCORE-ROLE-SHORT-HORIZON-221-dev.csv
(SHA256 301CCF88F0EB6D9E5B07CBBA535ECB572760185F984B1B335C76B5FECC426E17):
stratified-easy 7300001+24 and general 7320001+12 active,
stratified-medium 7300001+6 inertness control; sealed holdout
research/holdouts/SCORE-ROLE-SHORT-HORIZON-221.csv (SHA256
EFC26FFAAE4909A941035674C3C346D27D8C5DC5A565408E9350BB04C76DC446).

## 219 accepted production checkpoint

Experiment 218 is closed rejected after its causal application gate found a
fresh `6/30/174 -> 6/30/162` regression on seed `5021002`. Interleaved
`off/on/off/on` reproduced `174/162/174/174`; only the first on run accepted a
strict public-window takeover. The simple continuation therefore cannot protect
the complete current 5000-ms checkpoint across later deadline-bounded work. Its
sealed holdout remains unopened.

Successor 219 is registered before source change with frozen manifest
`research/holdouts/DEADLINE-CHECKPOINT-CLOSED-LOOP-219.csv`, SHA256
`48200C4B086EBE73DFC0E476A83ECDB72118B3BF1D24871318B70A29DC0D4794`.
It computes the main solver exactly once per day, runs the unchanged current
5000-ms protected checkpoint once on a checkpoint state, exact-replays that
same plan on the authoritative richer state, and only then permits protected
public-window continuation. On nonterminal days checkpoint and richer states
must preserve equal road footprint and ordered terminal cells with fuel/ledger
dominance. On the terminal day exact validity and official lexicographic strict
gain replace the future-transition certificate. There is no Long solve,
resubmit, shared retention, second solver or fixture dispatcher.
The first partial control run was discarded before any 15-second development
case because public work shared the checkpoint refiner cache and could warm a
later checkpoint day. The corrected implementation snapshots the checkpoint
cache after that day's unchanged protected prefix and discards all public-side
cache additions at day end. Its 5000-ms control is measured in-process against
`checkpoint_closed_loop_parent`; separate timed processes are not treated as a
byte-equivalence oracle because their deadline cutoffs can legitimately differ
under host load.

Frozen development is complete. The 24 in-process 5000-ms controls passed with
zero mismatch. The 60 public-15000-ms cases yielded `14/46/0`, net `+19`
servings, all tier 3, with 16 takeovers and zero deadline, invalid, emergency,
refiner or checkpoint failure. Gains span all fuel profiles, both role modes,
easy/hard/very-hard and five traffic families; medium and fuel-tight are pure
ties. This is broad strict benefit with bounded downside, so the preregistered
one-time sealed holdout is authorized. The 45/60-second escalation stays closed
because every 15-second continuation reached fixed point before its deadline.
The sealed `5060xxx` holdout was opened exactly once on the unchanged frozen
binary and completed all 108 cases at `29/79/0`, net `+88` servings, all tier 3,
with 50 takeovers and zero deadline, invalid, emergency, refiner or checkpoint
failure. Gains span all four difficulty tiers, all three fuel profiles, both
role modes and all six traffic families. The frozen research mechanism is
accepted; the 45/60-second escalation remains closed because no 15-second case
reached its continuation deadline.

Production integration is complete. The three-branch BTC runtime persists and
resumes virtual-main, complete 5000-ms checkpoint and authoritative richer state
without rerunning the timed solver. The independent Linux source-rank defect is
repaired by deterministic pruning tie-break and passes on Linux and Windows.
All 24 fresh five-second controls matched the checkpoint exactly. A separately
frozen 12-case production matrix passed at `3/9/0`, net `+6` servings, all tier
3, with zero loss, deadline, checkpoint/public failure, invalid or emergency.
Corrected BTC target-host runs `m-4153`, `m-4154` and `m-4155` completed every
day with exact replay; `m-4155` also proves the pure-5000-ms path emits no
continuation state. Production evidence and hashes are in
`research/evidence/DEADLINE-CHECKPOINT-CLOSED-LOOP-219-PRODUCTION.md`. The
consumed 219 development and holdout sets cannot be inspected for tuning or
reused for any successor.

No logic experiment is open after this checkpoint. Research may reopen only
from a fresh non-consumed BTC/opponent counterexample, a newly observed
telemetry mismatch, or an invariant-derived mechanism registered with a new
unopened manifest. Re-running or re-slicing the consumed 217--219 holdouts is
forbidden and is not evidence of further convergence.

### Closed predecessor — DEADLINE-PUBLIC-WINDOW-218

The rule-level review is closed: HEXUDON does not impose a universal 5000-ms
response deadline. Response time is published per match; archived BTC setups in
this repository include 5-second and 15-second days, and the supplied national
configuration includes 45-second days. The old 5000-ms universal hard cap was a
conservative project decision, not a competition rule, and is superseded for
prospective research.

Experiment 218 keeps the exact current 5000-ms result as an immutable checkpoint
but permits the accepted protected refiner to consume the trusted public time
remaining after transport safety. It never reruns the main solver with the long
budget. This directly tests the gap from 216 without the shared-retention change
rejected by 217. The frozen manifest is
`research/holdouts/DEADLINE-PUBLIC-WINDOW-218.csv`; development covers 5000-ms
equivalence and 15000-ms public windows, with preregistered 45000/60000-ms
escalation only when 15 seconds remains deadline-limited or has not reached a
fixed point. Source parent remains `177b588` until the candidate is built.

The non-propagating 15-second development probe is complete: `17/60` cases
contain 23 strict protected takeovers for `+28` servings, with zero tier-1 or
tier-2 change, deadline or refiner failure. Gains span all four difficulty
levels, all fuel profiles, both role modes and all six traffic families. Eleven
takeovers came from the sparse refiner; the remainder demonstrate additional
compatible WAIT detours and satisfy experiment 204's frozen reopen condition.
No probe reached the 15-second deadline, so the 45/60-second escalation stays
closed. This is candidate-supply evidence only: 218 cannot open holdout until a
causal application run preserves the complete 5000-ms checkpoint across the
full multi-day loop.

## Closed experiment — SCORE-MIDDAY-SHARED-TARGET-217

Experiment 216 is closed as an accepted scheduling-gap attribution. Across 45
nonterminal days, unconstrained sparse one-agent exchange improved 45/45 for
`+136` servings; the fully protected same-terminal/future-domain certificate
improved 11/45 for `+17`. Fresh `m-4108` fired on all 9 nonterminal days for
`+15`. The accepted 215 target suffix saw none of these routes because it starts
a second complete label traversal only after the accepted global traversal has
already exhausted the protected deadline.

Experiment 217 is the minimum semantic-preserving correction: one label search
retains both the existing global pool and the incumbent-terminal pool. The
global pool is still evaluated first in parent order; the target pool is then
available without repeating the expensive traversal. Before any score gate,
the combined enumerator must reproduce both standalone retained sets exactly
without a deadline. The fresh development and sealed-holdout manifest is
`research/holdouts/SCORE-MIDDAY-SHARED-TARGET-217.csv`, SHA256
`B17826FE51D17D0FA217E60699A26E6F6DC42A98E70C5053942E621FF7316940`.

The first compile-time-separated Linux tournament build, SHA256
`2DD7114D8BA5D6B3C2EC748D1F3A2CEEE1BF8FD02FD865F74FC53D16D34C6FAD`,
was rejected before holdout at `3/26/31`, net `-528` servings. Exact telemetry
showed the candidate produced zero target routes on all 60 cases: the combined
enumerator had been wired into the earlier terminal-sparse function instead of
the mid-day function, while the mid-day target suffix moved empty vectors. The
failure is implementation attribution, not a test of the registered mechanism.
The complete logs and hashes are preserved in the 217 evidence; the sealed
holdout remains unopened.

The corrected source-frozen tournament binary is SHA256
`F848AE893D5F9D38F30BC99EA68CD6AEC293151E9057B9A431A5EF44A66230A4`.
It restores the terminal-sparse parent path and enables combined enumeration
only inside `refine_midday_chains`. A one-case target-host smoke emitted 576
target routes, 318 dual-valid target plans and three target acceptances with
zero invalid/emergency/failure. The additional-terminal state remains
compile-time absent from the off path. An earlier 11-case unconditional-
allocation pilot is preserved under
`logs217-invalid-unconditional-target-reserve` and is not evidence.

The claim probe reproduced both standalone retained views exactly on 40
patrol/day comparisons across eight consumed replay-days: eight completion
markers, zero parity failures. The corrected claim-probe binary SHA256 is
`1A93C307D3D6747C7C7CDF4F340165762FC11C429B2A7E0BB0C96DA691673DF0`.

Corrected development passed at `29/28/3` W/T/L, net `+246` servings,
zero tier-1/tier-2 difference and zero invalid/emergency/mid-day failure. The
three losses were bounded tier-3 losses `-1,-10,-1`, all in very-hard long
windows; gains spanned every fuel profile and both role modes, with 29 wins and
maximum `+50`. Target acceptances increased from 239 to 358. The frozen sealed
holdout was therefore opened exactly once with the same binary and completed
all `108 + 108` pairs.

The sealed holdout finished `40/58/10`, net `+455`, gross gain/loss
`+491/-36`, tail `+75/-9`, with every difference at tier 3 and zero invalid,
emergency, mid-day failure or terminal-sparse failure. The candidate was
strongly positive in short protected windows (`35/14/5`, `+466`) but failed
the registered across-strata gate in long windows (`5/44/5`, `-11`); the
fuel-tight and rare-brand families were also net negative (`-4/-11`). The
unconditional flag is therefore rejected and must not be enabled in production.
Off/on/stderr SHA256 are `4E0E7448...`, `6D692282...`, and the empty-file
`E3B0C442...`.

This is new evidence for a simpler deadline successor, not permission to tune
217 by fixture: preserve the exact 5000-ms production decision and use an
authoritative public window above five seconds only for a bounded continuation
of the already accepted protected target-terminal refiner, followed by one
submission. Matches at or below 5000 ms remain byte-identical. Ordinary Long
replacement and profile-based resend remain rejected by 166--186.

An independent pre-existing Linux gate remains open outside experiment 217:
the direct 215 parent and 217 both retain harvest-extension source ranks 1 and
2 but can prune rank 0 in the old column-diversity fixture; Windows retains it.
Because 217 does not touch column generation, the causal A/B may proceed on the
same binary, but no production promotion may occur until that portability bug
is repaired and score-gated separately.

### Closed attribution — ATTR-NONTERMINAL-SPARSE-EXCHANGE-216

Fresh target-host match `m-4108` exposes an earlier and broader tier-3 gap than
the accepted 215 suffix: on day 1 the submitted bundle scored `6/6/44`, while
the existing full-route sparse enumerator reconstructs an exact `6/6/48`
one-agent exchange and a same-terminal protected `6/6/46` exchange at the
unchanged production caps. Because the gap exists before future-horizon effects
and the nonterminal production lane currently searches wait-detour suffixes
rather than complete sparse replacement routes, experiment 216 is sweeping all
nonterminal days in six frozen consumed BTC replays to measure recurrence and
strata. This is attribution-only: production remains exactly `177b588`, ranks
do not promote logic, and any successor must use fresh seeds plus an incumbent-
protected exact certificate and a BTC target-host budget gate.

Frozen manifest:
`research/holdouts/ATTR-NONTERMINAL-SPARSE-EXCHANGE-216.csv`, SHA256
`9DE6557133B1649B557D4C30C210D72029D83E2407500965093575E49A02AE4E`.

### Accepted checkpoint — SCORE-MIDDAY-TARGET-FOLLOWUP-215

Experiment 215 is closed and enabled in production. It preserves the complete
accepted 210 global-pool ascent as an order-identical protected prefix, then uses
only remaining time to enumerate routes conditioned on each patrol's already
protected terminal. Every takeover still requires exact simulator and
independent-validator agreement, equal road footprint, identical terminals,
patrol-fuel dominance, brand monotonicity and a strict official-score gain.

The frozen 60-pair development gate was `31/27/2`, `+507` servings. The sealed
108-pair holdout was opened exactly once and produced `57/47/4`, net `+721`,
with gross gain/loss `+740/-19`, maximum gain/loss `+63/-9`, and no tier-1 or
tier-2 difference. The suffix recorded 303 acceptances across 52 holdout cases;
conditioned on activation the result was `51/0/1`, `+712/-4`. Every difficulty,
fuel and role stratum was net positive. Invalid, emergency, validator mismatch
and lane-failure totals were zero. Frozen holdout log SHA256 values are
`5ED3FDDA...` off, `6510224F...` on and `E3B0C442...` empty stderr.

The promoted BTC binary SHA256 `500A5161...` passed three fresh target-host
matches under the canonical `5000 ms` cap. Across `m-4108`, `m-4109` and
`m-4110`, all 24 actions were HTTP-valid, all 21 observable transitions
reconciled and skip/WAIT/emergency/failure totals were zero. On `m-4109` the
target suffix executed 161 routes and produced 148 dual-valid plans without a
strict takeover or failure, proving runtime wiring and target-host safety. Exact
evidence is in `research/evidence/SCORE-MIDDAY-TARGET-FOLLOWUP-215.md`.

BTC ranks are not promotion evidence. The non-first `m-4108` (`6/60/496`) and
`m-4110` (`6/42/163`) results remain fresh independent serving-score
counterexamples for the next convergence sweep; neither is an attributed 215
regression because the target suffix made no accepted replacement in either
match.

### CLOSED target-terminal attribution: ATTR-MIDDAY-TARGET-TERMINAL-214 — accepted generator gap

The complete production-cap query covered all 48 registered nonterminal
replay-days: 8,311 routes, 7,347 generated plans, 7,347 dual-engine-valid,
zero invalid, and 45 strict certificates. Pre-210 matches contribute 43
certificates. Decisively, the 210-enabled `m-4044` still contains two admissible
witnesses: days 6 and 8 each have a same-terminal, same-footprint,
fuel-dominating `+1` serving replacement. `m-4043` and `m-4045` contain none.
Clean log SHA256
`1B16099F1012769626E0CC581DC28732E307EEC82A923CC986823C7223FDAC4C`;
empty stderr SHA256
`E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.
Therefore 212's lane-vindicated/practical-ceiling interpretation is falsified.
The six consumed replays authorize 215's mechanism only; they cannot promote it.

### Prior provisional competition checkpoint (superseded as a ceiling claim)

All evidence-backed axes are closed. The guaranteed competition build is
the `288d17f` lineage (+212/213 records): production binary SHA256
`341C7464FEBCA337A6468767CB6FE6ABEF0A1E41DF8D49C43CE6926676D2D874`,
decision-identical to the live-gated `B8D7DD21…` binary (differs only in
telemetry fixes and default-off research flags; replay-check reproduces
all three debt matches byte-exact: 6/42/127, 6/60/364, 6/60/144). Open
operational items live in COMPETITION_RUNBOOK.md. Remaining theoretical
gaps (positioning days, coordination-shaped m-4044 residual) are closed
pending new soundness theory (164/167/168) — not reachable by any
admissible mechanism surveyed in 209-213.

### CLOSED main-solve retention: SCORE-SERVING-RETENTION-213 — rejected (kill condition a)

Closed 2026-08-25. Unconditional serving-coverage retention in sub-3x fuel
regimes triggered the pre-registered kill condition on the dev gate:
25W/18T/17L +20 (198's mixed-nonmonotonic signature), a tier-2 loss (seed
4960007 daily 29→27) and large negative outliers (-51/-30/-27/-25) against
max gain +29; fuel:high control 2/14/0 stayed clean (pass already on
there). ALL 213 source changes reverted byte-identical to the accepted
lineage; full suite passes; holdout never opened. The 198 lesson is now
confirmed on a directed capability change: main-solve retention may only
be reopened with a per-day exact-improvement certificate mechanism, never
a blanket gate. Evidence: dev logs in `research/evidence/`, ledger row.

### CLOSED live-yield attribution: ATTR-MIDDAY-LIVE-YIELD-212 — lane vindicated

Closed 2026-08-25. Read-only probe over all six live replays, no
deadlines, production and raised caps: every certified improvement lies in
the PRE-210 matches (+13 servings at production caps — the class 210 was
built for is real at daySteps=100), while the three 210-enabled matches
contain ZERO certified improvements at both tiers — the live lane's 0
acceptances were the correct answer, no defect, no cap-adaptation
candidate justified. The residual m-4044 gap (~0.8 servings/day at 8
agents) is coordination-shaped, beyond every admissible certificate.
Evidence `ATTR-MIDDAY-LIVE-YIELD-212.md`.

### CLOSED mid-day pair exchange: SCORE-MIDDAY-PAIR-EXCHANGE-211 — rejected-inert

Closed 2026-08-25 per the pre-registered kill condition. The first dev
artifact (maximal-only pair pools) was DISCARDED after an external review
flagged a false-inert risk under the tight same-terminal pre-filter; the
amended rerun (maximal+supplemental pools, analyzer conditioned on
`midday_pair_acceptances`, witness-fuel telemetry fixed, direct unit tests
added) measured **0 pair acceptances on both sides across all 60 dev
cases**, W/T/L 1/57/2 delta -1 (57 ties — clean inertness between two
identical accepted configs), zero invalid/emergency, runtime parity. After
the 210 one-agent mid-day fixed point, the both-terminals-pinned
equal-footprint certificate leaves no joint improvement anywhere: the 041
result reproduces at mid-days. NO production change
(`enableMiddayPairExchange` default-off, byte-inert, unit-tested); the
sealed holdout was never opened. Reopen only with a sound certificate that
frees a terminal (blocked by the closed multi-day theory, 164/167/168).
Evidence `SCORE-MIDDAY-PAIR-EXCHANGE-211.md`. The same review pass also
fixed: 208 manifest provenance (post-amendment file SHA256 `738502F0…`
recorded), 210 certificate wording (state-identical → future-domain
dominance; loss relabel), HTTP `firstRoundScore` copy, and the 207/211
witness-fuel telemetry ordering.

### ACCEPTED protected mid-day deep-chain lane: SCORE-MIDDAY-CHAIN-ADOPTION-210

Accepted 2026-08-25 and enabled in the production HTTP client. The BTC
live-gate debt was paid on 2026-08-24 with the frozen production binary
SHA256 `B8D7DD216E91D0C921DC012905EE816B833BFAA6282A6D8B9011FA2E15990342`.
Same-binary causal A/B (both sides
terminal-pair ON, differing only in `--midday-chain 0|1`, quiet VM,
sequential sides): development paired=60 **39W/17T/4L, +336 servings**,
acceptance-conditional 38W/1T/2L; sealed holdout paired=108
**63W/42T/3L, +465 servings**, acceptance-conditional **62W/0T/2L**,
acceptances 222 across 64/108 cases, off-side control 0, zero
invalid/emergency/lane-failure, runtime parity (2461/2458ms mean,
3020/3021ms max). Every lane positive — fuel low 22/17/0, hard 24/11/1,
very-hard 28/11/1. Loss attribution (wording corrected 2026-08-24 after
external review): 3 losses with acc=0 have provably unchanged transitions
(pure 165 timed-search noise); 4 losses with acceptances have certified
DOMINATING transitions (patrol fuel may be strictly higher), i.e. bounded
realized regression / timed-policy sensitivity — the +801 aggregate
asymmetry bounds this far below the gain and rejects balanced-
nonmonotonic. Mechanism: `refine_midday_chains` one-agent deep-route
substitution after the wait-detour fixed point, accepted only via the
unchanged `strict_protected_improvement` certificate (future-domain
DOMINANCE: positions/footprint exactly preserved, patrol fuel ≥ — a
superset domain, not a state-identical transition). BTC `m-4043` (24x24/7d/100 steps/4 agents/18 spots/low),
`m-4044` (32x32/10d/8 agents/18 spots/default), and `m-4045`
(32x32/10d/4 agents/18 spots/low) produced 27/27 valid acknowledgements,
zero emergency, max response 4072 ms and max solver 2954 ms. The protected
lane was active on 24/24 nonterminal days with reserve 1100 ms, zero
mid-day deadline/failure, 2656 routes and 149/149 generated plans passing
both engines. `middayChainAcceptances` telemetry was emitted on every
protected-slack record but remained zero on these three fixtures, so this
gate proves live wiring/validity/reserve rather than a live score takeover;
the acceptance-conditional causal authority remains the frozen dev/holdout.
Replay-check rebuilt `6/42/127`, `6/60/364`, and `6/60/144` exactly.
Evidence `SCORE-MIDDAY-CHAIN-ADOPTION-210.md` (all log/replay hashes inside).
Successor axes
left open: main-solve retention (prune_columns spot-diversity gate in
sub-3× fuel regimes) as its own registered experiment; positioning days
(209 shape 2) stay closed pending a sound multi-day certificate.

Original registration: parent `e9d3962`, production-protected-lane candidate.
Target: the 209 position-then-sweep generation gap and the live m-4039
~3 servings/day chain-depth deficit (fuel=2x steps regime, tier-1/2 tied at
cap). Root causes located in code: `prune_columns` runs its serving-coverage
diversity pass only when `fuelLimit >= 3*daySteps` (planner.cpp:4229/4384 ->
1387) and skips brand-marginal-zero columns (planner.cpp:1359), so in
brand-saturated regimes deep serving chains survive retention only as sort
tiebreaks; the accepted terminal refiners (191/207) are guard-restricted to
`dayNumber == day_count()` and cannot reach mid-days.

Mechanism (minimal intervention): a new protected mid-day lane in
`ProtectedSlackRefiner` — one-agent deep-route substitution from
`enumerate_sparse_anytime_resource_routes`, iterated to a fixed point (191
pattern), accepted ONLY via the unchanged, production-accepted
`strict_protected_improvement` certificate (slack_refiner.cpp:93: equal
road footprint + same terminal cell + patrol fuel >= + brand monotone +
strict lexicographic day gain). The certificate makes mid-day acceptance
sound by construction (future domain preserved; no opponent-traffic side
channel — equality, not subset). Runs strictly after the certified
incumbent and existing refiners, consumes only leftover protected budget;
main timed solve untouched (194/195/198); incumbent replaced only by exact
improvement (201); no realized-suffix comparison (164). Same-binary causal
A/B flag `--midday-chain 0|1`. Frozen manifest
`research/holdouts/SCORE-MIDDAY-CHAIN-ADOPTION-210.csv` SHA256
`33E1591047103CE10BFF4A35A7A488826B8CB9B321BDF596AF0DDECFE110D4FB`
(fresh blocks: dev 4920000/4921000/4922000/4923000, holdout
4930000/4931000/4932000/4933000; standard fuel rotation straddles the
3*daySteps regime boundary). Pre-registered kill conditions: yield
starvation (~0 dev acceptances) closes rejected-yield and requires a
separately-registered relaxed-certificate successor; any
invalid/emergency/runtime-parity violation closes rejected; acceptance
needs positive dev W/T/L -> sealed holdout -> BTC live gate (207 chain).
Main-solve retention (the prune_columns findings) is explicitly OUT of
scope — a future experiment with its own registration.

### CLOSED coupled-suffix pre-gate: ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209 — accepted-attribution-generation-blocked

Closed 2026-08-24 on 3/3 witnesses (walker fidelity exact under all three
causal policies; every day dual-engine validated). Verdict per the
pre-registered rule: **generation-blocked with per-day attribution** — the
oracle's suffix advantage is a position-then-sweep couple, and its decisive
days are absent from BOTH witness-caps and production-caps candidate sets
at every cap 32..256, in exactly two shapes: (1) restrained positioning
days, dominance-pruned (1721100 d3; 1720100 d1 — the winning ROOT itself:
`3.2.-13` serves 2 spots then waits, buying three consecutive 6/6/5-serving
sweeps while the production head stops at 6/18/18 vs oracle 6/19/19); (2)
full-depth sweep days present in exact-orienteering reachability
(mask_maximal=1) but never adopted into portfolio columns — an adoption
gap (1721100 d4; 1721200 d3 max/status; min-dwell suffixes). Zero
present-but-dropped-by-selection days: a frontier selector has nothing to
select (076/081, 100/113/203 confirmed). All remaining days expressible at
first_cap=32. Axis redirected to targeted suffix GENERATION: protected-lane
adoption of deep maximal routes + restrained positioning columns enabling
next-day sweeps, honoring 194/195/198 and 201, runtime-signal gated;
converges with the live m-4039 ~3 servings/day chain-depth gap at
fuel=2x steps. Evidence `ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209.md`; logs
4017E640…/A1614100…/2C16917F… (full hashes in evidence and ledger).
Witnesses 2-3 solved on the 32GB VM (1720100 subtree 44.5M states / 47.3G
transitions, ~9h).

Original registration: parent `690728a`, research-probe-only. Question: can
the production generator even EXPRESS the oracle's suffix day plans (days
2-4) along the winning coupled trajectory of the three live 185
counterexamples (1720100 root 223 `3.2.-13` 6/19/19; 1721100 root 36
`3.2.2.2.0.1.5.-3` 5/17/18; 1721200 root 7 `3.2.2.2.0.0.5.5.-2` 5/20/21)?
200/202 proved the day-1 root is generated, retained and forceable while the
suffix still loses; if suffix routes are absent at witness and production
caps, any frontier selector is dead on arrival (076/081, 100/113/203) and
the axis redirects to targeted suffix generation. Method: singleton
root-stream re-solve of the winning root, walk the memoized argmax
trajectory per causal policy, per suffix day measure spot-mask membership in
exact-orienteering routes, witness-caps W1 contains-outcome, production-caps
contains-outcome and the first-cap ladder 32..256 — all from the coupled day
state with both teams' traffic. Frozen manifest
`research/holdouts/ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209.csv` SHA256
`849C5162492219F6230EFA82B4C898072E0C12A7A08788433BD9EB5D98024B73`
(amended pre-measurement: oracle_scope normalized to the probe-validated
string; winning root indices frozen in the ledger row and above). Venue:
local idle machine (the 207 holdout owns the VM).

### Queue note (unregistered pre-scan): opponent-traffic persistence axis CLOSED as low-value

2026-08-24, read-only replay scan (`research/probes/traffic_persistence_prescan.py`)
across all 13 multi-day archived BTC matches, 7007 road-day status
transitions: opponent-driven BUSY roads decay to smooth 92% of the time
(the production opponent-zeroing assumption in the future-witness rollout,
`decision.cpp:2552`, is empirically near-correct there); opponent-driven JAMMED
persists ~47% but is rare (34 opponent-only jammed road-days total, ~2.6 per
match, so a perfect model would fix ~1.3 mispredicted road-days per match on
the 2-day horizon). Combined with the closed 128 axis this does not justify
an experiment; do not reopen without a regime where opponent jams are an
order of magnitude more common.

### Closed: ATTR-COVERAGE-REGIME-208 — accepted-attribution-venue-artifact

Registered and closed 2026-08-23, parent `690728a`, research-only. The
m-3986 extreme-regime coverage collapse (and m-3897's starved-day servings
loss) attributes to **column-generation starvation whose dominant cause is
operator-machine CPU contention**, not an algorithmic gap:

- Replay side: sequential per-agent generation loop (`planner.cpp:2820`)
  under a shared deadline; starved days show trailing agents with zero
  Pareto queries, cache 0 hits, colGen 1200-1340ms. m-3986 starved 9/10
  days, m-3897 4/10, m-3908 (players=4 control) 0/10. Selection and master
  innocent.
- Registered synthetic grid (SYN-COLGEN-208, 48 matches): **zero starved
  days in 480 match-days at idle; tier-2 coverage 94-100% of the viability
  cap on every axis**, including brands==spots at 32 spots / 200 steps.
- Discriminator: starved production days lose 2-3x labels/ms throughput;
  controlled 8-core contention on the registered cell reproduces the live
  signature on demand (32/312/868 idle -> 32/269/683 contended; colGen
  1200-1450ms; master combos 0-129). m-3986 provably ran inside the local
  207-dev compute window.

**Operational rule adopted: never run experiment compute on the match
machine during a live match** (measured cost -43 tier-2 / -185 servings).
Reopen a generation-rebalancing candidate only on clean-venue evidence of
zero-query patrol agents; evidence `ATTR-COVERAGE-REGIME-208.md`.

### ACCEPTED terminal pair-exchange: SCORE-TERMINAL-PAIR-EXCHANGE-207

Registered 2026-08-23, parent `690728a`. Gap: accepted 191 iterates one-agent
sparse exchanges to a fixed point on the terminal day of the sparse
(dense-unsupported) domain, which no ceiling oracle covers (070/147 closed
only dense/small domains); a coordinate method stops at one-agent local
optima, so joint improvements that need two patrols to swap or re-partition
contested spots/stock simultaneously are unreachable (precedent: 041
one-exchange inert versus 042 multi-agent coordination finding +1).

Candidate (working tree, `src/slack_refiner.cpp`): after the byte-order
unchanged 191 ascent reaches its natural fixed point, the remaining protected
terminal budget evaluates joint replacements of two patrols' plans from the
already-enumerated sparse route pools (maximal routes, hash-deduped,
exact-simulated, independently validated, strict official-lexicographic
acceptance only); an accepted pair re-enters the one-agent ascent. A deadline
inside the ascent means the pair phase never runs, so the parent work prefix
is protected (the 201 lesson). Tests green; sparse smoke case ties parent
with zero invalid.

Frozen manifest `research/holdouts/SCORE-TERMINAL-PAIR-EXCHANGE-207.csv`,
SHA256 `D9506BFC73120CB30679271992EDAA71D333E94879AF1A78E47A3CF04B740D79`;
fresh dev 60 weighted to sparse tiers (4900000/4901000/4902000/4903000 =
8/8/20/24) and sealed holdout 108 (4910000/4911000/4912000/4913000 =
16/16/36/40). Parent binary `historical_tournament_parent_207.exe` SHA256
`08382B5231F15A2A66FD5880A34325D552056FD242F35DF0CE92FFF4EE20C783`; candidate
`historical_tournament_candidate_207.exe` SHA256
`E79CC1E3203680B2B4E727B9054072F005E230E4D1BAB9BB09B5791F35D726D0`. Sides run
sequentially.

**First development run invalidated by asymmetric environment load.** The
cross-binary local run finished 19/11/30 (all tier-3, +176/-367), but the
live BTC match `m-3986` executed 21:23:36-21:26:11 entirely inside the
candidate side's window (parent closed 21:16:38; candidate 21:16-21:39) and
its solver load overlapped exactly the stratified-hard tier, where the
candidate's losses concentrate (-44/-32/-22/-16/-15/-14 all on 4902xxx).
Per the load-sensitivity rules (028/031) this run carries no verdict either
way; both logs are retained as environment evidence only.

Causal redesign per the 044/046 lesson: a single binary with a research
switch (`ProtectedSlackRefiner::enableTerminalPairExchange`, default off =
byte-identical 191 behavior; harness flag `--terminal-pair 0|1`) plus a
dedicated `terminalPairAcceptances` diagnostic, so pair activity is
observable and binary-layout/timing confounds cancel. The rerun executes on
the isolated GCP VM (`udon-stream-185-0822`, the 199/201 venue), flag-off
then flag-on sequentially, same frozen manifest
(`run_terminal_pair_207_causal.sh`).

**Causal development PASSED (2026-08-23).** Same-binary off/on on the quiet
VM with spot-preemption-safe resume (`run_terminal_pair_207_resume.sh`):
paired 60, W/T/L **17/41/2**, +42 servings, zero
invalid/emergency/dominance-failure, zero tier-1/2 change. Mechanism alive:
16 pair acceptances across 12 cases (off-side control 0); **all 12
acceptance cases won** (+19) and none lost; both losses (-46/-4) and five
wins occurred at zero acceptances = parent deadline jitter, not mechanism.
Benefit concentrates in the target very-hard sparse tier (13/10/1) and is
positive in every fuel/role/window lane. Logs: off SHA256
`E0728F55BE5454B0445C8228D094071EABBB97C7E11A8E4EC261A3F08E1BD6D4`, on
`9E44DE6CEDD9FC0F4F299374E1D9538A9AF723E1E7244617AC44CD82493599ED`.
**Holdout PASSED and 207 is ACCEPTED (2026-08-24).** Sealed 108 pairs,
same-binary off/on on the quiet VM: W/T/L **24/72/12**, +40 servings, zero
invalid/emergency/dominance failure, zero tier-1/2 change across 216 runs;
runtime tails byte-similar (max_ms off 3016/3024 vs on 3016/3018).
Acceptance-conditional causal record: 18 cases with 21 acceptances (off
control 0) score **16W/2L (+30/-3)**; every other non-tie sits at zero
acceptances and nets +11 with symmetric tails — ambient jitter, and the
easy/medium losses all occur where the mechanism never fires (inert, not
harmful). Logs: off SHA256
`668D419D4B7B8F957EF119BF4D3CB8D11612C101C8036D04E5353BBFFA45C624`, on
`D7AAB15BB570752E1FCAD6219715FD4BCC446B23D68C04CDDF6BAB533E067CF1`.
Production enablement: `btc_main.cpp` sets
`slackRefiner.enableTerminalPairExchange = true`; the default stays off so
research harnesses keep a byte-identical 191 parent. Tests green. **BTC debt
PAID 2026-08-24.** Binary SHA256
`2D79A5CD0029F75E6C50D3CB1F37FE3856407364B95B0C244C92D7175F872DBF`.
The first two explicit-advanced probes correctly did not overclaim the gate:
`m-4037` (12 spots) stayed in the dense domain; `m-4038` (32x32, 10 days,
8 agents, 18 spots) entered the sparse refiner and improved terminal servings
43 -> 53, but its one-agent ascent reached the protected deadline before the
pair loop. The decisive debt gate was `m-4039`: hard/3 bots/7 days/24x24/
100 steps/5000 ms/4 agents/18 spots/6 brands/default fuel. Production selected
one tanker; terminal telemetry recorded `terminalSparse=true`,
`deadlineReached=false`, 3456 sparse routes, 3168 generated+dual-valid plans,
5 strict candidates and 3 accepted sparse rounds, so control flow necessarily
executed the enabled pair loop to completion. All 7/7 actions received HTTP
200 valid ACKs; zero skip/fallback/emergency, max response 3312 ms, max solver
3116 ms, and replay-check rebuilt exact `6/42/151`. Replay SHA256
`54200DCF9CC5D6AD74ADF4CA47F5FB819C72FFD481993D6C4346BFD9B1C666EB`.
The rank-4 result versus bots at `6/42/169..174` is a new tier-3 development
counterexample, not a 207 regression: the protected terminal refiner itself
raised day 7 from 20 to 27 servings while preserving lifetime/daily. Attribute
the earlier-day servings gap separately; bot rank is not promotion evidence.
Evidence
`research/evidence/SCORE-TERMINAL-PAIR-EXCHANGE-207.md`.

### Rejected zero-dwell rendezvous pool candidate: SCORE-REFUEL-NOWAIT-206

Registered and closed 2026-08-23, parent `690728a`. The candidate added an
additive no-wait escort pair beside every constructed WAIT(1) rendezvous
(one-step-larger departure window, same `{rendezvous, rendezvousStep+1}`
refuel event satisfied mid-move per the 205 server-pinned semantics).

Development (60 fresh paired closed-loop matches, frozen manifest SHA256
`22E3E21819E34B1ABFEA46258117D519ABA8984124AA031EA74B5C588C2E73D4`, sides
sequential): W/T/L **16/26/18**, zero invalid/emergency; **one tier-2 loss**
(4880007 low/fuel-tight `-2` daily `-35` servings); tier-3 +16/-17 with
+140/-162 servings and tails -36/-28/-24/-22; losses systematic in
default/fixed (1/3/6) and the very-hard tier; window-independent (long
9/11/10, short 7/15/8). This is the 194/195/198 phenomenon: escort-pool
growth redistributes deadline-sampled search and outweighs the reclaimed
step. Holdout stayed sealed; no BTC match. Source reverted; tests green.
Logs: parent SHA256
`07C56D3DA02083990CBEC15986480D1395859E687C12C2A050148ED0D0429F81`, candidate
`55E522322EB0F02566A7192D2FE450F81AFBDEBFC1A8526864D3BCAFC49170CF`; evidence
`research/evidence/SCORE-REFUEL-NOWAIT-206.md`.

Reopen only via a mechanism that captures the reclaimed rendezvous step
without growing the deadline-sampled candidate pool (e.g. an
incumbent-protected WAIT(1)-shift transform in the accepted 187/197 refiner
lane under strict componentwise dominance); never widen the escort pool or
tune on the consumed 488xxxx seeds. The 205 semantics themselves remain
accepted and unaffected.

### Closed mid-move refuel semantics: SEM-REFUEL-MIDMOVE-205 (accepted-semantics)

Registered and closed 2026-08-23, parent `690728a`, read-only; no production
or test source change (new research-only probe
`research/probes/refuel_midmove_probe.cpp`). Question: the canonical
rendezvous primitive always charges the patrol a hardcoded `WAIT(1)`
(`src/planner.cpp:3594`/`3619`), but the exact simulator keeps a moving agent
at its source cell until the move completes, so a departing patrol stands at
the rendezvous for two consecutive boundaries and refuels with zero dwell —
were these semantics real, both in-house and server-side?

Answer: **yes, fully pinned.** Micro-probes: both engines refuel mid-move
(immediate departure refuels at arrival+1 while moving; mountain-source
interception refuels; a tanker standing on its own move source provides;
one-step smooth-road departures do not refuel; arrival onto a tanker refuels
one boundary after arrival). Replay scan over the frozen fifteen-replay
manifest (`research/holdouts/SEM-REFUEL-MIDMOVE-205.csv`, SHA256
`E01CA37756C9CFD96FF6738BEFC3443A4738B6B6DB7048ABF3456D6C975E3962`): **555
mid-move refuel firings inside accepted production plans**, 111 distinguishing
days, **4 fuel-critical accepted days** (m-3878 x2, m-3879, m-3880) that a
stationary-only server must reject with `E_NO_FUEL`, and authoritative
next-day agents equal to the mid-move simulation on **109/110** transitions.
Evidence: `research/evidence/SEM-REFUEL-MIDMOVE-205.md`; scan log SHA256
`C04B60651C542509B558B7793A86500BC39A017F1D1C51B5D16CC6D1AF186313`.

Side finding: the single mismatch (`m-3876` day 9, patrol +48 with exact
position) fits a cross-team refuel-to-full at the unique fuel-152 boundary,
with the sole opponent tanker geometrically adjacent; it also explains the
open `m-0922` +7 anomaly. Plausible, not proven; fuel only increases, so
validity is never at risk and daily reconciliation already absorbs it. Never
assume cross-team refuel in planning until proven.

Consequence: a SCORE candidate reclaiming the hardcoded rendezvous `WAIT(1)`
(additive no-wait variants) is unblocked with server semantics confirmed.

### Closed exact root streaming with two new counterexamples: ORACLE-ROOT-STREAM-185

All eight registered slices are complete and mirrored locally byte-identical
(VM-side and local SHA256 verified equal). The three final slices closed on the
Spot VM `udon-stream-185-0822` with full per-root-action certificates and zero
invalid witnesses:

- `1720100 [172,258)` (86/86 actions): slice robust exact `6/19/19` at root
  index 223, plan `3.2.-13`, terminal `34@22`; recorded head `6/18/18` under
  all three causal policies. Log
  `research/evidence/185-1720100-172-258.log`, SHA256
  `B5665DBF6A8EF1E1EE001A60B60F71A6B706CA1EFC230812D51A4E87C81D2A33`.
- `1720100 [258,344)` (86/86 actions): slice robust `6/18/18`, ties head. Log
  `research/evidence/185-1720100-258-344.log`, SHA256
  `A95B0627C6699364A1A90E195ACB4DDF776DD59E11A21847786AB1320BAA80FF`.
- `1721200 [118,236)` (118/118 actions): slice robust exact `5/20/22` at root
  index 150, plan `1.2.2.2.4.3.5.5.5`, terminal `33@22`; recorded head
  `5/19/22`. Log `research/evidence/185-1721200-118-236.log`, SHA256
  `BDE3D78C8551BBC2F2D28F60C0EBB772F2FC7259679231BD29D8B9FF5C476A02`.

Exact root maxima over all completed slices, which by construction equal the
unsliced root maxima:

1. `1720100` (default fuel): exact `6/19/19` versus head `6/18/18` — a new
   tier-2 counterexample. The winning exact root is two moves plus `WAIT 13`,
   the strongest fuel/positioning front-loading witness recorded so far.
2. `1721100` (default fuel): exact `5/17/18` versus head `5/17/17` (tier 3),
   unchanged from the earlier slices.
3. `1721200` (high fuel): exact `5/20/22` versus head `5/19/22` — the tier-2
   counterexample strengthened; the previous best slice witness `5/20/21`
   traded one serving, the new one concedes nothing.

Verdict: `accepted-exact-method`. The sliced streaming schedule preserved
completed root-action certificates across Spot preemptions, kept peak memory
bounded, and the parity gate against the unsliced mode had already passed on
the consumed low-fuel row. Output cadence and VM elapsed carry no BTC or
production-performance authority. All three seeds remain consumed-only
attribution fixtures: no promotion, tuning or routing by them. Source work on
the gap they witness still requires a general coupled multi-day suffix-state
evaluator with fresh evidence, per 202/203. The Spot VM was stopped after
byte-identical mirroring.

### Rejected protected WAIT fixed-point attribution: ATTR-PROTECTED-WAIT-FIXEDPOINT-204

Accepted experiment 187 preserves the canonical bounded decision and selects
the best exact state-preserving one-patrol WAIT detour against that original
incumbent. Unlike accepted terminal experiment 191, the nonterminal refiner
does not continue from its certified improvement, so two compatible detours on
different patrols or WAIT anchors cannot be composed in one day.

Experiment 204 was a consumed-only research-harness attribution. It preserved
the complete 187 first round, then invokes the unchanged refiner on the current
certified incumbent for later rounds. Every round still requires exact
simulator and independent-validator agreement, identical terminal kinds/cells
and road footprint, no-less patrol fuel, componentwise ledger dominance and a
strict daily-distinct or servings gain. No-gain, invalidity, deadline or fixed
cap retains the best prior incumbent.

The causal gate stopped on already-opened `4411000`. Latest parent `690728a`
finished `6/60/482` against virtual `6/60/481`, with 24 generated/valid plans,
one liftable plan, one takeover and zero deadline, invalid or emergency events.
Across the full match only the protected first round improved; reapplying the
same refiner to that certified incumbent found no second strict gain. Therefore
the preregistered prerequisite failed. Controls `4310100`/`4310200`, fresh
development and holdout remained unopened. Research instrumentation was fully
reverted and production source remains `690728a`.

Frozen manifest: `research/holdouts/ATTR-PROTECTED-WAIT-FIXEDPOINT-204.csv`,
SHA256
`9082E0B478F9D920FB033EF77983F3F5263E5B389940842D80AE6BC92CC62B47`.

Causal evidence:
`research/evidence/ATTR-PROTECTED-WAIT-FIXEDPOINT-204-causal.log`, SHA256
`31C1E1DECC1A565CBEE88C1E39F7F3121ADF375536F791F54F6B1A70A2C5BC5F`.
Reopen only after an independent day exposes two compatible strict protected
WAIT detours; do not widen the route neighborhood or rerun consumed `4411000`.

Functionality preservation: nothing is deleted, disabled, deferred or reduced;
only opt-in research-harness telemetry may change during attribution.

### Rejected protected exact-suffix beam research: SCORE-EXACT-SUFFIX-BEAM-203

Experiments 200 and 202 prove on two independent completed experiment-185
witnesses that the exact day-1 root is supported, complete and master-retained,
but returning later days to the production suffix loses the oracle advantage.
The current W1 exact-bundle witness still collapses every exact day portfolio to
one greedy continuation before advancing to the next day. Experiment 203 tested
the smallest protected successor: preserve the complete greedy continuation,
then force up to three alternative exact-supported bundles on the first detailed
suffix day while leaving every later suffix day on the existing greedy path.

Parent: `690728a`. The consumed causal gate on seeds `1721100` and `1721200`
was completely inert. Their policy-stratified scores and plan hashes remained
identical at both protected reserves `1100 ms` and `0 ms`: respectively
`5/17/17`, `5/16/17`, `5/17/17` and `5/19/22` for all policies. No generated
plan, takeover, deadline or validity event occurred. Because neither known gap
was reduced, fresh development and the sealed holdout were not opened. The
source candidate was fully reverted; production remains parent `690728a`.

Frozen manifest: `research/holdouts/SCORE-EXACT-SUFFIX-BEAM-203.csv`, SHA256
`D2F4013BF79BD5CB32AB4EF9C6A4203B9BEC35DB2159183E23CD438B9991FE4D`.

Causal evidence:
`research/evidence/SCORE-EXACT-SUFFIX-BEAM-203-causal.log`, SHA256
`47C20C9A7001AB7ECFBCD6DCBF61C7FAC5C9FABD56D4BAD5279B87D171CE7327`.
Reopen only for a genuinely coupled suffix-state evaluator with a new general
witness; do not increase beam width or route by the consumed seeds.

### Closed default-fuel exact-root causal attribution: ATTR-ORACLE-ROOT-CAUSAL-202

The completed experiment-185 default-fuel witness on consumed seed `1721100`
reaches exact robust `5/17/18`, while accepted production `690728a` remains at
`5/17/17`, `5/16/17` and `5/17/17` under maximum-dwell, minimum-dwell and
status-toggle. Experiment 199 already proved that this result is unchanged by
the short and full protected windows, but only the separate high-fuel witness
`1721200` has been attributed through the day-1 root and production suffix.

Experiment 202 uses the existing research-only inspection path on the consumed
witness. It compares exact root `3.2.2.2.0.1.5.-3|-16|-16` with production root
`3.2.2.2.0.0.2.-3|-16|-16` at exact support, portfolio, master and certification
boundaries, then freezes only the exact root and returns days 2--4 to unchanged
production under the three registered policies. No production source behavior,
fixture, policy, score, simulator, validator or deadline changes.

Frozen manifest:
`research/holdouts/ATTR-ORACLE-ROOT-CAUSAL-202.csv`, SHA256
`1734EE65A8D79F6388E30799CEA5058B05C33AFBBED452177BCC80046EC193E7`.
The result can close the second completed exact witness or identify a distinct
timing-independent boundary; it cannot authorize seed- or policy-specific
routing.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. The inspection and forced-prefix
paths are research-only.

The exact root is supported and complete for all three agents (`111`), present
in the exact portfolio and retained by the ordinary master. Both roots score
`5/5/5` today, both are certified and neither profile dominates. The exact root
has lower/upper `5/16/17 .. 5/20/24`; the parent has
`5/17/17 .. 5/20/24`.

Forcing only the exact root and returning days 2--4 to unchanged production
finishes valid `5/17/17` under all three policies. This matches or improves the
current production suffix but still misses the exact oracle's `5/17/18`.
Therefore root generation, exact support, portfolio retention and master
selection are not the missing boundary; the extra serving requires a different
later suffix. Experiment 202 closes
`accepted-attribution-no-source-candidate` and independently confirms the same
coupled multi-day suffix/evaluator class as experiment 200. Result log SHA256:
`E73175319047A24852B0EEFFAD6874BE1B52DB89527946D11CCC79B803540CFC`.

### Rejected protected-refiner membership optimization: PERF-PROTECTED-HASH-MEMBERSHIP-201

Accepted experiment 197 makes the protected wait-detour and terminal sparse
refiners the only consumers of reclaimed BTC compute. Experiment 201 replaced
only their two non-iterated `std::set<uint64_t>` membership containers with
`std::unordered_set<uint64_t>` plus bounded reserve. Plan hashing, equality,
candidate and route order, exact simulator, independent validator, comparator,
incumbent, terminal rounds, dominance and deadlines were unchanged.

Direct fixed-input equivalence passed before timed development. WAIT generated
and validated 11 plans in both binaries with identical plan/state hashes.
Terminal sparse processed 3,584 routes and 1,547 valid plans, accepted 16 strict
improvements over seven rounds and returned the same `6/6/83` and plan/state
hashes. Equivalence log SHA256:
`3AAD2418BDCF2AFAAE7DE6D14DAA9CA7DEC5A419E0BF181CA8122CE0846E8900`.

Fresh paired development completed 60 pairs at `7/46/7`, gain/loss `+68/-63`,
tail `-22/+38`, with zero role mismatch, invalid, emergency or failure.
Fixed-role was `4/22/4`, `+48/-44`, tail `-22/+38`; low fuel lost 37 while
gaining 22. The candidate completed 27 more valid wait plans, 512 more valid
sparse plans and two more sparse rounds, but did not protect score. Development
log SHA256:
`23B16C01ED85A5BD4C9CD9F8B97E15CF390ACF814A8C6C72B8DCA78A9948282E`.

Experiment 201 is rejected before holdout and BTC; its production-source
candidate is reverted. The container replacement is semantically equivalent on
a complete fixed input, but at the hard cutoff it merely perturbs how much
refinement completes and produces balanced nonmonotonic score changes. Reopen
only with a mechanism that proves additional protected work cannot replace the
certified incumbent except by exact improvement.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. Rejection restores the canonical
ordered membership implementation; experiment-only evidence remains archived.

### Closed exact-root causal attribution: ATTR-ORACLE-ROOT-CAUSAL-200

Experiment 199 confirms that accepted production `690728a` remains at
`5/19/22` on consumed high-fuel seed `1721200` under all three causal opponent
policies and under both the 3900-ms short protected window and the full internal
5000-ms protected window. The completed experiment-185 root slice has an exact
robust `5/20/21` witness beginning with active-agent plan
`3.2.2.2.0.0.5.5.-2`; production begins with
`3.2.2.2.0.1.5.5.5`.

The exact root is dual-valid, exact-supported for all three agents, complete,
retained by the master and present as a master outcome. Its current score is
`5/5/5` versus parent `5/5/6`; certified lower/upper are
`5/19/19 .. 5/20/31` versus `5/19/20 .. 5/20/31`. Forcing only the exact root
and returning days 2--4 to production reaches exact `5/20/21` under
maximum-dwell and status-toggle, but only `5/18/21` under minimum-dwell, below
production `5/19/22`; the exact oracle uses a different suffix.

The gap is coupled multi-day closed-loop suffix/evaluation, not root generation,
master retention or a selection comparator bug. A root-only promotion would
cause a tier-2 regression. Experiment 200 is accepted attribution with no source
candidate. Frozen manifest SHA256:
`49B2B829E1D629BFE201976F09873DFEDBF714E9BE1EA07ED5BE684138D71C7E`;
log SHA256:
`39E59B43BA62BA5D1E0F20D82CA85D877C330D929EAC53F7E2C55CC42F8825F3`.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. Production source and behavior are
unchanged; forced-prefix execution exists only in the research executable.

### Closed latest-production exact-witness attribution: ATTR-ORACLE-LATEST-RESERVE-199

Experiment 199 revalidated both completed experiment-185 witnesses against the
accepted `690728a` lifecycle. The canonical solve retains its 1600-ms deadline
calibration; the protected phase was tested both until 3900 ms for a 5000-ms
authoritative window and until the full internal 5000-ms cap for a long outer
window.

Under all three policies, seed `1721100` remains respectively `5/17/17`,
`5/16/17`, `5/17/17` in both protected-window modes versus exact `5/17/18`.
Seed `1721200` remains `5/19/22` under all six policy-window runs versus exact
`5/20/21`. All runs are valid, plan hashes are invariant between short and long
windows, and generation, takeovers and deadline counts are zero. The accepted
extra interval cannot enter this dense exact domain; lack of protected time is
falsified as the cause.

Experiment 199 is accepted attribution with a residual gap and no source
candidate. Frozen manifest SHA256:
`DF7EF5A47271F4F171A2CA673F37F38384F499A0770D387116DD3608CE48984F`;
log SHA256:
`E0C732B71F5144C1BC70A11E2D40C76B7B0FC515FE0C122B71A605D42EA91A55`.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. The deadline switch exists only in
the research oracle executable.

### Rejected day-only semantic-equivalent master dedup successor: PERF-DAY-MASTER-DEDUP-198

Experiment 195 established that a fixed-endian length-prefixed key has exactly
the same plan-membership equality classes as canonical JSON and that the
optimization can be isolated from role selection. It was rejected before
holdout because the then-direct parent lacked a protected extra-time path and
the marginal benefit was not generality or target-host qualified. Accepted
experiment 197 is now the registered direct parent and satisfies the explicit
reopen condition.

Parent: `690728a`. Experiment 198 changes only the three `evaluatedPlans`
membership checks in the post-role main day `RouteMaster` and option copies used
by its recombination passes. The key encodes agent count, per-agent action count
and signed wire values using fixed-endian length prefixes. Every surviving
candidate `stableId` remains canonical JSON. Role selection, future-witness
repair, post-ACK proof, exact simulator, independent validator, operation caps,
official comparator and the accepted protected refiners remain unchanged.

Frozen manifest:
`research/holdouts/PERF-DAY-MASTER-DEDUP-198.csv`, SHA256
`BA23588A49AC6E431D401F63BB8538E919421844FB07484645F881780D87C34C`.
Algebraic and complete-master equivalence, role-path isolation and complete
archived replay equivalence passed. Fresh paired development completed
`22/30/8`, serving gain/loss `+366/-60`, with zero tier-1/tier-2 loss, invalid,
emergency or refiner failure. Four native-role mask mismatches and two large
positive outliers required a fixed-role A/B/B/A attribution gate. Across all
eight losses, both relevant mismatched masks and five material gains, the gate
closed at three confirmed gains, one confirmed loss, seven mixed and four ties.
The confirmed loss was `-12/-20` on very-hard seed `4863011`; it remains a real
bounded downside. The candidate is not accepted, but the distributed positive
development result and stronger confirmed gains qualify it to open the already
frozen holdout. Development and reproduction log SHA256 are respectively
`54642CB2E387B03067D766FA361943A4F4F7F1A30FCF7EFDA6B7C60897B779A0` and
`23F78F3B19D22E1143E502C9B6D2229DC7DB4FB5710A9ECE4056737B436C6BAD`.
Local elapsed time may only falsify a candidate; BTC target-host telemetry is
required for a performance claim or promotion.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. Parent and candidate use the same
single master pipeline and differ only in an injective internal membership-key
representation after roles are fixed.

The frozen holdout completed all 108 pairs at `24/58/26`, gain/loss
`+213/-345`, tail `-101/+41`, with zero lifetime/daily loss, invalid, emergency
or refiner failure. Seven native-role pairs selected different masks and are
not used as causal regression evidence. Fixed-role alone still finished
`13/33/14`, `+137/-72`, tail `-16/+41`, proving that the candidate can lose with
the role held identical. Low fuel was `7/22/11`, `+88/-241`; very-hard was
`10/9/11`, `+144/-226`. High fuel also lost six cases including a fixed-role
`-15`, so the development-only high-fuel pattern does not authorize a
dispatcher. Holdout log SHA256:
`32B7600DB5ACE1959665015AEA42E527819387FC284E940E8545598C14DBAEE2`.

Experiment 198 is rejected and its source wiring is reverted. The flat key is
semantically equivalent only when the same search completes; before a hard
cutoff it changes which later work receives time. Real gains therefore coexist
with systematic and materially larger losses. BTC is not opened after the
protected score gate fails. Reopen only through a mechanism that protects the
canonical parent work allocation or incumbent before additive work; never by
fuel, map, family, role, spot-count or seed dispatch.

### Accepted protected BTC reserve recovery: RUNTIME-PROTECTED-RESERVE-197

Experiment 196 proved two facts that direct reserve rollback cannot reconcile:
the 500 ms reclaimed from the historical 1100-ms reserve is score-relevant on
`m-3908`, but granting it to the complete solver changes the search trajectory
and does not protect the current incumbent. Experiment 197 separates these
responsibilities instead of selecting another global reserve.

Parent: `f9c0019`. Role selection and the canonical day decision keep the exact
1600-ms deadline calibration and therefore remain byte-identical to the parent.
Only after that decision has returned and passed exact simulator plus independent
validator does the HTTP path expose any reclaimed interval to the already
accepted protected refiners: wait-detour refinement on nonterminal days and
sparse coordinate ascent on the terminal day. The refiner deadline is
`min(received + 5000 ms, authoritative action deadline - 1100 ms)`: a 5000-ms
outer window retains a submission reserve, while a 45--60-second outer window
permits the full internal 5000-ms compute cap without converting that outer
window into extra compute.
The refiner starts from the immutable parent plan. Deadline, failure, invalidity,
non-improvement or failed transition/ledger dominance returns the parent plan
unchanged. There is no second role selector, shadow solver or direct long search.

Frozen manifest:
`research/holdouts/RUNTIME-PROTECTED-RESERVE-197.csv`, SHA256
`4A56330C4D3EA548FD522F7B21B0838521F90EC62C025EFE0ECE6F57D34AAD38`.
Development passed the parent-identity, validity and strict-gain prerequisites.
The frozen 108-case holdout then finished `83/25/0` with `+1021` servings and
zero lifetime/daily-distinct loss, invalid, emergency, dominance failure or
refiner failure. It was zero-loss across all four difficulties, low/default/high
fuel, fixed/native roles, all six families and every registered spot density.
Four terminal local slices reached their deadline and returned the certified
incumbent. Holdout log SHA256:
`F4F0C53F79DAC114618C9ED6DEA85CF0DAFD180D6C72FE54BE1E396CB3DBA00E`.

Two authenticated BTC target-host gates passed. On 15000-ms `m-3927`, all 10
actions were valid, maximum response was 3588 ms and the refined path improved
the protected-parent counterfactual `6/60/552` to `6/60/566`. On canonical
5000-ms `m-3928`, all 10 actions were valid, all nine transitions reconciled,
maximum response was 2963 ms and the refined path improved `6/60/494` to
`6/60/515`. Bot ranks are excluded. Canonical evidence:
`research/evidence/RUNTIME-PROTECTED-RESERVE-197.md`.

Experiment 197 is accepted. The next direct-parent comparison is the registered
day-only semantic-equivalent master dedup successor to experiment 195; the 185
exact-stream counterexamples remain an independent later axis.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. The canonical 1600-ms role/day
pipeline remains the sole producer of the protected incumbent; additional time
only extends the already active exact refiners and cannot replace the incumbent
without the existing official-score and state-dominance certificates.

### Rejected direct BTC reserve rollback: RUNTIME-BTC-RESERVE-196

Historical attribution on authenticated `m-3908` located the first large score
regression at `fa21950`: its only solver-budget change raised the BTC
`networkFloor` from 1100 to 1600 ms. The immediate parent `02df79d` repeated
`6/60/537` and `6/60/533`; `fa21950` scored `6/60/461`. Later accepted score
logic recovers some but not all of the lost lane. Current `f9c0019` scored
`6/59/466` and `6/60/472` under local cutoff variation.

The archive now contains 124 accepted action samples from 14 BTC matches:
client-observed ACK p50/p95/p99/max is `13/34/61/132 ms`; server response minus
local solve p50/p95/p99/max is to be frozen in experiment evidence; there are
zero recorded transport retries. These observations do not by themselves
authorize a rollback, but they falsify treating 1600 ms as an unquestioned
current p99 requirement.

Parent: `f9c0019`. Experiment 196 tested exactly one policy change: restore the
BTC day solver `networkFloor` to the previously deployed 1100 ms while retaining
the 1600-ms role path, independent 800-ms submission guard, 750-ms bounded ACK
attempt, idempotent resend, authoritative absolute deadline and 5000-ms hard
cap. It did not alter planner logic, scoring or validation.

Frozen manifest:
`research/holdouts/RUNTIME-BTC-RESERVE-196.csv`, SHA256
`0BFFCB1611443A1BDEC7EC4EE7610393BD158B4C5D135020FA2575A1EC150568`.
The fresh holdout remained sealed. On consumed `m-3908`, fixed role mask 2, the
1100-ms candidate reached `6/60/535`, reproducing the older `02df79d` lane and
confirming that the reclaimed 500 ms is causally valuable there. Fresh timed
development did not provide stable parent/candidate loss evidence: observed
differences reversed or disappeared when order and role masks were controlled.
However, direct extra search changes the complete search trajectory and retains
no invariant protecting the 1600-ms incumbent. Experiment 166 had already
established the same structural non-monotonicity.

Experiment 196 is therefore rejected before holdout and live BTC. Its source
and harness wiring are reverted to the 1600-ms parent. Canonical evidence:
`research/evidence/RUNTIME-BTC-RESERVE-196.md`. A successor may reclaim the
window only after freezing the byte-identical 1600-ms role and day incumbent;
extra time may run only an exact monotonic refiner whose failure, deadline or
non-improvement returns that incumbent unchanged.

Functionality preservation: no designed functionality was removed, disabled,
deferred or reduced, and nothing was deleted. Rejection restores the canonical
parent path rather than keeping a second reserve policy.

### Rejected day-only semantic-equivalent master dedup research: PERF-DAY-MASTER-DEDUP-195

Experiment 194 proved the flat membership representation itself collision-safe,
but applying it to the whole engine changed the amount of incomplete timed
evidence received by different role assignments. Experiment 195 therefore keeps
role selection, future witness repair and post-ACK proof on the canonical key.
Only the post-role main day master and its copied recombination options may use
the flat key. The exact simulator, validator, candidate comparator, stable IDs,
operation caps and all search stages remain unchanged.

Parent: `f9c0019`. Frozen manifest:
`research/holdouts/PERF-DAY-MASTER-DEDUP-195.csv`, SHA256
`6220B6E3AD298B182169432DC242301C79268C630CAB2772DDAE140701A3E28C`.
The holdout remained sealed. Algebraic key equivalence, complete-master
equivalence, role-path isolation and 24 complete archived replays passed.
Authenticated `m-3908` was valid and candidate 195 improved same-replay
counterfactual parent runs from `6/59/466` or `6/60/472` to repeatable
`6/60/495`, but the live candidate still ended `6/60/508` against a best bot
score of `6/60/579`.

Historical attribution found a separate protected reference: `02df79d`
repeated `6/60/537` and `6/60/533` on the same replay and role mask, while its
immediate successor `fa21950` fell to `6/60/461` after raising BTC
`networkFloor` from 1100 to 1600 ms. Later checkpoints ranged from `461` to
`496`; candidate 195 recovered part but not all of the lost score. This is a
historical response-budget allocation regression, not a flat-key regression,
and `02df79d` is not the marginal parent used to judge 195.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. The same master pipeline chooses
one injective membership representation only after roles are fixed. There is no
shadow solver, weakened validation, altered deadline or second search.

Experiment 195 is rejected as a standalone promotion before holdout and its
source wiring is reverted because the direct-parent benefit is not yet
generality-qualified: the consumed replay is positive, but the fresh timed
difference vanished under controlled reproduction and paired target-host proof
is absent. Canonical evidence:
`research/evidence/PERF-DAY-MASTER-DEDUP-195.md`. Reopen after the protected
reserve successor closes; compare that direct parent against parent plus the
day-only flat key, with `02df79d` retained only as a historical reference.

### Rejected all-engine master dedup research: PERF-MASTER-DEDUP-194

The accepted production path spends master-leaf time constructing a JSON tree
and serializing it solely to test whether an action plan has already been
evaluated. The membership relation depends only on the ordered agent/action
wire values, while survivor `stableId` values still require the canonical JSON
form. Experiment 194 replaces only the three hot master dedup keys with a flat,
length-prefixed, fixed-endian injective encoding. Candidate creation, survivor
stable IDs, comparator order, simulation, independent validation and every
deadline check remain unchanged.

This is deliberately narrower than the external performance proposal: trace
capture, simulator scratch storage, cache eviction, replay serialization and
brand-mask precomputation are not included, so any result is attributable to
one mechanism. The parent and flat-key modes execute one master pipeline; the
research switch changes only the key representation and never runs both.

Parent: `f9c0019`. Frozen manifest:
`research/holdouts/PERF-MASTER-DEDUP-194.csv`, SHA256
`AE8F787E3CDDAC2859A881BC4247C77F1104EE16E9CED3886D1E1CADE712DAC7`.
The holdout remained sealed. Development first required proof that flat-key equality
is exactly equivalent to canonical-plan equality, then complete no-deadline
master candidate/stable-ID equality and archived replay score/state/validator
agreement. Timed local lanes may falsify score regressions but have no
performance authority. Only BTC target-host telemetry may establish throughput
or latency improvement.

Functionality preservation: no designed functionality is removed, disabled,
deferred or reduced, and nothing is deleted. There is no second solver, shadow
pipeline, reduced validation or changed operation cap. The parent and candidate
dedup sets represent the same exact equivalence classes; any membership,
candidate ordering, action, score, state transition, validator, invalid or
emergency difference rejects and fully reverts the candidate.

The algebraic key-equivalence and complete no-deadline master gates passed after
correcting a development-only stable-ID wiring error. Fixed-role development
controls tied, but native seed `4770002` changed role mask and fell from
`6/30/163` to `6/30/150`. Reversed-order repetitions confirmed that applying the
speedup during role selection changes the allocation of incomplete timed
evidence. Experiment 194 is rejected before holdout and BTC. Canonical evidence:
`research/evidence/PERF-MASTER-DEDUP-194.md`.

### Rejected fair direct-frontier research: SCORE-FAIR-DIRECT-FRONTIER-193

The consumed `m-3897` attribution exposes a production-path scheduling gap in
column generation under the shared hard deadline. Generation is sequential by
agent and each agent performs direct-target search plus deeper staging and
multi-spot expansion before the next agent starts. On days 3, 5, 8 and 10 at
least one later agent received zero Pareto queries; on day 8 agents 4--7 each
received `0 ms / 0 queries` and retained only three columns, while earlier
agents consumed the available generation window. This is evidence of
agent-order starvation, not evidence that any particular `m-3897` action is
optimal or that the BTC bot score is an objective.

Experiment 193 tests one general mechanism: perform a bounded direct-frontier
phase for every agent before the existing sequential staging and multi-spot
intensification. Direct searches are cached and reused, so the new phase does
not duplicate the same route queries. A per-agent slice is derived only from
the public remaining deadline and number of agents still awaiting direct
coverage. After that minimum coverage phase, all existing exact guidance,
staging, pair/triple/quadruple generation, coordination, master search, ALNS,
simulation and validation remain available on the shared remaining budget.

Parent: `f9c0019`. Frozen manifest:
`research/holdouts/SCORE-FAIR-DIRECT-FRONTIER-193.csv`, SHA256
`3A289DE666A43FDE0BC332E35C278D8BBBB38708E951847198DC36411BC1A9CA`.
Development spans 8/14/20/26/32 maps, 4/5/7/8/10-day horizons, 4/6/8 agents,
low/default/high fuel, fixed/native roles and fresh generated families. The
holdout is sealed until consumed attribution and fresh development clear the
registered gates.

Functionality preservation: the candidate removes, disables, defers or reduces
no designed functionality and deletes nothing. It changes only the ordering of
already-designed direct route work and reuses its exact results before the
unchanged deeper stages. Official lexicographic score, exact simulator,
independent validator, role semantics, traffic model and the `5000 ms` hard cap
remain unchanged. Local elapsed time has no performance authority. Promotion
requires paired score/tier/tail evidence, zero invalid/emergency, reduced
starvation without a systematic family/fuel/role regression, and a final BTC
target-host gate.

The candidate reached direct-query coverage for all eight agents on every
consumed rerun day, but the full closed loop lost `6/60/461` against parent
`6/60/464`. More importantly, on the original fixed day-8 public state the
parent and candidate produced the same exact plan and the same `6/48`: removing
the strongest recorded zero-query symptom did not yield a score witness. Fresh
8x8 and 32x32 development controls tied at `5/20/42` and `6/60/492`, with zero
invalid/emergency. Because there was no positive score witness and one consumed
regression, the remaining development matrix and sealed holdout were not
opened. The candidate is rejected and fully reverted. Canonical evidence:
`research/evidence/SCORE-FAIR-DIRECT-FRONTIER-193.md`.

Agent-order starvation remains telemetry, but it is not currently a causal
score gap. Reopen only with a fixed-state exact-valid missing column whose
insertion strictly improves official score; any successor must preserve the
non-starved portfolio without running a doubled solver.

### Closed latest-production oracle revalidation: ATTR-ORACLE-LATEST-192

Experiment 185's exact certificates are independent of the bounded production
solver, so accepted experiments 190/191 cannot invalidate completed root-action
scores or require a restart.  However, experiment 188 compared the completed
`1721100 [0,88)` witness only with production `5c3aa7a` and deliberately skipped
terminal-day protected refinement.  Current production `d73a24a` adds the
accepted 190/191 sparse coordinate ascent on that boundary; the old statement
that the exact `5/17/18` witness survives current production is therefore stale.

Experiment 192 is a consumed-only attribution on the same opened seed and the
same maximum-dwell, minimum-dwell and status-toggle policies.  It preserves the
exact experiment-187 virtual-parent lifecycle on nonterminal days and invokes
the unchanged accepted-191 terminal fixed-point refiner only on the final day,
inside the same absolute `5000 ms` boundary and `1600 ms` reserve.  It records
terminal generated/valid plan counts, rounds, strict improvements, deadline,
final score and plan hash.  No new fixture, holdout or production source is
opened, and local elapsed has no performance authority.

Parent: `d73a24a`. Frozen consumed manifest:
`research/holdouts/ATTR-ORACLE-LATEST-192.csv`, SHA256
`AC449977F5822B61645CC0475C09F004B345BEF20AF14D36ED86957BF635A42B`.

Functionality preservation: the probe removes, disables, defers or reduces no
designed functionality and deletes nothing.  It adds an opt-in research mode;
the 188 control and all 185 jobs remain unchanged.

The experiment-188 control reproduced exactly under all three policies:
maximum-dwell `5/17/17` with hash `32dcfc4fa4a20fd6`, minimum-dwell
`5/16/17` with hash `a9d910dc3b481ff7`, and status-toggle `5/17/17` with
hash `32dcfc4fa4a20fd6`.  Enabling the current 191 terminal path produced the
same scores and hashes.  All six runs were valid; nonterminal takeovers,
solver/protected/terminal deadline days, terminal sparse routes, valid terminal
plans, strict terminal improvements and coordinate rounds were all zero.

This is structural rather than timing-related.  The consumed fixture has six
spots, so the canonical dense exact representation is supported; the accepted
190/191 sparse sidecar intentionally remains inactive there.  Experiment 192
is closed `accepted-attribution-residual-gap`: exact 185 score `5/17/18` still
exceeds current production `d73a24a`.  A successor must identify a general
multi-day closed-loop causal mechanism in the dense-supported domain before any
source candidate opens.  Canonical evidence:
`research/evidence/ATTR-ORACLE-LATEST-192.md`.

### Accepted terminal coordinate ascent: SCORE-TERMINAL-COORDINATE-ASCENT-191

Experiment 190 is committed as `994c33a`. Exact streaming 185 remains active on
the Spot VM, while experiment 191 investigates a separate residual BTC gap and
does not wait for 185.

Authenticated `m-3896` ended at `6/60/407`. The rank and bot score are excluded,
but replay attribution found a production-path fixed-point gap: applying the
same exact terminal sidecar once more to the already submitted day-10 plan
improves `6/60/407` to `6/60/410`, with 191/191 candidates simulator/validator
valid and two strict improvements. On the ten immutable replay states the best
single-agent sparse exchange exceeds the recorded cumulative score by
`+3,+3,+3,+3,+2,+2,+3,+3,+2,+3`. State/footprint-preserving exchanges improve
only days 2 and 6; therefore the gap is not missing route generation but the
one-round limit on safe final-day combination.

The proposed mechanism preserves the complete experiment-190 first round as a
protected incumbent, then reuses the already generated sparse route set. Each
additional round freezes the current incumbent, evaluates every one-agent
replacement against that same round base, and accepts only the best strict
official-score improvement after exact simulation and independent validation.
It stops at a fixed point or the unchanged deadline. A timeout, failure or no
gain returns the best already certified incumbent, so the result cannot be
worse than experiment 190. No route search, nonterminal planner, role logic or
experiment-187 behavior changes.

Parent: `994c33a`. Frozen manifest:
`research/holdouts/SCORE-TERMINAL-COORDINATE-ASCENT-191.csv`, SHA256
`0B6B9F89B4B3605A3F70E5BF7BEE60716B88EE3F941E4B40E806C07EE3ABAC35`.
Development and holdout use fresh seed ranges and compare the protected first-
round score directly with the final fixed-point score inside one run. Holdout
remains sealed until consumed plus fresh development pass. Local elapsed has no
performance authority; final promotion still requires BTC at 5000 ms.

Functionality preservation: the candidate removes, disables, defers or reduces
no designed functionality and deletes nothing. The accepted 190 result remains
an explicit protected incumbent at the start of every later round.

Implementation now passes the consumed gate. On `m-3896`, protected round 1
reproduces `6/60/410` and coordinate ascent reaches `6/60/413` after three
accepted rounds with 665/665 generated plans valid. On `m-3877`, round 1
reproduces `6/60/460` and the fixed point reaches `6/60/464` after five rounds
with 1,174/1,174 generated plans valid. Neither replay reaches its deadline.
Fresh development completed all 54 registered fixtures. Against the protected
experiment-190 first round, the fixed point scored `27/27/0` wins/ties/losses
and gained 251 servings in aggregate, with zero lifetime/daily regression,
invalid, emergency, sidecar failure or sidecar deadline. Both fuels, both role
modes, medium/hard/very-hard tiers and every generated family contain gains;
the 12 easy dense-feasible controls tie. All 25,154 generated plans validate.
Development evidence SHA256 is
`2DE9C68AFD203C7109410CF758BB2738E16CD6939AF83D4EB987716720FF4F18`.
The preregistered development gate is cleared and the frozen holdout may open.

The untouched holdout then completed all 108 fixtures at `49/59/0`, adding 412
servings with zero lifetime/daily regression. Medium, hard and very-hard tiers,
both fuels, both role modes and all six generated families contain wins; easy
and the 12-spot dense-feasible lane remain controls. All 39,653 generated plans
validate, with zero invalid, emergency or sidecar failure. Twelve local
sidecar-slice exhaustions safely returned a certified incumbent and caused zero
loss; local timing is not promotion evidence. Holdout evidence SHA256 is
`185A5986FD99B2505E043986A1FE12D3A297CC625C3DAA078E583A9B2B8AE086`.
The holdout gate is cleared without mechanism changes; BTC target-host at the
5000-ms hard cap is now the only remaining experiment-191 acceptance gate.

Fresh authenticated BTC `m-3897` used hard/one-bot/10-day/32x32/100-step/
5000-ms/8-agent/30-spot/6-brand/high-fuel configuration. It received 10/10 HTTP
200 valid acknowledgements with maximum response 2,888 ms. Replay-check and the
independent validator accepted all ten submitted plans. On day 10 the protected
experiment-190 first round reached cumulative `6/60/432`; coordinate ascent
used three accepted rounds, 376 sparse routes and 311/311 valid generated plans
to submit `6/60/435`. It recorded four strict improvements and safely returned
the stronger incumbent after its local sidecar slice expired. Rank and bot
score are excluded. Replay SHA256 is
`9A528FD0E5AC81B0A7A30E26AC5B7AB25AF76CA640EDC5D97ABC1A868176E9A8`.

SCORE-TERMINAL-COORDINATE-ASCENT-191 is accepted. It protects the complete 190
result, has zero paired loss on fresh development and holdout, and demonstrates
a real incremental takeover on the BTC target host inside the hard cap. No
route generation, threshold, dispatcher, nonterminal planner, role logic or
experiment-185 path changes. The accepted implementation commit is `c3ee753`.

The same fresh BTC replay is also a new read-only attribution input, separate
from the 191 verdict. Team A and the bot both reached `6/60`, but Team A ended
at 435 servings versus the bot's 589. Bot rank is not promotion evidence, yet a
154-serving same-map gap is large enough to retain as a possible counterexample.
Team A used one tanker at mask 2 and served
`44,46,48,46,29,28,46,48,45,55` by day; every submission and replay plan was
valid. No logic experiment may tune to `m-3897`: attribution must first separate
role choice, nonterminal route combination, stock contention and traffic effects
using the replay plus independent/exact evidence. Replay SHA256 remains
`9A528FD0E5AC81B0A7A30E26AC5B7AB25AF76CA640EDC5D97ABC1A868176E9A8`.

The first read-only attribution retained the exact live one-tanker mask 2 and
ran the canonical closed loop over the replay's public states. It produced
exact-valid `6/60/439`, so an alternative role is not required to exceed the
live 435. Days 1 and 2 reproduced the live scores and terminal agent states;
the first state/path divergence appears on day 3. This is not promotion or
performance evidence because the counterfactual uses a local timing path and a
fixed replayed opponent trajectory. It narrows further attribution to planner
cutoff/path sensitivity from the identical day-3 input; a candidate remains
forbidden until a timing-independent witness or BTC-target reproduction proves
the causal mechanism.

Direct comparison of the two serialized day-3 decisions confirms identical
state, ledger `6/12/90`, immediate day score `6/48`, 16 audited candidates and
deadline-incomplete master search. The live path selected
`certified-undominated-current-floor` in 2,763 ms; the local counterfactual used
`certified-lexicographic` in 3,376 ms. Their only first divergent action bundle
is an immediate-score tie with different terminal continuation. The live
profile predicted the chosen terminal more strongly, yet the fixed-public-state
counterfactual suffix eventually scored higher. This is a candidate evaluator/
cutoff mismatch, but not yet a sound logic gap: timing differs and the opponent
does not react to counterfactual own traffic. Experiment 185's exact root/suffix
evidence should be consumed before designing a new comparator change, avoiding
another circular tie-break experiment.

### Accepted dense-map sparse-frontier candidate: SCORE-DENSE-SPARSE-FRONTIER-190

Research does not wait for experiment 185. Exact streaming remains unchanged on
the Spot VM while experiment 190 tests the independent BTC capability boundary
identified by experiment 189.

On `m-3877`, every patrol can individually reach all 24 spots on all ten days,
but exact high-fuel/resource orienteering is unsupported: the dense
`spot-mask x cell` representation is capped at 16 spots and, on 32x32, exceeds
the independent 8M-state guard above 13 spots. Every bounded master day is
deadline-incomplete, exact support is `0/0`, and protected refinement generates
zero candidates. Raising the dense threshold would require approximately
`2^24 x 1024` states and is forbidden as an unbounded memory/time change.

The research-only sparse Pareto frontier keeps the same legal step/fuel/cell/
spot-mask transitions, caps each patrol at 1,250,000 settled states and retains
32 globally ranked routes. On all ten immutable `m-3877` replay states it found
a strict exact-valid single-agent exchange: the day-by-day cumulative serving
gains were +2,+2,+2,+3,+3,+2,+2,+3,+2,+2. Day 10 improves the recorded parent
from `6/60/458` to `6/60/460`; simulator and independent validator agree.

The first broad integration was rejected before holdout. Feeding sparse routes
through the ordinary multi-day planner changed earlier incumbents and reduced
the closed-loop replay from the recorded `458` to `454`. Retaining only routes
that also preserve the parent terminal cell/fuel/road-footprint produced zero
strict protected gains on the consumed states. The ordinary planner was restored
byte-for-byte to HEAD and the sparse API was isolated so no existing low/high-
fuel generator can call it.

The surviving mechanism is a terminal-day sidecar. It receives the immutable
parent after the canonical solve, runs only when the dense exact representation
is unsupported, replaces one patrol route at a time, and admits only a strict
official cumulative-score improvement after exact simulation and independent
validation. No terminal-state constraint is needed because no successor day
exists. Timeout, unsupported input, invalidity or no gain returns the exact
parent. The production-path probe reproduces `m-3877` at `6/60/460` with
224/224 alternatives valid and two strict improvements; a fresh 32x32/24-spot/
high-fuel fixture under one shared absolute 5000-ms cap improves `6/60/509` to
`6/60/512` with zero invalidity. These are semantic development results; local
elapsed time has no performance authority.

Fresh development ran across the registered map/fuel/role axes. Spot
counts are intersected with the published protocol invariant
`spotCount <= max(width,height)`, so 14x14 is an exact-feasible 12/14-spot
control while denser counts are exercised on larger maps.

Development completed at `25/29/0` on 54 paired fixtures, +92 servings,
maximum gain +9, zero invalid and zero loss. Exact-feasible easy controls and all
12-spot cases tied; strict gains span medium/hard/very-hard, fixed/native,
default/high fuel and all six families. The 4,366 generated alternatives were
all accepted by both simulator and independent validator.

The frozen 108-case holdout completed at `53/55/0`, +220 servings, maximum gain
+8, zero lifetime/daily delta, zero loss, invalid, emergency or failure. Results
by suite were easy `0/24/0`, medium `8/16/0`, hard `22/8/0`, very hard
`23/7/0`. Strict gains span fixed/native, default/high fuel, every traffic
family, and every sparse spot count: 14 spots `14/20/0`, 18 `21/0/0`, 24
`13/0/0`, 30 `5/1/0`; dense-feasible 12-spot controls tied `0/34/0`.
All 9,270 planned sparse alternatives were exact-valid. Two very-hard 30-spot
cases exhausted sidecar slack and returned the exact parent. The production
diff, manifest and runner hashes remained unchanged after freeze.

The semantic/general-score gate passed. Authenticated BTC match `m-3896` then
ran the 32x32, 10-day, 100-step, 8-agent, 24-spot, 6-brand, high-fuel profile
with a 5000-ms response window. All 10 ACKs were valid; maximum authoritative
response was 2,821 ms. On day 10 the sidecar evaluated 192 sparse routes, all
192 passed the exact simulator and independent validator, and two were strict
improvements. It raised the final-day parent from `6/37` to `6/42`, hence the
cumulative virtual parent `6/60/402` to submitted `6/60/407`. Replay-check
reproduced 10/10 valid days, 10/10 validator agreement and all nine reconciled
transitions. Bot rank and bot score are excluded from the verdict.

Experiment 190 is accepted. It closes the dense BTC capability gap without a
paired regression, without changing existing planner/187 behavior and without
exceeding the 5000-ms hard cap. Experiment 185 remains an independent active
oracle gap; research does not wait for it before promoting this candidate.

The production diff was frozen before opening holdout as
`77e1d3c28fa96352e01f629925c631bccc409108`; research/test diff
`8d2d355c82e78df1ae7805c7f43fcf081a18d9da`; matrix-runner SHA256
`FC28678699A3729C3645F12120025992735142A193EFAAB53EB6C88AFC43EF4A`.
No production logic may be tuned after the holdout opens.

Frozen manifest:
`research/holdouts/SCORE-DENSE-SPARSE-FRONTIER-190.csv`, SHA256
`0391DE241480EAA0A2188CAE9714A03DE129941625147E9F8C47967A09FA698C`.

Functionality preservation: the candidate removes, disables, defers or reduces
no designed functionality and deletes nothing. The existing planner and 187
path remain unchanged; the sidecar is additive, final-day only and keeps the
parent as an explicit fallback. Experiment 185 remains independent and active.

### Closed dual-counterexample attribution: ATTR-DUAL-COUNTEREXAMPLE-189

The exact and BTC signals are both genuine current-production gaps, but code-
level attribution proves they are not the same mechanism. Current production
already selects exact day-1 root action index 48 on `1721100`; its exact robust
continuation is `5/17/18`, so the loss occurs in later closed-loop replanning.
On `m-3877`, exact guidance cannot start because all 24 relevant spots exceed
the dense mask-state feasibility boundary. Canonical evidence:
`research/evidence/ATTR-DUAL-COUNTEREXAMPLE-189.md`.

Experiment 189 is closed `accepted-attribution-unrelated-gaps`. Experiment 185
continues the exact lane; experiment 190 opens only the independent dense BTC
lane. BTC rank remains excluded as a quality metric, while the 54-serving
deficit remains a required causal signal.

### Closed current-production oracle revalidation: ATTR-ORACLE-CURRENT-188

The completed `1721100 [0,88)` slice of experiment 185 compares its exact
root witness with stale parent `828ea78`. Current competition production is
`5c3aa7a`, whose accepted experiment-187 runtime adds protected WAIT refinement
and a virtual-parent closed loop after the bounded decision. The 185 probe calls
`MatchSession` directly and does not invoke that runtime layer, so its three
traffic-policy deltas are not yet counterexamples of current production.

Experiment 188 is a parallel consumed-only attribution authorized by the user.
It uses only already-opened seed `1721100`, first requires the stale-head hash
`32dcfc4fa4a20fd6` to reproduce, then applies the exact 187 state machine under
maximum-dwell, minimum-dwell and status-toggle policies. The three policy
witnesses count as one underlying seed/state counterexample. No fresh map,
holdout or production source is opened; local elapsed has no performance
authority. Frozen manifest:
`research/holdouts/ATTR-ORACLE-CURRENT-188.csv`, SHA256
`B5210335257EB5D7A17A6457E102DB29A492DD88665C6FF8E9DFA7A21B931905`.

Functionality preservation: the planned change is an opt-in research-probe
mode only. It removes, disables, defers or reduces no designed functionality,
deletes nothing and cannot alter the production call graph.

The rebuilt stale-head control reproduced `5/17/17` and plan-sequence hash
`32dcfc4fa4a20fd6`. The exact production-187 state machine was then replayed
twice under all three policies. Maximum-dwell and status-toggle remained
`5/17/17`; minimum-dwell remained `5/16/17`. Every run was exact-valid, actual
and virtual scores were equal, solver/protected deadline counts were zero and
the protected refiner generated zero plans, valid plans, liftable plans or
takeovers. The completed 185 oracle is `5/17/18` under every policy.

Experiment 188 is closed `accepted-attribution-current-gap`. One underlying
exact seed/state counterexample, expressed by three policy witnesses, survives
against current production `5c3aa7a`. It is not a 187 regression: 187 is inert
and preserves the stale-parent plan byte-for-byte. Canonical evidence:
`research/evidence/ATTR-ORACLE-CURRENT-188.md`.

### Active dominance-conditioned deadline research: DEADLINE-DOMINANCE-ANYTIME-187

The UET school-selection event is part of the Procon Vietnam qualification
pipeline and uses the official HEXUDON problem. Its 45/60-second table is not an
authoritative PTIT or national-match deadline, but it is strong enough evidence
that the hard-coded 5000-ms total-compute policy may leave legal competition
budget unused. Experiment 166 already proved isolated 32x32 score value from a
Long solve; experiments 184 and 186 proved why ordinary Long replacement and
post-hoc comparator relaxation are not safe.

Experiment 187 changes the missing capability rather than the admission rule.
The exact 5-second result is an immutable incumbent. On the final day, where no
future state exists, an extended candidate is admissible exactly when simulator
and independent validator agree and its official cumulative score is strictly
better. On earlier days the generator is conditioned on the forward-simulation
relation already proved by experiments 175 and 178: equal ordered kinds and
terminal cells, no-less patrol fuel, equal jam-saturated traffic history,
lifetime-mask superset, componentwise cumulative daily/servings dominance and a
persistent strict gain. The incumbent remains in every candidate set and all
incomplete or non-dominating extra work returns it unchanged.

The first source probe is additive and consumed-only. It must generate exact
protected-terminal route alternatives directly instead of asking the ordinary
Long pool to contain them accidentally. It first measures terminal-day strict
gains and nonterminal liftable existence on the already-opened 166/184 large-map
states, then compares the same boundary relation with the completed exact-178
roots. Only a natural witness permits opening fresh stratified development.
This relation may improve lower bounds or root ordering for experiment 185, but
187 may not truncate or approximate 185's legal state space.

Frozen manifest:
`research/holdouts/DEADLINE-DOMINANCE-ANYTIME-187.csv`, SHA256
`6598C60159E1397803B50FE8211BFA7B098CB1FF078FD01DE2032FDECB7583F6`.
The consumed existence gate found a natural nonterminal witness on 32x32
high-stock seed `4411000`.  A protected WAIT interval was replaced by an
off-road round trip that returned to the same cell and time, preserved the raw
road footprint, did not reduce patrol fuel and raised servings by one.  The
virtual-parent continuation retained the gain through day 10: actual
`6/60/467` versus the in-run virtual parent `6/60/466`, with zero
invalid/emergency and state/ledger dominance asserted after every day.  Low and
high-fuel consumed controls tied their virtual parents with zero takeover.
Fresh stratified development then produced `20/52/0` over 72 paired fixtures
across 14/20/26/32, fixed/native roles, all six map families and low/default/high
fuel.  Total persistent gain was 31 servings, loss was zero, and every
state/ledger assertion held.  Terminal continuation stayed inert; all gains
came from the nonterminal protected WAIT neighborhood. The sealed 144-fixture
holdout then produced `36/108/0`, total gain +55 servings and loss zero: easy
`11/25/0`, medium `10/26/0`, hard `6/30/0`, very-hard `9/27/0`; fixed
`25/47/0`, native `11/61/0`. Strict gains occurred in low/default/high fuel and
all six map families. Invalid, emergency, protected-state failure,
protected-ledger failure and protected-phase deadline were all zero.

Experiment 187 is accepted for production. Runtime uses one delayed submission,
not a resend: the canonical 5-second decision is the virtual parent and one
off-road round trip may replace a protected WAIT only after exact simulator,
independent validator, terminal-state, road-footprint and cumulative-ledger
dominance all pass. Future bounded decisions continue from the virtual parent;
the plan is revalidated on the authoritative richer state. Runtime remains on
the authoritative state until an improved protected plan is acknowledged, and
resets to authoritative state whenever the forward-simulation relation fails.

The refiner is charged only to unused time inside the same 5000-ms compute cap.
An outer 15/45/60-second server window does not enlarge search time. BTC matches
`m-3876`--`m-3881` produced 60/60 valid acknowledgements. Active takeovers
retained +2, +1, +1 and +1 servings in `m-3878`--`m-3881`; `m-3881` proved a
strict +1 gain under a configured 5000-ms window with maximum response 2466 ms.
Corrected state-machine run `m-3880` was inactive through day 6, active only
after its acknowledged takeover, and preserved the gain through day 10. Every
non-takeover action in corrected runs was byte-identical to the virtual parent.
Canonical evidence:
`research/evidence/DEADLINE-DOMINANCE-ANYTIME-187.md`.

Functionality preservation: no designed component is removed, disabled,
deferred or reduced; nothing is deleted and no equivalent-implementation proof
is needed.

### Closed consumed closed-loop certificate probe: DEADLINE-VIRTUAL-PARENT-186

Experiment 184 closed its tested mechanisms but did not prove the entire
extra-time axis inert. Its transition gate required exact equality of final
agent fuel and raw footprint, while experiments 175 and 178 already prove a
strictly broader relation: equal ordered kinds/cells, componentwise no-less
patrol fuel and exact equality after the jam-saturated traffic-history quotient.
The missing integration invariant is future planner state. Replanning directly
from the richer challenger is not monotone for the bounded heuristic and caused
the regressions in 180/184.

Experiment 186 measured a consumed-only successor before any production
integration. A Long challenger was `liftable` only when its authoritative
post-day state forward-simulates the protected 5-second parent: ordered
kinds/cells equal, patrol fuel no lower, opponent/public state equal, saturated
two-day traffic evolution equal, lifetime mask a superset, cumulative daily
distinct and servings componentwise no lower, and a strict advantage that
cannot disappear into a final response-time tie. Spot stock is not persistent:
both simulator and validator reconstruct it from `MatchConfig` at every day, so
it adds no cross-day state component.

For a liftable pair, future days use one canonical solve from a saved virtual
parent state and replay that plan on the authoritative richer state. Both paths
must exact-simulate and independently validate; after every day the same
simulation relation must remain true. Same-day rejected exploration must be
snapshot/rollback clean. This is not the sampled scenario comparison rejected
by 184 and not a dual future solver.

Frozen consumed-only manifest:
`research/holdouts/DEADLINE-VIRTUAL-PARENT-186.csv`, SHA256
`D7C730502AAE4BAD2DA38A08887DE6585DE57F177FDA325C9DD5EE6E84DE4543`.
It contains only the already-opened states from `4310000` and `4411000`;
all fresh development and holdouts remained sealed. Zero liftable selected or
pool candidates closed the successor as sound but inert. Production and the
5000-ms cap remained unchanged while the probe ran in parallel with experiment
185.

The first apparent `0/300` result was invalidated because the temporary
unclamp hit fixed-role-unused `select_roles_until` rather than `solve_day`.
The corrected executable asserted `parent_budget_ms=5000` and
`challenger_budget_ms=10000` on every day. It produced eight different
selected challengers per fixture, zero liftable selections and zero invalid
plans. The stronger pool audit then checked `316` different candidates on
`4310000` and `318` on `4411000`; all `634` simulator-validator agreed,
with `pool_liftable=0` and `pool_invalid=0`.

Experiment 186 is closed `rejected-sound-but-inert`. The certificate is
mathematically valid, but neither strongest consumed Long pool contains a
usable witness, so production integration would be inactive overengineering.
Fresh development, holdout and BTC remained sealed. Canonical evidence:
`research/evidence/DEADLINE-VIRTUAL-PARENT-186.md`. Reopen only for a new
independently generated exact candidate satisfying the full relation with
persistent score gain.

### Active exact root streaming: ORACLE-ROOT-STREAM-185

Experiment 178 proved an exact traffic-history quotient that reduced full
minimax states by about half, but the still-open default/high consumed rows can
run for more than a day without emitting a root result. Experiment 185 changes
only the evaluation schedule: enumerate the unchanged exact root own-action
frontier, solve a configured contiguous slice with one shared quotient memo,
and flush an exact robust score after every completed root action. Completed
action certificates survive an interrupted or OOM-killed run; a resumed job
starts at the first unfinished action. Combining the maximum over all completed
slices is exactly the original root max, with no action or opponent reduction.

Frozen consumed-only manifest:
`research/holdouts/ORACLE-ROOT-STREAM-185.csv`, SHA256
`10EDBB0D52E61E083D536C764493E8D3D4549F0D3F055A549E7A0D07C567FC5B`. It contains only the three
already-opened 177 rows `1720100`, `1721100` and `1721200`; no holdout is opened.
The first gate must reproduce a completed low-fuel result under unsliced versus
full-slice modes and validate every emitted witness. Only then may a disposable
Spot VM run default/high slices. Output cadence and VM elapsed have no BTC or
production-performance authority. Production source and the 5000-ms cap remain
unchanged.

Five registered slices are now complete and mirrored locally with immutable
hashes. Both `1720100 [0,86)` and `[86,172)` close at exact `6/18/18`, tying
the recorded 185 head. Both `1721100 [0,88)` and `[88,175)` close at exact
`5/17/18`; their recorded head is `5/17/17` under maximum-dwell/status-toggle
and `5/16/17` under minimum-dwell. Experiment 192 independently confirmed that
this `+1` serving exact witness still exceeds the then-current production path.
The completed `1721200 [0,118)` slice closes at exact `5/20/21` versus recorded
head `5/19/22`, an official tier-2 counterexample that still requires
revalidation against the latest production parent before it may open source
work. Completed-log SHA256 values are respectively
`D1C2FF4C5C2A7AF6983039B9678E8F18B087A387A7E7986E2861234B6883C0A0`,
`4F251FD4230DA7803CBBD01F2BDC90406F6571E3001F3C52650750801CC6E4E7`,
`4B7B3466E1B260F388BAC42760D5E528983E859C395812A32DF07AF5C0076DEC`,
`530D2EE425BEFF8601062A9BBD7E18A4E2B2B9F9298C0543602DC53AECC6ACE9`
and `04B0113736D6A9E3138D0A469B01CF4B26F7B9041A31B22C079FBFBB37062D14`.
The remaining `1720100 [172,258)`, `[258,344)` and `1721200 [118,236)`
slices continue unchanged on the Spot VM; no partial action may be promoted as
a completed slice result.

### Closed stratified deadline research: DEADLINE-STRATIFIED-ANYTIME-184

Experiment 184 is closed `rejected-closed-loop-regression`. The authenticated
PTIT practice form permits at most `15000 ms` per day and `32x32`; the external
UET table is not PTIT authority and does not justify 45/60-second production
compute or maps beyond 32.

The unchanged strict lower-versus-upper takeover remained inert on consumed
large-map gates. A transition-preserving delayed-selection gate rejected all
18 different challengers across `4310000` and `4411000`, accepting zero. A less
conservative Actual-vs-Actual gate compared certified outcomes on identical
scenario IDs/classes/weights and did activate, but high-stock `4411000`
reproduced `6/60/454` twice with five takeovers while adjacent protected
5-second controls scored `6/60/461` and `6/60/462`. The scenario profile did not
cover the endogenous closed-loop consequence and therefore was not a monotonic
certificate.

All stratified holdouts remained sealed and no BTC match was created for a
candidate already falsified in development. All experiment-only source and
harness changes were removed. Canonical evidence:
`research/evidence/DEADLINE-STRATIFIED-ANYTIME-184.md`. The production hard cap
remains 5000 ms. Reopen only after an authoritative PTIT window above 5 seconds
and a complete closed-loop certificate first eliminate the consumed regression;
do not weaken the gate or route by fixture metadata.

### Closed proof attribution: SCORE-SPATIOTEMPORAL-UPPER-183

Experiment 183 is closed `rejected-tight-but-inert`. Its candidate-independent
spatiotemporal relaxation was never below a realized final score on 75 consumed
general day states and never below the three completed exact-178 continuations.
It tightened 12/75 general states, all in the three high-stock families, and
10/10 opened BTC-large states. The strongest root serving reductions were
`96 -> 69`, `84 -> 57` and `104 -> 70`; BTC-large tightened `840 -> 621`.

The decisive protected reconstruction of experiment 168 still tied the parent
on all 18 fixtures with zero invalid and emergency outcomes. All registered
long trajectories completed and 48 scenario lower witnesses improved, but the
unchanged `may_submit` gate accepted `0/18` resubmissions. The upper is sound and
tighter, yet insufficient to create any certified strength improvement. No
sealed holdout or BTC performance gate was opened, all experiment-only source
was reverted, and no commit was created. Canonical evidence:
`research/evidence/SCORE-SPATIOTEMPORAL-UPPER-183.md`.

The user-supplied UET table is retained only as an external configuration
reference. It adds 14/20/26/32 map sides, 5/7/8/10 days and 4/6/8 agents to the
future research strata. Its 45/60-second server windows are not PTIT authority
and do not change the current internal 5000 ms cap. Difficulty and rank-point
multipliers aggregate tournament placement and do not change the official
within-match lexicographic objective.

Research is ready to return to the registered streaming exact-counterexample
path of experiment 178. Experiment 183 may reopen only with a stronger sound,
state-coupled upper that first creates a strict unchanged-`may_submit` takeover
on consumed evidence; the comparator must not be weakened.

### Competition artifact and runbook

The competition source is the accepted experiment-187 production line over
parent `0f01d69`. It adds only the protected slack module, authoritative deadline
normalization, virtual-parent runtime state, focused tests and architecture
wiring. The prior bounded planner/decision implementation remains unchanged.
The VS2022-authoritative rebuild completed through `VsDevCmd.bat`; direct `cl`
from the current Codex process is invalid because that process alone inherits
obsolete VC98 `INCLUDE`/`LIB` values.  Persistent user and machine environment
variables do not contain VC98, so no installed directory deletion is required.

The current unit/simulator/validator gate passes. Current
`build-release/udonshield_btc.exe` SHA256 is
`AE761118A7DD086B211FF9BB3CD99EBD0DBD3FEED332E9511E23742F25B8F356`.
Replay-check on target-host archive `m-2120.jsonl` validated all 10 days and
reconciled all 9 transitions, final score `6/60/415`.  BTC HTTPS returned 200;
all required MSVC runtime DLLs are installed; no BTC token exists in repository
text or persistent/process environment; no BTC process is running.  AC/DC sleep
and hibernate timeouts are disabled.

The only remaining manual readiness gate is privileged Windows time sync.  The
current host reports `Leap Indicator: 3 (not synchronized)` and
`Source: Local CMOS Clock`; this Codex process cannot resync because `w32tm`
returns access denied without elevation.  Before joining any competition match,
an Administrator PowerShell must run `w32tm /resync /rediscover` and
`w32tm /query /status`, and the bot must not start until the unsynchronized/CMOS
status is gone.  Complete commands, unique-replay rule, same-file resume and
post-match preservation are frozen in `COMPETITION_RUNBOOK.md`.

### Closed exact-neighborhood source gate: SCORE-PENULTIMATE-NEIGHBOR-182

Experiment 181 is closed `rejected-capability-link`. Restricting the sidecar to
the penultimate day removed the first-day regression, but produced no takeover
on any exact anchor. On `1720000` the coordinated team-bundle projection exposed
only one non-parent candidate at current `6/9/9`, certified exactly to
`6/11/11`; it omitted the already registered per-agent mask-`111` route whose
forced profile is `6/12/12 .. 6/13/13`. The exact frontier exists, but the small
team-bundle selector does not project it into the post-parent candidate set.

Experiment 182 tested that missing link without returning to the shared pipeline.
After the completed parent, each fuel-constrained patrol independently runs the
existing complete resource-route enumerator in its fair share of genuine slack.
Every completed route forms one team candidate by replacing only that patrol's
actions in the protected parent plan; all other agent actions remain byte-exact.
Each candidate is exact-simulated, independently validated and admitted only by
the unchanged bound-closed penultimate takeover. There is no Cartesian product
or new solver. Frozen manifest:
`research/holdouts/SCORE-PENULTIMATE-NEIGHBOR-182.csv`, SHA256
`A366F7E16D794696D36DFB457B0B775C9BC1DDC4D56E1CE0DDE217D441248516`.
Compile and unit gates passed, but the first causal gate rejected it. From the
unchanged parent day-3 state on `1720000`, the full completed per-agent exact
frontier contained exactly one non-parent exact-valid team plan; it tied current
`6/9/9` and certified `6/11/11 .. 6/11/11`. The mask-`111` challenger exists
only after the different state produced by the oracle day-1 prefix. Therefore
the remaining gap is genuinely multi-day path-dependent, not a missing route or
bundle from the parent penultimate state. Other anchors, 176 rows, fresh lanes
and holdout stayed unopened; production source was reverted and is content-
identical to `828ea78`.

The source branch is now closed as non-exploitable under the current evidence.
The only mechanism that reproduced the exact multi-day path was 176, which lost
`1/5/6` on fresh general development with four tier-2 regressions. Protecting the
complete parent then using the same static future certificate was tested in 180
and regressed an independent exact anchor. Per-day coordinated and full
single-agent exact neighborhoods were tested in 181/182 and cannot reach the
oracle state. A sound successor therefore requires the complete closed-loop
public minimax itself, whose completed low-fuel rows require roughly 6--8 million
memo states and 604--705 million legal transitions, while the first default-fuel
row OOM-killed after over 20 hours. That evaluator cannot fit the `5000 ms` cap
with the current method, and the exact wins all use an artificial one-active
patrol plus isolated controls. A structural guard for that fixture would not be
general competition logic. Reopen only with a new exact quotient/algorithm that
places the complete closed-loop proof inside the hard cap and demonstrates a
non-artificial multi-agent counterexample; no route-width, certificate or
fixture-family variant remains open.

### Closed coordinated penultimate sidecar: SCORE-PENULTIMATE-EXACT-181

Experiment 180 is closed `rejected-causal-regression`. Its post-parent boundary
did preserve the fully completed parent before extra work, and exact generation
and profile repair completed on all three consumed anchors. Nevertheless the
first-day takeover did not preserve the realized closed-loop continuation:
`1720000` tied parent at `6/11/11`, `1721000` regressed from `5/10/11` to
`5/9/10`, and `1722000` tied `5/11/11`. The registered certificate covers the
static public scenario manifest but not every state reached after several future
replans. Fresh development and holdout were not opened.

Experiment 181 kept the only proven monotonic control-flow boundary—complete
parent first, incomplete sidecar returns parent—but removes the speculative
first-day takeover. It activates only on the public penultimate day. This is the
shortest closed-loop horizon on which the identified fuel value can matter: the
existing exact final-day production path gets one immediate replan from the
retained terminal state, rather than requiring a multi-day static certificate to
predict repeated future replanning. Frozen manifest:
`research/holdouts/SCORE-PENULTIMATE-EXACT-181.csv`, SHA256
`7979A4B372D82A4B4BB6F62A8CD1C626809C5521CA8978CFB94883456F5DC12F`.
It tied all three exact anchors because the coordinated team-bundle exposure
omitted the registered exact mask-`111` witness. Fresh rows stayed sealed.

### Closed first-day sidecar: SCORE-POST-PARENT-EXACT-180

Experiment 180 addressed the unresolved exact fuel/horizon gap without reopening
the rejected shared-pipeline mechanism. Three independent exact low-fuel rows
prove that HEAD front-loads score before terminal day; 176 closed one anchor but
regressed fresh general development because exact work changed the shared
master/ALNS/F0 opportunity set. Experiment 179 proved that restoring the skipped
legacy checkpoint did not recover those losses.

The new proof boundary is later and strictly additive. The unchanged parent must
finish final certified selection and independent validation first. Only genuine
slack remaining before the unchanged `1600 ms` BTC network reserve and `25 ms`
final-validation floor may run the existing deadline-aware fuel-exact generator
and master as a post-parent sidecar on the public first-day or penultimate-day
fuel-allocation boundaries. The parent candidate/profile is immutable. A sidecar
candidate may replace it only after independent validation and the same complete
scenario-wise lower-versus-parent-upper bound-closure proof used in 176. Any
incomplete enumeration, deadline, invalid candidate, missing proof or lack of
strict upside returns the exact parent.

This is not a second full solver: it reuses only the existing exact route kernel
after the full parent result is already protected. Archive target-host telemetry
shows why it must be opportunistic rather than always-on: among 110 decisions,
57 had under 25 ms but 35 had at least 500 ms before the network reserve. Local
elapsed is attribution only; BTC remains performance authority. Frozen manifest:
`research/holdouts/SCORE-POST-PARENT-EXACT-180.csv`, SHA256
`B9817875EF78952A8A2E63EEFEA46EFAD68F04477CB0EECC3ACA3B18BAEEB463`.
Its causal result was `0/2/1` W/T/L on the three exact anchors. Sidecar generation
and certification completed, but the day-1 takeover on `1721000` caused a tier-2
and tier-3 closed-loop regression. This proves that protecting the parent before
extra work is necessary but not sufficient when a static future certificate is
used across several later replans. Source successor 181 retains the sidecar but
restricts takeover to the penultimate allocation boundary.

### Closed source attribution: SCORE-PROTECTED-EXACT-179

Experiment 179 is closed `rejected-causal-falsification`. The static omission
was real: 176 skipped `solve_legacy_until` when nonterminal exact was enabled.
However, reconstructing 176 and restoring that exact legacy checkpoint produced
identical official scores and identical recorded search counts on all twelve
consumed 176 fixtures. It changed no selected result and recovered none of the
paired regressions. Therefore the legacy checkpoint is not an exploitable cause;
its candidates are duplicated or noncompetitive after merge.

The unresolved cost is the shared post-merge bounded pipeline: exact columns
change master/ALNS/F0 work under one 5000-ms window. Preserving the complete
parent result would require two complete searches unless an equivalent-semantic
speedup creates headroom. That duplicate-solver design is forbidden. No fresh
179 development or holdout was opened; production/test source was reverted to
`828ea78`. Frozen manifest SHA256
`7602CFFBC108B39268B6A6D0201DE28814C0546C41793DB04D1945133F3A0B6F`;
canonical evidence: `research/evidence/SCORE-PROTECTED-EXACT-179.md`.

### Accepted exact-method quotient: ORACLE-TRAFFIC-HISTORY-QUOTIENT-178

Experiment 178 tested one independent exact quotient on consumed development
only while 177's proof-host processes were still active.  The
public minimax memo currently distinguishes `previousOwn` from
`previousOpponent`, but source tracing proves that every future transition
observes these arrays solely through their componentwise sum when computing the
next road-status vector.  Current public state, legal action enumeration,
official score, causal policy choice, simulator and validator do not observe the
ownership split.  Opponent same-cell/same-footprint maximum-fuel dominance is
already implemented, so it is recorded as duplicate rather than coded again.

The proposed canonical history is the exact componentwise sum saturated at
`players * jammedThreshold`: values at or above that level are observationally
equivalent because all new traffic contributions are nonnegative and the next
status must remain jammed.  This is a research-only mode switch, never a source
candidate.  Frozen consumed-only parity manifest:
`research/holdouts/ORACLE-TRAFFIC-HISTORY-QUOTIENT-178.csv`, SHA256
`AC97B64F4019476EA68EA69B05A09D997FA671324486924771955A18D47B5BF0`.
It matched the unquotiented oracle on all three consumed families at complete
one-day and two-day suffixes.  Full-horizon robust scores remained respectively
`6/12/12`, `5/11/11` and `5/11/12`; all nine causal-policy results, plan hashes,
exact simulations and independent-validator results were identical.  Memo states
fell by `48.78%`, `50.03%` and `49.16%`, while legal transitions fell by
`28.14%`, `29.22%` and `27.55%`.  Experiment 178 is accepted as canonical exact-
oracle infrastructure.  It does not change production strength or authorize a
source candidate.  Experiment 177 is now closed; no brute-force default/high
retry is authorized without a new exact quotient or algorithm that changes the
feasibility class while preserving exact public semantics.  All 27 holdouts
remain sealed.

### Closed exact-development sweep: ATTR-EXACT-FUEL-PREVALENCE-177

Experiment 176 is rejected and production remains content-identical to
`828ea78`. Successor 177 makes no source change. It uses the unchanged exact
oracle from 175 on the eight still-unopened development rows to decide whether
the fuel-preserving `oracle > HEAD` witness repeats as a public structural class.
The frozen manifest remains
`research/holdouts/ORACLE-BOUNDARY-DOMINANCE-175.csv`, SHA256
`79A7C16B8A37841C180C43F6480949DCB777D14777CD1463958C82BE148CBE12`;
all 27 holdout rows stay sealed. Runs are independent on a disposable 4-vCPU,
32-GB Spot proof host and each completed log is copied off-host immediately.
VM elapsed has no performance authority. No source successor is admissible
unless multiple fresh exact gaps share a public discriminator that excludes the
systematic losing families from 176; otherwise the branch closes.

The first three completed exact gaps span balanced, threshold and terminal
low-fuel constructions, but `low fuel` alone cannot reopen source work because
176 already lost systematically on fuel-tight and overnight development.  A
parallel read-only attribution is restricted to state-level marginal fuel,
remaining horizon, reachable-brand and exact-traffic quantities; no candidate
or dispatcher is active.  The reconstructed `1721000` trace now confirms the
same mechanism as the other two wins: oracle day 1 retains fuel 9 after taking
`2/2`, whereas HEAD retains fuel 2 after taking `5/6`; oracle then takes `3/3`
on each remaining day and wins tier 2.  Thus all three are pre-terminal fuel-
value/front-loading gaps.  Their declared one-active-patrol subdomain suggests
decomposability as the only current public discriminator, but this is not yet a
production guard because brand, stock, traffic and master coupling are unproved.
Default-fuel balanced seed `1720100` was OOM-killed
without a score result after `20:06:23` and maximum RSS `11,433,732` KiB.
Threshold default/high seeds `1721100` and `1721200` produced no score result.
They were terminated after `27:09:35` and `27:03:41`, with maximum RSS
`12,954,084` KiB and `13,280,704` KiB respectively; their result streams stayed
empty.  Immediately before termination the processes had elapsed about 97,655
and 97,302 seconds, and the host had `5,913,948` kB `MemAvailable`; afterwards
it recovered to `32,168,852` kB.  These are method-intractable rows, not oracle
ties or losses.

The processes were closed about 50 minutes before the prior 28-hour cutoff
because experiments 180--182 had already removed their remaining decision
authority: every completed win is confined to the artificial one-active-patrol
subdomain, the only source mechanism that reproduced the multi-day path lost
fresh general development, parent-protected static takeover regressed another
exact anchor, and complete per-day neighborhoods cannot reach the required
day-1-derived state.  A default/high result from the same artificial fixture
could not supply the missing general multi-agent discriminator.  Continuing the
same enumeration could therefore only add another synthetic oracle result, not
change the source verdict.  Experiment 177 is closed
`closed-method-intractable-nongeneral`; seeds `1720200`, `1722100`, `1722200`
and all 27 holdout rows remain unopened.  Reopen only for a genuinely new exact
quotient or closed-loop algorithm that fits the complete proof inside `5000 ms`
and a non-artificial interacting multi-agent counterexample.

The disposable Spot VM `udon-proof-177-0820` and its auto-delete boot disk were
deleted after evidence capture.  The dedicated 30-minute monitor was also
deleted; no experiment-177 process or recurring task remains active.

### Closed source gate: SCORE-BOUND-CLOSED-FUEL-176

Experiment 175 is now fully attributed. Its exact causal gap is repeated
pre-terminal fuel front-loading, not deadline loss, opponent clairvoyance or a
weak terminal solver. Forcing the oracle prefix only through day 2 leaves HEAD
at `6/11/11`; forcing through day 3 lets unchanged HEAD close exact `6/12/12`
on day 4. The canonical complete fuel-constrained exact generator contains the
required joint outcome on both missing days (day 1: route mask `111`, 251
states; day 3: route mask `111`, 50 states), whereas the production portfolio
does not.

The profile boundary is also exact. Day-1 challenger lower/upper is
`6/11/11 .. 6/15/15` versus parent `6/11/11 .. 6/11/11`.
Day-3 challenger is `6/12/12 .. 6/13/13` versus parent
`6/11/11 .. 6/12/12`. Wholesale removal of the protected current floor is
forbidden by experiment 057. Successor 176 instead permits a takeover only when
challenger certified survival is no worse than incumbent valid-upper survival
at every threshold under identical scenario weights, with strict challenger-
upper upside at at least one threshold. The no-regression proof comes only from
challenger lower versus incumbent upper; upper strictness is an anti-churn
condition, never a guarantee.

The source candidate may only wire the existing complete fuel-constrained exact
enumerator under unchanged caps/deadline and add this bound-closed relation. No
second solver, width increase, blind nonterminal flag experiment, seed/family
dispatcher or general current-floor relaxation is allowed. Frozen manifest:
`research/holdouts/SCORE-BOUND-CLOSED-FUEL-176.csv`, SHA256
`EE9EE677FD189EC61D37936C2B5B84FAD8157F8BFC4F17089D4C157165B16113`.
The final source candidate passed proof tests and causally closed consumed seed
`1720000`: it reproduced exact `6/12/12` versus parent `6/11/11`. Fresh general
fixed-role development then rejected it at `1/5/6` W/T/L. Four losses were at
tier 2 across threshold-corridor, high-stock, fuel-tight and overnight; two more
were tier-3 losses. Invalid and emergency remained zero. This is systematic
downside, not an acceptable small trade-off. Native, BTC-like and sealed holdout
lanes were not opened. Production and test source were reverted to `828ea78`;
the exact counterexample remains valid research evidence but has no accepted
production fix. Reopen only on a new public structural discriminator supported
by multiple fresh families, never by broad day-1/penultimate exact activation or
bound-closed takeover alone. Local latency has no authority; BTC was not run.

### Exact counterexample accepted; attribution active: ORACLE-BOUNDARY-DOMINANCE-175

The user authorized one narrow self-check of the only mathematically relevant
kernel from a new external suggestion. Production remains source-clean at
`828ea78`; no production candidate is active. Experiment 175 reopens the closed
172 method only under its explicit condition: a new sound exact quotient.

The original suggested state relation was rejected before source work because
it omitted opponent physical state and the two-day traffic memory. The corrected
proof obligation is max-node/day-boundary only. Own outcome `B` can be removed
only if another outcome `A` has the same terminal cell and exact saturated own
road footprint, at least as much fuel, a superset after union with the current
lifetime-brand mask, no fewer current daily distinct or servings, and at least
one strict improvement. Equal footprint is mandatory: less own congestion may
increase the legal adversary's future capability and is not monotone in a
two-sided minimax game.

The frozen manifest is
`research/holdouts/ORACLE-BOUNDARY-DOMINANCE-175.csv`, SHA256
`79A7C16B8A37841C180C43F6480949DCB777D14777CD1463958C82BE148CBE12`.
The exact relation passed complete filtered/unfiltered parity on all nine final-
day development configurations. On the two-day traffic subproblem for consumed
seed `1720000`, both modes returned `6/9/9`; states fell
`2,328,298 -> 1,430,951` and transitions
`40,956,276 -> 14,893,534`.

The full four-day run was then allowed to complete. Robust minimax returned
`6/12/12` after `8,016,487` states and `704,745,239` transitions; the dominance
relation removed `45,710,194` own outcomes. Under all three registered causal
opponent policies, the exact policy remained `6/12/12` while unchanged HEAD was
`6/11/11`, a valid tier-2 loss of one daily distinct with zero HEAD deadline-
limited days. This is an accepted public-information counterexample on the
declared one-active-patrol subdomain, not source-promotion evidence: the seed is
consumed and the oracle is far outside the 5000-ms compute cap.

The active gate is witness attribution. Identify the first day and general
capability boundary at which HEAD loses the twelfth daily distinct; do not port
the oracle, tune a threshold to seed `1720000`, open sealed holdout or create a
production candidate before the causal mechanism is understood. Fresh full-
match development may be used only after attribution defines a pre-registered
general prediction.

No designed functionality is removed, disabled, deferred or reduced; no
deletion is proposed. Local time and memory have offline method-feasibility
authority only and cannot support competition-performance or promotion claims.

### Practical convergence decision after sweeps 172--174

No source candidate is active. Canonical competition source and BTC executable
remain `828ea78`; the currently frozen rebuilt executable hash is recorded in
the operational artifact section above.
No commit was created in this research cycle.

Two independent counterexample sweeps are closed:

1. `CEILING-TRAFFIC-MINIMAX-172` attempted a non-clairvoyant exact legal-
   opponent ceiling. Full state, exact action caching and the final proved
   Markov quotient all failed to close even the first development fixture;
   continuing requires reduced rules, unproved dominance or an external proof
   host. No score or holdout was opened.
2. `ATTR-METAMORPHIC-INVARIANCE-173` found real representation sensitivity.
   The isolated source candidate `SEM-CANONICAL-LABELS-174` repaired the stated
   spot/brand invariance but lost its first frozen general holdout `6/19/11`,
   with tier-2 gain/loss `7/14`. It was fully reverted before native/BTC
   holdouts. A different canonical ordering would tune the opened holdout;
   multiple full representations would duplicate/split the 5000-ms solver.

Therefore offline pre-opponent research is at practical convergence for the
current architecture and evidence set. There is no remaining candidate with a
sound monotonic mechanism and positive expected global value; reopening the
consumed traffic, label or agent-order axes would repeat the same cycle or
overengineer around arbitrary representation. This is not a mathematical proof
against unknown adversaries. The only justified reopen triggers are a genuinely
new diverse human/adversarial replay, a new sound dominance/proof quotient, a
production call-graph/cap regression, or a changed public match rule. Until one
appears, `828ea78` is the final competition line and further offline heuristic
experiments are forbidden.

`SEM-CANONICAL-LABELS-174` is closed `rejected-general-regression`; no source
candidate is active. The candidate correctly made spot reversal and bijective
brand relabel score-invariant and passed development overall `4/11/3`, but the
first opened 36-fixture general fixed holdout was only `6/19/11`. Tier-2 total
gain/loss was `7/14`; tier-3 was `1/4`; zero invalid/emergency. All 72 holdout
spot/brand metamorphic pairs were equal, so this is not an implementation bug:
choosing one canonical representation selects a non-monotone bounded-search
lane and loses globally. Native and every BTC-like holdout remained sealed.
Choosing a different ordering from the consumed results is forbidden overfit;
running several full representations would split the same 5000-ms budget or
duplicate the solver without a dominance certificate. Candidate and harness
code were fully removed; all production and research source files are
content-identical to `828ea78`. Canonical evidence:
`research/evidence/SEM-CANONICAL-LABELS-174.md`; manifest SHA256
`4D7A79E2DACF1CB92AD2FE058075538C6E817F80464CA7EC41ADBBC554A6243D`.
Agent-order sensitivity from 173 is closed under the same non-monotonicity
lesson: an end-to-end canonical remap merely selects another lane, while a
multi-lane solver violates the current complexity/budget gate. Reopen only from
a single-pass representation-invariant mechanism that preserves every parent
candidate and proves monotonic dominance before new fixtures.

`ATTR-METAMORPHIC-INVARIANCE-173` is closed `accepted-gap`. Canonical evidence:
`research/evidence/ATTR-METAMORPHIC-INVARIANCE-173.md`; its sealed holdout was
never opened.

`CEILING-TRAFFIC-MINIMAX-172` is closed
`rejected-method-state-explosion`; no score experiment is active. It attempted
to replace the clairvoyant future-footprint premise of 124 with a conservative
exact four-day max-own/min-legal-opponent game on a valid one-active-patrol
subdomain. Only development seed `1720000` was touched. Full memoization and an
exact day-action cache each failed to close after more than 120 seconds. The
last proved Markov quotient retained maximum fuel only for equal terminal
position, footprint and day score, but still grew from about 251 MiB at 30
seconds to 431 MiB at 60 seconds without closing. Local measurements here have
method-feasibility authority only, never competition-performance authority.
Any further reduction requires beam/action truncation, shorter horizon,
synthetic opponent footprints or unproved congestion dominance, all forbidden
by the registered semantics. All 27 holdouts stayed sealed. The probe code was
removed and `research/probes/multi_patrol_oracle.cpp` is content-identical to
HEAD. Canonical evidence:
`research/evidence/CEILING-TRAFFIC-MINIMAX-172.md`; manifest SHA256
`6FD7914EE9AED02E0AC7CC7C0742D010A7D93D5365473031531CAF055B3F148A`.
Reopen 172 only with a new exact quotient/proof representation or external
proof host, never by reducing rules or tuning seed `1720000`.

`ATTR-OPPONENT-QUOTA-171` is closed as accepted comparison evidence with
no production candidate. Canonical production remains source-clean at
`828ea78`; no UDON production source changed. The peer is pinned outside the
repository at
`lethinh26/Hexudon-2026@49e78066ea8b25a9d3cbe4baf56b390d4419eb75`.

The earlier methods are excluded from strength conclusions. Experiment 169 was
stopped because local wall-clock load changed the peer restart count and repeated
score. Experiment 170 made small cases deterministic but unbounded exact search
did not finish on the first 12-spot large case. Its fixed comparisons were also
later found to use the adapter's default all-patrol UDON mask, not the manifest's
one-tanker-last lane.

The final frozen manifest is
`research/holdouts/ATTR-OPPONENT-QUOTA-171.csv`, SHA256
`01A78FBCAC3627242AC51FB29E261965D6AB6C5B84255FD7CDD8BF8AE4EECD3D`.
The peer ran three independent fake-clock match tracks at 20/10/5
pseudo-microseconds per deadline observation, corresponding to deterministic
200000/400000/800000-observation soft bounds. Each track owned its planner and
history. The oracle-best final common score deliberately over-grants peer while
removing local CPU dependence. UDON retains its canonical logical 5000-ms
research budget; this experiment makes no local or BTC performance claim.

All 24 peer development rows reproduced field-exact. A fixed-lane adapter error
was detected before verdict: UDON had `role_mask=0` instead of one tanker last.
Every all-patrol fixed row was invalidated; all four fixed development rows and
all 24 fixed holdout rows were rerun with mask 8 or 128. Corrected development is
`8/0/0` for UDON.

Corrected holdout is `47/1/0` overall: fixed `23/1/0`, native `24/0/0`,
generated-small `12/0/0`, low fuel `12/0/0`, default `11/1/0`, and high
fuel `12/0/0`. There are zero invalid plans, zero UDON emergency days and zero
peer repairs. The only tie is default/high-stock fixed seed 171214 at
`6/60/525`. The closest native win is low-fuel threshold-corridor seed 171110,
`6/60/362` versus `6/60/356`, a six-serving margin. Peer private score
disagrees with the common judge on 226/410 days across oracle-selected tracks, so
only the exact simulator plus independent validator has authority.

No losing current-HEAD counterexample emerged. The peer's exact/beam/team-B&B,
forced-first-spot LNS, default-off horizon and refuel scheduling do not expose a
general capability that UDON lacks; static native role selection is materially
weaker than UDON role search. The tie and closest win remain challenge fixtures,
not reopen evidence. Canonical evidence is
`research/evidence/ATTR-OPPONENT-QUOTA-171.md` and
`research/evidence/ATTR-OPPONENT-QUOTA-171-pairs.csv`.


Outside this read-only comparison, no score experiment is active. Canonical production is source-clean at
`828ea78` (`Enforce canonical competition compute cap`). Experiments 166--168
close the only newly reopened public-deadline branch without promoting source:
direct Long used extra search productively on some BTC-scale fixtures but
regressed two of six general fixtures; protected same-engine refinement was
safe but produced no certified resubmit; two complete wider W1 trajectories
improved lower witnesses but still produced `0/18/0` and no strict profile
dominance. Every candidate, harness and deadline-policy prerequisite source
change has been reverted.

Under the presently available frozen matrices, exact counterexamples and BTC
target-host evidence, `828ea78` is the pre-human-opponent practical convergence
checkpoint and the competition-ready line. This is not a claim of a mathematical
global optimum. Reopen research only for a genuinely new diverse counterexample,
a new sound general dominance proof, a production call-graph/cap regression, or
the user-timed human-opponent window. Retuning consumed fixtures, weakening
`may_submit`, treating a route-portfolio proof as a global legal-action upper,
or revisiting caps/widths/ordering without a new causal gap is prohibited.

`SCORE-LONG-WITNESS-168` is closed rejected. Its parent was `828ea78` plus the
unpromoted 166 deadline-policy prerequisite. The frozen split is
`research/holdouts/SCORE-LONG-WITNESS-168.csv`, SHA256
`F0EBA90F9A0E00B2BB1C80BB1AC7A13809549AD26C4346DA34A3F7499D678965`.

At 5000 ms the candidate matched the frozen parent on six fresh general
fixtures, every daily exact score and all 25 action hashes; invalid/emergency
were zero and Long counters stayed zero. Fresh protected 15000-ms development
was `0/18/0`: 75 refinements, zero certified resubmits, zero invalid/emergency.
Both fixed cap-16 trajectories completed `123/123` attempts each and improved
lower witnesses 48 times, while 37 scenarios were already closed by their valid
upper, but no improvement dominated the protected certified profile. On the
first fresh BTC-like default fixture, ten refinements yielded zero resubmits and
the optional wide phase did not start before the unchanged certification
boundary. Remaining development, all sealed holdouts and BTC target-host stayed
unopened after the promotion premise failed.

This resolves the earlier completion ambiguity: trajectory construction is not
the blocker. The remaining gap is a sound global proof over legal actions.
Existing strong post-ACK search is complete only over generated route portfolios
and cannot soundly tighten the scenario valid upper used by `may_submit`.
Without a new proof representation, further Long-W1 scheduling, cap or ranking
changes would be overfitting/overengineering rather than an admissible research
branch. Evidence is in `research/evidence/SCORE-LONG-WITNESS-168.md`.

`DEADLINE-ANYTIME-167` is closed rejected as inert and accepted as protection
attribution. Its parent was
`828ea78` plus the unpromoted 166 deadline-policy prerequisite. The frozen manifest
is `research/holdouts/DEADLINE-ANYTIME-167.csv`, SHA256
`E748A798B530DDB2945BE076F94E5DAF72A42B44FAE83BD01868A7BDF52E55E5`.

The mechanism uses the existing `MatchSession` and the same stateful engine. It
first solves/submits with the unchanged 5000-ms default. If a trusted public window
has time remaining, the same session may solve the same authoritative state again
against the same pre-day ledger. Existing `may_submit` and current-score-floor
logic must reject every non-dominant refinement; same-day response accounting must
replace the prior response time, while the match ledger/traffic footprint advances
exactly once from the final accepted plan. An invalid, incomplete, non-dominant or
failed optional resend leaves the first valid plan authoritative. No second engine
or shadow solver is permitted.

The research harness first proved all 25 consumed general refinements preserved
the first action and both known Long losses; resubmits were zero. Consumed
BTC-default also produced zero resubmits across ten days. Fresh general18 attempted
75 refinements with zero resubmits and zero invalid/emergency. Attribution on
seed4300000 found two of four refinement plans differed, but all four tied the
incumbent current-day official score and none strictly dominated its certified
suffix. Fresh BTC development and all holdouts stayed sealed. The harness changes
are reverted: wiring an inert second submission pass into HTTP would be
overengineering.

The next successor was 168: it strengthened lower-witness construction without
touching the resend comparator, but complete trajectories still could not create
strict certified dominance. That successor is now closed. Reopen only from a
new sound global proof representation; never relax `may_submit`, select by
realized suffix or resurrect a dual solver.

`DEADLINE-LONG-166` is closed rejected as a standalone policy and accepted as
causal attribution. BTC rules
make response time an explicit per-match parameter; the signed-in practice UI
currently permits up to 15000 ms and archived team-vs-team configuration has used
60000 ms. The architecture and source already define one
`Emergency/Short/Normal/Long` scheduler, but the accepted canonical cap makes
Long unreachable. This is a policy/capability mismatch, not authority to reopen
unrelated tanker, role, ALNS or master experiments.

The first gate measures the existing Long path before adding any new score logic.
The default and unknown budget remains fail-closed at 5000 ms; only an explicit
trusted public match budget may exceed it, with a maximum registered class of
60000 ms, and server `endsAt` may only tighten the configured bound. Every request
at or below 5000 ms must remain byte/score/state/validator equivalent to
`828ea78`. The paired manifest was frozen before source change at
`research/holdouts/DEADLINE-LONG-166.csv`, SHA256
`BB7244DC7798963D03E7C6E1E2864B1796D6F0AFC77FD386801503A8E9AE76BF`.
Development covered the same general fixed and BTC-like default/low/high fixtures
at 5000, 15000 and 60000 ms. The 5-second candidate was byte/score/state equivalent
to parent on all six general fixtures and every daily action hash. Direct Long at
both 15 and 60 seconds was `0/4/2`: overnight fell `53 -> 50` servings and
rare-brand `38 -> 37`, with zero invalid/emergency. The existing Long policy is
therefore not monotonic and cannot replace the 5-second incumbent.

Extra time nevertheless has measured value on BTC scale. At 15 seconds the three
frozen development fixtures changed default `6/60/340 -> 6/60/346`, low fuel
`6/60/405 -> 6/60/406` and high fuel `6/60/527 -> 6/60/527`; invalid/emergency
were zero and all 30 decisions remained deadline-limited. Local timing has no
performance authority, but exact score establishes a real search-resolution gap.
The 166 holdout stayed sealed. Direct Long is closed; its only admissible successor
is protected same-engine anytime refinement.

Direct Long remains rejected and its source prerequisite is reverted. Anytime
and the wider lower-witness successor are closed until a new sound general proof
creates strict certified dominance. No parallel/shadow engine,
map/family/seed/opponent dispatcher, weighted score or realized-suffix selection
is allowed. Local time is only a falsification tool; BTC target-host at the same
public deadline is mandatory for promotion.

`CORRECT-HARD-CAP-WIRING-163` is closed accepted from the restored 162 source and the
proven 161 core prerequisite. The rejected exhaustive deadline policy is not
retained. Instead, the unchanged no-budget role algorithm will be named
`select_roles_exhaustive_oracle` and remain available only to the benchmark and
historical research harness. Generic CLI, HTTP and sandbox production callers must use
the existing `select_roles_until(kCompetitionComputeHardCap)` path. No wrapper
or second production selector is allowed. The frozen call-graph/equivalence
manifest is `research/holdouts/CORRECT-HARD-CAP-WIRING-163.csv`, SHA256
`8F8FA420C5129AE28D9FBC5B3C8CF81A8B2A2F9E9A8CF515EE76BC81378B0EC1`.
Git diff proves the exhaustive body unchanged outside its explicit oracle name;
only research benchmark/harness callers use it, while CLI/HTTP/sandbox/session
use the bounded selector. The final all-target build and unit/simulator/validator
gate passed. Parent repeated different masks/scores on the
same consumed seeds, so local oracle output lacks causal equivalence authority.

BTC `m-3573` then passed the authoritative target-host runtime gate: hard,
three bots, 10 days, 32x32, 100 steps/day, 5000 ms/day, eight agents, 12 spots,
six brands and low fuel 1x; 10/10 HTTP submissions valid, 9/9 transitions
reconciled, validator agreement, max response 2556 ms and max solver decision
time 2447 ms. Replay SHA256 is
`7878AB5DC19B401F59D97DE077AA1C997A6E213C455ACF3092628427E5535ACA`.
The live result `6/59/296` missed one brand on day 10. `ATTR-BTC-SELECTION-165`
corrected the first diagnostic run, which omitted the production current-score
floor. With the floor enabled, current reached `6/60/266` and frozen parent
`cf7e4b4` reached `6/60/267`; both retained `6/60` through day 9 and the final
one-serving difference is local timed-search noise. The replay-preferred day-1
plan already existed as a certified live challenger, but tied the selected plan
on lower/upper bounds and had q50 lower by one serving. It therefore did not
dominate before the suffix was known; policy changes based on the realized
suffix would overfit. `ATTR-TANKER-STATE-DOMINANCE-164` independently closed the
known exact-witness insertion: exact and parent road footprints are both empty,
but their terminal positions/fuel differ, so no same-state monotonic certificate
exists without a forbidden dual suffix evaluator. No source candidate remains
from either attribution. 161/163 is accepted and promoted in the canonical
commit titled `Enforce canonical competition compute cap`.

`CORRECT-HARD-CAP-ROLE-162` is closed rejected. The core 161 cap passed unit and
explicit-budget semantic gates, but bounding the legacy no-budget
`select_roles()` API is a real role-policy change: even a hard-stop-only
deadline changed consumed general seed `100002` from mask 8 and `5/20/31` to
mask 2 and `5/20/35`. The first proportional-deadline implementation was
candidate-vs-parent `3/2/1` on the consumed six-family development set, all at
tier 3, gain/loss `8/1`, invalid/emergency zero. This is development evidence,
not promotion authority.

The mechanism retains the existing exhaustive algorithm and all masks/stages,
but partitions one canonical 5000-ms deadline across its existing seed and
full-horizon passes. The frozen general and BTC-like low/default/high exhaustive
holdout plus fixed/deadline controls is
`research/holdouts/CORRECT-HARD-CAP-ROLE-162.csv`, SHA256
`8868A31FD90EE8D5D548C49DADDC03CCABB6AB497B89778D85216FAC88391288`.
The sealed general/exhaustive block was candidate `2/28/6`, gain/loss `2/12`,
with two tier-2 losses and fuel-tight `0/3/3`; invalid/emergency were zero.
Remaining blocks stayed unopened. The exhaustive algorithm must be restored
unchanged and explicitly kept as a research oracle. Production callers must
use the existing bounded `select_roles_until` path. Local elapsed is excluded.

`CORRECT-HARD-CAP-161` is closed rejected-superseded as a complete candidate
but retained as the core prerequisite to 162. Runtime
tracing proved the documented 5000-ms rule is not enforced at the canonical
engine boundary: `--response-ms`, `solve_day`, `solve_day_until` and
`select_roles_until` accept an oversized caller budget unchanged, while
post-ACK precompute/proof can consume unlimited repeated idle slices. Thus a
60000-ms server lifecycle window can silently become extra search time.

The registered repair defines one canonical 5000-ms competition compute cap,
clamps solver and role selection at the engine boundary, derives BTC adapter
and action deadlines from the same effective budget, and accounts cumulative
post-ACK search against the unused part of that day's cap. It removes or
disables no designed component; requests at or below 5000 ms must remain
byte/score/state/validator equivalent, and oversized calls may differ only by
enforcing the pre-existing contract. The frozen manifest is
`research/holdouts/CORRECT-HARD-CAP-161.csv`, SHA256
`380D32C32768FC71A5204DB4CF3340CE6631F502FEB030AF0F4FF7E988B12311`.
Local elapsed has no performance authority; BTC target-host remains the final
hard-cap/performance gate.

`SCORE-TANKER-PARENT-TIE-PROTECTION-160` is closed rejected before fresh
development. It retained 159's empty-footprint certificate, snapshotted the
canonical portfolio before the extension, solved parent first under the same
absolute deadline and wired tie protection through master and production
comparators. On consumed complete-search seed `3760032`, direct provenance left
`5/20/41`; the parent snapshot plus end-to-end flags improved it to `5/20/45`,
still below frozen parent `5/20/47`. The remaining divergence is whole-pipeline
budget/order and W1 profile state, not one missing comparator. Exact equivalence
would require running the complete decision pipeline twice or a shadow solver,
violating single-path/non-overengineering and the same 5000 ms cap. No fresh
160 row or holdout opened. Runtime source was restored byte-identical to
`cf7e4b4`. Frozen manifest SHA256 is
`0DC2CF8314D224CB52AF232E03039DF12324C19CD3A47EC2B01B4FDA89F38E22`;
protected matrix SHA256 is
`4DE14CD78844356901062E2DA7CCF030645B71BF4D14525A4B7813CA7095EA72`.

`SCORE-TANKER-TRAFFIC-NEUTRAL-159` is closed rejected. It keeps
158's canonical caravan but requires every new multi-waypoint tanker provider
to have an exact empty `fullFootprint` before admission. Existing parent road
rendezvous, escort and tanker columns remain active; only the speculative
extension must prove traffic neutrality. This is a per-route certificate, not
a map/seed/family/fuel/role dispatcher.

Consumed general seed `300030` gives causal attribution: 158 tied day 1, gained
one serving day 2, then lost a daily brand day 4 after changing the road
footprint, ending `6/23/36` versus parent `6/24/38` with full search. Fresh
manifests are `research/holdouts/SCORE-TANKER-TRAFFIC-NEUTRAL-159.csv`, SHA256
`8B9F8EB2232ECD89EF7A8AEE16E28FA28B274D903327F16548F719D65080F2FA`,
and `research/holdouts/SCORE-TANKER-TRAFFIC-NEUTRAL-159-MATRIX.csv`, SHA256
`5EF08D907E76DD17E3A234D05B8EF49BD2922A7DF6065B27C1D01B1866F70F82`.
The consumed anchor must retain `3/12/16`; then fresh tanker and general
development must pass before any sealed holdout opens. Both development gates
passed: tanker `2/10/0`, gain/loss `4/0`, and road-containing fixed
`3/9/0`, gain/loss `7/0` (one tier-2 `+2`, two tier-3 `+1/+4`), with zero
invalid/emergency/deadline. Tanker holdout was `6/30/0`, gain/loss `11/0`,
invalid `0`. Road-containing fixed holdout rejected 159 at `6/25/5`, all tier 3,
gain/loss `18/11`; balanced was systematically `0/3/3`, loss `9`. Consumed
seed `3760032` reproduced with both searches complete: the extension changed a
day-1 score tie, then lost four servings day 3 and two day 4. Remaining lanes
were not opened. Reopen only through the parent-tie invariant now isolated in
160, never by family routing.

`SCORE-TANKER-CANONICAL-CARAVAN-158` is closed rejected. It
combines the already-attributed bounded provider construction from 152--156
with 157's canonical timestamped refuel representation: enumerate stable
permutations of at most four public-priority spot endpoints for each tanker,
materialize every dual-exact feasible multi-waypoint route as an independent
`RouteColumn`, then generate patrol join/follow/detach columns whose required
events are covered by that provider. The unchanged master chooses across these
joint columns; there is no atomic whole-plan candidate or separate comparator.

All parent columns remain after unchanged pruning and are not re-pruned. The
provider endpoint bound is inherited unchanged from the previously attributed
construction, not tuned on 157. Generation is bounded by existing
`maximumEscorts`; exact simulator, independent validator and master certification
remain final. Frozen manifest and SHA256:
`research/holdouts/SCORE-TANKER-CANONICAL-CARAVAN-158.csv`,
`F9FC8A1962CCE1B6C7F5408273087F7C91418B08F9D7F4BC2A50B78982E98473`.
The consumed 156 anchor is activation-only; 12 fresh development rows and the
disjoint sealed 36-row holdout were initially unopened. Anchor activation passed:
final `3/12/15 -> 3/12/16` and day 1 `3/3/3 -> 3/3/4`, with the chosen plan
coming through normal certified route/master selection. Fresh development is
`3/9/0`, invalid `0`. The gains are rendezvous-chain/low `3/12/15 -> 16`,
stock-cycle/low `2/8/18 -> 20`, and rare-return/low `2/8/20 -> 24`; each
candidate result equals the public sum-of-stock ceiling across four days.
Holdout remains sealed until the recorded parent/candidate pair is ready.

The holdout was opened exactly once with frozen parent/candidate binaries on the
same registered rows. Candidate versus parent is `8/28/0`, invalid `0`; every
difference is tier 3, gains are `+1..+4`, total first-tier gain `15`, loss `0`.
Wins span rendezvous-chain, split-duty, stock-cycle and rare-return at low fuel;
default/high rows tie. Stock-cycle winners reach `20` and the strongest
rare-return winner reaches `24`, their public stock ceilings. Protected fixed,
native/exhaustive and deadline-role lanes are now required, including the exact
role regression fixture that rejected 156. BTC remains unopened.

Roadless protection was strongly positive: fixed `98/106/12` with gain/loss
`319/18`, exhaustive `27/41/4` with `96/5`, and deadline-role `32/38/2` with
`90/2`; invalid/emergency zero and every family net positive. Full
road-containing fixed rejected 158 at `23/68/29`, gain/loss `61/70`; four of
six families were net negative, including high-stock and overnight.
Road-containing native/deadline lanes were not opened after the blocker. Do not
rescue 158 with a map dispatcher or accept this as a bounded global trade-off.

`SCORE-TANKER-SHARED-REFUEL-COLUMNS-157` is closed rejected before fresh
development. Its exact shared-refuel columns activated, but each patrol reached
at most one serving. A read-only no-deadline master audit enumerated 278
combinations and still had best day-1 `3/3/3`. Eligible independent tanker
providers were only one route leg plus waiting (`2.-14`, `0.-14`, `5.-14`,
`WAIT(16)`), so no multi-waypoint timeline existed for the working event
constraints to share. Unit tests passed; no fresh row or holdout row opened.
Do not reopen 157 by changing provider count, join boundary or detach ranking.

`SCORE-TANKER-CARAVAN-OVERNIGHT-156` is closed rejected. It
reuses the exact day-1-closing preseed from 155 and adds only the already-designed
explicit overnight harvest: if the tanker starts a nonterminal day on a spot,
every convoy member performs `WAIT(1)` before the unchanged route/join logic.
No endpoint, order, cap, join, guard or comparator changes. Outer parent/main
candidates stay intact and replacement still requires a strict official gain
behind the roadless/all-brand/all-stock/full-fuel/common-terminal gate.

Anchor `3200100` must reach exact final `3/12/16` before 12 fresh development
rows open. The sealed 36-case holdout is
`research/holdouts/SCORE-TANKER-CARAVAN-OVERNIGHT-156.csv`, SHA256
`9A6D838736DCE43142A95FB89A019FE0A15E117569B9CD96C9277FC4EB08B436`.

The anchor passed at `3/12/16`. Fresh paired development is `3/9/0` with
invalid `0`: tier-3 gains occur independently on split-duty/low (`+1`),
stock-cycle/low (`+1`) and rare-return/low (`+2`); the other nine rows tie.
Fresh split-duty seed `3470100` completed the exhaustive oracle and tied it at
`3/12/16`. The one permitted exhaustive runs for stock-cycle `3470200` and
rare-return `3470300` did not complete within 15 minutes, so their optimality
is inconclusive rather than pass or fail; both candidate results are dual-valid.
The holdout remains sealed until a paired parent/candidate execution is ready.

The holdout was then opened once using separately built parent and candidate
executables through the identical adapter. Candidate versus parent was
`7/29/0`, invalid `0`; all seven differences were tier-3 gains of `+1..+2` and
spanned rendezvous-chain, stock-cycle and rare-return at low fuel. Every
default/high row tied, as did the remaining low rows. No tier-1/tier-2 change or
losing tail occurred. Exact/absolute-bound validation of changed rows and the
protected matrix are still required before promotion.

Absolute-stock audit proves five of the seven winning holdout rows already
reach the public per-day servings ceiling. Stock-cycle seeds `3500200` and
`3500202` end at `2/8/19` where the absolute ceiling is `2/8/20`; both miss one
serving on day 1. A read-only attribution is now registered on only these two
already-open rows: expose the existing complete uncapped day enumerator, select
the exact lexicographic day-1 optimum, reconstruct it and dual-validate it. No
state cap, dominance change, production change, fresh seed, or tuning of 156 is
allowed. If exact reaches five servings, whole-architecture convergence remains
open even if 156 itself later passes promotion gates.

Both exact day-1 runs completed and reached `2/2/5` against candidate `2/2/4`,
with dual-valid reconstructed witnesses. The witnesses are identical at the
structural level and require patrol-specific loiter/detach around a common
tanker trajectory; simple common-suffix convoying leaves one stock unclaimed.
Therefore 156 may still qualify as a monotonic checkpoint after protected/BTC
gates, but it cannot be the convergence checkpoint. Seeds `3500200` and
`3500202` are consumed attribution and may never be used to tune a successor.

The separate-binary protected screen at the `5000 ms` logical cap passed.
General fixed and deadline each tied `0/6/0`. An initial exhaustive-role
tier-3 loss and a tied role-mask change disappeared under parent-first reversal:
both candidate rows returned to parent mask `8`, score, combinations and exact
counters, making the causal exhaustive result `0/6/0`. BTC-like default, low
and high each tied with identical exact settled counters; invalid/emergency were
zero. Local elapsed and deadline-limited BTC-like score timing are excluded.
The full protected matrix is the next gate; BTC target-host remains required.

Full road-containing general protection is causally `0/120/0` fixed,
`0/60/0` exhaustive and `0/60/0` deadline, invalid/emergency zero. All apparent
native/deadline differences were role-cutoff changes and vanished on reverse or
fixed-mask attribution. BTC-like full local first passes varied in both
directions and crossed under reverse order with settled-state changes; because
the new roadless-only generator cannot execute there, these results are local
deadline noise and have no performance authority.

The protected matrix lacked the actual activation domain. A fresh roadless
extension is frozen at
`research/holdouts/SCORE-TANKER-CARAVAN-OVERNIGHT-156-ROADLESS.csv`, SHA256
`0F67E8414EAE75CD13F34E1BB5513537960174E838BEE045EE9CCEFAD22BB7DE`:
fixed one-tanker 120 fixtures, exhaustive 60, disjoint deadline 60, six families
at 5000 ms. No post-open tuning is permitted.

Because that first extension has 4 agents and 6--8 spots while the candidate
admits only all-stock plans from at most four endpoints, it protects downside
but cannot measure activation breadth. A separately frozen scale lane is at
`research/holdouts/SCORE-TANKER-CARAVAN-OVERNIGHT-156-SCALE.csv`, SHA256
`79CD3F0F0158AB0EE39105637F0C5A285A1BA776703CA4320DAC884A7832A400`.
It uses fresh generated 8x8 roadless maps with three agents, six families,
3/4/5/6 spots, fuel 2/8/16 and 4/5-day horizons: 216 fixed two-patrol-one-tanker
cases plus 72 exhaustive and 72 deadline cases on disjoint seeds. The adapter
only constructs the preregistered fixture strata. The opened lane can only
accept or reject 156 and may not tune its endpoint cap, guard or routing.

That scale gate closed 156 as rejected. Fixed-role result was `2/214/0`, with
two reproducible tier-2 gains across fuel-tight/4-spot and
threshold-corridor/3-spot low-fuel fixtures. Exhaustive role was `0/71/1`:
fresh balanced seed `3541032` reproducibly changed mask `2 -> 4` and regressed
`3/8/8 -> 3/7/8`. Fixed-mask attribution tied candidate and parent exactly for
both masks, localizing the failure to evaluator selection. The independent
whole-plan preseed is overvalued by the reduced role rollout although the
production planner gains nothing for that chosen mask. Do not add a context or
budget gate: that would disable capability for one caller. The only admissible
successor is a canonical route/master join-loiter-detach representation shared
by role rollout and production. Production source must return to `cf7e4b4`
before that experiment opens.

`SCORE-TANKER-CARAVAN-PRESEED-155` is closed rejected and production source is
restored byte-identically to `cf7e4b4`. Accepted
attribution 154 proves a compact join-and-follow convoy reaches exact day-1
`3/4`. Candidate 155 pre-generates a fixed-operation atomic convoy set before
the independent static search: stable permutations of at most four highest
official-priority spot endpoints, capped at 24; patrols join at the earliest
fuel-feasible action boundary and copy the tanker suffix. Every plan is evaluated
by the existing exact simulator and independent validator.

The outer production incumbent and main route/master candidates remain intact,
so this lane is additive. Independent replacement requires a strict official
gain plus the roadless, all-brand/all-stock, full-patrol-fuel and common-terminal
guard. Terminal behavior is unchanged. Anchor `3200100` must close before the
12 fresh development rows open; the 36-case holdout stays sealed at
`research/holdouts/SCORE-TANKER-CARAVAN-PRESEED-155.csv`, SHA256
`81F341E7336CF6144C4EC795352D148868BCC75C2F98EBAD9C4140910FDE1DC9`.
The unit suite passed and day 1 closed exactly at `3/3/4`, proving preseed wiring
works. Final score nevertheless fell from parent `3/12/15` to `3/12/14`.
Attribution is direct: all agents ended day 1 full and co-located on spot 34;
on day 2 they left immediately, so the required start-of-day `WAIT(1)` harvest
was omitted and only three servings were collected. No fresh candidate or
holdout row opened. A successor may add only the already-designed canonical
overnight harvest wait before caravan departure; route cap/order/join/guard
tuning is forbidden.

`ATTR-TANKER-CARAVAN-CONSTRUCTION-154` is closed accepted attribution.
It uses only consumed anchor `3200100` to exact-simulate a compact public-rule
convoy: patrols join at successive action boundaries and follow a common suffix.
This determines whether rejected 153 failed because the construction itself is
insufficient or because the bounded independent lane never evaluated/admitted
the intended route before its deadline. It changes no production source and
opens no fresh row or holdout. The compact plan was dual-valid and scored
day-1 `3/4`, with all three agents at common terminal `26` and both patrols at
full fuel. Therefore common-suffix convoy construction is sufficient; 153's
failure is wiring/scheduling/ranking before exact selection, not route
expressivity. A successor must preseed a bounded atomic convoy before static
independent search, preserve every outer parent candidate, and admit only a
strict official gain through the frozen safe guard.

`SCORE-TANKER-JOINT-CARAVAN-153` is closed rejected and production source is
restored byte-identically to `cf7e4b4`. Exact 150
requires separated patrols to join one tanker trajectory at different action
boundaries before moving lockstep. Canonical master synchronization already
supports one tanker with multiple patrol columns, but source generation creates
that group only when all patrols are co-located at day start. Candidate 153 adds
one bounded atomic whole-plan primitive in the independent EventConflict lane:
at most four public start/spot tanker waypoints; each patrol takes a fuel-feasible
Pareto prefix to its earliest reachable tanker action boundary and then follows
the exact remaining tanker actions. Parent evaluation remains first and protected.

Nonterminal admission retains the roadless, all-brand/all-stock current-day,
full-patrol-fuel and common-terminal guard; terminal behavior is unchanged.
Anchor `3200100` must close before the 12 fresh development rows open. The
36-case holdout remains sealed at
`research/holdouts/SCORE-TANKER-JOINT-CARAVAN-153.csv`, SHA256
`6355744188DE9B8F2FEB5B605FAA9AC1D9F47A010B0398240416255E027C8AAE`.
The candidate compiled and passed the unit suite, but the consumed anchor stayed
`3/12/15` with day 1 `3/3/3`; no fresh candidate row or holdout row opened.
The generated joint plan was therefore insufficient. The exact witness requires
patrol-specific detach/continuation after joining the convoy, not a shared suffix
to one terminal. Do not tune route count, waypoint count or join order. Any
successor must reuse canonical route/master machinery for bounded join-and-detach
segments rather than add another standalone whole-plan generator.

`SCORE-TANKER-CARAVAN-152` is closed rejected and production source is restored
byte-identically to `cf7e4b4`. The accepted
exact gap 150 and rejected mechanism 151 prove that the missing day-1 serving
is a pre-master portfolio-coverage failure: the current two-window mobile hubs
cannot express the exact multi-patrol caravan witness even though the existing
route builder can consume repeated rendezvous windows. Candidate 152 adds only
a bounded deterministic tanker-waypoint beam over the same public start/spot
cells and `ParetoRouter`, exposing at most four consecutive windows. It keeps
the byte-equivalent static parent phase first, retains the strict roadless /
current-day absolute score / full patrol fuel / common terminal admission guard,
and leaves terminal-day `cf7e4b4` behavior unchanged.

The consumed exact seed `3200100` is an anchor only and must close before any
fresh seed is opened. Fresh development contains 12 cases crossing four
structural families and low/default/high fuel; the 36-case holdout remains
sealed. The immutable manifest is
`research/holdouts/SCORE-TANKER-CARAVAN-152.csv`, SHA256
`EB3BB794D53BE196AF63890266738DE3BD25620FAD9A659BE591ADC1ADA26E31`.
The bounded four-waypoint tanker beam compiled and passed the unit suite, but
the consumed anchor remained exactly `3/12/15` and day 1 remained `3/3/3` with
the parent plan/state. No fresh development or holdout row was opened. This
proves extra tanker waypoint windows alone are insufficient: the missing
capability is a shared escort/lockstep caravan route across multiple patrols,
not tanker trajectory enumeration. A successor may open only after tracing a
bounded joint caravan primitive; waypoint count/beam width/guard tuning is
forbidden.

`SCORE-TANKER-SAFE-MOBILE-151` is closed rejected and its production source is
fully restored to `cf7e4b4`. The candidate preserved the static phase and
enabled existing mobile EventConflict only behind the roadless/current-day
absolute/full-fuel/common-terminal guard, but the consumed anchor stayed
`3/12/15` and day 1 stayed `3/3/3`; no safe mobile plan reached admission. This
proves the existing two-window hub set cannot express the exact caravan witness.
Do not reopen by increasing hub limits, weakening the guard or enabling
unrestricted mobile logic. A successor requires a genuinely bounded joint
multi-patrol caravan primitive and must justify its complexity against a single
tier-3 serving gap before being opened.

The rejected candidate had been opened from
the accepted exact gap 150. It preserves the static parent phase and permits an
additive nonterminal mobile EventConflict result only on roadless maps when its
exact current-day score reaches the absolute all-brand/all-stock bound, all
patrols finish at full fuel and every agent shares one terminal cell. This
removes the known own-traffic and fuel-state causes from rejected 143 but does
not assume terminal-position dominance; fresh exact development and a protected
general matrix are required. The consumed seed 3200100 is an anchor only.
The frozen split SHA256 is
`DD7CADA659D6A497A1969E7F9E90843E2EF8800277D021E4F914883D2241BB6F`.


`CEILING-TANKER-MATCH-150` is closed accepted-gap-no-candidate. Unlike
preflight-rejected 148/149, every public config invariant was checked before
freeze: four days, 16 steps/day within the 8x8 bounds, three agents, three
spots, distinct non-spot starts, positive fuel and roadless public-information
semantics. Its fresh 12-case development and sealed 36-case holdout are at
`research/holdouts/CEILING-TANKER-MATCH-150.csv`, SHA256
`3120848D5D5642E44467B20903694FBBBA7F89AAA168F0844F8E7251C1B0ECA4`.
The complete joint step/full-match DP remains research-only; production stays
`cf7e4b4`, the logical HEAD cap is 5000 ms/day, and local elapsed has no
performance authority.

The first development attempt, seed `3200000`, produced no case result before
the 900-second research-wrapper limit. The wrapper left oracle PID `2616`
running; that exact orphan was verified and terminated. This is an
infrastructure-inconclusive result, not an oracle/HEAD tie or loss, and the
sealed holdout remains unopened. Before another score run, only a mathematically
exact, semantics-preserving state-quotient/performance audit of this same oracle
is permitted. If none is available, close 150 as proof-infeasible rather than
shrinking the valid fixture or opening another experiment.

The permitted exact audit found two semantics-preserving quotients already
proved by the accepted multi-patrol oracle pattern: canonicalize the two
interchangeable patrols while remapping the reconstructed witness to physical
agent identities, and memoize daily transitions by `(positions, fuels)` after
asserting all four days have the same 16-step roadless/stock dynamics. Neither
quotient caps a frontier or removes an action. With them, seed `3200000`
completed in 192 seconds and tied HEAD exactly at `3/12/12`, with zero
invalid/incomplete, maximum step frontier 69608, match frontier 893 and
87733918 settled states. The second development case, `split-duty/low/3200100`,
then exposed an exact tier-3 gap: HEAD `3/12/15` versus oracle `3/12/16`, with
zero invalid/incomplete, maximum step frontier 136549, match frontier 1437,
426359927 settled states and result hash `45ada1153ec4107b`. Development stopped
immediately at exact-vs-HEAD 1/1/0; the other ten development seeds and the
sealed holdout remain unopened. Whole-architecture convergence is disproved.
The only permitted next action is read-only attribution of this consumed exact
witness before any source candidate is designed.

Consumed-witness attribution reproduced the same exact `+1` and localized it
to day 1: oracle cumulative `3/3/4` versus HEAD `3/3/3`. Both then add the full
three daily brands on days 2--4, so the one-serving deficit is merely carried
to final `3/12/15` versus `3/12/16`. The oracle day-1 plan is absent from all
16 HEAD audit candidates (`oracle_in_audit=0`) even though it is immediately
better on today's official lex score. Therefore the earliest causal gap is
joint patrol--tanker candidate generation/portfolio coverage, before master
comparison, certification or suffix valuation. The next permitted work is a
read-only trace of the responsible generator and a fixture-independent bounded
mechanism; no seed/family dispatcher or blind beam increase is admissible.

`CEILING-TANKER-MATCH-149` is closed rejected-preflight with no score result.
Official config validation rejected its three-day horizon because published
matches require 4--10 days. A direct invariant audit also found its six day
steps below the 16-step minimum for an 8x8 map. No fixture reached HEAD or
oracle; its holdout remains unopened. Frozen 149 must not be edited or rerun.

`CEILING-TANKER-MATCH-148` is closed rejected-preflight with no score result.
After the schema-only correction, official config validation proved a 2x2
component cannot contain three distinct non-spot starts plus three spots. No
fixture reached HEAD or oracle. The domain will not be weakened by overlapping
starts; a separate 2x3 experiment is required.

Its fresh
12-case development and sealed 36-case holdout were frozen before oracle source
at `research/holdouts/CEILING-TANKER-MATCH-148.csv`, SHA256
`31AAECC56CB45DCDD06DB0E29B4E6A4C3F53FDCF0E417F962DBC5F5CFB9AA9F6`.
The first loader preflight rejected two rare-return/low rows containing one
extra CSV column before any fixture or score ran. Removing only that extra
column preserved every registered field; the hash above is the authoritative
corrected manifest hash.
It targeted the independent missing domain between terminal exactness and the
rejected nonterminal heuristic: a complete three-day roadless joint step DP for
two patrols plus one tanker. Position, fuel, terminal refuel, lifetime brands
and official accumulated score carry across days; only daily stock/visited
state resets. Production remains unchanged `cf7e4b4`, 5000 ms logical HEAD cap,
and local elapsed has no performance authority. Its holdout remains unopened.

`CEILING-TANKER-DURABLE-147` is closed accepted-ceiling. Its fresh
18-case development and sealed 54-case holdout were frozen before the research
adapter allow-list change at
`research/holdouts/CEILING-TANKER-DURABLE-147.csv`, SHA256
`552554C7C69F41EA5172E41A5A79F1D22FBCFFB90DF570BE18D8D6E50CF16E6C`.
It reuses the unchanged complete terminal-tanker DP and families but uses new
seeds. Development completed 18/18 exact ties and the holdout opened exactly
once completed 54/54 exact ties across all six families and all three fuel
strata, with zero HEAD win, incomplete or invalid. Holdout result hash is
`3a958862e744b7a6`. Combined evidence is 72/72 exact ties. Production source
remains checkpoint `cf7e4b4`; local elapsed has no performance authority. This
terminal tanker domain may reopen only from an independently generalized exact
domain or fresh dual-valid exact counterexample, never by retuning 147.

`CEILING-TANKER-POSTCHECKPOINT-146` is closed infrastructure-inconclusive. Its
one-time 54-case holdout process emitted no incremental stdout, exceeded the
900-second wrapper and remained as an orphan child with no observable result.
The verified oracle PID was terminated. No score, validity or completion
verdict exists, and the consumed 138 holdout must not be rerun. Its failure is
an evidence-durability defect, not a solver or oracle result. A replacement
sweep requires a fresh frozen split and one-case-per-process execution.

`SCORE-TANKER-TERMINAL-ONLY-144` is accepted in the checkpoint commit that
contains this recorded state.
Its fresh 18-case development and sealed 54-case holdout were frozen before
source at `research/holdouts/SCORE-TANKER-TERMINAL-ONLY-144.csv`, SHA256
`ED0860B586E447C9CC3E9DB8BCFC261974F694870864308973AAB3765A0F7398`.
It keeps days before terminal byte-equivalent to parent and enables the bounded
post-parent mobile EventConflict phase only when
`state.dayNumber == config.day_count()`. With a fixed role assignment there is
no suffix after that decision, so terminal lexicographic dominance is whole-match
dominance. Role selection remains an independent acceptance gate.

Its fixed-role fresh development scored `4/14/0`, then its sealed holdout was
opened once and scored `12/42/0`, invalid zero; all twelve gains were tier-3
`+1..+3` in the same four low-fuel families independently identified by the
exact oracle. `SCORE-TANKER-TERMINAL-ROLE-145` is now the active same-binary
role-layer gate. Its fresh/holdout manifest was frozen before score at
`research/holdouts/SCORE-TANKER-TERMINAL-ROLE-145.csv`, SHA256
`F26CECA60375253BB1F787E210FCA7F6B2D42E752A6AD34A81BEE917D29B6C96`.
Fresh deadline-role was `0/6/0`; sealed deadline-role `1/11/0`; sealed
exhaustive-role `0/12/0`, all invalid/emergency zero. BTC-scale local initially
showed a low-fuel tier-3 loss of one under the same mask, but the single
preregistered reversed-order repeat crossed to a candidate gain of four. Every
day was deadline-limited and action hashes diverged before terminal mobile logic
could execute, proving local timed-search noise. The role gate is accepted
locally; BTC 5000-ms target-host telemetry is now the only performance authority.

BTC gate `m-2159` passed on the explicit hard/3-bot/10-day/32x32/100-step/
5000-ms/8-agent/12-spot/6-brand/low-fuel configuration. Assignment and all ten
day actions were accepted and exact-valid; emergency zero; maximum solver total
was `3398 ms`, maximum server response `3706 ms`. Replay-check independently
validated all ten days and reconstructed `6/60/285`. Replay SHA256 is
`AECD94B9EA033CB0642C483673CA8306BF9DA116427B97FD2C8FD477021DB790`.
Bot rank is not strength evidence; paired matrices plus the terminal no-suffix
dominance proof supply promotion evidence. From this checkpoint, run one
independent convergence sweep using it as the new parent; do not reopen
non-terminal mobile rendezvous without exact suffix protection.

`ATTR-TANKER-CAUSAL-GENERALIZATION-143` is closed rejected. The first protected deadline-role screen was `1/4/1`:
fuel-tight gained tier 2, rare-brand lost tier 3. An identical-layout runtime
switch proved both effects can be causal and that mobile day-plan improvements
can change cross-assignment role ranking. A new untouched screen spanning
deadline/fixed roles, map scale and low/default/high fuel was frozen at
`research/holdouts/ATTR-TANKER-CAUSAL-GENERALIZATION-143.csv`, SHA256
`7BE1AFB44BC7440AA150BFAC44CE158BAD7970C00B2C5CA4668C9FBE804CFE76`.
Its first same-binary block was `2/8/2`; two tier-2 losses in twelve included
one loss with unchanged role mask. The preregistered rejection condition fired,
so remaining blocks were not opened and 142 was rejected without tuning.

`SCORE-TANKER-POST-PARENT-142` is closed rejected. Its fresh
18-case development and sealed 54-case holdout were frozen before source at
`research/holdouts/SCORE-TANKER-POST-PARENT-142.csv`, SHA256
`6E2D858D719DAABF128357FF0EDCA03642AC26C43236F984489CB2F331F32DBB`.
It leaves `hub_plans`, backward and MCTS static-only. The exact parent
EventConflict phase runs unchanged first; only if it returns before deadline
does a second mobile-only EventConflict phase start from the protected parent
incumbent. Attribution proved EventConflict alone reaches exact on 138.

`SCORE-TANKER-PARENT-FIRST-141` is closed rejected-proof-insufficient. Its fresh
18-case development and sealed 54-case holdout were frozen before source at
`research/holdouts/SCORE-TANKER-PARENT-FIRST-141.csv`, SHA256
`EF5CC5F838CAB3215DF81B07094D2DFFF17D3601913330258B86437B5B95BCD5`.
It scored fresh `4/14/0`, holdout `12/42/0`, and changed protected seed 300005
from a 140 loss to a tier-3 gain. However, mobile hubs still changed MacroMcts
root cardinality and RNG sampling. Since EventConflict alone reached the exact
138 witness, 141 was rejected rather than relying on empirical green scores
without a genuine parent-computation proof.

`SCORE-TANKER-MOBILE-WINDOWS-140` is closed rejected. Its fresh
18-case development and sealed 54-case holdout were frozen before source at
`research/holdouts/SCORE-TANKER-MOBILE-WINDOWS-140.csv`, SHA256
`859D233298AFAB22A7AD58BB3FA9E4F69C921F72E258054730A5AFE0EDDE53B3`.
The candidate completes designed multi-window `HubPlan` wiring inside the
existing independent planner: add bounded two-window mobile tanker plans while
retaining every static hub, and make rendezvous waiting match official
consecutive-step refuel. No terminal solver or seed/map routing is allowed.
Fresh development was candidate-vs-parent `4/14/0`; sealed holdout was opened
once and returned `12/42/0`, zero invalid. Every gain was tier 3 in the same
four low-fuel families, while every default/high and other low case tied. The
candidate then failed protected fixed-role screening at `3/2/1`: seed 300005
lost tier 2 from `7/35/51` to `7/33/54`. The implementation had jointly
resorted all static/mobile hubs and globally changed static refuel timing, so
it did not preserve the parent search prefix. It was rejected rather than
calling this an acceptable trade-off.

`ATTR-TANKER-TERMINAL-LOSS-139` is closed accepted attribution. On consumed
seed 2800000 the production-width portfolio had widths `21|14|21` and exact
route mask `000`; unchanged master search completed at HEAD score. Injecting
the exact atomic bundle made the same complete master retain and select exact
`+2`. The independent portfolio completed 1600 rollouts without deadline but
scored below HEAD because every `HubPlan` contains one static window. Thus the
earliest loss is repeated mobile rendezvous generation, not master selection.

`CEILING-TANKER-TERMINAL-138` is closed `accepted-gap-no-candidate`. Its
canonical connected-3x3 development sweep was exact-vs-HEAD `4/14/0`, invalid
0, incomplete 0. Low fuel was `4/2/0`: rendezvous, stock-contention,
rare-brand and delayed-claim each gained tier-3 `+2`; default and high fuel were
each `0/6/0`. Exact witnesses agreed between simulator and independent
validator including trace, claims and final state. An earlier eight-cell
corridor diagnostic was discarded before attribution because it violated the
preregistered 3x3 geometry. The frozen 54-case holdout remains sealed until a
fixture-independent candidate exists.

`ATTR-BOTTLENECK-ADDITIVE-ENDPOINTS-137` is closed rejected on consumed seed 2700400.
136 retained the exact state class for agent 1 but omitted agent 0, whose exact
route is already in the ordinary width16 portfolio. 137 therefore preserves
the complete non-exact parent width16 portfolio and adds the unchanged two
public endpoints per terminal, deduplicated, before one unchanged master solve.
The additive union reached route mask `111` at widths `25|24|2`, but the
unchanged master retained neither exact nor equivalent outcome among 32
candidates after 498 nodes. No parent route was evicted. This closes the
terminal-endpoint axis; no third endpoint, cap increase or supplemental solver
may be derived from 135.

`ATTR-BOTTLENECK-TERMINAL-ENDPOINTS-136` is a closed consumed-witness
capability attribution under 135. Seed 2700400 day 1 proves the exact team plan
needs two missing independent direct three-serving routes; adding only those
routes lets the unchanged master retain exact plan/outcome. Before any planner
change, 136 tests the fixture-independent union of two public endpoints per
served terminal (official current contribution first and remaining fuel first)
from the research-wide generated set. It is bounded by two times public spot
count per agent. Passing requires route mask `111` and unchanged-master exact or
equivalent outcome; it still does not prove normal width16 constructs the routes.
The result was route mask `011`, widths `14|13|1`, one retained exact state-class
match for agents 1 and 2 but zero for agent 0, and no exact/equivalent master
outcome. 136 is rejected and its two endpoint definitions are frozen; no third
endpoint or rank tuning is allowed.

`CEILING-BOTTLENECK-PATROL-135` is closed as an accepted exact gap with no
admissible candidate; its holdout remains sealed.
Production remains source-clean at `5dccb0f`; 135 changes only the research
oracle adapter. Its manifest was frozen before the adapter change at
`research/holdouts/CEILING-BOTTLENECK-PATROL-135.csv`, SHA256
`8C262075B9CFEDE8C4270F024C7D50EE93047393EE92C2CF668CC7DB810BFF31`.
It contains 18 development and 54 sealed holdout fixtures across six new
two-lobe/one-bottleneck structural families and low/default/high fuel. The
accepted exact resource-dominance quotient from 129 remains unchanged. The
development sweep completed exact-vs-HEAD `6/12/0`, invalid 0: two tier-2 `+1`
wins at low fuel and four tier-3 `+1` wins across low/default fuel. Result hash
is `e84fb672427b32bd`. The strongest counterexample is fuel-bottleneck/low seed
`2700400`, exact `6/24/24` versus HEAD `6/23/24`. Attribution must determine
whether its earliest loss is a bounded existing stage different from the closed
traffic/ladder causes. No production source candidate may open before that
causal proof; holdout stays sealed and local elapsed has no performance authority.

`CUMULATIVE-LINEAGE-134` is closed `inconclusive-partial`. Four
accepted commits follow `7ef3694` linearly, but bounded trade-off acceptance is
not transitive, so parent-by-parent evidence alone cannot exclude `HEAD <
7ef3694`. The fresh direct A/C manifest was frozen before either binary ran at
`research/holdouts/CUMULATIVE-LINEAGE-134.csv`, SHA256
`3AD728B698D9A207DC30D7C3649700854D24FB7F75C54E2138A3D36579FB6664`.
It covers balanced six-family 8x8 road and roadless maps plus 32x32 low,
default and high fuel, each in fixed all-patrol and exhaustive-role lanes at
the internal `5000 ms` cap. Only official score/validity has local authority;
elapsed/throughput does not. Differences must be rerun in reverse order, and
the manifest may not be tuned after opening. The completed 30 fixtures produced
HEAD-versus-`7ef3694` `6/19/5`, zero invalid/emergency. The 24 8x8 rows were
`5/14/5`, with tier-2 gain/loss `2/4` and tier-3 gain/loss `10/2`; six
32x32 low-fuel fixed rows were `1/5/0` with one tier-3 `+2` gain. Low-fuel
native was interrupted and discarded; default/high lanes were never opened.
This is a mixed bounded trade-off, not proof that HEAD globally loses or
dominates the ancestor. No source candidate follows, and no remaining local
lane has enough information value to justify continued execution now.

The first opened lane, fresh 8x8 road-containing fixed all-patrol, was
`HEAD` versus `7ef3694 = 0/4/2`, invalid/emergency `0/0`. Threshold-corridor
seed `2601002` reproduced `6/19/30` versus `6/20/30` (tier-2 `-1`) and
high-stock seed `2601004` reproduced `5/16/37` versus `5/18/37` (tier-2
`-2`) in reverse execution order. Neither reproduced difference hit a search
deadline; this is a real logic regression, not local performance noise. The
remaining frozen lanes stay unopened while the exact introducing commit is
attributed across the existing linear checkpoints. These consumed seeds may
only diagnose the regression; no mechanism may be tuned on them.

`OPPONENT-BTC-REAL-133` is closed `inconclusive-user-timed`. A canonical
`5dccb0f` BTC executable was rebuilt from source-clean production at SHA256
`6838A95B3AA9C015930C631201B7506463E68223583AF4C70DBB8EB439394096`, the
authenticated queue was entered once, and no match appeared before the UI
returned to the idle `Find opponent` state. No solver connection, score or
performance sample exists. Per user instruction, fresh human-opponent testing
is deferred until the user announces the appropriate time; do not reopen or
enter another queue before that instruction.

There is currently no open production score candidate or attribution; canonical
production remains `5dccb0f`. `ATTR-LADDER-CURRENT-EXACT-132` closed negative:
existing complete current-day fuel-exact enumeration finished 3/3 agents but
reached only oracle route mask `011`, and unchanged normal master retained no
exact/equivalent outcome. The ladder axis has no bounded candidate: flags alone
fail, while full frontier plus a second upper-aware solver is overengineered and
forbidden. Reopen only from an independent bounded representation or a fresh
counterexample with a different earliest cause.

`ATTR-LADDER-RESOURCE-FRONTIER-131` closed negative. The complete day-2
frontiers were 55/44 routes, with up to 11/7 routes at one terminal and 95 novel
routes total. Exact outcome remained absent under W1, normal32/8 and every cap
through256. Full frontier plus a new upper-aware master would duplicate search
and is not an admissible candidate from this witness.

`ATTR-LADDER-EARLIEST-LOSS-130` closed the exact gap at route representation.
Day 1 exact routes had mask `001` in every ordinary portfolio through wide64;
only oracle-seeded augmentation restored mask `111`. Current accepted 114 is
still insufficient: day-2 exact routes end at different terminals, the missing
agent-0 route is rank2 by both fuel and current score but rank1 by joint upper,
and complete fuel-exact W1 reaches only mask `011`. Do not tune one terminal
rank, flags, global caps, F0 or comparator from this witness.

`PERF-ORACLE-RESOURCE-DOMINANCE-129` is accepted research infrastructure. Its
pre-ladder gate preserved 085, 089 and 095 each at 18/18
exact oracle scores, for 54/54 total with every reconstructed result valid. The
four recorded 095 exact wins matched directly; its fourteen oracle=HEAD ties
matched a detached `f77c101` head-only reference on the exact 095 fixture
adapter. Its one authorized ladder run produced the exact gap above; all other
101 development and holdout rows remain sealed.

`ATTR-TRAFFIC-INFORMATION-128` closed the exact Markov traffic
gap as non-actionable under the current public information set. On consumed
seed `2300200` day 1, exact and parent profiles each won only one 5000-weight
scenario and neither certified-dominated the other. The exact match DP obtained
its realized `+7` tier-2 result while reading the fixture's frozen future opponent
footprints; production observes only current public state/history/belief. Traffic
score research may reopen only from independent public predictive evidence or a
candidate that robustly improves the unchanged public scenario profile.

`ATTR-TRAFFIC-F0-NO-GAIN-127` closed the sole reproducible
plan-hash change from rejected candidate 126. Candidate and source-frozen parent
had identical current/lower/upper envelopes on all four days and both ended at
`6/24/24`; only day-2 plan identity/disposition changed (candidate W1 incumbent
versus parent upside challenger). No downstream stage suppressed a score gain.
The road F0 membership/order/subwindow axis is closed unless independent evidence
shows a retained challenger with different certified value or exact final score.

Governance reconciliation found and closed two stale `active` labels without any
source or evidence change. `CEILING-MATCH-071` was resolved by the accepted
072+073 successor, protected as 083/084 and committed in `00711ba`.
`SCORE-QUEUE-048` was integrated through accepted `FINAL-QUEUE-065` and committed
in `7ef3694`. Neither predecessor is an open research axis at current HEAD.

`SCORE-TRAFFIC-F0-UPPER-126` is
closed as rejected and all candidate production/test/telemetry source is restored
to the canonical parent. Source-frozen A/B/B/A development was `0/18/0`, invalid
0 in both orders. The road-upper sweep attempted and completed on all 72 decision
days with zero fallback, proving activation; only high/traffic-stock seed2420400
changed plan reproducibly, but both scores were `6/24/24`. Holdout SHA256
`6BE6AA5E3B3A5A63B98A325562BA368E1479900D41BBFD0023460077918084FA`
remains sealed. Evidence is in
`research/evidence/SCORE-TRAFFIC-F0-UPPER-126.md`. Do not tune F0 upper order or
deadline on this split; F0 membership alone is insufficient to improve score.

`ATTR-TRAFFIC-F0-UPPER-125` is closed as accepted attribution. Of 33
source candidates on consumed seed2300200 day1, the exact oracle was outside the
12 frozen quality ids and current distance-only F0 excluded it. Ranking the same
four diversity slots by unchanged valid upper then distance retained the oracle,
kept F0 size16, and evicted zero quality ids; two diversity ids changed. Evidence
is in `research/evidence/ATTR-TRAFFIC-F0-UPPER-125.md`.

`CEILING-TRAFFIC-INDEPENDENT-124`
is closed as accepted attribution after its causal question resolved in two
distinct low-fuel development families. Traffic-duplicate seed2300100 produced
exact `5/20/22` versus HEAD `5/13/21`; threshold-loop seed2300200 produced
`6/22/22` versus `6/15/20`. Both are valid tier-2 gains of 7. On threshold-loop
day1 the exact route mask was `111`, the unchanged merged master retained exact
plan/outcome, and its candidate-specific upper `6/24/28` ranked first at
cap32/diversity8, yet it matched none of the 16 final audited candidates. This
localizes the repeated gap after master retention, not route generation. The
remaining 124 development and all holdout fixtures remain sealed; SHA256 is
`B8E5EBA30D76198628CE6DBCB09504027C56E24D15250E4994D0C7D2EA90B401`.
Evidence is in `research/evidence/CEILING-TRAFFIC-INDEPENDENT-124.md`.

`ATTR-TRAFFIC-PARETO-ENDPOINTS-
123` is closed as rejected and the terminal-sidecar axis is exhausted. Its
two-endpoint portfolio naturally reached route mask `111` at widths `10|12|1`,
but unchanged master cap32/diversity8 retained neither the exact nor equivalent
joint outcome among 32 candidates after 240 nodes. Per its frozen gate, no cap,
diversity or endpoint tuning is allowed; all capability source is restored to
canonical `5dccb0f`. A future traffic attempt requires an independent master-
aware pricing/value mechanism rather than post-hoc terminal projection.

`ATTR-TRAFFIC-TERMINAL-VALUE-122` is closed as rejected. It produced the exact
opposite collapse from 121: score-first retained patrol1's three-serving route,
but at patrol0 terminal34 replaced the oracle two-serving/fuel10 route with a
three-serving/fuel8 route under equal zero traffic. Sidecar mask became `011`;
exact/equivalent outcome remained absent. Evidence is in
`research/evidence/ATTR-TRAFFIC-TERMINAL-VALUE-122.md`.

`ATTR-TRAFFIC-STRATIFIED-CONSTRUCTION-121` is closed as rejected. It completed
the finite direct-triple loop without changing canonical columns, yet sidecar
mask remained `101` and exact/equivalent outcome remained absent. Patrol1's
terminal19 winner was a zero-traffic one-serving route at fuel11; the zero-
traffic three-serving oracle route at fuel8 lost solely because the selector
ranked fuel before official contribution. Evidence is in
`research/evidence/ATTR-TRAFFIC-STRATIFIED-CONSTRUCTION-121.md`.

`ATTR-TRAFFIC-TRIPLE-STRATA-120` is closed as accepted attribution. Its
post-prune route masks at structural widths `16/24/32/48/64` were respectively
`001/011/011/111/111`. Combined with 119's pre-prune width16 mask `101`, this
proves patrol0 is constructed then pruned, while patrol1 is not constructed at
16 but appears by 24. Patrol1's wide route is a direct three-serving triple,
not a harvest extension or exact-orienteering artifact. Evidence is in
`research/evidence/ATTR-TRAFFIC-TRIPLE-STRATA-120.md`.

`SCORE-TRAFFIC-UPPER-LANE-119` was rejected before opening its first
development score seed. Its width16 production sidecar had widths `7|7|1` but
route mask `101`, so patrol 1's required route was never generated and the
unchanged master retained neither the exact plan nor an equivalent outcome. The
earlier width64 attribution lane remained `111`, proving that 119 confused
post-generation projection with bounded route construction. Its manifest SHA256
`E5E2E7ABB40F7584A016837EEF1AAD6DE5D80832E367F86EE6E6C9085ED99F76`
remains sealed and may not be reused. Production candidate source was removed;
evidence is in `research/evidence/SCORE-TRAFFIC-UPPER-LANE-119.md`. The next
permitted action is consumed-witness attribution of bounded terminal-stratified
triple enumeration. No fresh score split may open until that mechanism exposes
all required routes without widening the canonical portfolio or reducing any
existing deadline work.

`ATTR-TRAFFIC-UPPER-LANE-118` is closed as accepted attribution. Its public
`7|7|1` sub-portfolio retained the exact oracle plan/outcome under unchanged
master cap32/diversity8 in 98 nodes; oracle upper `6/23/29` ranked fifth and lane
best was `6/24/26`. Evidence is in
`research/evidence/ATTR-TRAFFIC-UPPER-LANE-118.md`.

`ATTR-TRAFFIC-TERMINAL-PROJECTION-117` is closed as accepted attribution. Both
missing routes exist in generic width64, are independent/resource-undominated,
and are rank 1 under the public minimum-traffic terminal selector despite global
ranks 54 and 14. The full nondominated frontier is rejected as too large at
51/46 routes; the successful selector is bounded to one route per public spot per
agent. Evidence is in
`research/evidence/ATTR-TRAFFIC-TERMINAL-PROJECTION-117.md`.

`ATTR-TRAFFIC-EXACT-BUNDLE-116` is closed as accepted negative attribution. The
existing exact-resource enumerator completed 3/3 agents, 578 states, two terminal
variants and seven bundles; the unchanged master evaluated all seven. It still
produced route mask `101`: agent 1's oracle route was absent, and neither the
joint plan nor equivalent outcome appeared among 32 candidates. Evidence is in
`research/evidence/ATTR-TRAFFIC-EXACT-BUNDLE-116.md`. Nonterminal low-fuel exact
activation is therefore not a candidate.

`ATTR-TRAFFIC-MARKOV-115` is closed as accepted attribution; production remains
the accepted champion `5dccb0f`. Its mathematically exact quotient keeps physical
state, lifetime and accumulated score, `F[d-1]`, and the already-derived current
road statuses `R[d]`; reconstruction recomputes every `R[d+1]` from exact plans.
The first permitted 097 development fixture completed with dual-valid oracle
`6/22/22` against HEAD `6/14/19`, a tier-2 gain of 8 and result hash
`5d2f3c73d5d99dbf`. The first divergence is day 1: oracle accepts `5/5/5`
instead of HEAD's `6/6/11`, and its plan is absent from all 16 audited runtime
candidates. Attribution proves the exact routes and joint outcome appear through
existing wide/additive capabilities, while the unchanged candidate upper
`6/23/29` is stronger than the selected path's `6/16/21`. This is a real
generation/retention wiring gap, not comparator blindness and not authority to
raise caps globally. Evidence is in `research/evidence/ATTR-TRAFFIC-MARKOV-115.md`.
The 097 holdout remains sealed. Architecture convergence is disproved; any score
candidate now requires a fresh split and a public bounded structural projection
that preserves every existing route/candidate and the 5000-ms hard cap.

Canonical committed parent is `f77c101`, which composes the accepted deterministic
F0 intake promotion `f574d4e` with the accepted semantics-preserving master
bundle prefilter `PERF-MASTER-BUNDLE-092`. `SCORE-W1-CLOSED-LOOP-114` has now
cleared every promotion gate and is the accepted canonical successor included in
the commit containing this recorded state. All consumed
071/083/084/085/087/088/092/109/110/113/114 fixtures may not be tuned further.

`ATTR-W1-TERMINAL-RANK-108` is closed as accepted attribution. On the consumed
095 seed1600000 day-2 state, six nondominated served outcomes end at the missing
patrol's oracle terminal cell 26. The exact mask-4/fuel-6 outcome occurs once:
it is current-score rank 6/6, but unique remaining-fuel-first rank 1 and unique
conditional FastViability-upper rank 1 at `6/24/30` when the other patrol keeps
its exact route. Evidence is in
`research/evidence/ATTR-W1-TERMINAL-RANK-108.md`. This rejects an arbitrary
one-route-per-cell projection, but authorizes a fresh candidate that exposes at
most one maximum-remaining-fuel route ending on every served spot. The public
spot bound is 16 routes per agent. Existing maximal-mask routes and bundles must
remain unchanged; the new lane is additive and requires a fresh frozen split.

The active candidate is `SCORE-W1-TERMINAL-FRONTIER-109`, parent `f77c101`.
Its frozen manifest contains 18 development and 54 sealed holdout fixtures over
the six branched structural families with low/default/high fuel; new seed ranges
start at 1800000/1810000/1820000. SHA256 is
`2864E9070E03DC031469CEA1309D035ED9BAB38148EF19EAA6F18459C713CBFB`.
The candidate may only add a bounded exact capability: each complete exact
enumerator exposes at most one route ending on each public spot cell, ranked by
least fuel used before exact current contribution and deterministic resource
ties. The planner adds at most one aligned complete bundle per spot and retains
all old bundles unchanged. W1 must finish its unchanged certified baseline
first; optional work may then build one full dual-valid exact-bundle witness,
guided only by the unchanged candidate-state FastViability upper. A scenario is
replaced only by strictly better final official score; timeout, invalid, tie or
worse preserves the old witness. No cap, threshold, beam or dispatcher changes.
Development compares directly with source-frozen `f77c101` at 5000 ms; local
elapsed is not performance evidence. The source-frozen A/B/B/A development gate
is candidate-vs-parent `3/15/0`, invalid 0, reproduced exactly in both orders:
branched-duplicate/low and fuel-split/low gain one daily distinct, and
terminal-fork/low gains one serving. Day-1 audits attribute all three gains to
selection of a W1 `upside-challenger` while parent selects `floor-leader`.
Evidence is in
`research/evidence/SCORE-W1-TERMINAL-FRONTIER-109-development.md`. The immutable
54-fixture holdout opened once with no retuning and rejected the candidate at
`4/46/4`, invalid 0. Tier-2 gain/loss sums were `5/2`, tier-3 `2/2`, but the
downside was systematic: fuel-split/low was `0/1/2`, and low fuel was
net-negative by case. All four losses reproduced exactly in reverse-order
A/B/B/A, so they are causal rather than local cutoff noise. Evidence is in
`research/evidence/SCORE-W1-TERMINAL-FRONTIER-109-holdout.md`. 109 is closed;
no terminal-rank, fuel-gate or dispatcher tuning is allowed on its opened data.
BTC and protected matrices remained sealed. Production source is restored to
`f77c101`.

`ATTR-W0-CACHE-RETENTION-110` is closed as accepted negative attribution. On
18 fresh development matches, 54/54 next-day cached plans were eligible,
exact-reused and never rejected; 51 survived F0 and 14 were selected. The three
evictions were harmless: two were strictly current-and-upper dominated by the
selected candidate and one was exactly current-and-upper equivalent. Evidence
is in `research/evidence/ATTR-W0-CACHE-RETENTION-110.md`; its holdout remains
sealed and no F0 identity-protection candidate is authorized.

`ATTR-W0-CACHE-SUFFIX-111` is closed as accepted attribution. On consumed 110
development, 36/54 next-day transitions truncate a multi-day certified
witness: 18 contain three plans and 18 contain two. Every source final score is
strictly above the current score retained by cache repair. Evidence is in
`research/evidence/ATTR-W0-CACHE-SUFFIX-111.md`.

`ATTR-W0-SUFFIX-REVALIDATE-112` is closed as accepted attribution: all 36/36
roadless suffixes are dual-valid from the actual next-day state and reproduce
their source final official score exactly, mismatch 0. Evidence is in
`research/evidence/ATTR-W0-SUFFIX-REVALIDATE-112.md`.

The rejected candidate `SCORE-W0-SUFFIX-PRESERVE-113`, parent `f77c101`, used a
fresh cyclic manifest has 18 development and 54 sealed holdout fixtures over
six families and low/default/high fuel, with new seeds
2000000/2010000/2020000. SHA256 is
`429C56F8A336B969AF27AF96DABA4EC2BDA4C055038CE37C01568D598B0484C1`.
It retained and whole-suffix revalidated roadless certificates before exposing
them to the unchanged comparator. Unit tests passed and four plan hashes
changed, proving the mechanism was active, but development candidate-vs-parent
was `0/18/0`, invalid 0. Holdout and BTC stayed sealed. Evidence is in
`research/evidence/SCORE-W0-SUFFIX-PRESERVE-113.md`; all candidate source is
restored to `f77c101` and this split may not be tuned.

The accepted candidate is `SCORE-W1-CLOSED-LOOP-114`, parent `f77c101`. It is the
single final composition of two frozen, independently attributed capabilities:
109's bounded served-spot exact frontier creates the missing stronger W1
certificate, while 113's roadless whole-suffix revalidation keeps that exact
certificate authoritative across replanning. Neither rank, cap nor acceptance
rule may change. Existing maximal bundles and W1 baseline execute first; the
new witness must be complete, dual-valid and strictly better, and every cached
suffix must reproduce its official final score from the authoritative state.
No plan is forced. The fresh diamond manifest contains 18 development and 54
sealed holdout fixtures across six new families and low/default/high fuel, seeds
2100000/2110000/2120000; SHA256 is
`EF74321329127E4B34B45C74386B4EC6464EAFE3EF0EADDF8F454FEE19D46DCB`.
This is the final representation/persistence attempt: failure closes the axis
without tuning. The source-frozen development gate passed identically in both
execution orders at `10/8/0`, invalid 0. Two first differences were tier 2 with
total gain `+2`; eight were tier 3 with total gain `+13`; no fixture lost. Gains
span low/default/high fuel and four diamond families. On seed 2100000, cached
whole-suffix replays at authoritative days 2 and 3 were dual-valid and exactly
matched their source final scores (`6/17/25`, then `6/19/22`), while the unchanged
comparator remained in control. Evidence is in
`research/evidence/SCORE-W1-CLOSED-LOOP-114-development.md`. The candidate is
frozen. Its immutable holdout has now passed at `26/25/3`, invalid 0. There was
no tier-1 difference; tier-2 gain/loss was `11/1`, tier-3 gain/loss `42/2`, and
every loss was only one unit at the first differing tier. Gains span all three
fuel strata and five diamond families; no stratum is systematically losing. The
three losses reproduced in candidate-first order. The preregistered four-run
collection completed but its report parser failed after collection; with source
unchanged, a paired rerun recovered the score table and is disclosed in
`research/evidence/SCORE-W1-CLOSED-LOOP-114-holdout.md`. This clears the global
score holdout under the bounded-downside rule, not absolute dominance. Promotion
remains blocked on protected semantic/score matrices and BTC target-host hard-cap
validation; no candidate tuning is permitted.

The frozen protected matrix is now closed as passed; detailed evidence is in
`research/evidence/SCORE-W1-CLOSED-LOOP-114-protected.md`. Fixed general 120 was
`31/77/12`, with no tier-1 difference, tier-2 gain/loss `14/1` and tier-3
`43/24`; all twelve losses reproduced candidate-first and remain disclosed.
Conservative exhaustive-role 60 was `13/38/9`, tier-2 `4/1`, tier-3 `15/14`;
reverse order converted two apparent losses to ties and one to a win. Conservative
production-deadline 60 was `16/37/7`; reverse attribution removed the apparent
tier-2 loss of 3 and one tier-3 loss, leaving an order-attributed result around
`17/38/5` and tier-2 gain/loss `6/1`. All lanes had zero invalid/emergency and no
tier-1 loss. A 32x32 low/default/high screen was valid throughout; low changed
from candidate -2 servings to +1 when order reversed, so local cutoff variation
has no performance authority. Candidate source remains unchanged. The sole
remaining promotion gate is BTC target-host hard-cap/lifecycle/exact validity.
The final BTC target-host gate passed on fresh explicit advanced match `m-2120`:
hard, three bots, ten days, 32x32, 100 steps/day, 5000-ms response, eight agents,
twelve spots, six brands and high fuel 3x. Frozen executable SHA256 is
`4A285FF58BA845C699C03048CDDDAB1FA276DA8AE500FF99A12AC2DC96C33A98`; replay
SHA256 is `40B85BD5D1C448A4444A1F7EAE507C718106A34583BB0F2DEB070639C67D6A12`.
All 10 action responses were HTTP 200 and valid, replay-check accepted 10/10 with
simulator/validator agreement, 9/9 transitions reconciled, and there were zero
WAIT, skip, emergency, invalid, exact overrun or hard-cap breach. Target-host
decision p95/p99/max was `2714/2714/2714 ms`; end-to-end action response was
`2771/2771/2771 ms`. Exact frontier supported/completed 64/62 agent-days with
zero overrun and cache exact-reused on nine days. Final `6/60/415` and rank 1 are
only the asymmetric bot failure gate, not promotion evidence. The unchanged
`competitionReady=false` reflects the separate uncalibrated persistent p99 flag,
not this match's runtime; one ten-day match does not rewrite calibration. Evidence
is in `research/evidence/SCORE-W1-CLOSED-LOOP-114-btc.md`. Candidate 114 is the
accepted canonical successor on the paired holdout plus protected matrix and this
final BTC target-host gate.

The most recent closed performance axis is `PERF-MASTER-BUNDLE-092`. It is independent of
the consumed roadless score fixtures and is rooted only in frozen BTC target
telemetry. Every recorded day in `m-2029` and `m-2034` ended the master before
search completion. The bundle-aware score upper checked/pruned `45691/29` and
`221991/2` branches respectively, while the direct bundle-compatibility gate
rejected `24239` and `183322` route columns. The direct gate currently constructs
and stable-sorts those certainly incompatible columns, then rescans the selected
prefix for each rejection even though the recursion already carries
`activeBundle`. Experiment 091 will filter them before rank construction using
the exact existing compatibility relation: root accepts every column; otherwise
equal bundle ids are compatible, as are two negative ids. Filtering retains the
source sequence of all compatible columns, so the unchanged stable sort and DFS
order are identical whenever work completes. No search, bound, cap or capability
is removed. The frozen manifest is
`research/holdouts/PERF-MASTER-BUNDLE-092.csv`, SHA256
`47A01C938522D5382D83D10FCA5B3B329C7C555733273C44301C544F8395A5E6`.
Local elapsed time has no authority. Its predecessor 091 was stopped before
opening a replay holdout when the preregistered 30 `PERF-P99-055` replay files
proved absent: their hashes remain, but raw inputs do not. The source was restored
byte-identical to `f574d4e`; 092 then froze the three extant hash-authoritative BTC
replays (`m-1986`, `m-2029`, `m-2034`, 30 recorded days) before the identical patch
could be reapplied. No 091 replay result was claimed. Candidate executable SHA256
is `247C143CBB6FE09AF4479DD23A472F4A3DFF5161CF5C3EEE4B828C584871CB7D`.
The source predicate proof covered 24 valid negative/positive bundle-prefix cases
with zero mismatch and the full unit suite passed. The first local deadline screen
was `0/26/4`, actions equal `23/30`, with four tier-3-only differences and all
three replay final totals tied. A targeted A/B/B/A reproduction showed none of
the four losses followed the candidate binary: two became exact ties, one crossed
with run order, and the last varied `299..304` within both binaries. This is local
cutoff noise, not performance evidence or a causal regression. Source/binary are
now frozen. BTC target-host then passed the full preregistered matrix: low,
default and high fuel on easy 8x8/64-step and hard 32x32/100-step maps, all at
5000 ms. Across `m-2077`, `m-2078`, `m-2081`, `m-2082`, `m-2084` and `m-2085`,
all 60 decisions received HTTP 2xx, all 54 transitions reconciled, replay-check
agreed on every day, and emergency/hard-cap failures were zero. Maximum solver
time was 3383 ms; all six final scores retained lifetime 6 and daily 60. The one
`bundlePrunes` site counted 610522 incompatible columns rejected before rank/sort
on BTC, a direct mathematical work reduction independent of random-map latency.
Bot ranks and cross-map timing are ignored. `PERF-MASTER-BUNDLE-092` is accepted
as a semantics-preserving global performance improvement.

The rejected independent performance axis `PERF-MASTER-STOCK-093`, parent
`f77c101`. The six frozen 092 BTC replays contain 574509 bundle upper checks but
only 81 bundle upper prunes. Each feasible check currently scans all spots to
recompute stock capacity, although the recursion already maintains the exact
generic stock upper. For each bundle/depth, 093 will precompute only offsets where
`bundleSuffixClaims` differs from `genericSuffixClaims`, initialize from the
generic upper, and subtract the exact difference of the two capped contributions
at those offsets. This is an algebraic identity because bundle-compatible columns
are a subset of generic columns. It neither disables nor weakens the bundle bound.
showed reference/difference visits `119936/45` on one state and passed the full
unit suite, but failed the frozen 60-state deadline-equivalence gate at `6/47/7`,
action-equal `36/60`, tier-3 gain/loss sums `32/52`. A/B/B/A attribution removed
or crossed six losses; `m-2077` day 4 remained binary-causal with parent servings
`155/164` versus candidate `142/141`. The target matrix stayed sealed. All source
and telemetry fields are restored to `f77c101`; do not retune on consumed 093.
Frozen manifest SHA256 is
`DAB0096E18863B8AC753AD06EEB808FB11DB9ED5CE48558486533BD7BF715F1B`.

The rejected score axis `SCORE-MASTER-PRESERVE-094`, parent `f77c101`, used the 092
target matrix ended master search incomplete on 59/60 days. DFS already computes
`preservedServingPotential`, the exact amount of the child stock upper retained by
choosing a column, but does not use it after ties on escort, lifetime, daily and
current serving gain. 094 adds only descending preserved potential at that tie;
stable source order remains final. No branch, bound, cap or completed-search result
changes. Fresh fixed-role development and holdout seeds span all six generated
families, short horizons and BTC-like low/default/high lanes. Holdout SHA256 is
`E35CFBF193EBB3B4C7C5AA953FE18CA9F59F0651D9854A7092AA7E2DFF15CCE4`.
The fresh general development result was `0/18/0`, exact score ties with zero
invalid/emergency across all six families. The mechanism was reachable because
operation counts changed, but it created no score benefit. BTC-like development
and all holdouts stayed sealed; source is restored to `f77c101`. Do not tune more
ordering keys on consumed 094.

The accepted independent exact-gap axis is `CEILING-CYCLE-PATROL-095`, parent `f77c101`.
It does not reuse the consumed 085 line or 089 branch topology, families or seed
ranges. Its new 3x3 cyclic roadless component has two active patrols, six spots,
multiple approaches and mixed plain/mountain movement; an isolated third patrol
preserves the complete two-active-patrol proof scope. The unchanged complete
joint DP will compare exact official score against HEAD at the 5000-ms hard cap
on 18 development fixtures across six new structural families and low/default/
high fuel. Its 54-fixture holdout is sealed. Manifest SHA256 is
`C20467B8E28226710D6F310A3E24B166A8BD1582B955FD5B5E9F102BA8E4C866`.
Its complete development result is exact-vs-HEAD `4/14/0`, zero invalid, with
three tier-2 gaps and one tier-3 gap across cycle-balanced low, cycle-duplicate
low/default and fuel-circuit low. Result hash is `6c160390445423ed`; maximum
exact frontier is 42282. On seed 1600000, all exact day-1 agent routes exist in
the 16-column portfolio and its admissible full-match upper is joint-best, but
the normal 32-candidate master population with eight diversity slots discards
the exact outcome. The same unchanged master at 48 candidates and twelve
diversity slots retains it. The holdout remains sealed.

The rejected candidate `SCORE-MASTER-PORTFOLIO-096` changed only the Normal deadline
class changes master population retention from 32 to 48; the one-quarter
diversity ratio is preserved. Search combinations remain 40000, downstream F0
remains 16 candidates/24 operations, and no route generation, harvest extension,
bundle, bound, simulator, validator, ALNS, role or hard-cap logic changes. It will
first faced the four 095 development gaps, then all 18 development fixtures.

The development gate passed candidate-vs-parent `2/15/1`, zero invalid. It
improved cycle-duplicate low from `5/17/29` to `5/19/29` and fuel-circuit low
from `6/22/25` to `6/24/28`, both at tier 2. Terminal-loop low regressed only
one tier-3 serving, `5/20/30` to `5/20/29`. The two higher-tier gains outweigh
the isolated small downside under the global promotion rule, so the immutable
54-fixture 095 holdout opened exactly once. No parameter was retuned. Direct
paired holdout was `8/43/3` in both P/C orders, zero invalid, with tier-2
gain/loss `14/1` and tier-3 `0/5`.

The candidate nevertheless failed the road-containing fixed protected lane on
seeds `830000..830035`: `2/27/7`, zero invalid/emergency, tier-2 gain/loss `0/6`
and tier-3 `4/7`. Reverse-order reproduction kept seven losses, including
overnight seed 830021 at tier 2 `-1` and rare-brand seed 830029 at tier 2 `-4`.
This systematic road/traffic downside dominates the roadless gain for global
promotion. Native and BTC gates stayed unopened. The one-line production change
is fully restored to `f77c101`; do not tune cap 32/48, fuel/road guards or
diversity ratios on consumed 095/096.

The attempted independent axis `CEILING-TRAFFIC-PATROL-097`, parent
`f77c101`. The contradiction exposed by 096 cannot be resolved with another
road guard or population-cap tune: roadless future-value gains and road/traffic
losses are both causal. Existing exact multi-patrol suites omit the two-day own
traffic state, so they cannot derive a globally safe retention relation. The new
research-only complete DP uses a distinct cyclic topology with two road cells,
frozen deterministic opponent load, and carries both previous exact own
footprints in every physical state. Daily Smooth/Busy/Jammed costs follow the
official players-times-threshold rule, and every witness must agree with the
exact simulator and independent validator. The frozen 18-development/54-holdout
manifest hash is
`6027B078DA6BB80D9B613FB57210C55B10DCB012B67185BDD9C5F62F044B1263`.
No production change or holdout opening was authorized. The first exact state
representation reached about 6.8 GB private allocation. Exact compaction from
64 cells to the two actual roads, byte storage, saturation at the jam threshold
and componentwise traffic dominance reduced intermediate cost but the first
fixture still reached about 5.8 GB before producing a score. It was stopped to
protect the shared local host; development and holdout remain unopened and no
score is claimed. 097 is infrastructure-inconclusive, not evidence of convergence
or of solver quality. Reopen only with a mathematically exact quotient or a
bounded external proof host; do not simplify traffic semantics to force it.

`ATTR-MASTER-NESTED-098` is closed as accepted attribution. On overnight seed
830021, the parent-selected hash `12994778012826007410` is absent from all 16
candidate48 F0 records: the wider upstream population erased the parent witness.
On rare-brand seed 830029, parent hash `9046975828446139618` survives and is
certified, but is left `certified-not-selected` for hash
`16946503130334369313`. Both report identical current, provisional lower, valid
upper and certified lower scores on day 1; the replacement later loses tier 2.
The comparator removes strict certified dominance first, then may break an
unresolved tie by terminal slack, traffic fields and stable ID. Existing
certification therefore does not make the 48-population path additive or prove
the replacement globally better. Evidence is in
`research/evidence/ATTR-MASTER-NESTED-098.md`; temporary harness output was
removed. A successor requires a fresh split and must preserve the canonical
parent reservoir/ALNS/F0 lane, allowing supplemental takeover only under strict
certified dominance. Consumed 830021/830029 remain attribution-only.

`ATTR-MASTER-EVICTION-099` is closed as accepted attribution. On consumed 095
seed 1600000 day 1, canonical 32/8 reports
`target_evaluated=1,target_retained=0`: the exact joint plan was already evaluated
and then evicted. Cap32 quality-only retains it while 8 diversity slots do not,
identically with branch-and-bound enabled and disabled. Temporary target audit
bits were removed. Evidence is in
`research/evidence/ATTR-MASTER-EVICTION-099.md`. This proves a future successor
can observe the same evaluated stream in an independent quality reservoir while
leaving canonical 32/8 branch pruning, ALNS and baseline F0 untouched; it does
not require a second search. Supplemental takeover must still require strict
certified dominance, and a fresh split is mandatory.

`SCORE-MASTER-ADDITIVE-100`, parent `f77c101`, is rejected. Its frozen
manifest is `research/holdouts/SCORE-MASTER-ADDITIVE-100.csv`, SHA256
`AE7269ED4F73429C04128AFEABE50E105E55B743695852D05F76ADB6DAB39E4C`:
18 development and 54 sealed holdout fixtures across a new 3x4 perimeter
topology, six new families and low/default/high fuel. The additive 32/0 observer
left canonical 32/8 BnB, ALNS and F0 unchanged and permitted takeover only after
strict certified dominance. The exact full-match oracle exceeded the 120-second
probe limit before any score and was stopped without semantic reduction. Direct
source-frozen candidate-vs-parent development was `0/18/0`, invalid `0/0`; all
18 full-match action hashes were identical and strict takeover days were zero.
Holdout, protected/native and BTC lanes remained unopened. All production/test
changes were removed. Evidence is in
`research/evidence/SCORE-MASTER-ADDITIVE-100.md`. Reopen only from fresh telemetry
proving strict supplemental dominance is achievable inside the remaining
5000-ms window; do not weaken dominance or tune on consumed 095/100.

`CEILING-LADDER-PATROL-101`, parent `f77c101`, is closed as
infrastructure-inconclusive.
Its frozen manifest is
`research/holdouts/CEILING-LADDER-PATROL-101.csv`, SHA256
`B9D1D990CDBE224AA848C92175B3ACDDA94E83D2B7F73F3EDA2EE1A3F72DD4A3`:
18 development and 54 sealed holdout fixtures on a new 2x4 plain ladder with
three linked cycles, six new families and low/default/high fuel. The unchanged
complete two-active-patrol DP will compare exact full-match score with unchanged
HEAD at the logical 5000-ms budget. The first development fixture exceeded the
180-second exact-probe limit before producing a score; the lingering child was
stopped by verified executable path. No semantics were reduced and holdout
remained sealed. This is not evidence of solver convergence. Reopen only with an
exact quotient or bounded external proof host; do not shrink the frozen suite.

`ATTR-PROFILE-DOMINANCE-102` is closed as accepted attribution. It uses only
consumed 095 cycle-balanced/low seed 1600000 day 1. The exact oracle plan and
the parent-selected plan will each be evaluated with the same frozen production
belief, manifest, candidate-specific viability upper, W1 repair and risk
comparator, with an independent 5000-ms proof window. It reports all scenario
outcomes/uppers and both strict dominance directions. This peer-favorable probe
has attribution authority only; it cannot promote a candidate or make a local
performance claim. Exact current/lower/upper are `6/6/9`, `6/20/29`,
`6/24/30`; parent values are `6/6/12`, `6/19/29`, `6/23/29`. Both profiles are
certified and neither dominates. Exact-to-parent first fails at threshold
`6/23/29` with certified/possible weight `0/10000`. Valid uppers already
distinguish the states; the blocker is the exact W1 lower, four daily distinct
below the known exact result. Evidence is in
`research/evidence/ATTR-PROFILE-DOMINANCE-102.md`. The next gap is W1
continuation availability/retention/selection, not dominance or master cap.

`ATTR-W1-CONTINUATION-103` is closed as accepted attribution. On consumed 095
seed 1600000, it replays the exact day-1 plan and then reconstructs W1 with the
production repair settings: days 2-3 use three columns per agent and one retained
candidate with the 200 cap split evenly; day 4 uses the smaller terminal lane.
At the first plan divergence it reports the oracle per-agent route mask, joint
outcome retention, chosen/exact score, portfolio widths and master nodes, then
stops causal comparison because later states differ. No production candidate or
cap change is authorized. Day 2 is the first divergence: W1 portfolio widths
are `3|3|1`, oracle route mask is `001`, master nodes are 1, and chosen/exact
current scores are `6/12/20` versus `6/12/16`; neither exact plan nor equivalent
outcome is retained. Evidence is in
`research/evidence/ATTR-W1-CONTINUATION-103.md`. This is route availability,
not master selection. Prior cap16/dual/terminal-fuel candidates 077/079/082 stay
closed; only nonterminal fuel-anytime membership remains untested.

`ATTR-W1-NONTERMINAL-FUEL-104` is closed as accepted negative attribution. At consumed 095
seed 1600000 day 2, it keeps W1 cap3 and every ordinary generation setting, but
enables the existing exact-harvest/fuel-constrained-anytime enumerator with its
unchanged 32-route and 1,250,000-node bounds. It reports whether the oracle mask
moves from `001` to `111` and whether exact enumeration completes. It does not
run a candidate, widen caps or tune the enumerator. The result is mask `011`,
widths `5|5|3`, supported/complete `3/0`, 863 settled states, nine variants, two
bundles and no deadline. One patrol route remains absent, so no candidate opens.
Evidence is in `research/evidence/ATTR-W1-NONTERMINAL-FUEL-104.md`.

`ATTR-W1-NONTERMINAL-EXACT-105` is closed as accepted negative attribution. It repeats only
the membership query with the distinct complete fuel-constrained exact
enumerator, anytime disabled and cap3 unchanged. Mask `111` plus complete
enumeration is required; otherwise the nonterminal fuel axis closes. Complete
exact returned mask `011`, widths `5|5|3`, supported/complete `3/3`, 570 states,
eight variants, two bundles and no deadline. The fuel-flag axis is closed.
Evidence is in `research/evidence/ATTR-W1-NONTERMINAL-EXACT-105.md`.

`ATTR-W1-MASK-DOMINANCE-106` is closed as accepted attribution. It inspects the
complete per-agent reachability before RouteColumn pruning on the same day-2
state: oracle spot mask, terminal/fuel, membership in maximal/terminal routes
and strict reachable supersets. No generator option or candidate changes. Agent
0 oracle mask `4`, terminal 26, fuel 6 is absent from maximal/terminal routes;
one strict superset exists but zero preserve terminal+fuel. Agent 1 mask `63` is
present. This proves invalid single-agent horizon projection. Evidence is in
`research/evidence/ATTR-W1-MASK-DOMINANCE-106.md`.

`ATTR-W1-UPPER-FRONTIER-107` is closed as accepted attribution. On the same exact
day-2 state it composes the complete two-patrol day-outcome frontier and ranks
unique joint terminal states by the unchanged candidate-specific FastViability
upper for day 3. The per-agent frontiers are 53 and 72 outcomes, composing 3324
unique joint states. The oracle class exists and its upper `6/24/30` equals the
maximum, rank 1 with 39 ties. Evidence is in
`research/evidence/ATTR-W1-UPPER-FRONTIER-107.md`. This authorizes only a fresh
candidate with a structurally bounded served-terminal route per cell and an
additive bundle lane; baseline W1 must remain an exact fallback.

The closed exact-gap axis `CEILING-BRANCH-PATROL-089` used parent `f574d4e`. The accepted 088
evidence is confined to one seven-cell line component, while exact residual
gaps remain and no evidence yet establishes multi-day value quality on a
branched component with mixed plain/mountain movement and six spots. A new
research-only suite freezes 18 development and 54 holdout matches across six
new structural families and low/default/high fuel. It keeps two active patrols
plus one isolated control so the same complete joint DP remains proof-capable,
but uses a new branched topology, brands, stock patterns and seed range. No
production source changes are authorized. Holdout SHA256 is
`1A826B829284C7AD7CB6938C7DB18AE1C2179CCDC75687EB1DD20BD94906955E`.
The research oracle first replaced a redundant global `O(N^2)` dominance scan
with identical comparisons grouped by joint physical state; it reproduced the
consumed 085 development result and hash `6e4072b927d9fa2a` exactly. The one
valid 089 development sweep then found oracle-vs-champion `4/14/0`, zero
invalid, no tier-1/tier-2 gap and four tier-3 serving gaps: +5 branched-
duplicate low, +5 fuel-split low, +2 terminal-fork low and +1 branched-
duplicate high. Result hash is `33080960367d1631`; maximum exact frontier was
39489. Holdout remains sealed. Read-only attribution on branched-duplicate/low
seed 1500100 found the exact day-1 plan absent at the 16-column portfolio and
present at 32; injecting only its missing routes makes the exact outcome survive
the unchanged master cap 32 and its full-match upper ranks joint first. The
earliest loss is therefore column retention before F0. `prune_columns` already
implements spot diversity but the caller enables it only at fuel at least three
day-step budgets; every 089 gap is below that threshold. This opens
`SCORE-ROADLESS-SPOT-DIVERSITY-090`, limited to enabling the existing lane on
deterministic roadless maps with unchanged cap and untouched road behavior.
That candidate is now rejected and fully absent from production. Direct
candidate-vs-parent development W/T/L was `2/15/1`, zero invalid: terminal-fork
low and fuel-split low gained +2 servings each at tier 3, but branched-duplicate
low lost one daily distinct at tier 2 (`5/20/25` to `5/19/29`). Candidate
oracle-gap hash was `28a37d0058be368e` versus parent `33080960367d1631`.
The 089 holdout stayed sealed. Spot-diversity ranking, slots and thresholds may
not be tuned on this consumed cohort.

The next independent axis is `CEILING-MULTI-PATROL-085`. Existing exact evidence
covers a four-patrol terminal day and a one-active-patrol full match, but not
multi-day allocation between two simultaneously active patrols. A new manifest
freezes 18 development and 54 holdout fixtures across balanced,
duplicate-brand, stock-contention, coverage-trap, fuel-allocation and
terminal-separation families under low/default/high fuel. Every fixture has a
seven-cell roadless active component, two interchangeable active patrols, one
isolated control, five spots and four days. The research-only oracle must be a
complete joint DP over positions, fuels, lifetime mask and official score, with
stock-aware daily composition and exact simulator/independent-validator replay.
Production source stays unchanged. The holdout remains sealed unless the exact
oracle beats `00711ba` on development. Manifest SHA256 is
`A0EA8A417B4F63B4B5F2E0069A1948EB2C7CB9C827DC6C80A31DC1D6EE188D7B`.

The complete 085 development sweep found a recurrent exact gap: oracle-vs-
champion W/T/L `4/14/0`, zero invalid, all four gains at tier 2 under low fuel.
Balanced improved `5/17/24` to `5/20/26` (+3 daily distinct); duplicate-brand,
fuel-allocation and terminal-separation each gained +1 daily distinct. Low-fuel
stock-contention/coverage-trap and every default/high fixture tied. Maximum
exact frontier was 6399 states and result hash was `c7324c01ff35e2e8`.
The holdout remains sealed. Read-only attribution is now closed. Every exact
winning trajectory has a candidate-specific admissible upper tied for best, but
the candidate is lost earlier: `RouteMaster` reserves a quarter of its beam for
terminal diversity while its signature contains tanker terminal cells only.
On the all-patrol 085 fixtures that signature is empty, so diversity slots
degenerate into current-score continuation. Exact outcomes required arbitrary
master caps from 32 through 96; widening a cap is therefore rejected. The one
shared capability loss is missing patrol terminal position/fuel diversity in
the existing beam lane.

`SCORE-STATE-DIVERSITY-086` is rejected and fully absent from production. Its
generalized all-agent terminal position/fuel signature left the 18-case result
exactly unchanged at oracle W/T/L `4/14/0`, four tier-2 gaps and result hash
`c7324c01ff35e2e8`; the first divergent oracle plan remained absent from every
16-candidate F0 audit. The holdout stayed sealed. This proves the tanker-only
signature is not the earliest causal loss on 085, and forbids more signature,
slot-count or beam-width tuning without a new causal counterexample.

`SCORE-F0-UPPER-087` is active. It keeps the existing 12 of 16 current-quality
F0 slots unchanged. For the remaining four existing diversity slots, it ranks
all unchanged master outputs first by the already-authoritative candidate-
specific admissible future upper, then uses the existing max-min plan distance
and current comparator as tie-breaks. Candidate count, master search, W1 count,
simulator, validator, ALNS, roles and hard cap remain unchanged. The purpose is
to make F0 intake consistent with the horizon value function without replacing
or weakening designed plan diversity. Development passed: candidate-vs-parent
W/T/L `4/14/0`, zero invalid, with tier-2 gains `+2,+1,+1,+1` across all four
opened low-fuel winning families. Three exact tier-2 gaps became tier 3 and the
balanced gap shrank from +3 to +1; all other 14 fixtures tied. Candidate oracle
gap hash is `6e4072b927d9fa2a` versus parent `c7324c01ff35e2e8`. The frozen
54-case 085 holdout may now open once for a paired candidate/parent gate using
the identical frozen probe binaries; local elapsed is ignored. That gate is now
closed: candidate-vs-parent W/T/L `8/42/4`, zero invalid, first-tier gain/loss
sums `18/4`, maximum gain `+4`, and all four losses exactly `-1`. Wins span
duplicate-brand, fuel-allocation, terminal-separation and balanced low fuel;
every default/high and the remaining two low families tie. Frozen binaries were
candidate `8B184AC3B32E20309F98D3CF5A2CF1A71C5D15ECA374EE8A8B99063D7ACC762D`
and parent `C8D68941D6317D207811D3C3C7BC60CC30420E3034055803E48BB0B4545A81E6`.
The four losses are immutable tails and may not be tuned. The unchanged 087
candidate then failed the road-containing fixed protected lane at `1/32/3`,
gain/loss sums `2/6`, tails `+2/-4`, zero invalid/emergency. Losses crossed
rare-brand, overnight and threshold-corridor, including one tier-2 loss. Direct
day traces showed active plan changes without deadlines on three of four changed
fixtures; this is a causal domain mismatch, not merely local performance noise.
087 is rejected and fully absent from production; native/BTC were not run.

`SCORE-F0-DETERMINISTIC-088` is accepted from clean `00711ba`. It reuses 087's
12-quality/4-upper-then-distance intake only when `config.roadCells` is empty,
the exact domain where 073 already proves a single deterministic traffic world.
Road-containing maps execute the parent intake and do not compute the additional
upper bounds. No seed, family, fuel or opponent route exists. The already opened
085 development/holdout establish the candidate lane; an independent consumed
but untuned roadless one-active-patrol matrix is used only as a protected gate.
Any general loss rejects 088 without adapting the condition or queue ratio.
The independent roadless one-active-patrol protection passed byte-for-byte at
the score/result level: 18-case development hash `6c9c4ac2e6aee047` and 54-case
holdout hash `468afde7057cf112`, both identical to `00711ba`, zero invalid.
Together with the frozen multi-patrol development `4/14/0` and holdout
`8/42/4` (gain/loss `18/4`, all losses -1), 088 passes score/protected gates.
The full unit/validator suite passed and the frozen BTC executable SHA256 is
`B6DF85B0410EF81E77454DBE5BCE9BF0650A0558014185EE6C1124EF1F9E1E1F`.
Fresh explicit-advanced BTC runs `m-2029` and `m-2034` exercised the exact binary
at 5000 ms. They recorded respectively 10/10 and 10/10 HTTP-200 valid actions,
10/10 independent-validator agreement, 9/9 reconciled transitions, zero
emergency and maximum solver times 2988 ms and 2454 ms. Replay-check rebuilt
`6/60/214` and `6/60/345`; replay SHA256 values are
`0ADE1C005720A4D2525E49200A38D1F6697D09786740711DC474A0904229D7A9` and
`0B3320FAC779AD5B97328974FF7F352861C0451B8CF8BABC01BB52108E263E04`.
The authoritative setups contained 63 and eight road cells, so 088's new branch
was inactive in both runs. BTC exposes no terrain editor in its advanced match
configuration; these runs therefore authorize unchanged-road-path lifecycle,
validity and hard-cap evidence, not active-branch score or latency. Bot rank is
ignored. The roadless quality claim remains confined to the frozen paired
matrices and no local elapsed is used as target-host performance evidence.

## Closed 070-084 history

The parent champion for this closed batch was `7ef3694` (`FINAL-QUEUE-065`).
The combined `SCORE-HORIZON-072` and `SCORE-SCENARIO-073` production diff cleared
`PROMOTE-HORIZON-SCENARIO-083` and `PROTECT-HORIZON-SCENARIO-084`, then became
canonical commit `00711ba`. Later witness successors failed cleanly and are
absent from production source.
`CEILING-ORACLE-070` is now closed accepted as a read-only capability audit:
all 36 development fixtures and all 108 one-time holdout fixtures tied a
complete terminal oracle. Oracle wins, HEAD wins, incomplete frontiers and
invalid plans were all zero in every one of the 18 family x fuel strata. The
development result hash is `151496ca6d63ee47`; the holdout result hash is
`6b4fecc3cc1eec37`.

`CEILING-MATCH-071` was the predecessor exact multi-day axis. It used 18 fresh
development matches and a sealed 54-match holdout from
`research/holdouts/CEILING-MATCH-071.csv`, SHA256
`6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`.
Each road-free 8x8 match lasts 4 or 5 days and has three fixed patrols: one in
the connected spot component and two in isolated no-score components. This
makes a proof-complete full-match DP over the active patrol's position, fuel,
lifetime mask and accumulated official score tractable without altering or
simplifying production logic. Low/default/high fuel and six independently
generated structural families are frozen. HEAD remains capped at 5000 ms per
day; local elapsed is ignored. Any development oracle win opens attribution and
keeps the holdout sealed. A complete tie can close only this road-free,
one-active-patrol multi-day value axis.

Development has now exposed a repeatable exact gap, so the 54-match holdout
remains sealed. HEAD versus full-match oracle is `7/11/0` losses/ties/wins from
HEAD's perspective (`oracle W/T/L 7/11/0`); all seven gaps are tier 2, with
daily-distinct improvements of `+2..+6`, zero lifetime loss and zero invalid.
The gaps span low fuel (2/6) and default fuel (5/6); all six high-fuel matches
tie. On mountain-detour default seed `1310500`, HEAD and oracle tie after day 1
at `5/5/5`, then diverge on day 2: HEAD takes `5` current distinct and falls to
fuel `4`, while the oracle takes `4` and preserves fuel `8`; final scores are
`5/14/14` versus exact `5/20/20`. This reproduced identically three times. A
verification run checked all `42036` retained daily outcomes from `939`
physical-day enumerations against both independent engines. The current task is
code-level attribution of this finite-fuel horizon-value error; production
source must remain unchanged until that path is proven.

Attribution is now exact: on target day 2, the oracle action is already in the
16-candidate F0 set at current score `5/9/9`, active terminal `29@fuel8`, and
coarse valid upper `5/24/24`; it is marked `not-shortlisted`. The selected action
is `5/10/10`, `26@fuel4`, upper `5/25/25`. `candidate_valid_upper_bound` applies
the same remaining-day brand allowance after each candidate, so the one-point
current gain dominates before candidate-specific resource value can be
certified. Route generation and master capability are therefore exonerated.

`SCORE-HORIZON-072` is the active implementation candidate. It tightens only
the existing `FastViabilityAnalyzer` upper bound when no tanker exists. In `D`
remaining days, a patrol can receive at most `D` no-movement day-start claims;
every additional claim follows a completed movement, and every official
movement consumes at least one fuel. A patrol whose current fuel-feasible
component contains a spot therefore contributes at most `D + currentFuel`
future claims; a no-spot component contributes zero. Summing this capacity and
capping the existing `D * brandCount` / per-day-stock allowances is admissible
for both daily distinct and servings. Any tanker retains the old coarse bound
because refuel invalidates a current-fuel cap. The bound does not select a plan:
unchanged upside shortlist, witness certification and final risk comparator
remain authoritative. On seed `1310500` this worked exactly as designed: greedy
upper tightened from `5/25/25` to `5/17/17`, oracle-action upper became
`5/20/20`, and the oracle action entered W1 and certified. It was nevertheless
not selected because a separate false pessimistic scenario pinned its certified
lower bound to current score. `SCORE-HORIZON-072` is therefore inconclusive as a
standalone score candidate and is retained only as a proven prerequisite.

`SCORE-SCENARIO-073` is now active. Audit shows every road-free day receives
`likely:5000` plus `fallback-pessimistic-bound:5000`. With zero road cells,
traffic footprints and carry cannot change any movement cost or state
transition, so the fallback is not a distinct possible world. The successor
collapses only this semantically deterministic domain to one 10000-weight
scenario and leaves every road-containing manifest unchanged. Combined with
the resource-admissible upper, unchanged W1 repair/comparator must produce the
gain; otherwise both source diffs are rejected. The sealed holdout remains
unopened.

The combined 072+073 development result is oracle W/T/L `3/15/0`, improved
from `7/11/0`, with no invalid or new loss; result hash
`6c9c4ac2e6aee047`. It closes four gaps, including seed `1310500`, but is
not declared complete: the three remaining gaps expose the next
canonical mismatch: `FutureWitnessRepairer::repair_profile` asks the future
master for exactly one candidate and commits `candidates.front()` independently
each day. On fuel-allocation/default and terminal-position/default, the exact
first-day resource state is already certified, but the greedy continuation
proves only `5/12` vs exact `5/16` and `4/12` vs exact `4/15`.

`PROMOTE-HORIZON-SCENARIO-083` now evaluates the combined candidate by the
correct practical rule rather than requiring all exact gaps to disappear. A
clean parent `7ef3694` is built in an isolated external worktree and compared
with the unchanged candidate using the same score-only oracle probe. Paired
development passed at candidate-vs-parent W/T/L `5/13/0`; every gain is tier 2,
ranges from `+1` to `+6`, spans low/default fuel and three families, and both
binaries were valid 18/18. The parent oracle result was `7/11/0`, while candidate
was `3/15/0`. The 54-case frozen holdout may now open exactly once. Frozen
binary SHA256 values are parent
`F495E4ED86BF3A233CBBB5A259B1A8113759981D639E518C17FD02623599B9DF` and candidate
`71B4895170D6213E2CC987E64B71E018D64F3A039E21AF6AF77B2CBAA14AEC95`;
the shared probe is
`784ED885908E7B4D522C5DCEF4D14A1C958713B656D77D355796B527ACCC0AA1`.
The 54-case holdout was opened exactly once and closed as a score-gate win.
Candidate-vs-parent W/T/L is `22/30/2`; 21 gains are tier 2 and one is tier 3,
first-tier gain sum is `51`, and maximum gain is `+6`. The two losses are both
only `-1`: terminal-position/default seed `1310403` at tier 3 and
terminal-position/high seed `1320401` at tier 2. Parent remained below exact
oracle on 30/54 fixtures, candidate on only 16/54; both had zero invalid.
Candidate oracle hash is `468afde7057cf112`, parent hash `d6db972a0f27cdd4`.
This is a broad practical win with bounded downside, not absolute dominance.
The holdout is consumed and may not be reopened. Commit is still forbidden until
the unchanged candidate clears road-containing fixed/native protected lanes and
BTC target-host at the 5000-ms internal cap; terminal-position downside is an
explicit protected tail.

`PROTECT-HORIZON-SCENARIO-084` is active with the unchanged 083 source. Its
frozen matrix uses 36 new road-containing 8x8 horizon-4/5 fixtures across all six
historical traffic families, under both fixed all-patrol and native exhaustive
roles. If that screen has no material/systematic regression, six BTC-scale lanes
run 12 new 32x32 horizon-10 fixtures each: low/default/high fuel crossed with
fixed/native roles. Every day budget is 5000 ms. Fixed mask 0 directly exercises
072's no-tanker path; native protects role selection. Exact simulator and
independent validator remain mandatory. Local latency is ignored and BTC retains
final performance authority.
The frozen 084 manifest SHA256 is
`A00548DF115C09CDED7530A7268153091354EFBDB38C57B89D9B130FE97BA1BE`.
External harness SHA256 values are parent
`7EF7143531B5280826709D7EE8018C5526B73E06277994A67F25FCD6A8415AF0` and
candidate `D0DC94ECC238808DEBBB7A4EEB3436A45069A7B56DBCD645F176E20360A7A7E6`.
Phase A closed as an acceptable bounded tradeoff: general fixed W/T/L
`4/30/2`, gain/loss sums `9/4`, maximum tails `+3/-3`; native exhaustive
`3/31/2`, sums `13/2`, tails `+11/-1`, with 8/36 role-mask changes. Combined
is `7/61/4`, zero invalid/emergency. No family loses systematically, although
rare-brand seed `830017` loses in both role lanes and remains an explicit tail.
Phase B now opens one BTC-scale lane at a time, starting with low-fuel fixed.

Phase B low-fuel fixed closed as a clean causal win: candidate-vs-parent W/T/L
`4/8/0`, first-tier gain sum `18`, maximum `+8`, zero invalid/emergency, with
gains in four of six traffic families. Low-fuel native then returned `3/1/8`
with large tier-3 spread, but this is not causal logic evidence: all 12 role masks
matched exactly and every mask contained a tanker. Therefore 072 preserved the
old upper bound and 073 preserved the road-containing scenario manifest on every
case. Cross-binary score differences under local wall-clock cutoff are
compile-layout/performance noise, precisely the class local is forbidden to
judge. Invalid/emergency remained zero. The native lane is closed
performance-inconclusive; remaining default/high native local rows stay unopened.
The unchanged candidate passed its direct unit/semantic gate. A roadless-map
theorem test freezes exactly one `deterministic-no-road` scenario at total
weight 10000 without a false fallback. A no-tanker theorem test isolates one
fuel-zero patrol at a spot for four remaining days and obtains an exact claim
upper of four; changing an isolated agent to tanker restores the old coarse
relaxation. The complete unit suite passed. The candidate now advances only to
BTC target-host validity, lifecycle and hard-cap telemetry at 5000 ms, where
performance authority resides.

The authoritative toolchain built `udonshield_tests` and `udonshield_btc` once;
the full unit suite passed. The frozen BTC candidate SHA256 is
`784E0E8F5156E063AD572F9946BBDA7EB158C00D62D267EEB35812A434993032`.
It was unchanged by the test-only rebuild and remained the exact target-host
artifact. Fresh explicit-advanced BTC match `m-1986` used hard/three bots/ten
days/32x32/100 steps/5000 ms/eight agents/twelve spots/six brands/low fuel. The
candidate selected mask 8, returned 10/10 HTTP 200 valid actions, had zero
emergency, skip, server WAIT, invalid, hard-cap breach or reconciliation
mismatch, and scored `6/60/219`. Target-host decision `totalMs` maxed at 3376 ms
and end-to-end response at 3458 ms; setup-to-assignment was 4623 ms. Independent
replay-check accepted all ten actions and all nine transitions. Replay SHA256 is
`7B0B83411B21550F20C4691B0EC9077FCB6E553212722BA70A3E1F1EEF706F53`.
Rank 1 is not promotion evidence. The causal paired score, semantic and
target-host gates together authorize the canonical commit.

`SCORE-WITNESS-074` is rejected and fully absent from source. Its two-branch W1
repair split the existing per-day combination cap across live branches. That
changed the route portfolio available to the exact-score branch, so the claimed
dominance over the old greedy lower witness was false. On the same 18-case
development split, oracle W/T/L worsened from the combined 072+073 result
`3/15/0` to `6/12/0`; none of the three residual gaps closed,
terminal-position/default widened from tier-2 `+3` to `+4`, and three previously
closed gaps reopened. There were zero invalid plans, but this is still a direct
logic regression. Result hash: `4c781fad088b38f5`. The 54-case holdout was not
opened. A future witness candidate may reopen only if the complete old greedy
witness is computed with its original portfolio/cap and retained as an exact
fallback before any separately bounded recourse work; merely retuning beam width
or the three development seeds is forbidden.

`SCORE-WITNESS-075` is rejected and fully absent from source. It correctly ran
the complete old scenario pass before an optional resource-upper-guided rollout,
so exact dominance held: no old gap reopened and no invalid appeared. However,
it changed no selected action or final score on any of the 18 development
matches. Oracle W/T/L and result hash remained exactly `3/15/0` and
`6c9c4ac2e6aee047`. The additional 278 source lines and W1 work therefore had
zero demonstrated value. It may not be retuned by candidate cap or tie-break on
these opened fixtures. The next permitted action is read-only attribution of
whether exact continuation actions are absent from the future route portfolio,
absent from retained master candidates, or merely misvalued after retention.

`ATTR-WITNESS-076` is closed accepted as read-only attribution. All exact oracle
continuation plans were dual-valid. At all eight first/middle days where the
oracle continuation was not retained, the active patrol route itself was absent
from the W1 portfolio (`portfolio_mask=011`) and no semantically equivalent team
outcome survived even with `maximumCandidates=32`. Seven of those eight active
routes reappeared when only `maximumColumnsPerAgent` was widened, at first caps
`12,12,8,8,16,4,12`; the terminal-position final-day route remained absent even
at cap 32 and four paths per target. This proves the primary gap is future route
capability/pruning before master ranking. It also exposes dead designed wiring:
W1 sets `enableHarvestExtensions`, but generator harvest extensions require a
column cap in `12..16`, while W1 supplies 3 detailed / 2 terminal; triple routes
also require cap 12 and quadruple routes cap 16. Evidence is in
`research/evidence/ATTR-WITNESS-076.md`; holdout stayed sealed.

`SCORE-WITNESS-COLUMNS-077` is rejected and fully absent from source. Raising the
two W1 column caps to 16 activated the missing designed sources, but the unchanged
greedy repair then replaced prior lower witnesses. Development worsened from
oracle W/T/L `3/15/0` to `4/14/0`: all three residual gaps remained and
rare-late/low reopened at tier-2 `+2`; invalid stayed zero. Result hash:
`a4b38642a336b758`. The holdout was not opened. Cap widening may not be applied
as an in-place replacement again; it can be reconsidered only as optional work
after the complete old witness is retained, and only if a read-only probe first
shows a strictly better full trajectory.

`ATTR-WIDE-078` is closed accepted as read-only attribution. Starting from exact
oracle day-1 candidates, cap-16 greedy reached the daily-choice oracle exactly
(`3/9/10` vs baseline witness `3/7/8`), cap-16 resource-upper guidance reached
the fuel-allocation oracle exactly (`5/16/16` vs `5/12/12`), and guided terminal
position improved `4/12/13` to `4/13/13` though the oracle remains `4/15/16`.
All trajectories were complete and dual-valid. Thus wide capability plus the two
existing canonical selection views contains actionable horizon value; neither
view alone dominates the other. Evidence is in
`research/evidence/ATTR-WIDE-078.md`; holdout stayed sealed.

`SCORE-WITNESS-DUAL-079` was rejected at its preregistered development gate. W1 first computed
all original scenario witnesses unchanged. Only afterward, within the same
candidate deadline and fixed cap16, it attempts one current-score-greedy and one
candidate-valid-upper-guided full future rollout per non-fallback scenario. A
scenario outcome is replaced only by the lexicographically best complete,
dual-valid final witness, so the old exact lower bound is retained by
construction. No cap, width, map/fuel threshold or risk policy is tuned. The
18-case development remained byte-identical to 072+073 at oracle W/T/L
`3/15/0`, invalid 0, hash `6c9c4ac2e6aee047`: the optional trajectories proved
offline did not complete/change a profile in the production-deadline engine.
Local timing cannot decide target-host feasibility, while BTC ranking cannot
causally prove completion of optional work when no exact output changed. The
registered improvement gate therefore failed, the holdout stayed sealed, and all
079 source was removed while its attribution evidence was retained.

`SCORE-WITNESS-UPPER-SKIP-080` was also rejected and removed. Before optional work for a scenario,
it checks the already-authoritative `scenarioValidUpperBounds`. If the retained
baseline score is at least that upper, it skips both rollouts. This is exactly
equivalent to 079 because 079 accepts only a strictly higher official score, which
the valid upper proves impossible; the same baseline witness bytes and profile
remain. Nevertheless the one allowed 18-case development run remained exactly
`3/15/0`, invalid 0, tier2 3, hash `6c9c4ac2e6aee047`. It exposed no actionable
completion or exact-score change, so the holdout was not opened and the guard was
removed with 079. No cap, beam or skip retuning on these opened residual fixtures
is permitted without a new independent counterexample.

`ATTR-ROUTE-CLOSURE-081` is closed as accepted read-only attribution. It did not
retry 079 or tune seed `1310400`. Across every oracle transition in all 18 opened
development fixtures, it classifies exact active-route membership through one
fixed ordered lattice of existing generator capabilities: production W1,
cap16, cap32/paths4, cap128/paths8, uncached harvest, harvest orienteering, exact
orienteering, fuel-constrained exact and anytime enumeration. The order is
attribution precedence, not a set-monotonicity claim across algorithm switches.
Across 81 dual-valid transitions the first membership stage was W1 25, cap16 23,
cap32/paths4 4, cap128/paths8 2, fuel-exact 6 and absent 21. The six fuel-exact
routes occur in four independent fixtures and span balanced-low,
rare-late-default, terminal-position-default and fuel-allocation-high; this
includes seed `1310400` day 5. Thus the target is a repeated public capability
class, not a unique terminal-position exception. The 21 routes absent from the
whole lattice mostly occur in score ties and authorize no source work. Holdout
remained sealed and no production source changed.

`SCORE-FUTURE-FUEL-EXACT-082` was rejected and fully removed. Current-day production already enables
the canonical fuel-constrained exact/anytime resource enumerator when a terminal
day patrol has less than two day-step fuel budgets. All three provisional/W1
future-generation blocks omit both the terminal exact-search exception and the
fuel-constrained flags, so the evaluator can rank candidate states with less
capability than the planner it is supposed to model. 082 wires the identical
public conditions into those three blocks without changing caps, deadlines,
master selection or any solver. The one registered 18-case development run was
exactly unchanged at oracle W/T/L `3/15/0`, invalid 0, tier2 3 and hash
`6c9c4ac2e6aee047`. Route availability alone did not change a retained witness,
shortlisted candidate or final score. The holdout stayed sealed, all 082 source
was removed, and cap/order retuning on the 081 cohort is forbidden.

The frozen manifest is `research/holdouts/CEILING-ORACLE-070.csv`, SHA256
`6C738A9E9B811C27A2AA7A25405BFFDBED971B0E929B330C330ABE99AC45B0B5`.
The proof scope is deliberately narrow: official-valid 8x8 terminal states,
four fixed patrols, six structural families and low/default/high fuel. Complete
per-agent resource enumeration plus exact stock-capped team DP proves the
terminal optimum because score is monotone in terminal spot claims and no
tanker/refuel coupling exists. Every HEAD/oracle plan agreed under the exact
simulator and independent validator. This closes the fixed-role terminal
resource/team-allocation axis only; it does not establish multi-day, native-role
or whole-architecture convergence. Local elapsed time was discarded. The next
permitted score research is a separately registered and frozen full-match
small-instance oracle. These 144 opened fixtures may not be tuned or reused as
its holdout.

## Historical experiment log

The text below is chronological provenance. Any older statement about the
"current" champion or an open axis is superseded by the current-phase block
above.

Forward research starts from the current global champion `afcd2da`. Re-running or
rebuilding every historical checkpoint is not a research objective; old
checkpoints remain lane champions for targeted A/B only. `PERF-DEADLINE-003` is
accepted as a semantics-preserving BTC transport repair. `SCORE-ROLE-030`, 031
and `SCORE-FINAL-001` are rejected and no opened holdout may be retuned. Source
and tests are restored to the `afcd2da` planner/decision line. `SCORE-ROLE-032`
is also rejected: deterministic work stabilized an evaluator that still selected
the losing role. `PERF-SYNC-033` and `SCORE-EXACT-034` are rejected as well and
fully absent from source. No implementation axis is currently open.
`ATTR-MASTER-035` and `ATTR-ROUTE-036` have closed the exact claim-mask and
frontier-membership ambiguities. `ATTR-POOL-037` and `ATTR-ALNS-038` narrowed the
remaining source to regular one-agent ALNS but cannot distinguish portfolio from
synthesized route. `SCORE-ALNS-039` is rejected and absent from source. No
implementation candidate is active. `ATTR-DENIAL-040` falsified exchange over
the retained production frontiers. `ATTR-OPTIMAL-041` proves complete
one-patrol infeasibility and `ATTR-TEAM-042` proves a dual-valid multi-patrol
score of `6/60/322`. `SCORE-QUEUE-043` is rejected and absent from source. No
production implementation candidate is currently active. `ATTR-QUEUE-044`
localized the observed -72 tail to a trajectory that changes action bytes on
day 1 and loses score from day 2, before the intended terminal anytime queue is
directly enabled. `ATTR-QUEUE-045` exposed a protected-harness wiring flaw:
`exact_supported=0` because the harness used default harvest mode 6 while BTC
production uses mode 7. `ATTR-QUEUE-046` then closed the causal comparison:
cardinality-first improved exact-local evidence but tied the final official
score on the alleged negative state. No production candidate is active; the
temporary switch/probe must be reverted before a new mode-7 holdout is frozen.
The temporary attribution source/probe is fully reverted. `EVAL-PARITY-047` is
accepted: the forward harness now declares harvest/future mode 7 and exposes
terminal exact wiring telemetry; historical reports remain untouched mode-6
artifacts. `SCORE-QUEUE-048` supplied the frozen score mechanism for accepted
final candidate `FINAL-QUEUE-065`. The final source combines only its
canonical cardinality-first capped queue with exact HTTP deadline alignment,
identical chunked exact-state initialization and recursive feasibility
cancellation. Rejected HTTP retry/header experiments and attribution-only
decision telemetry are absent. `FINAL-QUEUE-065` is closed accepted and no
implementation or score axis remains active.

`ATTR-OPPONENT-066` is a separate read-only external comparison against the
frozen `thing-or-think/hexudon-procon` clone at `1f0d22e`. It does not reopen a
production implementation axis, cannot justify a source commit, and is not a
BTC performance claim. Its frozen manifest is
`research/holdouts/ATTR-OPPONENT-066.csv` SHA256
`D9FD6CA08AC94C985B308AB759367C174CEA3C711FBACBA27135DE8135D6AD53`:
24 structural official-valid fixtures across six families, including all three
BTC-like fuel regimes, evaluated in fixed-role and native-role lanes at the
same 5000 ms cap. Both solvers must pass the UDON exact simulator and
independent validator; an adapter parity pilot is a prerequisite to scoring.

That `ATTR-OPPONENT-066` pilot is closed rejected before the full holdout: the
peer's own simulator and validator credit a patrol already standing on a spot
at day start, while both independent UDON engines credit only a completed
move. On the pilot's day 2 the peer self-score was `7/7`; the common score was
`6/6`. Direction encoding, movement terminals and road occupancy agreed, so
this is a scoring-transition divergence rather than an adapter defect. The
pilot fixture is consumed and must not be used for comparative aggregate
evidence. `ATTR-OPPONENT-067` is the successor action-output comparison: it
uses the remaining 23 frozen fixtures, preserves each solver's source exactly,
and scores emitted peer actions by the common official engines without
repairing peer behavior. Its manifest is
`research/holdouts/ATTR-OPPONENT-067.csv` SHA256
`EDA237490DC2C94239ECE93B85CE3DCE2B2EC459374FC68D34D6E776C1F8F47A`.

`ATTR-OPPONENT-067` is now closed and accepted as external comparative evidence,
not as a production candidate. Across 46 fresh paired matches, UDON recorded
`40/0/6` overall and `33/0/6` among the 39 both-valid pairs; fixed/native splits
were `19/0/4` and `21/0/2`. UDON produced zero invalid outputs. The peer produced
seven invalid outputs in the frozen run; six reproduced as insufficient-fuel
movement acceptance, while the seventh high-fuel threshold sample did not recur
and is classified as cutoff-sensitive local behavior. At the first differing
official tier, UDON won six pairs at tier 2 and 27 at tier 3; all six peer wins
were tier-3-only. UDON won all 12 high-fuel pairs, while the peer's real but
narrow strength concentrated in overnight and selected low/default-fuel lanes.
Peer self-semantic parity failed in 45/46 runs across 338 mismatch-days.

The durable report is `research/evidence/ATTR-OPPONENT-067.md` SHA256
`EC4B5DE0BB3D7BF4A7111DE29251DA63156EB3B1522F39FA4540D3A41A845A07`;
raw, paired and invalid-attribution evidence hashes are recorded in
`research/EXPERIMENTS.csv`. Local cutoff/elapsed observations are deliberately
excluded from the strength verdict and have no BTC performance authority. No
solver source changed, no production commit is authorized and no optimization
axis is opened from opponent behavior; any future source research still requires
an independent UDON telemetry gap and the full protected gate.

`ATTR-OPPONENT-068` is an explicitly peer-favorable compute-ceiling attribution,
not a new general holdout. It replays only the already opened strongest peer
counterexample from ATTR-OPPONENT-067: low-fuel BTC-like overnight seed `72005`,
fixed one-tanker-last roles, where the 5000 ms comparison favored the peer by 59
servings at tier 3. UDON remains hard-capped at 5000 ms. The peer receives its
documented 45000 ms daily profile under production-default, whole-match-MLNS
fuel-aware, and release-b-all configurations; the best common-evaluator-valid
peer score is treated as an oracle upper bound. This deliberately answers whether
the prior loss was mainly peer cutoff, but cannot support general promotion,
performance claims or a UDON source change. The frozen manifest is
`research/holdouts/ATTR-OPPONENT-068.csv`, SHA256
`627BA277755FF9D11D988E92C5C80220E9700985FD6ADACA41D67CA8EE12E72E`.

`ATTR-OPPONENT-068` is closed accepted as external attribution evidence. The
contemporaneous UDON result was valid `6/60/418`. Peer production-default and
whole-match-MLNS-fuel-aware were both valid `6/60/462`; whole-match completed
all `9/9` robust epochs on days with future horizon but did not improve the
default actions/score. The peer oracle therefore wins this known counterexample
by 44 servings at tier 3. Release-b-all was ineligible: the common evaluator
rejected day 8 at agent 2 step 47 for accepting a patrol movement without enough
fuel. Both valid peer profiles disagreed with common authoritative semantics on
all ten days.

The previous 5000 ms raw row was UDON `6/60/379` versus peer `6/60/438`. Because
both solvers changed score across separate local wall-clock runs, the score delta
cannot be causally assigned to additional peer compute. The controlled conclusion
is only that UDON does not win this deliberately selected overnight/low-fuel
counterexample on the peer's 45000 ms field, while whole-match search does not
beat the peer default result. The report is
`research/evidence/ATTR-OPPONENT-068.md` SHA256
`21705C0F4A84A5928FAD768CB9E052822F6AD2C4629A436618FF14E30A75F63C`;
raw and profile telemetry hashes are recorded in `research/EXPERIMENTS.csv`.
No solver source changed, no general aggregate is overturned, and no UDON
optimization axis or commit is authorized.

`ATTR-OPPONENT-069` corrects the question tested by 068. It selects the closest
both-valid BTC-like fixture that UDON won in ATTR-OPPONENT-067: low-fuel
high-stock seed `72004`, fixed one-tanker-last roles, where the opened score was
UDON `6/60/416` versus peer `6/60/404`. UDON remains capped at `5000 ms`; the
unchanged peer receives `45000 ms` under production-default,
whole-match-MLNS-fuel-aware and release-b-all profiles. The test asks whether
the best common-evaluator-valid long-compute peer profile can reverse that prior
UDON win. Its frozen manifest is `research/holdouts/ATTR-OPPONENT-069.csv`,
SHA256 `B469189A5B192772A352A48C54F47183D956FA51721AC04ED390D52D02A8740D`.
No implementation axis is opened and no source change is allowed from this
single opponent-attribution fixture.

`ATTR-OPPONENT-069` is closed accepted as corrected external attribution
evidence. The contemporaneous UDON result was valid `6/60/489`. All three peer
profiles were common-evaluator valid: production-default `6/60/448`,
whole-match-MLNS-fuel-aware `6/60/448`, and release-b-all `6/60/414`. Thus the
best valid peer profile did not reverse the prior UDON win; UDON retained a
41-serving tier-3 advantage. Whole-match robust search completed on `7/9`
future-bearing days and did not improve default. All peer profiles mismatched
their private semantics against the common transition on `9/10` days.

The prior opened row was UDON `6/60/416` versus peer `6/60/404`. Because both
scores moved across separate local wall-clock runs, no causal compute delta is
claimed. The controlled result is the contemporaneous `489` versus best-valid
`448` comparison. The report is `research/evidence/ATTR-OPPONENT-069.md`,
SHA256 `DC211EE245E9D0517359F0BE940BD54A0AB8E4FE04BE1D2B837F6C40C766D59B`;
raw and profile telemetry hashes are recorded in `research/EXPERIMENTS.csv`.
No solver source changed, no implementation axis is opened and no commit is
authorized.

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

The current score champion is `08771f1` (`SCORE-ROLE-023`). SCORE-ROLE-024 through
029 are closed or inconclusive and absent from source; their opened evidence may
not be retuned. `PERF-DEADLINE-003` fixes the independent ACK-loss counterexample
from `m-1278` without changing role selection, planner, comparator or action JSON.
Its normal and forced-resend BTC gates both completed 10/10 valid days and ranked
first. No further ACK-slice tuning is permitted against these opened fixtures.

Fresh discovery matches from transport champion `afcd2da` used explicit advanced
configuration and the 5000 ms hard cap. Low-fuel `m-1285` selected two-tanker
mask96, completed 10/10 valid days and ranked first at `6/60/320`; exact
counterfactual mask160 reached `6/60/319` while one-tanker mask64 lost tier 2 at
`6/57/238`, so that map does not open a score mechanism. Replay SHA256 is
`D638BE3D0E136EF94542BBCE58AA6B086D6A7C872732EF27188392D5D19D2F77`.

High-fuel `m-1286` selected one-tanker mask4, completed 10/10 valid days and
ranked first at `6/60/412`; maximum server-reported response was `2725 ms` and
maximum solver time was `2619 ms`. Replay SHA256 is
`8C3F6D8A975EA4263C921438517307478B66F8174700F1A3970666B512C45122`.
Same-binary exact counterfactuals reproduced mask4 at `6/60/412` and mask1 at
`6/60/422` in both run orders. The incomplete role rollout instead ranked them
`6/60/231` and `6/60/230`. Day 1 ties at 41 servings; the exact gain develops on
days 2, 4, 5, 7 and 10, so a central-tanker heuristic or another day-1 probe is
falsified.

`SCORE-ROLE-030`, parent `afcd2da`, targets only this measured resolution gap.
For horizons above five days with fuel strictly above twice the maximum daily
steps and incomplete full-horizon role evidence, it will take the top two
assignments having the leader's patrol count, split a bounded refinement window
equally, rerun the canonical exact rollout with the existing 8000-combination
portfolio and retain each mask's lexicographic maximum old/new evidence. It does
not use centrality, facility score, seed, map family or a tier trade. Short,
low/default/medium fuel paths remain unchanged, returned width is unchanged and
the role wall is at most 96% of 5000 ms. Frozen holdout SHA256 is
`4F399B26E148A0B58DAEB44551C12BF7502711D987B24C7DBFE73126EA2D859C`.

The candidate failed its first causal gate and is absent from source. The richer
paired refinement consumed the available role window through `4794 ms` but left
mask4 at rollout `6/60/231` and mask1 at `6/60/230`; it therefore did not expose
the stable exact +10. The frozen holdout remained unopened and no BTC candidate
was run. Wider or longer variants of the same coarse rollout are now closed.
Reopen this gap only after trajectory-consistent attribution explains why the
roles tie at 41 on day 1 but mask1 gains on days 2, 4, 5, 7 and 10. Source and
tests are byte-identical to `afcd2da`.

Further exact attribution rejects two easy proxies. In an all-patrol production
solve, agent0 served 6 on day 1 and 18 over days 1--3 while agent2 served only 5
and 16; a marginal-patrol rule would therefore choose agent2 as tanker, exactly
the losing mask4. Tanker centrality also chooses mask4 and was already falsified
on `m-0929`. The causal difference is the multi-day joint trajectory: mask1 moves
its tanker through hubs 790 then 593 and keeps the team at 43--44 servings on
several later days, while mask4 begins at hub712 and loses coordinated servings
on days 4--5 despite identical day-1 score. A static placement proxy is not an
admissible successor.

`SCORE-ROLE-031`, parent `afcd2da`, changes allocation rather than scoring. For
only long horizons whose fuel is strictly above twice the maximum daily steps,
the independent full-horizon rollouts for the existing beam run concurrently in
isolated engines after the common one-day probe. Every assignment uses the same
canonical 256-combination/day exact rollout and the same absolute 85% deadline;
there is no shared mutable router/master cache and merge order remains the
official comparator. This exploits the mathematical independence of role
subproblems to replace eight serial fragments with equal wall-clock evidence,
without a proxy or more than 5000 ms wall time. Low/default/medium and short paths
are unchanged. Frozen holdout SHA256 is
`BAEC90234791109009D56D10402C5DD720B2399DB0A1A67DFF38036A14144EA2`.

The parallel candidate is rejected and absent from source. Isolated tasks did
remove serial wall allocation, but their ranked evidence became a function of CPU
competition: on the contended development host mask4 remained first at rollout
`6/60/228` while exact-better mask1 fell to `6/55/218`. The observed `2947 ms`
elapsed value has no performance authority; the rejection is that the causal rank
remained wrong and scheduling/load now changed evidence quality, violating the
requirement that production quality not depend on the local host. The frozen
holdout remained unopened and no live candidate was run. Parallel role evaluation
may reopen only with fixed operation counts per assignment and deterministic
evidence independent of CPU scheduling. Source is byte-identical to `afcd2da`.

Planner telemetry opens a separate, non-role gap on terminal day. In `m-1285`
day 10, the selected exact score was `6/60/320`, guidance/optimistic upper score
was `6/60/322`, the master hit its deadline, and certification consumed zero
measured milliseconds. In the negative control `m-1286` day 10, selected and
guidance scores were both `6/60/412`. Code tracing shows why: every master
candidate has already passed the exact simulator and independent validator;
there are no future days, so every traffic-scenario witness is exactly
`scoreAfterToday`, and the final selected plan is independently validated again.
Nevertheless the BTC schedule reserves 20 percent for future-witness
certification and the F0 phase reserves another 25 percent of its window for
future profiles.

`SCORE-FINAL-001`, parent `afcd2da`, may act only when
`state.dayNumber == config.day_count()`. It certifies each provisional scenario
profile directly from the already independently validated current-day candidate,
collapses the certification bucket to the existing 25 ms validation floor,
transfers only the released duration to search, and sets the F0 future-profile
reserve to zero. It retains the existing 150 ms F0 boundary guard, 1600 ms BTC
network reserve, official lexicographic comparator, exact final validation and
total 5000 ms cap. Non-terminal deadlines, profiles, actions and search must be
byte-equivalent. Frozen holdout SHA256 is
`CDBC63E7AF1FFA22E0B6C9CD5342CA3C0BC8A2E269B668C18567DACDC0116E66`.

`SCORE-FINAL-001` passed its direct development gate: the unit suite passed,
`m-1285` day 10 improved from `6/60/320` to `6/60/321`, and the `m-1286`
negative control tied `6/60/412`, all with zero invalid/emergency output. The
frozen general-fixed lane then tied `0/12/0`. In the native lane, seed `923007`
changed from parent mask `8` and `6/24/38` to candidate mask `4` and `6/24/32`.
An exact parent rebuilt from `08771f1` reproduced mask `8`; planner/decision are
byte-identical between `08771f1` and transport parent `afcd2da`. The role rollout
attribution is causal within the candidate binary: fixed mask `8` reproduces
`6/24/38`, while fixed mask `4` reproduces `6/24/32`. The role rollout
does not execute the terminal mechanism, but its independent planner is bounded
by 60 ms wall time even in `select_roles`, so a changed binary layout/local
schedule can change rollout evidence and the selected mask. Local duration is
not a performance verdict; the decisive failure is that the claimed unchanged
role surface was not invariant and its observed downside of six servings was
larger than the one-serving causal gain. The remaining holdout and BTC gate were
not run. Candidate source/tests were fully reverted. Reopen only after role
evidence is operation-bounded and independent of code layout and host load.

`SCORE-ROLE-032`, parent `afcd2da`, is preregistered before source change at
holdout SHA256
`FAB2F1E71CC90185AB56D11ADF7FBD604F1FD1331AD77C528C8F3C9E8BDDA44E`.
It retains EventConflict, MacroMCTS, the route master, exact simulator and
independent validator. Each role assignment/day receives the same fixed work
allowance derived only from the existing `maximumCombinationsPerDay`; wall-clock
is an outer hard abort, not a ranking budget, and partial unequal-depth evidence
cannot outrank completed equal-depth evidence. There is one shared implementation
for low/default/high fuel and short/long horizons; no seed, map, match or bot
dispatcher is allowed. Development must first recover mask `8` on seed `923007`
and preserve the `m-1285`/`m-1286` role controls. Local timing has no promotion
authority; BTC target-host telemetry remains the final performance gate.

The unit suite passed, but the first causal development gate rejected
`SCORE-ROLE-032`. In the paired run both parent and candidate selected mask `4`
and scored `6/24/32`; fixed mask attribution already proves mask `8` reaches
`6/24/38`. Thus equal operation counts remove one noise source but merely make
the wrong coarse evaluator reproducible. The holdout stayed unopened and no BTC
candidate ran. Work-limit, equal-depth and test changes were fully reverted.
This also confirms that retrying wider/longer/fixed-count versions of the same
role rollout would be circular; reopen only for a newly attributed capability
mismatch not already falsified by `SCORE-ROLE-008`, 012, 030, 031 and 032.

`PERF-SYNC-033`, parent `afcd2da`, is preregistered before source change at
holdout SHA256
`C40DD7A1FDA69FEA54ABE1ED3B374506C56B82628386855465F30664C9B74CA1`.
The causal telemetry is not the leaderboard rank: low-fuel `m-1285` leaves a
2--5-serving master guidance gap on every day and performs 80 to 116625 partial
synchronization checks per day while deadline-bound; high-fuel `m-1286` is the
closed-bound control. The current DFS predicate reconstructs assigned-agent,
escort and refuel coverage state by scanning the entire partial selection on
every node. The candidate may replace only that DFS rescan with reversible
push/pop state implementing the identical predicate. Portfolio construction,
branch order, beam search, complete synchronization validation, simulator,
independent validator, official comparator and the 5000 ms internal work cap
remain unchanged. Local runs may establish equivalence and falsify the mechanism,
but only fresh BTC target-host telemetry may establish performance or promotion.

The candidate passed the unit suite and matched all 27 day plan hashes, official
scores and combination counts on fresh fixed-role seeds `936000..936005`. Its
final causal gate nevertheless failed: on `m-1285` day 10 the exact frozen parent
reached `6/60/321` with 5119 combinations and 195961 partial checks, while the
candidate reached `6/60/320` with 1129 combinations and 9685 checks. Both were
exact-valid, deadline-bound and retained guidance `6/60/322`. Local elapsed time
has no performance authority; the official-score regression and failure to create
useful search headroom reject this implementation before BTC or the remaining
holdout. Source and tests are restored byte-identical to `afcd2da`. Do not retune
incremental containers against this replay; reopen synchronization performance
only from a different target-host-profiled bottleneck.

Exact-route attribution opens `SCORE-EXACT-034`, parent `afcd2da`, at frozen
holdout SHA256
`6A9D06F06DBB49FDE8B3EAC1CA983C3CD8D2C33683CD012DADFEB775A7D2655B`.
On `m-1285` day 10 the accepted exact bundle serves 34; coordinated exact search
visits 504176 feasibility nodes, including 500129 overlap nodes, and records zero
improvements. The canonical master/ALNS can reach 35 while the root guidance is
36. Source tracing shows the low-fuel anytime enumerator retains only its 32
`maximalRoutes` once lifetime coverage is complete: `preferredBrands == 0`
disables the separately capped `supplementalRoutes` frontier, although daily
distinct and servings are still live official tiers. The candidate changes only
the existing anytime call: when no lifetime brand is missing, all public config
brands become the preferred set, activating the existing supplemental frontier
and enhanced coordination pass. Incomplete-lifetime behavior, high-fuel exact
enumeration, route semantics, settled-state and per-frontier caps, simulator,
validator and the 5000 ms cap remain unchanged. Development must first improve
the exact bundle or final selected score on `m-1285` day 10 and preserve the
closed high-fuel `m-1286` control before any holdout or BTC run.

The first causal gate rejected `SCORE-EXACT-034`. The unit suite passed, and the
candidate did activate the intended path: exact bundles increased from one to
two and each patrol portfolio gained one exact column. Nevertheless the exact
seed/local bundle remained at 34 servings and the final selected score remained
`6/60/320`, with guidance `6/60/322`, zero invalid/emergency and zero exact
deadline overrun. The high-fuel control, remaining holdout and BTC were not run.
Source/tests are restored byte-identical to `afcd2da`. Supplemental rank/cap may
not be tuned against `m-1285`; the next admissible step is to extract the exact
per-agent/per-spot claim masks from the already observed 35-serving canonical
witness and compare them to the 34-serving exact bundle.

`ATTR-MASTER-035` freezes the authoritative replay at SHA256
`D638BE3D0E136EF94542BBCE58AA6B086D6A7C872732EF27188392D5D19D2F77`
and the exact-valid parent `6/60/321` day-10 plan at SHA256
`787F2C1774D11D08125115FD3F75488A222572CB0CB79692F14A877F5EB81DA4`.
A research-only probe may reconstruct the accepted ledger and day-10 state,
parse that frozen plan and print exact served/denied claims by agent/spot. It may
not call search, alter planner/decision source, infer performance from local time
or tune a successor directly against action bytes.

The attribution completed with exact simulator/validator agreement. Both plans
start day 10 from ledger `6/54/286`; the exact bundle ends at `6/60/320` and the
canonical witness at `6/60/321`. Agents 1 through 7 are byte-identical. Agent 0
alone replaces mask `0x999 = {0,3,4,7,8,11}` with
`0x9D8 = {3,4,6,7,8,11}`. Both routes claim and serve six spots. The exchange
removes one excess claim from spot 0 (stock two, four claims in the exact bundle)
and adds one claim to spot 6 (stock six, four claims), removing exactly one
denial. Evidence is `research/evidence/ATTR-MASTER-035.md`.

`ATTR-ROUTE-036` may now reconstruct the same frozen day-10 state and call only
the canonical agent-0 anytime resource enumerator with the production semantic
parameters: minimum five spots, 32 retained routes, 1250000 settled states and
zero preferred-brand mask. It uses no wall deadline because local elapsed time
has no authority; the fixed work cap is the deterministic boundary. It may
report membership and ranks for `0x999` and `0x9D8` but may not change a rank,
cap, planner or coordinator. This membership result must close before any new
implementation candidate or holdout is opened.

The membership result is decisive. The canonical no-deadline fixed-work probe
is supported but incomplete at exactly 1250000 settled states, with 32 maximal
routes and no supplemental/terminal routes. `0x999` is present at index 1;
`0x9D8` is absent. Because eight retained masks have only five spots, a discovered
six-spot `0x9D8` could not have been evicted by the cardinality-first rank. Its
frozen action path consumes 96 movement steps plus wait four and raw fuel 73
from initial fuel 100, so it is not refuel-dependent. The omission is therefore
cap-limited queue exploration before discovery. Evidence is
`research/evidence/ATTR-ROUTE-036.md`.

`ATTR-POOL-037` must now trace where the unchanged canonical pipeline obtained
the exact-valid `0x9D8` route: initial portfolio, independent candidate, ALNS
candidate-route augmentation or recombination. It permits current source
tracing and one frozen-parent replay-solve attribution run, but no repeated
sampling, local performance inference, source change or enumerator-cap variant.
Any successor must reuse a public, exact-valid route surface and must not route
on `m-1285`, agent 0 or mask bytes.

The one permitted frozen-parent provenance run reproduced the exact 321 witness.
Exact seed/local stayed at 34, exact feasibility made zero improvements over
504176 nodes, final score reached 35, ALNS recorded one improvement and late
recombination recorded zero. This narrows the gain to the one-agent ALNS interval,
but `btc_main` does not print pre-ALNS best, per-operator accepted counts,
synthesized counters or proof-guided counters. `ATTR-POOL-037` is therefore
inconclusive only at the portfolio-column versus synthesized-repair substage;
evidence is `research/evidence/ATTR-POOL-037.md`.

`ATTR-ALNS-038` may extend only the untracked research probe and link it against
the unchanged frozen parent library. It must perform one solve at the canonical
5000 ms cap, require byte equality with the frozen 321 plan, and print the
already existing `AlnsDiagnostics` fields. Local elapsed time is ignored. If the
plan differs or the counters remain ambiguous, the attribution closes
inconclusive and no implementation assumption is allowed.

The wrapper reproduced the exact witness bytes and score `6/60/321`. Regular
ALNS ran 35 iterations, accepted 25 candidates and recorded one improvement;
it generated 66 synthesized routes and accepted 11. `StockMultiVisit` (operator
index 4) was attempted and accepted five times. Proof-guided iterations and
recombination improvements were both zero. These aggregate counters do not
identify whether the winning route was an existing column or synthesized, so
`ATTR-ALNS-038` closes inconclusive on that narrow provenance question. They do
prove a separate schedule gap: guidance remained `6/60/322` while the existing
proof-guided phase, placed after generic ALNS, received no iteration. Evidence
is `research/evidence/ATTR-ALNS-038.md`.

`SCORE-ALNS-039` may open only after hashing
`research/holdouts/SCORE-ALNS-039.csv`. Its sole mechanism is schedule priority:
when the valid official upper bound is lexicographically above the current best,
execute at most one existing proof-guided patrol sweep before generic ALNS, then
continue the unchanged generic loop under the same absolute deadline and
iteration caps. When the score is already at the valid bound, behavior must be
unchanged. No new operator, route rank, map/fuel dispatcher, cap increase or
fixture-derived mask is allowed.

The holdout is now frozen at SHA256
`83CB73EF1A1EB78C841E2AFB966E12FEF511A1F85EB3680EBD14E71428F43E18`.
The first causal gate is only `m-1285` day 10, followed immediately by the
closed-bound `m-1286` day-10 control. The fresh generated and protected lanes
remain unopened until both gates pass. No BTC candidate may run before the
paired local semantic/score gates establish a promotable opportunity.

The first causal gate rejected `SCORE-ALNS-039`. Unit tests passed and the proof
prefix executed as intended: six proof iterations produced 36 routes and
accepted six exact candidates, but improved none. Generic ALNS then fell from
the frozen parent's 35 iterations with one improvement to 31 iterations with
zero improvements; selected score regressed from `6/60/321` to `6/60/320`.
Exact seed/local remained 34 and recombination remained unchanged. The high-fuel
control, all frozen holdout lanes and BTC stayed unopened. Source is restored
byte-identical to `afcd2da`; evidence is
`research/evidence/SCORE-ALNS-039.md`. Do not tune prefix length or placement on
this replay. Proof scheduling may reopen only from a new diverse counterexample
where a specific existing proof repair is already shown to improve before
generic ALNS.

`ATTR-DENIAL-040` freezes the 35-serving witness and enumerates the unchanged
canonical 32-route anytime frontier for every patrol at the fixed 1250000-state
cap, without a wall deadline. Each route may replace only the corresponding
agent path in the frozen plan and must pass both exact engines. The result is an
official-score feasibility oracle, not a performance benchmark. It may not
change caps/ranks or synthesize a target route. No new implementation candidate
may open until this probe proves or falsifies a 36-serving one-exchange.

The canonical-frontier oracle evaluated 192 exact-valid, independently agreed
mutations: all 32 routes for each patrol `0,1,2,3,4,7`. Every per-agent and
global best remained `6/60/321`. Thus no retained exact route can close 35 to
the guidance value 36, and an exchange layer over the current 32-route frontiers
is closed. Evidence is `research/evidence/ATTR-DENIAL-040.md`.

`ATTR-OPTIMAL-041` may run `enumerate_exact_resource_routes` to completion only
for the two unique start/fuel states among agents 2, 4 and 7, the agents claiming
oversubscribed spot 0, with no wall deadline. The initial method call through the
anytime wrapper was not interpreted because that wrapper deliberately forces
`complete=false`; it settled 3148570 states but produced no proof verdict. The
proof API must return `complete=true`, and every inclusion-maximal route must
pass dual exact validation after substitution. Inclusion-maximal masks suffice
for servings because adding a first-visit claim cannot reduce the team total
`min(claimCount, stock)` at any spot. This remains a local mathematical oracle;
elapsed time and work are forbidden as production performance or cap evidence.

The corrected proof run returned `supported=1, complete=1`, 3148570 settled
states and 78 inclusion-maximal routes for each of the two unique claimant
states. All 156 substitutions passed both engines; every best remained
`6/60/321`. The 35-to-36 gap is therefore impossible through any one-patrol
resource-feasible exchange. Evidence is
`research/evidence/ATTR-OPTIMAL-041.md`.

`ATTR-TEAM-042` may enumerate complete frontiers for all unique patrol
start/fuel states and run a deterministic DP keyed only by per-spot claim counts
capped at public stock. It must retain an exact route-choice witness, reconstruct
the full plan with unchanged tanker paths and pass both exact engines. The DP
uses official daily brands then servings; no weighted sum or local timing claim
is allowed. No production candidate may open before this multi-agent feasibility
question closes.

The complete team DP found the guidance score after 38 nodes and seven memo
states. All six patrol frontiers were proof-complete; the reconstructed masks
were agent 3 `0xB59`, agents 0/1/2/4 `0x9D9`, and agent 7 `0x1EC`. The exact
simulator and independent validator agreed on `6/60/322`. This proves a genuine
multi-agent capability gap rather than a loose upper bound. Evidence is
`research/evidence/ATTR-TEAM-042.md`.

`SCORE-QUEUE-043` may open only after hashing its frozen holdout. Its sole
mechanism is to align the capped resource-search priority with the existing
retention objective: greater visited-spot cardinality first, then lower used
steps/fuel and stable label order. The state graph, Pareto dominance, route
reconstruction, 32-route retention, 1250000-state cap, coordinator, exact
engines and high-fuel path remain unchanged. No replay mask, agent identity,
map/fuel dispatcher or cap increase is allowed.

The frozen holdout SHA256 is
`BFFB6312262CC28A21D8131968BF5A603CB37B12EB087472F1D0E2CBDE8AE48B`.
The first gate is the attributed `m-1285` day-10 route/frontier score, followed
by the separate high-fuel `m-1286` closed-bound control. All fresh and protected
lanes remain unopened until those gates pass; BTC is prohibited until paired
local semantic/score evidence is promotable.

`SCORE-QUEUE-043` proved the deep-route mechanism but failed global downside.
On `m-1285` day 10 it changed the representative capped frontier from 24
six-spot plus eight five-spot routes to 24 seven-spot plus eight six-spot routes;
exact seed/local rose from 34 to 37 and final score rose from `6/60/321` to
`6/60/323`. High-fuel `m-1286` tied `6/60/412`, and the fresh general-fixed lane
was `0/18/0`. On the first four BTC-like low-fuel cases, however, seed 957002
threshold-corridor regressed from 346 to 274 while the other three tied. A
tier-3 tail loss of 72 is not bounded relative to a gain of two, so the candidate
is rejected without opening remaining lanes or BTC. Source is restored
byte-identical to `afcd2da`; evidence is
`research/evidence/SCORE-QUEUE-043.md`. Do not tune a mixed priority or weight
against the opened positive and negative fixtures.

`ATTR-QUEUE-044` may run one parent/candidate paired fixture for frozen
BTC-like low-fuel seed 957002 with day details. It may report cumulative/day
official scores, plan hashes and the first differing day only. Frozen binaries
must be used; no queue variation, source change or local timing conclusion is
allowed. Any successor must preserve parent route evidence by construction and
use a new unopened holdout.

The single paired run reproduced parent `6/60/346` and candidate `6/60/274`
with zero invalid/emergency. The plan hash already differs on day 1 while both
score `6/36`; the first official-score difference is day 2 at cumulative
`6/12/74` versus `6/12/72`. Day 3 adds a further 13-serving loss and the
candidate remains lower every later day. This is a full-trajectory divergence,
not a terminal-day localization. Source tracing confirms the production
current-day anytime low-fuel queue is guarded to the terminal day, so the -72
cross-binary tail cannot be assigned directly to the intended queue mechanism.
Evidence is `research/evidence/ATTR-QUEUE-044.md`.

`ATTR-QUEUE-045` may add a temporary research-only queue-policy switch and one
attribution harness. In one binary, generate a shallow-policy prefix through day
9, retain the exact submitted decisions, then instantiate fresh engines and
replay those identical decisions into both so belief/response-ledger history is
equal and route caches are equally cold. On the identical day-10 state, ledger
and traffic, compare shallow versus cardinality-first capped anytime queues in
both execution orders. Only official score, exact plan hash and existing
orienteering diagnostics may decide causality; local elapsed values are ignored.
The 1,250,000-state/32-route caps, exact full-proof path and both validators must
remain unchanged. The research switch must be fully reverted when the probe
closes; no holdout or BTC is authorized.

The same-binary result used state hash `2130807422699737667` and prefix ledger
`6/54/244`. Forward shallow, forward cardinality, reverse cardinality and reverse
shallow all returned exact day score `6/30`, cumulative `6/60/274` and plan hash
`14218929452539799733`, with zero invalid/emergency. This equality is not queue
equivalence: every run reported `exact_supported=0`, `exact_settled=0` and zero
exact bundles. The historical harness constructs `UdonShieldEngine(config)`,
whose default harvest mode is 6; BTC runtime explicitly defaults to mode 7. The
terminal fuel-constrained anytime path requires mode greater than 6. Therefore
the `SCORE-QUEUE-043` generated protected lanes never executed the candidate
mechanism, and their -72 cross-binary tail is not causal downside. Evidence is
`research/evidence/ATTR-QUEUE-045.md`.

`ATTR-QUEUE-046` may change only the two research-harness engine constructors to
explicit harvest mode 7 and future mode 7. It then repeats the already frozen
prefix/fresh-engine terminal fork once. `exact_supported` and settled states must
be nonzero before any policy comparison is interpreted. This remains opened-seed
attribution only; it cannot promote the prior candidate or authorize BTC. Any
successor logic requires a new production-parity holdout because every
`SCORE-QUEUE-043` generated seed has already been opened.

The explicit mode-7 probe used state hash `7024081118606904448` and prefix
ledger `6/54/312`. Both shallow runs selected exact `6/39`, cumulative
`6/60/351` and plan hash `3450257480091618005`; both cardinality runs selected
the same official score with plan hash `108925725917910114`. All four were
dual-valid with zero invalid/emergency and `exact_supported=7`. Cardinality
raised exact local servings from 38 to 39, but shallow's full planner already
found 39 through another path, so the causal official delta on this state is
zero. This disproves the prior -72 attribution without promoting the mechanism.
Evidence is `research/evidence/ATTR-QUEUE-046.md`.

The next admissible implementation experiment must begin from restored
`afcd2da`, correct the research evaluator to explicit production mode 7, freeze
a completely new diverse holdout before queue source changes, and compare
same-terminal-state paired official outcomes. The old mode-6 matrix has no
promotion authority for this axis, and no opened SCORE-QUEUE-043 fixture may
serve as tuning evidence.

`EVAL-PARITY-047` may change only `old/harness/historical_tournament.cpp` and the
stale budget wording in `old/README.md`. The harness must instantiate production
harvest mode 7 and future mode 7 explicitly and print those values together
with exact-orienteering supported/settled/seed/local counters. One already-opened
seed 957002 may be used only to prove the runtime path is entered. Existing
historical score reports remain mode-6 artifacts and must not be relabeled or
rerun as a substitute for forward research. No solver source, candidate score,
holdout or BTC action is permitted in this experiment.

The wiring gate passed. The harness emitted harvest/future mode `7/7`; days 1--9
reported zero exact-supported agents, matching the terminal-only guard, while
day 10 reported seven supported agents, 711197 settled states and exact
seed/local servings `31/32`. Both exact engines agreed and invalid/emergency
were zero. Local score and elapsed values have no performance authority.

`SCORE-QUEUE-048` is frozen at holdout SHA256
`13EC4F536FA573AA3158D35D93CDC5A5D6F0DEBF6572E1D2BDEADDA471122231`.
Its only logic change is cardinality-first pending-label order inside the capped
anytime resource path: greater visited-spot count, then lower steps/fuel and
stable label ID. The exact full-proof path, state graph, dominance, 1,250,000
settled-state cap, 32-route retention, coordinator, current terminal/fuel guard,
simulator and validator remain unchanged. A temporary same-binary policy switch
is allowed only for causal evaluation and must be removed from a final candidate.
Development first compares archived `m-1285` day 10 and the already-opened neutral
957002 control. The frozen screen then covers six general families, deadline and
fixed roles, one/two-tanker low fuel and default/high controls on fresh seeds.
No BTC run is allowed before both screen and full paired gates pass.

The development gate passed causally in one temporary binary. On archived
`m-1285` day 10, shallow runs in both execution positions selected
`6/60/320` with exact seed/local `34/34`; cardinality-first runs in both
positions selected `6/60/323` with exact seed/local `37/37`. All four runs had
six supported agents, exact validation and zero emergency. The official tier-3
gain is therefore `+3` and policy-stable. Local elapsed is ignored. This opens
only the frozen screen; BTC, promotion and commit remain prohibited.

The frozen screen closed at `3/24/0`, with only tier-3 differences, three
independent `+1` gains and no loss tail. Gains span low/default fuel,
one/two-tanker compositions and fuel-tight/threshold-corridor/balanced
families. General fixed/deadline lanes were `0/12/0`; high fuel was `0/3/0`
with identical exact settled counts across policies. Invalid and emergency were
zero. The already frozen full rows are now opened exactly once. BTC and commit
remain blocked until the full gate closes.

The first full general deadline-role execution for seeds 971010--971015 lost
its raw result lines because a PowerShell summary expression was malformed and
printed empty `//` fields. Those synthetic fields have no evidence value. With
the candidate and binary frozen unchanged, the same six already-opened seeds
may be repeated once solely to recapture raw output. No source or gate tuning is
allowed from this instrumentation recovery.

The full frozen matrix is complete at `4/74/0` over 78 unique terminal states.
All first differences are tier 3; gain tail is `+1`, observed loss tail is zero,
and invalid/emergency are zero. Lane results are general `0/36/0`, low fuel
one tanker `2/10/0`, low fuel two tankers `1/5/0`, default fuel `1/11/0` and
high fuel `0/12/0`. Gains span two independent fuel-tight seeds plus separate
threshold-corridor and balanced seeds across one/two-tanker and low/default
fuel. Together with the archived causal `m-1285` gain `+3`, the local semantic
gate supports canonicalization. The temporary switch must be removed and the
single cardinality-first capped-anytime order rebuilt before BTC. No commit is
authorized until BTC target-host validity, telemetry and real-opponent gates
pass.

Canonicalization is complete. The temporary policy API, replay switch and A/B
harness scaffolding are removed. The only production logic delta from `afcd2da`
is 31 lines in `src/orienteering.cpp`; it makes cardinality-first the single
pending-label order only when `minimumSpots` is present. The canonical unit suite
passes, and one canonical `m-1285` replay reproduces `6/60/323`, exact-local 37,
six supported agents, exact validity and zero emergency. Its BTC executable
SHA256 is `CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`.
Local elapsed output is not performance evidence.

The BTC admin statement supplied on 2026-08-09 confirms that response time may
vary by round. Therefore each fresh match configuration remains authoritative
for its outer response window, while the solver's internal compute cap stays at
5000 ms. `POST /practice` with the bot match token returned HTTP 401 because it
cannot create matches; an authenticated BTC UI then created fresh advanced match
`m-1796`: hard, three bots, 10 days, 32x32, 100 steps/day, 5000 ms response,
eight agents, 12 spots, six franchises and low fuel 1x. The frozen canonical
binary ran with harvest/future mode 7/7 and no rebuild or source change.

`m-1796` completed 10/10 valid applied days, reconciled 9/9 authoritative
transitions and independently validated every decision. Final score was
`6/60/312`; zero deadline skip, retry, emergency, exact overrun or hard-cap breach
occurred. Server response p95/p99/max were `2759/2759/2759 ms`, and decision
`totalMs` max was `2699 ms`. Day 10 exercised the candidate mechanism on six
supported agents: seed/local exact route servings were `39/40`, settled states
were `7,500,000`, enumeration plus finalization used `655 ms`, and overrun was
zero. The selected `6/60/312` equalled the master's optimistic upper bound even
though its separate search-guidance bound remained loose at 314. Replay SHA256
is `DCC0A6D36DFB7C2EFECB382ED220B977E37EBA145B25EF291DAF879C079EF272`.
Rank 1 over practice bots is only the asymmetric BTC failure gate. Target-host
validity/lifecycle/hard-cap evidence passes for this match, but the manifest still
reports `competitionReady=false` because `p99Calibrated=false`; this ten-day
sample does not replace calibration or real-opponent evidence. SCORE-QUEUE-048
remains frozen, active and uncommitted.

`ATTR-QUEUE-049` compiled clean parent `afcd2da` in an isolated worktree at
executable SHA256
`AB2556D6436C8AB14F53D6657E2C28FBA9E504DFD4BDF477A80C6CEC960D618A` and solved
only authoritative `m-1796` day 10 with its recorded prefix ledger/state.
Reverse-order repetitions were stable: candidate `6/60/312` with exact seed/local
`39/40`, parent `6/60/307` with `35/35`. Both sides supported six agents and had
zero emergency, overrun or validation failure. This is a fresh `+5` tier-3
development gain, not a new frozen-holdout row; local elapsed is ignored. Only
the terminal state is paired because the prior authoritative trajectory was
generated by the candidate, so no whole-match parent claim is made.

`ATTR-REAL-050` inspected the only two complete archived team-vs-team replays as
non-tunable observational controls. Both are two-team, four-day, 8x8, 32-step,
60-second-outer-window, fuel-64 fixtures with four agents, eight spots and four
brands. Live `m-1038` was `4/16/60` versus `4/16/52`; live `m-1042` was
`4/16/59` versus `4/16/53`. Candidate and clean parent replay-solve terminal
states tied `4/16/60` and `4/16/59` respectively with byte-identical plans,
closed optimistic/guidance bounds, exact validity and zero emergency/overrun.
This `0/2/0` control finds no short/small-map regression but is too narrow to
promote the low-fuel long-horizon queue change. Fresh competition-configuration
real-opponent evidence remains open.

`ATTR-GAP-051` checked whether fresh `m-1796` opens the next current-day score
axis. Days 1--8 have incumbent-to-optimistic serving gaps
`+1,+3,+5,+2,+1,+2,+3,+3`, but every exact-evaluated audit candidate ties the
selected current-day official score; no better feasible witness exists in the
pool. Day 9 optimistic/guidance are closed. Day 10 selected and optimistic are
`6/60/312`, while guidance `6/60/314` is only a loose unwitnessed bound. The
master deadline flag therefore cannot justify another master-width/search tuning
cycle. A post-QUEUE score axis requires an exact-valid higher bundle or another
first-differing feasible counterexample; this is not a ceiling claim.

`ROBUST-QUEUE-052` is the preregistered independent structural-downside audit of
the still-frozen `SCORE-QUEUE-048` candidate. Its manifest was frozen before any
harness change at `research/holdouts/ROBUST-QUEUE-052.csv`, SHA256
`D017068D6F47C73D22164265A235D545A24C2EB3396463264CB7D30AF8AA5359`.
Development contains 18 unopened small-map proof cases across balanced,
duplicate-brand and stock-contention profiles. The one-time holdout contains 72
unopened terminal cases covering low-fuel one/two-tanker roles, depleted default
fuel, a missing-lifetime tier guard and a high-fuel control. A research-only
same-binary policy switch must run shallow and cardinality-first in both orders
on identical state and caps; an exact stock-capped team DP reports official
lexicographic score, while small development cases also compare against complete
resource enumeration. Local elapsed has no authority. Any tier-1/tier-2 loss,
invalid or validator disagreement rejects the candidate; material/systematic
tier-3 downside also rejects it. The holdout may be opened once only after the
harness proves these invariants, and it cannot be retuned.

The `ROBUST-QUEUE-052` development proof passed `0/18/0`. Every capped shallow
and cardinality frontier yielded the same official team optimum as complete
resource enumeration; all repetitions were stable across reversed call order,
and every selected witness agreed between the exact simulator and independent
validator. Cardinality recall of complete maximal masks was never lower than
shallow and was strictly higher on 6/18 cases, without changing the optimum on
these small maps. This proves the harness/oracle is coherent but is not promotion
evidence. The 72-case holdout remains unopened at this point; local elapsed was
not recorded or used.

The `ROBUST-QUEUE-052` holdout was then opened exactly once and closed accepted
at `4/67/1` over 72 independent terminal fixtures. All five differences were
tier 3: gains `+1,+2,+1,+4`, with one `-1` loss. Low-one-tanker was `2/22/0`,
low-two-tanker `0/11/1`, depleted-default `2/10/0`, missing-lifetime `0/12/0`,
and high-fuel control `0/12/0` with identical frontier signatures. All 72 cases
were stable across reversed call order and dual-valid. The evidence file
`research/evidence/ROBUST-QUEUE-052-holdout.csv` has SHA256
`6B1DF3B756BF54EE00C25A595623F6814819AD957266F472FE137B4BACDA2EFD`.
The sole regression is bounded and not repeated across the second balanced
two-tanker seed; the larger and more diverse positive tail retains
`SCORE-QUEUE-048`, but does not authorize commit or replace BTC/real-opponent
final gates.

`ATTR-QUEUE-053` was opened as a bounded attribution. It inspected only the
already-opened low-two-tanker balanced seed `982008` and reported the shallow,
cardinality and union-frontier exact team witnesses. It did not change queue
priority, retention, caps, fixture or metric, and cannot tune against this loss.

`ATTR-QUEUE-053` closed accepted. Shallow's `6/60/96` witness requires patrol 4
mask `0x27C`, which is absent from that patrol's cardinality frontier; the
cardinality witness therefore stops at `6/60/95`. The union of both frozen
frontiers restores exactly `6/60/96` using the same shallow-only patrol-4 mask
and does not exceed it. Thus the one-serving loss is a real capped-frontier
trade-off, not DP or validation noise, but one isolated opened seed cannot
justify a mixed queue or dispatcher. Evidence is in
`research/evidence/ATTR-QUEUE-053.txt`.

`ARCHIVE-QUEUE-054` is the sole active non-opponent logic audit. Before any
replay-adapter change, one complete replay per unique 32x32, 10-day, 8-agent,
12-spot, six-brand BTC match was frozen in
`research/holdouts/ARCHIVE-QUEUE-054.csv`, SHA256
`FC874C1CCDC9785EE3D178AE31C3E4262B4B54913CB421E6B856D30A3655FB3D`.
The manifest contains 75 exact file hashes: already-opened `m-1285` and
`m-1796` are development, while the 73-case holdout contains 17 low-fuel, 36
default-fuel and 20 high-config matches. Ten high-config terminal states have no
fuel-constrained patrol and are whole-policy controls; the other ten contain one
or two depleted patrols, so only their per-agent high-fuel paths are controls.
This classification was derived from manifest metadata before source change.
Selection did not inspect score or rank.
The adapter must reconstruct every accepted prefix ledger and the terminal
authoritative state with dual validation before the existing same-binary
shallow/cardinality team oracle may run. Rank and local elapsed have no
authority; high-fuel policy signatures must be identical. The holdout opens
once only after both development replays reproduce coherent terminal evidence.

The `ARCHIVE-QUEUE-054` adapter proof passed `2/0/0` and reproduced the two
previously established independent deltas exactly: `m-1285` shallow/candidate
`6/60/320 -> 6/60/323` (`+3`) and `m-1796` `6/60/307 -> 6/60/312` (`+5`).
Both were stable under reversed policy order and dual-valid after reconstructing
the accepted nine-day prefix ledger. This is an adapter/oracle gate, not new
promotion evidence. The 73 replay holdout remains unopened at this point.

The `ARCHIVE-QUEUE-054` holdout was then opened exactly once and closed accepted
at `18/55/0` over 73 unique authoritative BTC terminal states. All differences
were tier 3 and every delta was positive: gains ranged from `+1` to `+4`, with
no loss. Low fuel was `5/12/0`, default fuel `12/24/0`, and high-config
`1/19/0`; wins span role masks `1,4,16,32,64,128,192` and terminal constrained
patrol counts `1..7`. All 73 cases were stable and dual-valid. The ten states
where the queue policy was unreachable had byte-identical signatures and score.
Evidence SHA256 is
`B8C3F78B702BB84BCE47592C6CD91F41D56329EBB464C7946875025B2B4B0776`.

Across the three independent frozen non-development matrices,
`SCORE-QUEUE-048` now has aggregate `26/196/1` over 223 same-state pairs. Every
difference is tier 3; frozen gain tail is `+1..+4` and the only loss is `-1`.
This is broad non-opponent evidence with bounded downside, not a weighted-sum
promotion and not proof against real opponents. The candidate remains frozen,
active and uncommitted because target-host p99 calibration and diverse fresh
real-opponent evidence are still final gates.

`PERF-P99-055` is the sole active non-opponent final gate. Its manifest was
frozen before creating a match at `research/holdouts/PERF-P99-055.csv`, SHA256
`52ADA0B0ECBF3ADE92381D2B4AF8926578D21FFF9433795B71A25A94BC4F225B`.
It preregisters 30 fresh explicit-advanced BTC practice matches: ten each at
low, default and high fuel, all hard/three-bot/10-day/32x32/100-step/5000-ms/
8-agent/12-spot/six-brand. The unchanged executable SHA256 must remain
`CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`.
Exactly 300 fresh decisions are required. With zero hard-cap breaches, the
one-sided 95% binomial upper bound on breach probability is
`1 - 0.05^(1/300) = 0.994%`, which is below 1%. Any invalidity, transition or
validator mismatch, skip/retry/emergency/overrun, hard-cap breach or binary drift
rejects calibration. Bot rank is not quality evidence. Interrupted work may
resume only the still-unfilled preregistered replicate count; failed matches are
not replaced or hidden.

`PERF-P99-055` progress: all ten preregistered low-fuel replicates `m-1803` to
`m-1812` are complete. The 100/100 decisions were HTTP-valid and dual-valid,
90/90 observable transitions reconciled, and every explicit advanced config
matched the manifest. There were zero fallback actions, emergencies, exact
overruns or hard-cap breaches. The worst per-match server-response maximum was
`3965 ms` and the worst decision timing was `3358 ms`; final quantiles remain
deferred until all 300 samples are complete. Per-replay hashes and summaries are
in `research/evidence/PERF-P99-055-progress.csv`. This is progress only; the
calibration gate remains open at 100/300 fresh decisions.

All ten preregistered default-fuel replicates `m-1813` to `m-1822` are also
complete. BTC materialized the UI default profile as `fuelLimits=200`; all
other explicit advanced fields matched the manifest. The 100/100 decisions
were HTTP-valid and dual-valid, 90/90 transitions reconciled, with zero
fallback, emergency, exact overrun or hard-cap breach. The worst per-match
server-response maximum was `3574 ms` and the worst decision timing was
`3514 ms`. The high-fuel lane then completed all ten frozen matches, but exposed
three submission skips: `m-1827` day 6 and `m-1832` days 6-7 recorded
`actions_deadline_skip` plus `actions_server_wait` and never POSTed the valid
plans. Consequently high fuel returned only 97/100 accepted action results and
BTC exposed tier-2 daily sums 57 and 52 in those two matches. Across the 297
accepted responses, p50/p95/p99/max was `2989/3575/3965/4328 ms`, but these are
censored by the skipped submissions and cannot calibrate p99. `PERF-P99-055` is
closed rejected. Progress evidence SHA256 is
`8DCDAE465FB065FA0E0CEACFB78B906BA43BE82F37C950B9B7E963136B90976A`;
attribution evidence SHA256 is
`7F8628EC949D36F50E685F8BC719C1B0D189A4226B2EFF7F34DD4BBFDF81A7AF`.

The confirmed non-opponent gap has two aligned deadline defects. First,
`deadline_seconds` validates but ignores raw server `endsAt` and floors
`(receivedMs + responseBudgetMs) / 1000`; the affected raw windows
`5030/5198/5032 ms` became only `4030/4198/4032 ms`, losing
`970/802/968 ms` against the configured 5000 ms cap. Second, certification did
not consistently stop at the compute boundary implied by its existing 1600 ms
network reserve: planner timing consumed `3264/4171/3678 ms`, leaving only
`760/25/349 ms` at the 800 ms submission guard. Full decision replay recording
took only 2-3 ms and is not the material cause. The next experiment must align
planner and POST to one exact millisecond deadline capped by both the configured
5000 ms and raw server deadline, then enforce the existing reserve without
reducing solver logic or hiding skips.

`DEADLINE-ALIGN-056` froze exact deadline plumbing before source change. Its manifest
was frozen before source change at `research/holdouts/DEADLINE-ALIGN-056.csv`,
SHA256 `6D6270CEBBD12D96ACDBF779A8298466A0378793F2173C25D8514A3E843DAC3F`.
The candidate may only plumb one exact HTTP deadline equal to
`min(raw server endsAt, receivedAt + configured response budget)` into both the
planner and POST path. It may not change search/certification logic, the 1600 ms
network reserve, the 800 ms submission floor, action durability or recovery.
Exact alignment is tested first as the minimal general fix. If any of five fresh
high-fuel development matches still skips, this candidate is rejected rather
than weakening the guard; certification boundedness must then be a new,
separately frozen mechanism.

The candidate is frozen as BTC executable SHA256
`4636C380578F1AB79973A25E03AAE26E20809F8A5720F914D7DB7F38D1C3EFD6`.
The unit suite passed once. The parent and candidate replay-check outputs were
byte-identical across all 30 `PERF-P99-055` replays and all 300 recorded days,
with zero failure or mismatch. This establishes offline action/state/validator
equivalence only; local elapsed time is deliberately excluded. Evidence SHA256
is `54F2C1EE87964495ACA50F317A400072CBA75829D5C0554FF27E3D34D561D854`.
No fresh `DEADLINE-ALIGN-056` BTC development sample was consumed. The
candidate is closed inconclusive rather than promoted or rejected: exact
deadline alignment is a necessary protocol/runtime component and passed offline
equivalence, but `ATTR-CERT-058` proved a separate certification boundary hole
that this candidate deliberately leaves unchanged.

While BTC practice creation was unavailable, `ATTR-SELECT-057` completed an
independent read-only attribution over the already-opened 30-replay
`PERF-P99-055` set. The scan found zero non-selected candidate with a higher
`scoreAfterToday` and zero accepted exact bundle above the selected current-day
score. Two apparent lower-bound improvements (`m-1803` day 8 and `m-1804` day
9) lose current official score and are correctly excluded by the production
current-floor gate. The remaining q50 improvements tie current score and lower
bound but q50 is only third in the full q95/q80/q50/q20/q05 comparator (or the
candidate is outside the relative confidence gate). None is a complete-tuple
domination witness. This closes the evaluator axis without a source change;
evidence SHA256 is
`DAEF6B1842D8EDE357FA23BE2DA7DBD68CBFDE3DE572641C2718C83C8CC01F8F`.

`ATTR-CERT-058` is an accepted independent read-only target-host attribution and
does not modify the frozen `DEADLINE-ALIGN-056` candidate. In all 42/300
decisions that crossed the scheduler compute boundary, pre-certification still
ended before the boundary; certification alone crossed it. Counts by
low/default/high fuel are `0/5/37`; overrun p95/max is `405/1574 ms`, and the
three submission skips are the three largest tails. The first boundary hole is
dense initialization in `enumerate_exact_high_fuel_routes`: after one deadline
poll, each worker initializes at least 36 MiB over 4,194,304 states before the
next poll, and four joined workers can touch at least 144 MiB. The general
semantics-equivalent successor is same-sentinel chunked initialization with the
existing absolute deadline polled between chunks. Evidence SHA256 is
`6DF72E0342AA141A39DD8DA1D030D72FFB05293BBCD8E0FACF08D5E81C9151C8`.

`PERF-CERT-059` is now the sole active implementation experiment. It retains
the frozen exact-deadline plumbing and changes only dense exact-state
initialization to construct identical sentinels in fixed 65,536-state chunks,
polling the existing absolute deadline between chunks. At BTC scale this bounds
state initialization between polls to 576 KiB per worker and 2.25 MiB across
four workers, without changing graph/order/caps/worker count or the result of a
completed search. Its manifest was frozen before source change at
`research/holdouts/PERF-CERT-059.csv`, SHA256
`E9A6BF19348A1AE04325E59F9CAEC1552FE4690A5506C2EC2040F018D66D2B20`.
Local elapsed time has no promotion authority.

The implementation is frozen as BTC executable SHA256
`83EF15BE2139705ACE27B08727ED18074C8B9305E8FBD52566000BCC207B47E0`.
The authoritative cached toolchain built the candidate once and the full unit
suite passed. Against the frozen `DEADLINE-ALIGN-056` executable, replay-check
stdout is byte-identical for all 30 `PERF-P99-055` replays and all 300 recorded
day states: 10/10 low, 10/10 default and 10/10 high replay files matched, all
replay hashes matched the frozen progress evidence, all processes exited zero,
and there were zero invalid actions or validator disagreements. This clears the
offline semantic gate only; it says nothing about local performance. Evidence
SHA256 is
`394FF66499700E060B09BCFD299109045CCE8CA8610B794CBB87DE2F7CE54C75`.
The follow-up source-boundedness audit proves that sentinel construction between
polls is now at most 576 KiB per worker (2.25 MiB across four workers) at both
BTC scale and the architecture state limit. Main traversal and terminal scans
already poll by bounded entry/mask/cell intervals. Individual allocator calls,
the small pre-traversal metadata pass and bounded simulator/validator calls
remain explicit observation points rather than speculative patch targets.
Boundedness evidence SHA256 is
`51856B408CE1A5C63FE9CEEA22BB0C6615E2F3AFBD80CB58578106E132946CDD`.
BTC practice creation recovered and the first frozen high-fuel development
fixture was consumed as `m-1836`. It used hard/3 bots/10 days/32x32/100 steps/
5000 ms/8 agents/12 spots/6 brands/high fuel. The binary produced 10/10 valid
actions, replay-check confirmed 9/9 authoritative transitions and exact score
`6/60/407`, and there were zero emergency decisions. Rank 1 is ignored.
Nevertheless, certification crossed the `total - 1600 ms network` compute
boundary on days 3, 6, 8 and 9 by `363/47/307/218 ms`; pre-certification stayed
inside the boundary on every day. Current-day exact orienteering also overran
its own supplied deadline by 42 ms on day 4. This fails the preregistered zero-
crossing development gate, so the balanced BTC holdout remains unopened and
`PERF-CERT-059` is rejected as a promotable candidate. Replay SHA256 is
`B866ACB6A63C548D8BE05FBED03DDBD355422F78566CAC436A1C2E5BD6AC6676`;
BTC evidence SHA256 is
`DCD0C051D782B247CD3ADABA241DF0417CB1ABD003C8B3AFD09D6A26197A4419`.
The chunk mechanism remains only as a frozen diagnostic parent: offline
semantics are byte-identical and the first large initialization interval is
removed, but no successor may be promoted until the next crossing operation is
independently attributed. Do not tune chunk size or deadline guards from this
single match.

`ATTR-CERT-060` was opened as the sole certification-attribution experiment. The
separate frozen score candidate `SCORE-QUEUE-048` remains active pending its
runtime/real-opponent gates; `ATTR-CERT-060` neither changes nor reevaluates its
score evidence. This experiment is audit-only telemetry on the frozen rejected
`PERF-CERT-059` diagnostic parent, not another solver candidate.
It will time the existing future-witness phases and record the first operation
ending after both its candidate slice and the shared global certification
deadline, including nested exact-orienteering diagnostics. It may not change
control flow, deadlines, caps, candidate/scenario order, actions or score logic.
The manifest was frozen before source instrumentation at
`research/holdouts/ATTR-CERT-060.csv`, SHA256
`FA868D82D6880A408DC5C146293A4194EDCD59E716347787024C6D27CF78D402`;
design evidence SHA256 is
`4CC31C75472D5D51DDE481B3F3B1F8BD7526C536FD5250A111771C5200AF915B`.
Before any BTC diagnostic run, replay-check stdout must remain byte-identical to
the frozen `PERF-CERT-059` binary on all 300 states. A logic successor requires
a new manifest after attribution; no threshold or reserve change is allowed.
The audit binary is frozen at SHA256
`2A1BDA4F9E67FC3EC70F3A4144AA14145B9DA5DF64F5DBE29C5B45CC3F203C04`.
The full unit suite passed and replay-check stdout is byte-identical to the
frozen `PERF-CERT-059` binary on all 30 low/default/high replays and all 300
states, with correct replay hashes, exit zero, zero invalid and zero validator
disagreement. Offline diagnostic-integrity evidence SHA256 is
`183043B7992E1DA2359C965CE7D96DEE05B42F9C446FAFA9FD1236B2578D77BA`.
The three fresh high-fuel BTC diagnostic fixtures were consumed without
replacement; their ranks are irrelevant. `m-1837` and `m-1838` each completed
10/10 HTTP 200 valid actions and 9/9 reconciled transitions. Replay-check rebuilt
`6/60/440` and `6/60/384`; replay SHA256 values are
`7325885ADEDC16356CF1D4A694C563A994EE532335B9A30872564B5DA56E4C85` and
`C2D84D6A48132EDD8208C133A8EB0F858321FBC177F1CF6AE0CAA34A91AD2017`.
`m-1839` stopped after setup with no day state/decision/action and was not
replaced. Across the 20 complete decisions, every action was accepted and no
solver total exceeded either its exact authoritative deadline or the 5000 ms
hard cap. The diagnosed events are crossings of the earlier 1600 ms network-
reserve boundary, not invalid responses or hard-cap breaches.

Every certification-caused global crossing ended in column generation.
Material `m-1838` days 4/5/6/9 spent `1394/879/802/791 ms` in nested exact
orienteering and crossed globally by `381/250/23/69 ms`; maximum nested exact
overruns were `609/250/254/69 ms`. Days 1 and 7 first crossed in monotone-floor,
but certification consumed only 1/0 ms because pre-certification had already
used the reserve; they are not certification regressions. Current-day day 1
independently spent 252 ms enumerating and 2485 ms finalizing exact routes,
overran its supplied exact deadline by 1756 ms and visited 3,000,173 feasibility
nodes.

The source cause is exact: `capacity_feasibility_search` polls every 1024 nodes,
but deadline expiry returns ordinary `false`, indistinguishable from an
infeasible subtree. Parent recursion then continues sibling routes, so repeated
expiry polls prune only one subtree at a time and traversal can reach the
3,000,000-node phase cap. `ATTR-CERT-060` is accepted attribution. Evidence
SHA256 is
`6D762D6C3D19DEF3B77B08ABB4F4BD404D228A83D79ECCBE0F693DAE108B47A1`.

`PERF-CERT-061` is the sole active runtime implementation experiment. Its
manifest was frozen before source change at
`research/holdouts/PERF-CERT-061.csv`, SHA256
`D798DD62F7AFA090F2E2801200336B8A44768554569F7AAD50D792EA9057CD9E`;
design evidence SHA256 is
`FF42323F9C88D56967899695E92AEEA8483AD73F80957AB55E50320D41726C78`.
It may add only explicit cancellation propagation after the existing absolute-
deadline poll, restoring recursion state while unwinding and retaining the best
completed incumbent. It may not change poll cadence, route order, bounds,
memoization, node caps, beam, worker count, reserve, submission guard or score
logic. This does not remove, disable, defer or reduce designed functionality;
no code is deleted. It repairs the designed deadline contract and preserves all
exact work before expiry.

The candidate is now frozen as BTC executable SHA256
`F452AA3936801A68C7E1209D95B5E1724AF8484F480BE44F6475B8677A22039F`.
The cached MSVC/CMake/Ninja toolchain built the test and BTC targets and the full
unit suite passed once. Against the frozen `ATTR-CERT-060` audit parent,
replay-check stdout is byte-identical on all 30 preregistered low/default/high
replays and all 300 states; every process exited zero with zero invalid action
and zero validator disagreement. Local elapsed time is excluded. Offline
evidence SHA256 is
`F8AB1787E4400C72416D860A1F854FE87EF88E00C91EB2AE3BCFDA3CB0BCC4EA`.
The next and only open gate is one fresh high-fuel BTC development fixture with
this exact binary. The balanced target-host holdout remains closed until that
development fixture has zero material certification crossing, no POST skip,
zero hard-cap breach, zero emergency and complete transition reconciliation.

Fresh high-fuel BTC development fixture `m-1862` used the exact registered
hard/three-bot/10-day/32x32/100-step/5000-ms/eight-agent/12-spot/six-brand/high-
fuel configuration. The frozen candidate produced 10/10 HTTP 200 valid actions,
reconciled 9/9 transitions and replay-check rebuilt `6/60/410`; rank is ignored.
Maximum solver total was 2339 ms, remaining authoritative window at POST was at
least 1604 ms, and there were zero emergency, retry, server WAIT, no-POST or
hard-cap events.

The prior material future-certification tail did not recur. Days 1--9 ended
their global certification boundary by only 2--7 ms, and maximum nested exact
overrun was 1--9 ms, structurally bounded by the existing 1024-node poll block
plus recursive restoration/unwind. Current-day exact separately overran its
own slice by up to 166 ms, but the whole planner still retained the complete
1600 ms network reserve; this remains observable rather than being mislabeled
as fully closed. Replay SHA256 is
`693DC792B9419937DA455BFCDEEFF89684BB8EED2C529F082DD8BC29AF6ADEC8`;
development evidence SHA256 is
`9D4E1D7F40C1863EE9F6575FF98B3A4B959DA8C302E9FE19FCBF8C0EF4978187`.
The development gate passes and the unchanged binary may now open the frozen
balanced low/default/high BTC holdout exactly once.

The balanced holdout was opened once without replacement. Low-fuel `m-1863`
passed 10/10 valid and 9/9 reconciled with zero emergency/crossing/no-POST,
solver max 1915 ms and at least 1618 ms remaining at POST. Default-fuel
`m-1864` produced seven valid decisions, then the HTTP client terminated during
receive with WinHTTP 12002. Immediate same-match resume retrieved only the final
result, so days 8--10 were lost. High-fuel `m-1865` produced ten decisions and
nine accepted actions; authoritative day 2 arrived with only a 1380 ms exact
window, already below the unchanged 1600 ms reserve, causing one emergency and
one `insufficient-authoritative-day-window` server WAIT.

Aggregate holistic result is 26/30 accepted actions, 24/24 reconciled observed
transitions, one emergency and four missing POSTs. Replays for low/default/high
are `BE7E3753B95C3A566EA03DAC2592CBF02A36D9059A7FDDC5655EFD1F11198C2E`,
`B4F187FD1D1B98DAD4FF8A6016131528A64DE2B497639AF03AA0DBD8E7834880`
and `F2F28156A9CA2F84A427E61F333017ADEE9766466454C924D5FA92917808C9FC`.
The cancellation mechanism itself remained clean on all 27 decisions reaching
the solver: global certification overrun max 6 ms and nested exact max 10 ms,
with no tail moved to another future-witness phase.

`PERF-CERT-061` is therefore rejected as a promotable composite and cannot be
committed. Its cancellation repair is retained only as the frozen
semantics-equivalent research parent because the causal target passed. The new
independent gap is HTTP state reception: a receive timeout can terminate the
whole match, and a state can arrive after the solver's 1600 ms reserve is already
unavailable. Holdout evidence SHA256 is
`53541BE0FD0ED26EAB77DBD0A94BBF67AB7CFBD6B27CC49AA7F12A6F58E82745`.
No reserve, submission floor, map, fuel or bot threshold may be tuned from these
fixtures.

`ATTR-HTTP-062` is accepted read-only attribution. `WinHttpClient` gives every
request a session-default 5000 ms receive timeout unless the caller supplies a
shorter slice. The accepted action path supplies a bounded slice and catches
retryable timeout/resend exceptions, but `wait_for_get` catches no transport
exception and the main `/state` and `/result` callers use raw default-timeout
GETs. Therefore one receive 12002 escapes `run_http` and terminates the process,
exactly matching `m-1864`. All `/setup`, `/start`, `/state` and `/result`
operations are idempotent GETs but are not wired to a common retry contract.

`m-1865` wire day 1 was recorded with exactly 1379 ms before raw `endsAt`, which
proves the late-state counterexample but not its exact transport cause because
request-start telemetry is absent. It is only consistent with the same unbounded
GET receive interval. Evidence is `research/evidence/ATTR-HTTP-062.md`.
No source, solver, reserve, submission floor or timeout changed in this
attribution. A separately frozen successor may add bounded retry across every
idempotent GET caller; it may not lower the 1600 ms reserve or 800 ms guard, tune
against `m-1864`/`m-1865`, or suppress idle post-ACK work.

`PERF-HTTP-063` is preregistered before source change. Its frozen holdout is
`research/holdouts/PERF-HTTP-063.csv`, SHA256
`FB5C49FF5CCDC2AAD89C56099BFE4F17ED0731920AC59E131DAA783EAD1F6F40`;
design evidence SHA256 is
`C1748922EC660E514CF55280F03F765CE7C2B0D09113B953B446DF29974F3DCE`.
The candidate may reuse only the pre-existing 750 ms transport slice and the
existing timeout/resend classification. It must wire `/setup`, `/start`,
`/state` and `/result`, preserve transient-status handling and idle post-ACK
work, and leave every POST, solver, action, ledger, reserve and guard unchanged.
The first gates are source proof, frozen replay equivalence and one controlled
localhost receive-timeout recovery. Only then may one fresh default-fuel BTC
development match be consumed; the low/default/high holdout remains unopened.

`PERF-HTTP-063` passed the unit suite and the frozen 30-replay/300-state
byte-equivalence gate. Its frozen BTC executable SHA256 is
`21D9C7E6BC606C80E1B12A3886745006DE524380612E2235306C372B4EE07B6B`.
The corrected controlled fixture completed exactly one action, one accepted ACK
and one result, but a 3000 ms first-state response-header stall produced zero
`state_transport_retry`. Therefore `WinHttpSetTimeouts` alone did not implement
the declared header-wait bound, and the candidate is rejected before BTC or its
holdout. Evidence SHA256 is
`00FF3AFF0F7AB2E0B518A20F2B31A2A1EC3A420D559A2909DF4D0554869FAE12`.

Official WinHTTP documentation identifies
`WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT` as the request option that specifically
bounds receipt of all response headers; its default is 90 seconds.
`PERF-HTTP-064` is preregistered before adding that option. Its sole new mechanism
sets the option to the same already-frozen positive `ioTimeoutMs` on requests
that already have a bounded slice. Assignment POST remains unchanged at timeout
zero; existing action ACK recovery still permits only identical-body resend, and
GET recovery remains idempotent. Frozen holdout SHA256 is
`517A3CAFE052C25E1340ECA0C46A2601130FF02789DF97174679B93201C08781`;
design evidence SHA256 is
`841D56D3B63A8CF7A5F1FF6E569949BA8C820EE0B4E06285F4CC3E24451416A8`.
No 750/1600/800 ms value, poll interval, solver, action or ledger logic may
change. The corrected controlled header-timeout fixture is the first gate; BTC
development and the balanced holdout remain unopened.

`PERF-HTTP-064` is rejected. Its frozen binary SHA256 is
`ECB20530412264CF5C4DCCC1340A163D447272A9AB28610FF7ED87A1C5AF6E85`.
The unit suite passed once and all 30 frozen replays/300 states were
byte-identical to 063, but the corrected 3000 ms silent-header fixture again
produced zero `state_transport_retry` while completing exactly one action, one
accepted ACK and one result. No BTC match was opened. The GET retry and header
option are removed from final source. The natural WinHTTP 12002 remains one
sample after 30 prior fresh BTC matches on the same GET path produced none;
transport may reopen only after repeated target-host evidence under a stable
network, not another synthetic timeout variation.

`FINAL-QUEUE-065` is preregistered as the only final candidate before its clean
binary is built. Frozen manifest SHA256 is
`05BCA7A18146284615B367AE84EA687BBA37557A25336950F34A20C409CD0AF3`.
Its score authority remains the 223 independent frozen pairs from
`SCORE-QUEUE-048`: aggregate `26/196/1`, every first difference tier 3, gain
tail `+1..+4`, only loss `-1`, zero invalid/emergency. The final cumulative
runtime changes must be byte-identical to frozen SCORE-QUEUE-048 on all 30
PERF-P99 replays/300 states before opening exactly twelve fresh BTC recurrence
matches: four low, four default and four high fuel, with no replacement. Local
elapsed is excluded. No additional score research is permitted during closure.

The clean final build and unit suite passed once. Frozen BTC executable SHA256
is `8A8AD48AD5A9227AA43C9FF685D20CE7CA1EF657EBCDFC2A675C6FEA9A16816D`.
Against frozen canonical `SCORE-QUEUE-048`, replay-check stdout was
byte-identical on all 30 preregistered replays and all 300 states with zero
failure. Design/offline evidence SHA256 values are
`ED27742A75F4C67199803D7ED8EF9838EB6B2ABCFF114DFBDC2294936E37B180` and
`6DA98FC48FEB2103778CF4E7DC7AE77F33DC493733470A222999C4B0A7317BA0`.
This opens only the frozen twelve-match BTC recurrence row; source and binary
must not change during it.

The twelve-match target-host row is complete and accepted. All twelve explicit
advanced setups matched the frozen manifest. Aggregate results are 120/120
HTTP-valid accepted actions and 108/108 independently replay-reconciled
transitions, with zero emergency, deadline skip, server WAIT, fallback,
transport retry, WinHTTP 12002 and 5000 ms breach. Maximum decision `totalMs`
was 2753 ms. Maximum exact-orienteering deadline overrun was one bounded 6 ms
tail in high fuel with no submission effect; all other matches recorded zero.
BTC evidence CSV/MD SHA256 values are
`92990A8D7113CA9CA72597A6750504196E4F6DDFB10F2D00ADD7B294143060FF` and
`8F8511E305405703D44B439F461D04676055A10E05BCC138C0AB0DE1BEDD6B21`.
Rank is ignored. The same frozen binary entered the real-opponent queue once;
no second queue entry or new logic axis is authorized during closure.

The fresh real-opponent queue entry expired with no other team and created no
match, so no live human result was lost or silently omitted. Archived human
terminal controls remain `0/2/0`. This unavailable non-paired sample cannot
promote the candidate, but it also cannot override the causal promotion
authority: 223 frozen paired rows at `26/196/1`, broad tier-3 gain `+1..+4`,
only one `-1` loss, zero invalid/emergency, 300-state cumulative semantic
equivalence and the clean twelve-match target-host gate. `FINAL-QUEUE-065` is
therefore accepted and commit-authorized. This is a practical promotion, not a
claim that diverse fresh human opponents or the entire architecture have
reached a mathematical ceiling.

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

`SCORE-ROLE-022` investigated fresh BTC `m-1264`, which selected one-tanker
mask128 and scored `6/60/261`, while same-budget production counterfactuals on the
same replay reached `6/60/326` with two-tanker mask192 and `6/60/330` with mask144.
The live mask counterfactual exactly reproduced `261`, so this is not a budget
artifact. On independent low-fuel replay `m-1255`, current one-tanker mask64 scored
`6/60/286`, while two-tanker masks80 and48 scored `313` and `305`. A blanket
two-tanker rule is false: on `m-1241`, one-tanker mask32 scored `331` and the best
two-tanker rollout mask96 scored only `282`.

The preregistered discriminator is visible before exact counterfactual score. On
`m-1264` and `m-1255`, the best one- and two-tanker incomplete rollouts tie on
lifetime and daily distinct; on `m-1241`, two tankers already lose daily distinct
`56` versus `58`. `SCORE-ROLE-022` may prefer the best existing two-tanker beam
member only for long-horizon low-fuel matches when evidence is incomplete, at
least one patrol remains per brand, and its rollout ties the best one-tanker on
the first two official tiers. It preserved all other paths and did not inspect
seed, match ID or map family, but it is rejected because the production
`beam-width 8` never retained a two-tanker assignment. The apparently successful
development attribution used beam width 16. On the frozen screen, all six short
horizons were exact role/score ties; all four low-fuel cases kept the same role
mask, with two raw local tier-3 cutoff deltas of -4 and -2 but causal `0/4/0`.
The unopened archive probes `m-1252`, `m-1138` and `m-1134` likewise contained no
two-tanker member at width 8. The source and unit-test candidate were fully
reverted; no production change is retained.

The next gap is therefore beam diversity, not the fallback comparator. Timed role
selection spends its probe phase only on all-patrol and at-most-one-tanker masks;
those refined masks fill the eight production slots before scanned two-tanker
masks can enter. Any successor must preserve the caller-visible beam width and the
same 5000 ms deadline while giving a bounded comparison pool enough tanker-count
diversity for the already demonstrated m-1264/m-1255 counterfactual. It must still
reject m-1241 through a pre-score lifetime/daily discriminator.

The active candidate is `SCORE-ROLE-023`, preregistered before source changes at
holdout SHA256 `B2F0A6E8532F0CEFA8408BAF5A6378133FBCFE441943D6C99D9091799081676A`.
For only long-horizon low-fuel configurations where two tankers leave at least one
patrol per brand, the internal comparison pool may expand to at most the smaller
of all masks and `max(requested width, 2 * agent count)`. It receives no extra
time: the existing rollout deadline is divided across the larger pool. After the
same tier-preserving fallback, the vector is reduced to the caller's requested
width before return. This composes the causal beam16 evidence with the rejected
candidate's strict lifetime/daily equality guard without changing public width or
the 5000 ms hard cap.

`SCORE-ROLE-023` passed its frozen matrix and the BTC final gate. Fresh low-fuel
results were `4/1/3`, all at tier 3: gains `+9,+1,+46,+5` totaled 61, losses
`-3,-23,-8` totaled 34, with tails `+46/-23` and zero invalid/emergency. The six
short-horizon cases were five exact ties plus one raw timed crossover gaining one
serving while the candidate branch was disabled. All eight default/high cases
kept the exact parent role masks with the candidate branches disabled; their raw
local cutoff deltas are retained in the experiment log but the causal result is
`0/8/0`. No protected case regressed lifetime or daily distinct.

Fresh explicit-advanced BTC `m-1266` used hard difficulty, three bots, ten days,
32x32, 100 steps/day, 5000 ms, eight agents, 12 spots, six brands and low fuel.
The candidate selected mask1 and finished rank1 at `6/60/228`; the next bot was
`6/59/158`. Parent `5599e76` selected mask4 on the same setup, whose exact
counterfactual was `6/60/220`, so the live candidate gain is causal `+8` at tier3
in addition to the tier2 victory over bots. Target role wall time was 4641 ms;
action p95/p99/max was 3178 ms and maximum recorded decision time was 3105 ms.
All 10 submissions were HTTP 200 valid, all nine transitions reconciled, and
emergency count was zero. Replay SHA256 is
`11F037E67D344D14D0E9A8AA9230D4DC482D3A5272DA764732C340980F690649`.
The candidate is accepted as the new global production champion.

`SCORE-ROLE-024` started from parent `08771f1`. On BTC
`m-1266`, the best retained two-tanker mask192 scores exact `6/60/275`, which is
47 servings above the live champion mask1, even though its incomplete rollout is
`6/48/176` versus `6/51/192`. A daily-gap threshold alone is unsafe: on m-1241
the best two-tanker rollout is only two daily distinct behind (`56` versus `58`)
but loses 49 exact servings. The pre-score discriminator is patrol-normalized
tier-3 productivity. Cross-multiplication gives `176*7 > 192*6` on m-1266 and
`160*7 < 193*6` on m-1241; it also preserves the accepted equal-daily m-1264 and
m-1255 cases. The candidate may relax exact daily equality by at most one full
brand-day only when two-tanker servings per patrol are no worse. This is a
conditional comparison inside official tiers, not a weighted score.

`SCORE-ROLE-024` is rejected and not present in source. Its fresh short-horizon
lane was `0/6/0` exact. All 12 low-fuel cases reached `6/60` with zero
invalid/emergency; seven retained the same mask and revealed local cutoff noise as
large as -83. Seed861001's flip was unstable across fixed repeats and was excluded
from causal verdict. Stable same-binary fixed attribution on the other four flips
was `2/0/2`: gains `+6,+5` versus losses `-54,-15`, tails `+6/-54`. The high-stock
and threshold losses dominate the overnight and rare-brand gains. This is a
material global defeat even though m-1266 is positive, so default/high and BTC
were not opened. Patrol-normalized early productivity cannot protect total
late-horizon capacity. Source and tests are byte-identical to champion `08771f1`;
the opened masks may not be used to retune this axis.

`SCORE-ROLE-025` tested terminal patrol fuel per patrol as an independent
late-horizon discriminator. It is rejected before holdout and absent from source.
On m-1266 it would select mask192, whose terminal patrol fuel is `600/6` versus
one-tanker `486/7`, and whose exact score gains 47 servings. But on the development
counterexample m-1241 it selects mask96: rollout `6/56/211`, terminal patrol fuel
`466/6`, over one-tanker mask32 at rollout `6/57/195` and fuel `328/7`. Exact
counterfactual mask96 is only `6/60/302` versus the previously measured mask32
`6/60/331`, a 29-serving loss with the first two tiers tied. Terminal fuel is
therefore surplus after under-serving, not a safe capacity signal. The frozen
holdout remained unopened and may be reused only for a mechanism independently
derived from new telemetry.

`SCORE-ROLE-026`, parent `08771f1`, tested the measured structural
gap is budget allocation, not another terminal proxy: the expanded low-fuel beam
coarsely rolls all 16 masks only through 85% of the role budget, while the interval
to the established 92% role wall is used only to prewarm the already selected
mask. The preregistered mechanism spends that interval on a paired richer rerun of
the current best one-tanker and best two-tanker, with equal deadline slices and
the same exact simulator/validator. Each mask retains the lexicographic maximum
of its old and new evidence, then the unchanged lifetime/daily equality guard is
applied. There is no daily relaxation or new feature threshold. Development must
first had to admit profitable m-1266 mask192. A loaded-local run left mask192 at
`6/48/176` behind mask1 at `6/51/192`, but this deadline-sensitive result has no
performance authority because the local host was contended. The candidate is
inconclusive and absent from source; m-1241 and fresh holdout SHA256
`A785F6F89FA8AEBD7BA4924CF1FA4FDFA529798F5C3B9D9507C0F6D67B65D238` remained
unopened. It may be resolved only by a BTC live run at the explicit 5000 ms cap.

Temporary per-day attribution identified that mismatch. On m-1266 mask192, the
role rollout selected the master plan on all ten days and patrol fuel reached the
full `600/600` by day 4, yet cumulative daily coverage advanced by only four or
five brands on most later days while servings repeated at 18 per day. On m-1241,
the same evaluator usually retained five or six brands and fuel was not saturated.
`RouteColumnGenerator` orders target spots by lifetime rarity and stock, then its
anytime query and seed caps retain distinct spots, not distinct brands. With six
target slots and two spots per brand, duplicate high-stock brands can therefore
erase lower-stock brands before the official master sees them.

`SCORE-ROLE-027`, parent `08771f1`, tested a preregistered change that
promotes the first target of each brand ahead of duplicate-brand targets in both
query order and retained-seed order, then fills remaining capacity with the
unchanged priority/spot rules. Query counts, seed counts, deadline and official
comparison remained unchanged. It is rejected and absent from source. On m-1266
it raised one-tanker mask2's rollout to `6/60/169` and selected that mask, while
the best visible two-tanker was still only `6/52/179`. Frozen exact development
scores are mask2 `6/60/206` versus mask1 `6/60/228`, so the apparently improved
coverage chose a role with a known 22-serving loss and still hid mask192's gain.
Fresh holdout SHA256
`4CFF75E3D849C82466BBCCB1FEC1ED2FFB8ADAE98C800083474877C1BAF763D8` remained
unopened. Rollout coverage is not promotion evidence unless the retained columns
also improve the actual day solver under exact attribution.

Exact day-level attribution now shows why two-tanker capacity is invisible. A
fresh champion counterfactual for m-1266 mask192 scored `6/60/267` (the same
positive direction as the prior `275`) versus live mask1 `6/60/228`. Mask192's
daily servings were `29,29,27,25,27,25,25,25,26,29`; mask1 was
`31,32,30,23,23,15,17,18,21,18`. The benefit begins after day 3 and comes from
maintaining harvest throughput, not from a first-tier or second-tier trade. The
role evaluator instead saturates all six patrols at fuel 100 but remains near 18
servings/day. Code attribution confirms full `solve_day` enables production
harvest extensions and conditional orienteering, while `rollout_role_assignment`
leaves every harvest-extension option disabled.

`SCORE-ROLE-028`, parent `08771f1`, enabled long-horizon low-fuel
full role rollouts only, it will reuse the canonical production harvest-extension
configuration: enabled extensions, uncached target allowance, configured source
count and day-dependent depth. One-day probes and short/default/high lanes remain
unchanged; all work stays inside the current 85% rollout wall and 5000 ms hard cap.
configuration inside the existing slices. It is inconclusive and absent from
source. A loaded-local m-1266 run kept one tanker ahead and showed mask192 at
`6/46/164`, but neither score nor elapsed time is authoritative on the contended
host. Fresh holdout SHA256
`0EF4FBFEA6850208765A92E5772849A08D52EC8A28544E8E9DBC7B55F850EA1D` remained
unopened. Resolve only with a BTC live run at 5000 ms.

The exact terminal day itself provides a causal discriminator. On m-1266, live
one-tanker day 10 served 18 while mask192 served 29 in the current exact
counterfactual. On m-1241, live one-tanker day 10 served 37 while the exact
two-tanker mask96 served 30. Both comparisons cover all six brands, so this is a
tier-3 late-capacity difference inside an official single-day score, not a
weighted aggregate or a fuel proxy.

`SCORE-ROLE-029`, parent `08771f1`, shortens coarse full-horizon work
to 75% instead of 85% and preserves the state/ledger at the start of its
final day. Best one-tanker and best two-tanker then split the interval to the
existing 92% role wall and re-solve that day with production-like columns,
harvest extensions and conditional exact orienteering. Two tankers may relax the
coarse daily gap only when lifetime ties and their executable terminal official
score is no worse in its first two components and lexicographically better. The
mechanism is inconclusive and absent from source. A loaded-local diagnostic
returned only `4/4/9` for the best one-tanker and `3/3/10` for mask192, but that
deadline-sensitive score cannot reject the candidate. Fresh holdout SHA256 is
`61BE95EAB2FCDA6886CF8EB68CC3009C42F39B55DE3FEDC3B320F8C993C8A6DB`.

Two explicit-advanced BTC runs used the frozen candidate binary SHA256
`ABC213A7F21A11C3C9CFD5AC94FB8B5CA77560470EB9F8F9062F0BF3255C81ED`.
`m-1278` exercised the two-tanker branch (mask192), but the HTTP client stopped
after sending day 7 with WinHTTP `12002`; only six action results were received,
all valid at `3121..3130 ms`, and days 8--10 were lost. The final `6/44/196`
rank4 result is therefore a lifecycle failure, not a role-score verdict. Replay
SHA256 is `4D538FD15E3545848191F1803B40BE32CBB4D5E5172AD5CACECFA89639236E4B`.
`m-1279` completed 10/10 valid acknowledgements and ranked first at `6/60/310`
over `6/60/220`, `6/59/201` and `6/55/198`; response total was `29190 ms` and
per-day maximum `3061 ms`. It selected the existing one-tanker mask1, so it
validates BTC runtime but does not attribute a gain to the new terminal probe.
Replay SHA256 is `79E964C81D16C11604C3EF8BF9B706C409CA46FCB2F6DD6B61BA9920390E56E1`.
`m-1280` completed 10/10 valid acknowledgements with two-tanker mask192 and ranked
first at `6/60/300`, response maximum `3131 ms`. Replay SHA256 is
`FAB35645B9FEBA89E4A2DC0DEF0C864B24F8172D2DBB5542E97875CF65E86F44`.
However, replay-role attribution showed all leading one- and two-tanker coarse
rollouts tied on lifetime/daily (`6/51` on m-1280, `6/60` on m-1278), so the
already-accepted SCORE-ROLE-023 equality fallback can select two tankers without
the terminal probe. The probe itself saw only `4/4` on m-1280 and did not recover
the production trajectory's `6/6`; m-1266 likewise returned one/double terminal
scores `4/4/9` and `3/3/10`. SCORE-ROLE-029 is rejected and absent from source:
re-solving only the last day from an already-diverged coarse trajectory does not
measure the multi-day state that creates the known m-1266 gain. Reopen only with
a trajectory-consistent multi-day finalist witness inside the same 5000 ms cap.

`PERF-DEADLINE-003`, parent `08771f1`, is accepted. BTC
`m-1278` proved that a single WinHTTP receive timeout can terminate the client
after an action is sent, discard three remaining days and turn a live lead into
rank4. The six earlier ACKs were valid at `3121..3130 ms`; this is a transport
lifecycle failure, not a solver overrun. The official BTC API page states that
`POST /actions` may be resent. The frozen mechanism therefore keeps the solver,
role selection, action body, comparator and 5000 ms cap unchanged; it gives each
action ACK a bounded request-level receive slice and resends the identical body
only for retryable WinHTTP timeout/resend errors while the authoritative day
deadline still permits another attempt. Ledger state advances exactly once and
only after an accepted ACK whose day equals `wireDay + 1`. Frozen holdout SHA256
is `AEF7B9C72F08FEAA2E79BE69CAFB6C16AC8D6FAC6942AD1A598C91DEFE680A6A`.

A controlled semantic probe first delayed the initial ACK beyond a `250 ms`
request slice and confirmed two byte-identical POST bodies, one applied ACK and
one retry telemetry event. This probe is not local performance evidence. Fresh
explicit-advanced BTC production match `m-1283` used hard difficulty, three bots,
10 days, 32x32, 100 steps/day, 5000 ms, eight agents, 12 spots, six brands and
fuel 1x. It completed 10/10 valid ACKs, reconciled 9/9 transitions and ranked
first at `6/60/312`; maximum server-reported response was `2958 ms` and maximum
solver time was `2925 ms`. No transport retry was needed. Replay SHA256 is
`E085ACA345C2361FBBD05142905FF64EC4341D27EB6AF2795A3C6A965CF6A173`.

Fresh BTC conformance match `m-1284` used the same explicit advanced class and a
temporary probe binary that discarded the first valid day-1 ACK, then resent the
exact frozen body. BTC accepted both submissions for day 1; the runtime applied
only the second ACK, completed 10/10 valid applied days, reconciled 9/9 transitions
and ranked first at `6/60/277`. Maximum server-reported response was `3060 ms` and
maximum solver time was `2848 ms`. Replay SHA256 is
`F0454045E278E7D696FBA365335CB41764971D0E5EC0587A2808DBDE9EEBAAA9`.
The temporary forced-resend flag was removed before the final production rebuild.
The final executable SHA256 is
`ACA63DCC68DA5339DF98A7D5BCC792AF168D97CF09F382861540A1341B4BF7E0`;
that binary replay-checks both matches to their exact `6/60/312` and `6/60/277`
summaries. The candidate therefore closes a catastrophic lifecycle downside while
the no-timeout production path preserves action/score semantics and the 5000 ms
cap.

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

1. Keep the promoted 161/163 correctness/wiring commit canonical. The full diff
   audit proves rejected experiments 151--162 left no production logic behind;
   research probes/evidence are isolated behind the benchmark build option.
2. The all-target build and unit/simulator/validator gate passed. BTC `m-3573`
   supplies target-host validity, lifecycle and hard-cap authority; bot rank
   remains excluded and local elapsed has no performance authority.
3. With 164 and 165 closed negative and no admissible source candidate left,
   treat this commit as the pre-human-opponent practical ceiling. Reopen
   research only for a genuinely new diverse counterexample or a new general
   dominance proof, never by retuning a consumed fixture.

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
