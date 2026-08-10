# PERF-HTTP-064 design

Date: 2026-08-10

## Proven gap

`PERF-HTTP-063` wired all idempotent GET callers to bounded retry, but a
controlled 3000 ms response-header stall produced no timeout under
`WinHttpSetTimeouts` alone. Official WinHTTP documentation defines
`WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT` as the request-handle option that
specifically bounds receipt of all response headers; its default is 90 seconds.

## Candidate mechanism

When and only when `WinHttpClient::request` already receives a positive
`ioTimeoutMs`, keep the existing `WinHttpSetTimeouts` call and additionally set
`WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT` to the identical value on that request
handle before sending it.

This makes the already declared request-level slice effective at the response
header boundary. It applies to:

- the accepted bounded `/actions` ACK path, whose only recovery is identical-body
  resend before the authoritative deadline; and
- the `PERF-HTTP-063` idempotent GET helper, whose only recovery is a later GET.

Unbounded assignment requests (`ioTimeoutMs == 0`) are unchanged. No request
method, path, body, retry count, delay, status policy or deadline changes.

## Preservation answers

1. Does the candidate remove, disable, defer or reduce designed functionality?
   No. It enforces the already designed positive request timeout at the missing
   WinHTTP response-header boundary. GET and action-ACK recovery remain active.
2. Does it delete anything requiring a proven active equivalent? No code or
   capability is deleted.

## Invariants and gates

- Keep the exact 5000 ms cap, raw server deadline, 1600 ms reserve, 800 ms floor,
  750 ms request slice and 220 ms minimum poll.
- No planner, role, comparator, simulator, validator, assignment, action body,
  ledger or score change.
- Non-retryable errors remain fatal. `/actions` may only resend byte-identical
  frozen JSON; GETs remain idempotent.
- One build/unit pass and the frozen 300-state equivalence screen; local elapsed
  is ignored.
- The corrected controlled fixture must record at least one state transport
  retry and still apply exactly one action/ACK/result.
- Only after that passes may one fresh default BTC development match run. Only
  after development passes may the separately frozen low/default/high holdout
  open once without replacement.

Passing remains a lifecycle repair, not p99 calibration or real-opponent score
evidence.
