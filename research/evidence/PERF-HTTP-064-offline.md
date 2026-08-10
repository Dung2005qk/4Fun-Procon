# PERF-HTTP-064 offline result

Date: 2026-08-10

The frozen candidate executable SHA256 was
`ECB20530412264CF5C4DCCC1340A163D447272A9AB28610FF7ED87A1C5AF6E85`.
The full unit suite passed once. Against frozen `PERF-HTTP-063`, all 30
preregistered replay-check runs and all 300 states exited zero with
byte-identical stdout. Local elapsed time was ignored.

The corrected localhost fixture deliberately withheld the first `/state`
response headers for 3000 ms, kept `/result` unavailable until an action was
accepted, and then served one frozen valid state, action ACK and result. The
candidate completed normally with exactly one action POST, one unique action
body, one accepted action result and one result, but recorded zero
`state_transport_retry` events. Adding
`WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT` therefore did not establish the
registered causal recovery contract for a silent response-header wait.

Verdict: rejected before BTC. The GET retry and response-header option are
absent from final source. The natural `m-1864` WinHTTP 12002 remains a single
target-host sample and may reopen only after repeated evidence under a stable
network; no further synthetic timeout variant is authorized from this sample.
