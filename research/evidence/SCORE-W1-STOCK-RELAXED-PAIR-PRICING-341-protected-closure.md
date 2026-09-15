# 341 protected closure — 2026-09-07

## Verdict and preservation

The frozen 341 candidate does NOT qualify for production. The protected gate
failed. This is not a reversal of its successful narrow development/holdout,
nor proof that the stock-relaxed bound is unsound. Unactivated runtime differences
prevent attribution of the protected score changes to improved witnesses.
No designed function is removed, disabled, deferred or reduced. Nothing is
deleted. Canonical c76a8ea/accepted258 source and binaries remain unchanged;
the isolated candidate and every measured artifact are retained. No commit,
integration, BTC promotion attempt or competition signature is authorized.

## Completion and exact provenance

Completed at 2026-09-07 20:00:58+07: 108 pairs, 216 atomic side results,
108 pair markers, one run_complete, 1368 valid actions and 1152 reconciled
transitions. Resume runner3596 exited with stderr0 and no remaining child.
The historical runner5096 memory-floor stop is preserved separately.
All1464 frozen dependencies and all side/pair/complete hashes passed validation.
The entire frozen summary was recomputed in memory with create-only output
captured, and compared exactly equal to the saved summary. No gameplay reran.

- Protected input: A2AC983D560603AF8835D8173F52305D6BAD82733439E2BE06A02DD212A50203
- Execution: CED506D5BE2F20B9B273200A8FB8BABABAF6053273016E498593B883BDD62BB9
- Runner/summarizer: 36584356540445EFFBBC2C2F590D9A457780071985C44E3C226C22D9C034796B
- Parent BTC: F97F168FE76FEF1B6226D2C2CFDAF954F116D39F427B6F157594151C461E1275
- Candidate BTC: C8603EB32E33B44EF1E3A2BE56DF2CC8A1C8E1001E11A5E28248814E028BE2A4
- Completion: F6187AD49A43684830F294BFECD6EA5B89879C3EFD68BE8C4F3679034EE94FC0
- Summary: 04A4D611EAE4D99908B9671F00E967228C4C12DFC90C3C4997E58808CEF8F80B

Exact full evidence is the adjacent `341-protected.summary.json` (full filename
prefix SCORE-W1-STOCK-RELAXED-PAIR-PRICING). It preserves every row, both scores,
plan hashes, roles, state/ledger/road equality, per-day safety and work, all gain
and loss tails and every registered stratum. There are102 stock-vector strata,
18 step-vector strata, two agent-count and three brand-count strata in addition
to the core strata below. No stratum was dropped or reweighted.

## Paired results (descriptive, not causal promotion evidence)

W/T/L15/84/9; component delta0/+14/+230. First differing tier:84ties,
5tier2 differences and19tier3 differences. Four tier2 wins, one tier2 loss.
The total is NOT a weighted score and does not waive a failed gate.

All gains, written as seed:delta(lifetime,daily,servings):

202609073410020:0/0/+19; 202609073410038:0/+2/+12;
202609073410062:0/0/+8; 202609073410086:0/+3/+9;
202609073410159:0/0/+81; 202609073410280:0/0/+3;
202609073410310:0/0/+10; 202609073410341:0/0/+1;
202609073410389:0/0/+7; 202609073410413:0/0/+12;
202609073410450:0/0/+1; 202609073410486:0/+2/0;
202609073410541:0/+8/+21; 202609073410559:0/0/+66;
202609073410631:0/0/+7.

All losses:

202609073410123:0/0/-11; 202609073410165:0/0/-2;
202609073410207:0/0/-10; 202609073410304:0/0/-1;
202609073410419:0/0/-2; 202609073410492:0/0/-1;
202609073410534:0/0/-3; 202609073410589:0/0/-1;
202609073410637:0/-1/+4 (a tier2 loss, NOT a serving win).

