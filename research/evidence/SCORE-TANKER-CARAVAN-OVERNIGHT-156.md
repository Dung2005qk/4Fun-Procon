# SCORE-TANKER-CARAVAN-OVERNIGHT-156

Date: 2026-08-14
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/SCORE-TANKER-CARAVAN-OVERNIGHT-156.csv`
SHA256: `9A6D838736DCE43142A95FB89A019FE0A15E117569B9CD96C9277FC4EB08B436`

155 proved bounded convoy preseed closes day 1 but regresses the suffix solely
because a common terminal can be a spot and the next day departed without the
explicit `WAIT(1)` harvest required by the rules. Candidate 156 composes that
already-designed overnight action with the unchanged 155 convoy: when the tanker
starts a nonterminal day on a spot, every convoy member first waits one step,
then uses the same cached path permutations and earliest feasible join rule.

No cap, ordering, endpoint, guard or comparator changes. Outer parent/main
candidates remain intact, candidate replacement requires strict official gain,
and the roadless/all-brand/all-stock/full-fuel/common-terminal admission remains.
Anchor `3200100` must reach exact final `3/12/16` before fresh development opens.

## Development result

The consumed anchor closed exactly: parent `3/12/15`, candidate `3/12/16`,
with every day reaching four servings and zero validator mismatch. On the 12
fresh rows, paired candidate versus the frozen parent was `3/9/0`, invalid
`0`: split-duty/low gained `+1`, stock-cycle/low gained `+1`, and
rare-return/low gained `+2`; all other fuel/family rows tied.

The complete oracle finished for fresh split-duty/low `3470100` and tied the
candidate at the exact match score `3/12/16` (all four cumulative day scores
also tied), proving that gain reaches the exact ceiling. The complete oracles
for stock-cycle/low `3470200` and rare-return/low `3470300` did not finish in
their single 15-minute research-wrapper runs, so optimality for those two rows
is explicitly inconclusive; their candidate plans remain exact-simulator and
independent-validator valid. Neither exact run was repeated or used as a local
performance claim.

## Holdout result

Opened once with separate frozen-parent and candidate executables through the
same research adapter. Paired candidate versus parent was `7/29/0`, invalid
`0`. Every difference was a tier-3 gain of `+1..+2` servings. Gains covered
rendezvous-chain/low (one of three), stock-cycle/low (three of three), and
rare-return/low (three of three); all default/high rows and the remaining low
rows tied. There was no tier-1/tier-2 change and no losing tail.

Exact/absolute-bound validation of the seven changed rows and the protected
general matrix remain pending; this result alone does not authorize promotion.

Before any adapter change, day-1 exact attribution is restricted to the two
already-open stock-cycle gains `3500200` and `3500202`, whose candidate match
score is `2/8/19` against the public absolute stock ceiling `2/8/20`. The probe
may only expose the existing uncapped complete day enumerator, reconstruct its
witness, and require exact-simulator/independent-validator agreement. It may not
cap state, change dominance, tune 156, or open another seed. The result decides
only whether a residual day-1 capability gap exists.

Both registered day-1 enumerations completed. On `3500200` and `3500202`, the
candidate scored `2/2/4` while the complete oracle scored `2/2/5`; both exact
witnesses were reconstructed and dual-valid, with maximum step frontier
`216790` and `1601691` settled states. Thus 156 closes the original whole-match
gap broadly but does not close the architecture: the remaining generic
capability is patrol-specific loiter/detach around a shared tanker trajectory,
not another endpoint/order/cap/guard adjustment. These opened holdout witnesses
are attribution only and are forbidden as tuning fixtures for 156 or a successor.

## Protected screen

Candidate and parent were built separately from the same historical harness at
the `5000 ms` internal cap. General fixed was `0/6/0`; production deadline was
`0/6/0`. The first exhaustive-role pass showed one candidate loss with role
mask `8->1` and one tied mask change, but parent-first reversal returned both to
the parent mask `8`, identical score, combinations and exact counters; causal
exhaustive result is therefore `0/6/0`, with the first pass retained as local
cutoff noise. BTC-like default/low/high each tied exactly with identical settled
counters; invalid/emergency were zero throughout. All BTC-like local runs were
deadline-limited, so their elapsed time has no performance authority. Full
protected lanes remain pending.

Full road-containing general protection is now causally closed: fixed
`0/120/0`, exhaustive `0/60/0`, deadline `0/60/0`, invalid/emergency `0`.
Every initial native/deadline score difference was accompanied by a timed role
mask change and disappeared under reverse-order or fixed-mask attribution; at a
fixed mask, candidate and parent score/combinations/exact counters tied. The
full BTC-like first passes were default `2/8/2`, low `3/8/1`, high `1/11/0`,
but reverse-order differences either became exact ties or crossed direction
with large settled-state changes. Since all those maps contain roads and the
new generator returns before search, these are retained as local cutoff noise,
not candidate gain/loss; target-host BTC remains authoritative.

The existing matrix did not include the roadless domain where 156 can run. A
fresh protected extension was frozen before execution at
`research/holdouts/SCORE-TANKER-CARAVAN-OVERNIGHT-156-ROADLESS.csv`, SHA256
`0F67E8414EAE75CD13F34E1BB5513537960174E838BEE045EE9CCEFAD22BB7DE`:
120 fixed one-tanker cases plus 60 exhaustive and 60 disjoint deadline-role
cases over six families, all at the 5000-ms internal cap. It may be opened only
once and never tuned.

The first roadless lane is a downside control but has 4 agents and 6--8 spots;
because the candidate enumerates at most four endpoints and requires all current
stock before admission, it cannot establish useful activation breadth. A second
scale lane was therefore frozen before its research-harness adapter was opened:
`research/holdouts/SCORE-TANKER-CARAVAN-OVERNIGHT-156-SCALE.csv`, SHA256
`79CD3F0F0158AB0EE39105637F0C5A285A1BA776703CA4320DAC884A7832A400`.
It retains generated 8x8 maps, six families and 4/5-day horizons, removes only
road terrain, and stratifies three agents over 3/4/5/6 spots and fuel 2/8/16.
The fixed lane has 216 cases (three repetitions of every structural cell), with
72 disjoint exhaustive and 72 disjoint deadline-role cases. Fixed role mask 4
is exactly two patrols plus one tanker. This lane is pass/reject only; no source,
cap, endpoint or guard tuning is allowed after opening.

The 216-case fixed lane completed `2/214/0`, invalid/emergency `0`. Both gains
reproduced parent-first with complete search: fuel-tight/low/4-spot improved
`4/12/12 -> 4/14/14`, and threshold-corridor/low/3-spot improved
`3/6/6 -> 3/12/15`. Thus the capability is useful beyond the exact tiny map.

The exhaustive-role lane nevertheless failed at `0/71/1`. On fresh balanced
seed `3541032`, candidate changed the chosen tanker mask `2 -> 4` and regressed
the official whole-match score `3/8/8 -> 3/7/8`. The result reproduced in
parent-first order. At fixed mask 2 and fixed mask 4, candidate and parent tied
exactly in score, combinations and exact counters, proving the causal failure
is role-evaluator selection, not plan validity or local cutoff.

Verdict: rejected. The independent whole-plan shortcut makes a fast caravan
look better inside the reduced 60-ms role rollout even when the production
planner at 5000 ms gains nothing for that mask. Budget/context guards are
forbidden because they would disable the new capability in one caller and form
two planner semantics. A successor may only implement join/loiter/detach through
the canonical route/master representation shared by role rollout and production,
then start from fresh splits; it may not tune 156's endpoint cap or guard.
