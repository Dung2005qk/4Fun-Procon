# ATTR-OPTIMAL-041 complete resource one-exchange

Date: 2026-08-09
Parent: `afcd2da`
Verdict: accepted one-exchange infeasibility proof

The proof-returning `enumerate_exact_resource_routes` API was run without a wall
deadline for the two unique start/fuel states among agents 2, 4 and 7, the
patrols claiming oversubscribed spot 0 in the frozen 35-serving plan.

For both representative agents 2 and 4:

- `supported=1`;
- `complete=1`;
- settled states: `3,148,570`;
- inclusion-maximal routes: `78`;
- exact-valid, validator-agreed substitutions: `78`;
- best official score after substitution: `6/60/321`.

All 156 complete-frontier one-agent mutations tie or lose to the frozen witness;
none reaches guidance `6/60/322`. Inclusion-maximal routes are sufficient for
this serving proof because extending a reachable claim mask cannot reduce the
team total `min(claimCount, stock)` at any spot.

This proves the final gap cannot be closed by any single-patrol resource-feasible
route, not merely by the capped production frontier. It does not rule out a
coordinated exchange of two or more patrol routes whose individual substitutions
tie or lose but whose joint stock allocation wins. No one-agent repair candidate
is authorized.
