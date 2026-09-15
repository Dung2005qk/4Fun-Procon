# 343 source/development feasibility — positive, not promotion

No designed functionality is removed, disabled, deferred or reduced. Nothing is
deleted. Source and binaries remain unchanged. This audit performs no solver run,
opens no holdout and does not reinterpret consumed341 as acceptance evidence.

Frozen manifest1C3215B9483CDB95FBCCB05B61D7599B25F02FC98BDE891A4AE20F0BC359BB24;
summary265E3DCFAD62F2900906F1C2EEF0B79A109DA74F24B2F2C7D91F9B67B6DFFD9F.
Three headroom arithmetic/rounding/reserve unit contracts pass. Every one of the
24 complete DEVELOPMENT parent matches was validated; all96days included,24
terminal days reported separately. No candidate result or holdout was analyzed.

## Quantitative scope

At the earlier end-of-original-W1 boundary, a conservative recorded headroom
estimate subtracts the ENTIRE completed main solve,1600ms network reserve,
min(certification budget,25ms validation floor) and1ms rounding guard from its
recorded total budget. Source btc_http_deadline_calibration leaves the default
25ms validation floor unchanged. The estimate does not promise this time on a
future loaded host, measure pricing throughput, include free public continuation
time, or authorize spending the reserved validation/transport time.

All72nonterminal selected original profiles had complete, non-lower-bound-only
W1 schedules and positive estimated headroom; all72could fit the unchanged100ms
ceiling in that recorded interval. Each family has12such days:

| Development family | Nonterminal days | Estimated headroom range(ms) |
|---|---:|---:|
|three-balanced|12|3173..3306|
|three-duplicate|12|3228..3340|
|three-stock|12|3143..3226|
|three-coverage|12|3152..3238|
|three-fuel|12|3106..3292|
|three-terminal|12|3211..3323|

This is a narrow small-roadless/fixed3Patrol feasibility sample, not a statement
about map32, native roles, all candidate profiles or competition speed. The full
record preserves96rows and marks every terminal day. No gate threshold/cap was
tuned from these values;100ms was already frozen in332/338/341.

## Preserved runtime boundary and consumers

Canonical repair_profile has one production caller in solve_day (plus its unit
contract). It completes each original scenario witness or retains its complete
monotone floor. The repairIndices loop allocates existing deadlines but has no
pricing work in canonical production. After that loop, certifiedPool contains
only all-outcomes-certified profiles. At this point all original search, F0,
cached suffix admission and W1 work are finished for this invocation. No
subsequent call restarts generation or repairs another original profile.

The integration boundary under consideration is after certifiedPool construction
and the unchanged empty-pool emergency handling, BEFORE finalization/dominance
filtering moves or erases candidates. Refinement must preserve the entire base
pool and original outcome whenever no complete strict certified improvement is
produced. It must preserve scenario identity/weights, current actions and their
simulation, stable candidate ids, valid upper bounds, current-day floor and all
failure/expiry behavior. Lower-bound-only/invalid/incomplete witnesses must not
be mislabeled as exact pricing inputs. Pricing keeps the same resource model,
100ms/all-pair caps, shared caller deadline and whole-invocation rollback.

All consumers stay canonical:

1. finalize_profiles derives quantiles and certified lower bounds from the pool.
2. certified_dominates and choose apply the same official lexicographic/current-
   floor policy; no oracle-defined winner or new weighted score.
3. ExactStepSimulator and IndependentDayValidator validate the selected current
   plan, then result.profile is the selected complete witness profile.
4. MatchSession::on_authoritative_state_for retains that pending result; a valid
   authoritative ACK alone calls record_submitted.
5. record_submitted retains full roadless certifiedSuffix in cachedContingencies,
   alongside the selected profile/candidate and traffic/agent prediction. Existing
   post-ACK consumers and next-day revalidation remain unchanged (333 audited
   this chain; no new evidence permits forcing suffix actions or ACK rebasing).

Source anchors: src/decision.cpp repair_profile2187, certification5153-5179,
certifiedPool5181, finalization5224, selection5271, finalvalidation5288,
record_submitted5364-5435; src/runtime.cpp on_authoritative_state_for60 and
acknowledge_submitted89; include/udon/decision.hpp DeadlineCalibration291.

Unlike closed303, this bounded W1 route-portfolio completion is NOT a fixed point
of joint whole-horizon stock-aware resource pricing. The different feasible-set
capability was already demonstrated in340/341. Finite original W1 can finish
with spare compute time and still leave a strictly improvable feasible witness.
The proposed boundary therefore does not rely on rerunning an output-equivalent
finished oracle or borrowing a deadline-only phase's exhausted budget.

## Decision / next gate

GO for a SEPARATE fresh complete-pool pricing design; NOT for production or an
unregistered source patch. No evidence yet that the new scheduling is broadly
better, preserves every intended pricing invocation under all deadlines, or
qualifies on map32/native/traffic. Every caller and both final selection and ACK
consumers must be covered by implementation tests before a new score run.

The design must expose a same-invocation original pool/selection reference for
causal audit. Unsupported/exhausted/no-gain refinement cannot change that
invocation's pool or chosen action/state/ledger. This is not a claim of byte
identity between independently timed processes:342 falsified that premise for
the parent itself. Independent repeated parent/candidate performance and score
differences must still be reported and evaluated under a NEW preregistered gate,
never used to revise341's frozen gate after the fact.

Before new source: register one SCORE successor, freeze genuinely fresh
development/holdout/protected inputs and its exact acceptance/control policy;
retain all-pair100ms/six resource ceilings, canonical5000ms, all scenario/role/
cache/current-floor functionality. Do not route on seed/family or disable role
selection to manufacture repeatability. No competition signature or microcommit.
