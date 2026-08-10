# DEADLINE-ALIGN-056 offline gate

Date: 2026-08-09
Parent source: `afcd2da` plus the active cardinality-first `src/orienteering.cpp` candidate
Candidate binary SHA256: `4636C380578F1AB79973A25E03AAE26E20809F8A5720F914D7DB7F38D1C3EFD6`
Manifest SHA256: `6D6270CEBBD12D96ACDBF779A8298466A0378793F2173C25D8514A3E843DAC3F`

The production-code delta for this experiment is confined to
`include/udon/runtime.hpp`, `src/runtime.cpp` and `src/btc_main.cpp` at
`+40/-4`. It adds an explicit millisecond-available MatchSession entry point and
uses one HTTP deadline equal to the earlier of raw server `endsAt` and
`receivedAt + responseBudgetMs`. No decision, planner, orienteering, score,
reserve, submission-floor or recovery logic changed.

- `udonshield_tests`: passed.
- Frozen replay equivalence: parent and candidate `replay-check` outputs were
  byte-identical on all 30 PERF-P99-055 replay files and all 300 recorded days.
- Failures: 0; output mismatches: 0.
- No local elapsed time is evidence in this gate.

The offline gate passes. BTC development remains unopened at the time of this
record and is the only authority for submission-window behavior.
