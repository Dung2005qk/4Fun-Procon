# PERF-HTTP-063 offline result

Date: 2026-08-10

## Frozen binary

- BTC executable SHA256:
  `21D9C7E6BC606C80E1B12A3886745006DE524380612E2235306C372B4EE07B6B`
- Unit executable SHA256:
  `307C8ADBE1CA5361CCCBCD42502A3B0703A1BA2553E2921B587B9E1F9A05AB1B`

The cached MSVC/CMake/Ninja toolchain built the BTC target once and the full
unit executable passed once. Candidate and frozen `PERF-CERT-061` replay-check
stdout was byte-identical on all 30 frozen low/default/high replays and all 300
recorded states; every process exited zero. Local elapsed time was ignored.

## Controlled GET result

The first localhost attempt was an invalid fixture because it exposed
`/result=200` before any action, so the runtime correctly ended the match and
the result was not interpreted.

The corrected fixture held the first `/state` response for 3000 ms, kept result
unavailable until after an action, then supplied the frozen valid state and ACK.
The runtime completed normally with exactly one action POST, one accepted action
result, one result and one unique action body. However it recorded zero
`state_transport_retry` events. The server observed three state GETs: the
delayed first response, the authoritative state and the post-action result
probe. Thus the caller wiring is coherent, but the registered 750 ms receive
slice did not cancel response-header wait.

Microsoft's WinHTTP documentation distinguishes the transaction receive timeout
from `WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT`, which specifically bounds the
wait for all response headers. The latter defaults to 90 seconds. The controlled
result proves that `WinHttpSetTimeouts` alone is not a sufficient causal
implementation for this runtime path.

References:

- https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpsettimeouts
- https://learn.microsoft.com/en-us/windows/win32/winhttp/option-flags

## Verdict

Rejected at the controlled development gate. No BTC development match or frozen
holdout was opened. Preserve the already verified idempotent GET caller wiring
as a frozen research parent only. A successor requires a new manifest and may
add the documented response-header timeout option for requests that already
carry a positive bounded I/O slice. It may not change the 750 ms value, reserve,
submission floor, poll interval, solver or action/ledger semantics.
