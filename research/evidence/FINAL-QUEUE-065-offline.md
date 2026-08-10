# FINAL-QUEUE-065 offline result

Date: 2026-08-10

The cached MSVC/CMake/Ninja toolchain built the clean final source once. The
full unit suite passed once. Frozen executable hashes are:

- BTC: `8A8AD48AD5A9227AA43C9FF685D20CE7CA1EF657EBCDFC2A675C6FEA9A16816D`;
- unit tests: `2374B50281B431DF50F32688A603DC279CD2E66663F9DE3610CDB112A9C39ED3`.

Against frozen canonical `SCORE-QUEUE-048` executable
`CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`,
all 30 preregistered low/default/high replays and all 300 states exited zero
with byte-identical replay-check stdout. Every replay hash matched the frozen
PERF-P99 manifest. Local elapsed time was ignored.

The production source contains only canonical cardinality-first ordering, exact
HTTP deadline alignment, chunked identical exact-state initialization and
recursive feasibility cancellation. Rejected GET retry/header code and
attribution-only decision telemetry are absent. This passes the offline gate and
authorizes only the twelve frozen BTC recurrence matches; it does not authorize
promotion or commit by itself.