| Stratum | Pairs | W/T/L | Delta L/D/S |
|---|---:|---|---|
| balanced |18|4/14/0|0/+5/+48|
| rare-brand |18|1/14/3|0/0/+58|
| threshold-corridor |18|2/15/1|0/0/+12|
| fuel-tight |18|3/14/1|0/0/+18|
| high-stock |18|2/14/2|0/+2/-3|
| overnight |18|3/13/2|0/+7/+97|
| low fuel |36|5/30/1|0/+8/+97|
| default fuel |36|6/27/3|0/+4/+107|
| high fuel |36|4/27/5|0/+2/+26|
| map8 |54|2/51/1|0/+10/+32|
| map32 |54|13/33/8|0/+4/+198|
| fixed all-Patrol |54|5/44/5|0/+4/+17|
| native roles |54|10/40/4|0/+10/+213|
| 4days |36|3/30/3|0/0/+11|
| 5days |36|3/31/2|0/0/+15|
| 10days |36|9/23/4|0/+14/+204|
| 8players |36|4/29/3|0/+2/+23|
| 9players |36|6/27/3|0/+4/+102|
| 10players |36|5/28/3|0/+8/+105|
| 5000ms |36|9/25/2|0/+13/+186|
| 10000ms |36|3/29/4|0/+1/+40|
| 15000ms |36|3/30/3|0/0/+4|
| roads present |54|8/43/3|0/+13/+154|
| roadless |54|7/41/6|0/+1/+76|

## Gate, safety and work

Passed: zero operational failure. Failed: no lifetime/daily component loss;
worst serving loss<=1; no negative family/fuel serving net (high-stock -3);
no unactivated score difference; inactive exact role/plan/state/ledger trajectory.
This is not rejection merely because there are some small losses.

All1368 actions exact-valid/ACK-valid, all1152 transitions reconciled; no
registered deadline, checkpoint, validator or pricing safety failure.
Synthetic maximum main time parent2893ms/candidate2878ms; maximum response
parent9745ms/candidate8511ms, within each authoritative synthetic window.
These are contract observations, NOT target-host performance evidence.

Pricing:1459calls,357supported,72complete,285exhausted,4improvements,0failures.
Settled7416864; created7633388; storedActions18755934; memo135004;
charged transitions14249467; dayQueries23124. Exhaustion retains original W1.

## Causality audit and remaining uncertainty

All24 score-different pairs diverged before a pricing certificate gain. Of these,
17never entered the supported pricing domain and7 had supported work but no
certificate gain before divergence. Only two cases show any measured certificate
gain:202609073410347 (tie, no plan divergence), and202609073410637 (first plan
divergence day1, certificate gain only day8). There are28 trajectory differences
with no certificate gain and5 differing native assignments, including one tie.
Thus neither the15wins nor the9losses can be claimed as causal witness gains.

Confirmed source facts in the frozen candidate:

- `src/decision.cpp:2589-2596` copies DayState/agent state and MatchLedger and
  advances a pricing ledger before calling the supported-domain test.
- `src/horizon_pricing.cpp:342-357` increments calls, rejects unsupported domains,
  then permits supported attempts to share the caller's deadline, up to100ms.
- The returned witness is unchanged on no gain/incomplete work. That does NOT
  refund elapsed work to later scenario/candidate certification. The immediate
  witness non-regression proof is not runtime operation equivalence.
- `select_roles_until_impl` and `rollout_role_assignment_impl` use wall-clock
  scan/probe/rollout and generator/master deadlines. The role path does not call
  `price_horizon_witness`; its5 mismatches therefore cannot be explained as
  selected pricing witnesses. Independent cutoff/build-layout/load variation
  remains an unresolved confound, not an established quantitative cause.

Do not claim that moving the support guard alone fixes all protected differences.
Do not discard the whole functional pricing capability, relax gates, tune a
dispatcher, widen the100ms or5000ms caps, or replay these consumed protected cases.
The narrow holdout's current-day-floor tradeoff is a DIFFERENT issue; removing
that floor is neither a proven remedy nor authorized by this evidence.

Next bounded work is a separate, preregistered fresh attribution of inactive
runtime/budget effects: identical-binary A/A control and a source-level accounting
proof that distinguishes unsupported setup cost, exhausted supported work and
independent role/search cutoff variability. Freeze fresh diagnostic inputs before
any new measurement; no candidate tuning or promotion from this consumed matrix.
Require evidence of a general causally isolated mechanism before a new SCORE
successor. This closure does not establish an architectural ceiling.
