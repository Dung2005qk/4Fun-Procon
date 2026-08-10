# ATTR-CERT-058

Date: 2026-08-09

This is read-only attribution over the 300 already-opened target-host decisions
from `PERF-P99-055`. It neither changes nor rejects the frozen
`DEADLINE-ALIGN-056` candidate.

## Target-host decomposition

The scheduler compute boundary is
`deadline.totalMs - deadline.networkMs`. Pre-certification elapsed is
`incumbent + fastPath + search + candidatePreparation`. Certification elapsed
is the final phase through exact validation and comparator selection.

- Decisions: 300, exactly 100 low/default/high fuel.
- Decisions finishing past the compute boundary: 42.
- By fuel: low 0, default 5, high 37.
- Every one of the 42 entered certification before the compute boundary; none
  had already exhausted it in pre-certification work.
- Compute-boundary overrun p95/max across all decisions: 405/1574 ms.
- The three recorded submission skips are the three largest tails:
  - `m-1827` day 6: 798 ms remained at certification entry; certification used
    1633 ms, crossing the boundary by 835 ms.
  - `m-1832` day 6: 929 ms remained; certification used 2503 ms, crossing by
    1574 ms.
  - `m-1832` day 7: 886 ms remained; certification used 2133 ms, crossing by
    1247 ms.

These are BTC target-host timings, not local performance measurements.

## Source path

`UdonShieldEngine::solve_day` assigns each repair candidate an absolute slice
ending no later than the shared compute boundary. `FutureWitnessRepairer::repair_profile`
checks that deadline between scenarios and future days, but in high fuel it can
enter `RouteColumnGenerator::generate` with exact harvest orienteering enabled,
followed by `RouteMaster::solve`, using only the shared wall deadline. The first
nested operation capable of crossing that deadline is the dense exact-state
initialization described below.

## First non-preemptible operation

The first confirmed boundary hole is inside
`enumerate_exact_high_fuel_routes`. After one deadline check, each worker
value-initializes four dense arrays over `2^spots * cellCount` states before the
next poll:

- `distance`: `uint16_t`;
- `patrolFuel`: `uint16_t`;
- `parent`: `uint32_t`;
- `incoming`: `uint8_t`.

At the BTC configuration of 12 spots and 32x32 cells this is 4,194,304 states,
or at least 36 MiB initialized per unique patrol start. Four workers can touch
at least 144 MiB concurrently, and clearing the `jthread` vector must join all
workers before `generate` can return. The next poll inside the state traversal
occurs only after all four arrays and the bucket vector have been initialized.

This source structure matches the target-host stratification: low fuel does not
enable this high-fuel exact path and has 0/100 compute-boundary overruns;
default/high enable it and have 5/100 and 37/100 respectively. It also explains
why the current top-level exact diagnostic can report zero overrun: the slow
calls are nested inside future-witness certification and are invoked without a
diagnostics sink.

## General successor mechanism

The minimal semantics-equivalent successor is cooperative dense-state
initialization. Reserve the same four arrays, construct the same sentinel values
in bounded chunks, and poll the existing absolute deadline between chunks. If
initialization completes, every state, graph transition, queue order, cap and
reconstruction byte is unchanged. If the deadline expires during initialization,
the old code eventually returns the same `supported=true`, `complete=false`,
empty reachability after overshooting; the chunked code returns that state at
the deadline instead. No search heuristic, candidate limit, role path, score
policy or certification semantics is removed.

This closes the attribution. Implementation must be a separately frozen
candidate layered after exact deadline alignment, with byte/exact-score replay
equivalence and BTC target-host reserve telemetry before any promotion.
