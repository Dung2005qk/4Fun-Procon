# PERF-P99-055 attribution

Date: 2026-08-09
Parent: `afcd2da`
Frozen executable SHA256: `CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`
Frozen manifest SHA256: `52ADA0B0ECBF3ADE92381D2B4AF8926578D21FFF9433795B71A25A94BC4F225B`

## Frozen result

- All 30 preregistered matches were run exactly once: ten low, ten default and
  ten high fuel. No failed match was replaced.
- All explicit advanced fields matched the manifest. BTC materialized the fuel
  profiles as `100`, `200` and `300` respectively.
- All 300 generated day plans passed the exact replay simulator and independent
  validator; all 270 observable day transitions reconciled. There was no
  emergency decision, exact-orienteering overrun, HTTP-invalid action result or
  transport retry.
- Low and default fuel returned 100/100 accepted action results each. High fuel
  returned only 97/100 because three submissions were deliberately skipped by
  the runner's authoritative-window guard.
- Across the 297 accepted server responses, nearest-rank p50/p95/p99/max was
  `2989/3575/3965/4328 ms`. These quantiles do not pass calibration because the
  three missing submissions are gate failures, not censored observations.

## Counterexamples

### `m-1827`, high replicate 5, solver day 6

- The raw server deadline was `5030 ms` after receipt, but the normalized state
  exposed only `4030 ms` to the planner and runner.
- The planner reported `3264 ms`; only about `760 ms` remained at the guard.
- The runner recorded `actions_deadline_skip` and `actions_server_wait` and did
  not POST `/actions` for day 6.
- BTC final score exposed the failure at tier 2: team A ended with daily sum 57.

### `m-1832`, high replicate 10, solver days 6 and 7

- Day 6 raw server time was `5198 ms`, normalized to `4198 ms`; planner timing
  was `4171 ms`, leaving about `25 ms` at the guard.
- Day 7 raw server time was `5032 ms`, normalized to `4032 ms`; planner timing
  was `3678 ms`, leaving about `349 ms` at the guard.
- Both days recorded `actions_deadline_skip` plus `actions_server_wait` without
  a POST. BTC final score exposed a daily sum of 52.

## Runtime attribution

The initially suspected full-replay serialization is not the material cause.
`ReplayWriter::record` timestamps the decision before dumping it; the following
skip events were timestamped only 2-3 ms later in all three failures.

The first confirmed cause is millisecond loss at the adapter boundary.
`deadline_seconds` in `src/btc_protocol.cpp` validates but ignores the raw server
`endsAt`, then returns integer seconds from
`(receivedMs + responseBudgetMs) / 1000`. HTTP mode later multiplies that value
back by 1000. This floors away 0-999 ms from both the solver budget and POST
deadline. The three failures lost `970`, `802` and `968 ms` relative to the
configured 5000 ms budget. Their raw server deadlines were exactly one second
later than the normalized second boundaries.

The second confirmed cause is reserve enforcement. The deadline profile reserved
`1600 ms` for network, yet the affected total planner times were
`3264/4171/3678 ms` inside normalized total windows `4030/4198/4032 ms`.
Certification accounted for `1633/2503/2133 ms` and allowed the solve to approach
the full normalized window rather than consistently stop at the compute boundary.

The next candidate must use one exact millisecond deadline for both planning and
POST, capped at the configured 5000 ms and the raw server deadline, and must
enforce the existing network reserve across certification. Merely moving replay
telemetry is insufficient. It must preserve pre-POST durable action recording,
idempotent recovery, exact validity and the 5000 ms hard cap; it may not reduce
network reserves or hide skips.
