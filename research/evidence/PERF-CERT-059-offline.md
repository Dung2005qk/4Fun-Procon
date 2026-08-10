# PERF-CERT-059 offline equivalence gate

Date: 2026-08-09

## Frozen scope

- Parent executable: `.tmp-deadline-align-056-bin/udonshield_btc.exe`
  - SHA256: `4636C380578F1AB79973A25E03AAE26E20809F8A5720F914D7DB7F38D1C3EFD6`
- Candidate executable: `.tmp-perf-cert-059-bin/udonshield_btc.exe`
  - SHA256: `83EF15BE2139705ACE27B08727ED18074C8B9305E8FBD52566000BCC207B47E0`
- Candidate test executable: `.tmp-perf-cert-059-bin/udonshield_tests.exe`
  - SHA256: `553606868042CB39794F813D5A302DBB66E1E647D769E97796A6CB3987D87B4D`
- Frozen manifest: `research/holdouts/PERF-CERT-059.csv`
  - SHA256: `E9A6BF19348A1AE04325E59F9CAEC1552FE4690A5506C2EC2040F018D66D2B20`
- Replay identity authority: `research/evidence/PERF-P99-055-progress.csv`
  - SHA256: `8DCDAE465FB065FA0E0CEACFB78B906BA43BE82F37C950B9B7E963136B90976A`

The source delta under test changes only exact high-fuel dense-array
initialization: the same four arrays reserve the same capacity and receive the
same sentinel values in fixed 65,536-state chunks, with the existing absolute
deadline polled between chunks. The cardinality-first queue and exact HTTP
deadline changes are shared parent context and are not attributed to this gate.

## Gate execution

The authoritative cached Visual Studio/Ninja toolchain built
`udonshield_tests` and `udonshield_btc` once. The full unit suite then passed.
No elapsed time from this machine is recorded or used as performance evidence.

Both frozen executables ran `replay-check --response-ms 5000` on every one of
the 30 `PERF-P99-055` live replay files. Before execution, each replay SHA256
was checked against the frozen progress evidence.

## Result

| Fuel lane | Replays | Recorded days | Replay hashes correct | Byte-identical stdout | Exit 0 | Candidate invalid | Validator disagreement |
|---|---:|---:|---:|---:|---:|---:|---:|
| low | 10 | 100 | 10 | 10 | 10 | 0 | 0 |
| default | 10 | 100 | 10 | 10 | 10 | 0 | 0 |
| high | 10 | 100 | 10 | 10 | 10 | 0 | 0 |
| total | 30 | 300 | 30 | 30 | 30 | 0 | 0 |

The parent and candidate outputs are byte-for-byte identical across all 300
recorded day states. This is stronger than the fallback score/state/validator
equivalence gate and establishes no semantic drift on the frozen suite.

## Verdict boundary

The offline gate passes. It does not establish target-host latency or prove that
certification no longer crosses the BTC compute boundary. `PERF-CERT-059`
therefore remains active and uncommitted until fresh BTC development and frozen
balanced BTC holdout gates can run. Bot rank is not evidence for this experiment.
