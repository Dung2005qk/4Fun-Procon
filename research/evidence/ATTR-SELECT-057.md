# ATTR-SELECT-057

Date: 2026-08-09

Scope is read-only attribution over the already-opened `PERF-P99-055` replay
set. It creates no new quality holdout and authorizes no source change.

## Initial complete-set scan

- Replays: 30, ten each low/default/high fuel.
- Decisions: 300, exactly 100 per fuel profile.
- Selected audit candidate: exactly one in every decision.
- Selection reason: `certified-undominated-current-floor` in all 300.
- Higher non-selected `scoreAfterToday`: 0.
- Accepted exact bundle better than selected `scoreAfterToday`: 0.
- Master deadline reached/search incomplete: 296/300; this is not by itself a
  score witness and no local timing conclusion is permitted.
- Certified non-selected candidate with a lexicographically higher
  `finalCertifiedLowerBound`: 2 decisions.
- Certified non-selected candidate with equal `finalCertifiedLowerBound` and a
  higher `finalQuantile50`: 5 decisions.

The seven signals are `m-1803` days 6, 7 and 8; `m-1804` day 9; `m-1806`
day 8; `m-1809` day 6; and `m-1813` day 9. These comparisons are not yet proof
of a bug because the production comparator may include conditional tiers,
confidence coverage, survival signature or other dimensions not represented by
the two projected scores. The next step is to trace the exact production
comparator and reconstruct its complete tuple for these seven pairs.

## Production-path attribution

`MatchSession` constructs `UdonShieldEngine` with
`requireUndominatedCurrentFloor=true`. After certification, production first
removes candidates proven dominated by the full scenario distribution, then
computes the best current-day official score among candidates within the
relative confidence gate. Any candidate below that score is excluded. The
remaining candidates are ordered by the complete frozen tuple:

1. official score at q95;
2. official score at q80;
3. official score at q50;
4. official score at q20;
5. official score at q05;
6. survival signature;
7. certified lower bound;
8. current-day official score;
9. terminal slack, traffic safety and stable ID tie-breakers.

The replay field `finalQuantile50` is therefore only the third projection in
that tuple, while `finalCertifiedLowerBound` is seventh. Neither can establish
domination alone.

The seven signaled decisions contain eight challenger pairs:

- `m-1803` day 8: the challenger improves the final lower bound from
  `6/58/236` to `6/58/239`, but its current score is `6/47/208` versus selected
  `6/48/210`; the protected current floor correctly excludes it.
- `m-1804` day 9: the challenger improves the final lower bound from
  `6/54/278` to `6/60/298`, but its current score is `6/54/274` versus selected
  `6/54/278`; the protected current floor correctly excludes it.
- `m-1803` days 6 and 7, `m-1806` day 8, `m-1809` day 6 and `m-1813` day 9:
  selected and challenger have equal current-day scores and equal certified
  lower bounds, while the challenger is better only at serialized q50. Since
  the production result marks each challenger `certified-not-selected`, its
  loss occurs at the earlier q95/q80 prefix or at the relative confidence gate;
  it is not a complete-tuple domination witness.

Across all 300 decisions there is still no higher non-selected
`scoreAfterToday` and no accepted exact bundle above selected. The apparent
seven-case anomaly is a telemetry projection mismatch: audit records expose
q50 and lower bound but not the earlier q95/q80 prefix or per-candidate
confidence coverage. This does not justify a logic candidate. The score/evaluator
axis remains closed until a new exact dominated-selection witness is recorded.
