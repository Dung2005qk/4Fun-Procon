# PERF-CERT-061 offline semantic gate

Date: 2026-08-10

## Frozen candidate

- BTC executable SHA256:
  `F452AA3936801A68C7E1209D95B5E1724AF8484F480BE44F6475B8677A22039F`
- Test executable SHA256:
  `307C8ADBE1CA5361CCCBCD42502A3B0703A1BA2553E2921B587B9E1F9A05AB1B`
- Frozen audit parent BTC executable SHA256:
  `2A1BDA4F9E67FC3EC70F3A4144AA14145B9DA5DF64F5DBE29C5B45CC3F203C04`
- Manifest SHA256:
  `D798DD62F7AFA090F2E2801200336B8A44768554569F7AAD50D792EA9057CD9E`

The candidate changes only cancellation propagation inside exact team
feasibility after the existing absolute-deadline poll observes expiry. It does
not change polling cadence, route order, bounds, memoization, node caps, beam,
worker count, graph, comparator or incumbent.

## Result

The cached MSVC/CMake/Ninja toolchain built `udonshield_tests` and
`udonshield_btc`. The full unit suite passed once.

Parent and candidate then ran `replay-check --response-ms 5000` on the 30 frozen
`PERF-P99-055` replays. Every replay hash matched the preregistered progress
evidence.

| Fuel | Replays | Recorded states | Both exit 0 | Byte-identical stdout | Invalid | Validator disagreement |
|---|---:|---:|---:|---:|---:|---:|
| low | 10 | 100 | 10 | 10 | 0 | 0 |
| default | 10 | 100 | 10 | 10 | 0 | 0 |
| high | 10 | 100 | 10 | 10 | 0 | 0 |
| total | 30 | 300 | 30 | 30 | 0 | 0 |

No local elapsed measurement is used. This closes only the offline semantic and
validator gate. Performance/boundedness remains pending one fresh high-fuel BTC
development match with the frozen binary.
