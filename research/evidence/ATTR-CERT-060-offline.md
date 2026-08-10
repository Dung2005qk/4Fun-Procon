# ATTR-CERT-060 offline diagnostic-integrity gate

Date: 2026-08-09

## Frozen binaries

- Parent `PERF-CERT-059` BTC executable SHA256:
  `83EF15BE2139705ACE27B08727ED18074C8B9305E8FBD52566000BCC207B47E0`
- Diagnostic `ATTR-CERT-060` BTC executable SHA256:
  `2A1BDA4F9E67FC3EC70F3A4144AA14145B9DA5DF64F5DBE29C5B45CC3F203C04`
- Diagnostic test executable SHA256:
  `2A7C7C640D9E4FC05AA8F08B6CE0E6B3A2045043713D6DDBA2499CF176F556DB`
- Manifest SHA256:
  `FA868D82D6880A408DC5C146293A4194EDCD59E716347787024C6D27CF78D402`

The diagnostic adds audit-only phase clocks and counters around the existing
future-witness certification operations. It observes both the per-candidate
slice deadline and the shared certification deadline. It does not modify any
branch, deadline, cap, candidate/scenario order, simulator, validator or action.

## Result

The authoritative cached toolchain built the two targets once and the full unit
suite passed. Parent and diagnostic binaries then ran replay-check at 5000 ms on
all 30 frozen `PERF-P99-055` replays. Every replay hash matched the frozen
progress evidence.

| Fuel lane | Replays | Recorded states | Exit 0 | Byte-identical stdout | Invalid | Validator disagreement |
|---|---:|---:|---:|---:|---:|---:|
| low | 10 | 100 | 10 | 10 | 0 | 0 |
| default | 10 | 100 | 10 | 10 | 0 | 0 |
| high | 10 | 100 | 10 | 10 | 0 | 0 |
| total | 30 | 300 | 30 | 30 | 0 | 0 |

No local elapsed measurement is used. The gate establishes action/state/
validator integrity only. The diagnostic binary is eligible for the three
preregistered fresh BTC attribution fixtures; it is not a production candidate.
