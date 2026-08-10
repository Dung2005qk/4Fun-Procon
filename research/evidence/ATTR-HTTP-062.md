# ATTR-HTTP-062 idempotent GET transport attribution

Date: 2026-08-10

## Scope

This is a read-only attribution over the already-opened `PERF-CERT-061` BTC
holdout and the current HTTP runtime. It does not change source, retry timing,
the solver, the 1600 ms network reserve, the 800 ms submission floor or any
score logic.

## Target-host evidence

- Default-fuel `m-1864` completed seven accepted decisions and six reconciled
  transitions, then the process terminated with `BTC HTTP request failed during
  receive with WinHTTP error 12002`. Immediate resume of the same match could
  retrieve only the final result; days 8--10 were already lost. This is direct
  evidence that one retryable receive failure can terminate the lifecycle.
- High-fuel `m-1865` wire day 1 was recorded at Unix millisecond
  `1786374173621` with `endsAt=1786374175`, leaving only `1379 ms` before the
  authoritative deadline. This is below the unchanged `1600 ms` solver network
  reserve and correctly triggered the emergency/no-POST path. The observation
  proves a late state-receipt counterexample. It is consistent with a long
  receive attempt, but the replay does not record request-start time, so it does
  not by itself prove whether server availability, network delay or one blocked
  GET was the cause.

## Runtime-path proof

`WinHttpClient` configures a session-wide receive timeout of `5000 ms`. A
request-level shorter timeout is used only when a caller passes `ioTimeoutMs`.

`post_until_deadline` passes the bounded action-ACK slice, catches
`WinHttpRequestError`, classifies WinHTTP timeout/resend errors as retryable and
retries the identical frozen POST body while the authoritative deadline permits.
This is the accepted `PERF-DEADLINE-003` action path.

The idempotent GET path is not wired to that recovery contract:

- `wait_for_get` calls `client.request("GET", path)` with the session default
  and retries only transient HTTP status codes; it does not catch retryable
  transport exceptions. This affects `/setup` and `/start`.
- The main loop calls `/state` directly with the same default timeout and no
  retryable-exception handler. One WinHTTP 12002 therefore escapes `run_http`
  and terminates the process, matching `m-1864`.
- `/result` uses the same raw GET path and can terminate final-result recovery
  for the same transport class.

All four operations are HTTP GETs and are idempotent. Retrying them does not
duplicate an assignment, action, ledger transition or score update.

## Preservation answers

1. Does the attribution remove, disable, defer or reduce designed
   functionality? No. It is read-only and identifies missing wiring in the
   existing lifecycle contract.
2. Does it delete anything requiring a proven active equivalent? No source or
   capability is deleted.

## Verdict

Accepted attribution. The next implementation experiment may add one general
bounded-retry contract for retryable idempotent GET transport failures across
`/setup`, `/start`, `/state` and `/result`. It must preserve polling rate,
transient-status handling, idle post-ACK work, solver/action/ledger semantics,
the exact 5000 ms cap, the 1600 ms reserve and the 800 ms submission floor.

`m-1864` and `m-1865` are attribution/development evidence only. A successor
must freeze a new holdout before source change and may not tune a retry slice,
reserve or guard against either match.
