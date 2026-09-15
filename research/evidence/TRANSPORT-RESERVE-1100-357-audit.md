# Existing BTC transport reserve audit, 2026-09-09

User explicitly requested serious consideration of reclaiming some1100ms if
search is short of budget. This is a read-only audit, not a SCORE experiment or
permission to alter frozen357. No functionality removed/reduced/disabled/deferred,
nothing deleted, no production edits. Existing checkpoint and lifecycle remain.

Actual source is src/btc_main.cpp: btcProtectedRefinementFloorMs=1100,
btcSubmissionFloorMs=800, ACK slice750, poll220. Both refinement/public deadlines
use action deadline minus max(submissionFloor,1100). The reserve includes final
certificate checks, replay serialization/flush, action serialization, submission
and a possible identical-body retry. It is not1100ms of required computation or
an official rule. Main scheduler's networkFloor1600 is a separate responsibility;
do not conflate these values or the stale1500/1000 prose in API.md.

The12 most recently modified listed BTC replays were selected before measurement.
One is pre-match only; the other11 contain60 action/ACK pairs. Input hashes and
all per-day observations are preserved in the adjacent JSON. These may include
user-operated builds; exact current-binary provenance is not inferred.

Action-log timestamp to ACK-log:median12ms, empiricalp95 75ms,max208ms,min5ms.
Post-certificate protected_slack record to action record:0--1ms. All60 ACKs200/
valid, zero recorded transport retries/deadline events. Minimum observed send-to-
authoritative-deadline1120ms. This supports investigating an oversized normal-
path margin, NOT a p99 guarantee or demonstrated safe replacement for1100ms.

Critical limitation:protected_slack is emitted AFTER final certificate checks.
These logs cannot isolate their full worst-case cost. No retry occurred, so they
cannot bound resend/slow-network failure tails. One750ms ACK slice plus220ms
retry spacing already consumes970ms before the next attempt/overhead.

Decision:do not reject the idea, do not silently reduce the reserve. Finish357
with its frozen budget. If its complete evidence proves a useful candidate is
deadline-limited, register a separate transport-aware reserve-policy experiment,
including delayed/lost ACK and identical-body resend contracts, unchanged5000ms
checkpoint and actual BTC latency/safety gates. Do not claim VM process-RPC
latency certifies WinHTTP/network behavior. If357 has headroom, reserve tuning
does not address its mechanism gap and is not the priority.

Historical203 reserve0/1100 inert result concerns a different suffix mechanism;
it is neither proof of universal budget sufficiency nor permission to revive203.
