# ATTR-CERT-060 BTC attribution

Date: 2026-08-10

## Scope and integrity

The frozen audit-only executable was
`2A1BDA4F9E67FC3EC70F3A4144AA14145B9DA5DF64F5DBE29C5B45CC3F203C04`.
All fixtures used hard/three bots/10 days/32x32/100 steps/5000 ms/eight
agents/12 spots/six brands/high fuel. Rank is excluded from the evidence.

- `m-1837`: 10/10 HTTP 200 actions with `valid=true`, 9/9 reconciled
  transitions, replay-check `6/60/440`, replay SHA256
  `7325885ADEDC16356CF1D4A694C563A994EE532335B9A30872564B5DA56E4C85`.
- `m-1838`: 10/10 HTTP 200 actions with `valid=true`, 9/9 reconciled
  transitions, replay-check `6/60/384`, replay SHA256
  `C2D84D6A48132EDD8208C133A8EB0F858321FBC177F1CF6AE0CAA34A91AD2017`.
- `m-1839`: transport-unusable after the accepted setup response; it contains
  no day state, decision or action. It was not replaced, as preregistered.
  Replay SHA256
  `7F6197F6018E7F25F6901ED3A0B6DA09CB45B77B47C5720C360C69DF932C44CB`.

Across the 20 complete decisions, solver total was always below both the
configured 5000 ms cap and the exact authoritative per-day deadline. Every
action was posted and accepted. Therefore these fixtures contain no 5000 ms
hard-cap breach and no no-POST failure. The crossings below concern the earlier
`deadline - 1600 ms` compute boundary reserved for submission/network time.

## Target-host localization

All certification-caused global compute-boundary crossings ended in
`column-generation`. The material samples in `m-1838` were:

| day | certification | future column generation | nested exact | global overrun | maximum nested exact overrun |
|---:|---:|---:|---:|---:|---:|
| 4 | 1398 ms | 1397 ms | 1394 ms | 381 ms | 609 ms |
| 5 | 982 ms | 980 ms | 879 ms | 250 ms | 250 ms |
| 6 | 808 ms | 806 ms | 802 ms | 23 ms | 254 ms |
| 9 | 1079 ms | 1063 ms | 791 ms | 69 ms | 69 ms |

`m-1837` independently put every recorded global certification crossing in
column generation. Its overruns were only 0--3 ms after millisecond truncation,
while nested exact calls overran their supplied slices by up to 43 ms. Two
`m-1838` decisions (days 1 and 7) first crossed in monotone-floor by 670 and
718 ms, but certification took only 1 and 0 ms: pre-certification had already
consumed the compute reserve. They are explicitly not attributed to future
witness repair.

Current-day diagnostics expose the same exact finalization tail independently.
On `m-1838` day 1 exact enumeration took 252 ms, exact finalization took
2485 ms, the supplied exact deadline was exceeded by 1756 ms, and finalization
visited 3,000,173 feasibility nodes. Day 2 similarly visited 3,000,124 nodes
and exceeded the supplied exact deadline by 798 ms.

## Source-level cause

`select_coordinated_exact_orienteering_routes` runs the recursive
`capacity_feasibility_search`. It polls the absolute deadline every 1024 nodes.
When a poll observes expiry, the recursive call returns `false`, which is also
the ordinary value for an infeasible subtree. Parent frames therefore continue
with sibling routes instead of propagating cancellation. Repeated polls prune
only the current subtree and the search can continue until the 3,000,000-node
strict-feasibility cap. This exactly explains the target-host node counts and
the finalization-dominated deadline tails.

The first general successor is not a smaller cap or a tuned polling interval.
It is explicit cancellation propagation through the existing DFS: preserve the
same incumbent, route order, bounds, node caps and deadline, but unwind all
frames once an existing poll observes expiry. Completed searches are identical;
only work already forbidden by the absolute deadline is stopped.

## Verdict

Accepted attribution. The concrete cause is lost deadline cancellation inside
exact team feasibility, observed through certification nested-exact telemetry
and independently through current-day exact finalization/node telemetry. The
third fixture is transport-unusable and contributes no positive evidence, but
was not replaced or tuned around. A separately preregistered semantics-equivalent
successor is required before source change.
