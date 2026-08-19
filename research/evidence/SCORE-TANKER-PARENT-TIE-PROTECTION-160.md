# SCORE-TANKER-PARENT-TIE-PROTECTION-160

Date: 2026-08-14
Production parent: `cf7e4b4`
Frozen tanker manifest SHA256: `0DC2CF8314D224CB52AF232E03039DF12324C19CD3A47EC2B01B4FDA89F38E22`
Frozen protected matrix SHA256: `4DE14CD78844356901062E2DA7CCF030645B71BF4D14525A4B7813CA7095EA72`

159 established that exact empty road footprint alone does not protect future
state: a post-prune coordination route can tie today's official score, win a
secondary terminal/stable tie breaker, and reduce later serving opportunity.
160 snapshots the canonical portfolio immediately before any post-prune
coordination extension. RouteMaster first solves that untouched snapshot under the same absolute deadline and retains
its incumbent, then searches the expanded portfolio using only time remaining.
The canonical comparator first applies unchanged official lexicographic score;
on a tie it protects the parent incumbent before secondary tie breakers. Any
strict official-score gain still promotes the extension.

All parent columns/candidates remain active and the extension remains fully
searchable. Nothing is deleted, disabled, deferred or reduced. Provenance is
structural, not a seed/map/family/fuel/role/bot/match dispatcher. The same
comparator is used by master, ALNS, role rollouts and production. Exact simulator,
independent validator and final certification are unchanged. Internal hard cap
is 5000 ms; local elapsed is excluded and BTC target-host remains performance
authority.

## Result and verdict

No fresh row was opened. On consumed complete-search seed `3760032`, direct
column/candidate provenance left the result at `5/20/41`. Snapshotting the
untouched parent portfolio and wiring parent-first tie flags through master and
the production search comparator improved it to `5/20/45`, but the frozen
production binary remained `5/20/47`. The residual divergence is caused by
whole-pipeline budget/order and W1 profile state, not one missing comparator.
Exact equivalence would require running the entire decision pipeline twice or a
shadow solver, violating the single-path/non-overengineering contract and the
same 5000 ms budget. Runtime source was restored byte-identical to `cf7e4b4`.

Status: rejected. Reopen only with a single-path whole-pipeline incumbent
certificate that does not duplicate the decision solver or extend the hard cap.
