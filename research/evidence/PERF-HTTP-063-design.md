# PERF-HTTP-063 design

Date: 2026-08-10

## Proven gap

`ATTR-HTTP-062` proves that retryable WinHTTP receive error 12002 escapes every
idempotent GET caller and can terminate a match. It also proves that `/state`
may reach the runtime after the unchanged 1600 ms solver network reserve is no
longer available. The current GET path uses the session-default 5000 ms receive
timeout, whereas the accepted action-ACK path already uses a bounded 750 ms I/O
slice and retryable-error classification.

## Candidate mechanism

Use the already frozen `750 ms` transport I/O slice for each idempotent GET
attempt; this value predates `m-1864`/`m-1865` and is not tuned from them. Add a
single helper that calls the existing `WinHttpClient::request` with that slice,
catches only the already classified WinHTTP timeout/resend errors and reports a
retryable miss to its caller. Non-retryable errors still fail closed.

Wire the helper to all GET callers:

- `/setup` and `/start` continue their existing wait loop and transient-status
  policy, accumulating bounded transport retries.
- `/state` treats a retryable transport miss as one polling miss, records
  telemetry and reaches the unchanged idle post-ACK work before the next poll.
- `/result` treats a retryable transport miss in the same loop and retries later.

The candidate does not retry `POST /assignment`, does not change the accepted
identical-body `/actions` retry path and does not fabricate a state or result.

## Preservation answers

1. Does the candidate remove, disable, defer or reduce designed functionality?
   No. Every GET remains active and retains its status handling. The candidate
   completes missing recovery wiring for idempotent operations and preserves
   idle precompute/proof work.
2. Does it delete anything requiring a proven active equivalent? No code or
   capability is deleted.

## Invariants

- Exact 5000 ms internal cap, raw server deadline, 1600 ms network reserve and
  800 ms submission floor are unchanged.
- No planner, role, comparator, simulator, validator, action body, action retry,
  assignment, ledger or score semantics change.
- Poll delay and transient HTTP status policy are unchanged; no GET retry is
  performed for a non-retryable transport error.
- Replay action/state/validator results remain equivalent on the frozen suite.
- Local time is not performance or promotion evidence.

## Gates

1. Source proof covers `/setup`, `/start`, `/state` and `/result` and preserves
   the existing idle-work branch.
2. One build and full unit suite, followed by all 30 frozen replay checks against
   the frozen `PERF-CERT-061` binary. Local elapsed is ignored.
3. A controlled localhost protocol probe must stall the first `/state` response
   beyond one 750 ms receive slice. The runtime must retry GET, submit exactly
   one action, apply exactly one acknowledgement and finish normally.
4. One fresh default-fuel BTC development match with 10/10 accepted actions,
   9/9 reconciled transitions, zero emergency/no-POST/hard-cap breach and no
   uncaught retryable transport failure. Rank is ignored.
5. Only after development passes, open one fresh low/default/high BTC holdout
   exactly once without replacement. Require 30/30 accepted actions, 27/27
   reconciled transitions, zero emergency/no-POST/hard-cap breach and complete
   retry telemetry. The existing certification-cancellation tail must remain
   bounded and solver/action semantics must remain exact-valid.

Passing these gates closes this bounded lifecycle repair only. It does not by
itself calibrate p99 or prove score strength against real opponents.
