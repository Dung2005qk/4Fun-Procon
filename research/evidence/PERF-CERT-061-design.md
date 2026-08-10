# PERF-CERT-061 design

Date: 2026-08-10

## Proven gap

`ATTR-CERT-060` localized material future-certification overruns to nested exact
team feasibility. Its recursive DFS detects deadline expiry but returns the
same boolean as ordinary infeasibility, so callers continue sibling traversal.
BTC telemetry reached 3,000,173 feasibility nodes and spent 2485 ms in exact
finalization after only 252 ms of enumeration on one independent decision.

## Candidate mechanism

Add one invocation-local cancellation state to
`select_coordinated_exact_orienteering_routes`. When the existing 1024-node
deadline poll expires, set cancellation and unwind every recursive frame after
restoring its local spot counts. The outer feasibility loop must not begin an
overlap or improvement pass after cancellation.

This does not change the polling interval, route ordering, feasibility bounds,
memoization, node caps, beam, worker count, exact graph, score comparator or
incumbent. A search that finishes before its deadline is byte-identical. At an
expired deadline it returns the same best already-completed incumbent that the
function is designed to return, without continuing work after cancellation.

## Preservation answers

1. Does the candidate remove, disable, defer or reduce designed functionality?
   No. It repairs the existing cooperative-deadline contract and preserves all
   exact feasibility work while the supplied deadline is live.
2. Does it delete anything requiring a proven active equivalent?
   No code or capability is deleted.

## Gates

- Source proof must show cancellation is propagated only after an existing
  absolute-deadline poll and that all modified recursion paths restore state.
- Full unit suite once after implementation.
- Frozen replay screen against the audit parent: zero invalid action, zero
  simulator/validator disagreement and identical official score/state whenever
  actions differ because expired work is no longer consumed. Local elapsed is
  never performance evidence.
- One fresh high-fuel BTC development match must have 10/10 accepted valid
  actions, 9/9 reconciled transitions, zero emergency, zero 5000 ms hard-cap
  breach and zero material certification crossing of the compute boundary.
- Only after development passes may the frozen low/default/high target-host
  holdout be opened. Rank is ignored.
