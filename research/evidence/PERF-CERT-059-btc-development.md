# PERF-CERT-059 BTC development result

Date: 2026-08-09

## Frozen candidate and target fixture

- Candidate executable SHA256:
  `83EF15BE2139705ACE27B08727ED18074C8B9305E8FBD52566000BCC207B47E0`
- Match: `m-1836`
- Replay: `artifacts/btc/m-1836-perf-cert-059-high-dev-r01-live.jsonl`
- Replay SHA256:
  `B866ACB6A63C548D8BE05FBED03DDBD355422F78566CAC436A1C2E5BD6AC6676`
- Configuration: hard, 3 BTC bots, 10 days, 32x32, 100 steps/day,
  5000 ms response cap, 8 agents, 12 spots, 6 brands, high fuel 300.

This was the first unopened fresh high-fuel development fixture in the frozen
manifest. It was not replaced after exposing a failure.

## Validity and semantic result

- HTTP action results: 10/10 valid.
- Replay-check: exit 0, 10/10 valid, 10/10 independent-validator agreement.
- Authoritative transitions: 9/9 reconciled.
- Emergency decisions: 0.
- Final official score: `6/60/407`.
- BTC rank: 1, recorded only as a runtime sanity result and excluded from
  promotion evidence.

Accepted-response telemetry was p50/p95/p99/max
`3266/3645/3645/3645 ms`. These values are target-host observations, but the
candidate's gate is the internal compute boundary, not rank or response average.

## Preregistered boundary gate

The compute boundary is `deadline.totalMs - deadline.networkMs`, retaining the
existing 1600 ms network reserve. Pre-certification work remained before that
boundary on every day. Total solver work nevertheless crossed it on four days:

| Day | Available ms | Compute boundary ms | Pre-certification ms | Certification ms | Total ms | Crossing ms |
|---:|---:|---:|---:|---:|---:|---:|
| 3 | 4686 | 3086 | 2243 | 1206 | 3449 | 363 |
| 6 | 4799 | 3199 | 2223 | 1023 | 3246 | 47 |
| 8 | 4834 | 3234 | 2835 | 706 | 3541 | 307 |
| 9 | 4710 | 3110 | 2180 | 1148 | 3328 | 218 |

Current-day exact-orienteering telemetry also crossed its own supplied deadline
on day 4 by 42 ms (`1211 ms` remaining at start, `1254 ms` total), although the
overall solver still finished 23 ms before the compute boundary that day. The
other nine days reported zero exact-orienteering overrun.

## Verdict

`PERF-CERT-059` fails its first BTC development fixture: the registered gate
requires zero certification-boundary crossing and zero exact-orienteering
overrun. The semantic and validity gates pass, but they cannot override the
target-host boundary failure. The candidate is not promotable and remains
uncommitted.

The 65,536-state chunk mechanism is retained only as the frozen diagnostic
parent for the next attribution because it is byte-equivalent offline and
removes the first proven large unpolled initialization interval. No further
logic change is authorized until telemetry distinguishes allocation,
preprocessing, traversal, finalization, master, simulation and validation inside
future-witness certification.
