# FINAL-QUEUE-065 design

Date: 2026-08-10

This is the sole closure candidate from `afcd2da`. It contains exactly four
previously registered mechanisms:

1. canonical cardinality-first pending-label ordering when `minimumSpots` is
   present (`SCORE-QUEUE-048`);
2. one exact HTTP action deadline equal to the minimum of raw server `endsAt`
   and receipt time plus the configured 5000 ms cap (`DEADLINE-ALIGN-056`);
3. identical exact-route sentinel initialization in fixed chunks with the
   existing absolute deadline checked between chunks (`PERF-CERT-059`);
4. invocation-local recursive feasibility cancellation that restores state and
   unwinds all frames after the existing deadline poll expires
   (`PERF-CERT-061`).

Rejected GET retry/header-timeout logic and attribution-only phase telemetry are
not part of the candidate. No deadline value, reserve, submission floor, queue
cap, graph, dominance rule, route reconstruction, worker count, action body,
ledger transition, simulator or validator is changed. Nothing is deleted from
the designed production architecture; only unaccepted experiment code is
returned to the `afcd2da` implementation.

Promotion authority is the frozen 223-pair score evidence plus the manifest at
`research/holdouts/FINAL-QUEUE-065.csv`. Build and local replay checks may only
verify semantics. Runtime and deadline claims require the frozen candidate on
the twelve preregistered BTC matches; rank is ignored.
