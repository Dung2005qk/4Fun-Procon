# SCORE-FUTURE-FUEL-EXACT-082

Date: 2026-08-12

Parent: champion `7ef3694` plus research diffs 072 and 073.

082 mirrored current-day production's terminal exact-search exception and
fuel-constrained/anytime exact flags into all three provisional/W1 future
generation option blocks. It added no solver, route selector, cap, deadline or
fixture condition. The only inputs were simulated terminal day, patrol kind,
remaining fuel and public day steps.

One preregistered CEILING-MATCH-071 development run under the internal 5000-ms
cap returned:

```text
summary,split=development,cases=18,oracle_wins=3,ties=15,head_wins=0,invalid=0,tier1=0,tier2=3,tier3=0,max_gain=3,result_hash=6c9c4ac2e6aee047
```

This is exactly the 072+073 result. No tie reopened and no invalid appeared, but
the candidate did not alter any official score. ATTR-081 proved that the
fuel-constrained generator can contain relevant routes; 082 proves availability
alone is insufficient to make W1 reach or retain those trajectories under its
existing trajectory/master structure.

Verdict: rejected. The frozen holdout remained sealed, BTC was not run, and all
082 source was removed. Reopen only with a new independent trace isolating flag
wiring after the route already reaches W1 master selection; do not tune cap or
ordering on the ATTR-081 cohort.
