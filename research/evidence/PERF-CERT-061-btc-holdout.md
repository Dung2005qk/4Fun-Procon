# PERF-CERT-061 balanced BTC holdout

Date: 2026-08-10

## Frozen scope

All three fixtures used the frozen candidate BTC executable
`F452AA3936801A68C7E1209D95B5E1724AF8484F480BE44F6475B8677A22039F`
and explicit hard/three-bot/10-day/32x32/100-step/5000-ms/eight-agent/
12-spot/six-brand configuration. Only fuel changed by the preregistered lane.
No failed fixture was replaced. Rank is excluded.

| Fuel | Match | Decisions | Accepted actions | Reconciled observed transitions | Solver max | Minimum window at actual POST | Global certification overrun max | Nested exact overrun max | Emergency / no-POST |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| low | `m-1863` | 10 | 10 | 9/9 | 1915 ms | 1618 ms | 0 ms | 0 ms | 0 / 0 |
| default | `m-1864` | 7 | 7 | 6/6 | 2098 ms | 1611 ms | 6 ms | 10 ms | 0 / 3 |
| high | `m-1865` | 10 | 9 | 9/9 | 2092 ms | 1613 ms | 3 ms | 3 ms | 1 / 1 |

Replay SHA256 values are:

- `m-1863`: `BE7E3753B95C3A566EA03DAC2592CBF02A36D9059A7FDDC5655EFD1F11198C2E`
- `m-1864`: `B4F187FD1D1B98DAD4FF8A6016131528A64DE2B497639AF03AA0DBD8E7834880`
- `m-1865`: `F2F28156A9CA2F84A427E61F333017ADEE9766466454C924D5FA92917808C9FC`

Replay-check exited zero with independent-validator agreement for every recorded
decision. It reconstructed `m-1863` as `6/60/258`, the seven recorded days of
`m-1864` as `6/42/229`, and `m-1865` including the server WAIT as `6/59/320`.
These scores and all bot ranks are validity/counterexample observations, not
strength or promotion evidence.

## Cancellation mechanism result

Across all 27 decisions that reached the solver, material future-certification
tails did not recur. Global certification overrun was at most 6 ms and nested
exact overrun at most 10 ms, versus the attributed parent tails of 381 and
609 ms. Low fuel had zero crossing. Default/high remained bounded by an existing
1024-node poll block plus restoration/unwind. No crossing moved to master,
simulation, validation or final bookkeeping.

Thus the registered DFS cancellation mechanism passed its causal target on all
observed fuel lanes. This does not by itself clear the holistic production gate.

## Holistic failures

`m-1864` terminated after day 7 with `WinHTTP error 12002` during receive. An
immediate resume on the same match and replay retrieved only the final result;
days 8--10 had already been lost. The replay contains seven accepted actions and
no decision for the last three days. It was not replaced.

`m-1865` received authoritative day 2 with only a 1380 ms exact window, already
below the unchanged 1600 ms network reserve. The engine therefore produced its
designed emergency plan, and the final submission gate recorded
`insufficient-authoritative-day-window`, followed by one server WAIT. The other
nine actions were accepted. This is not a 5000 ms solver overrun: the usable
window was already too short at state receipt.

The preregistered requirement was 30/30 accepted actions, 27/27 transitions and
zero emergency/no-POST. Actual result was 26/30 accepted actions, 24/24 observed
transitions, one emergency and four missing POSTs. The holistic gate therefore
fails despite the cancellation fix working as intended.

## Verdict

Rejected as a promotable composite candidate; no commit is authorized. Retain
the semantics-equivalent cancellation repair only as the frozen parent for a
separately registered transport attribution/successor. The next gap is not
another exact-search heuristic: it is the HTTP state-receive path that can
terminate on WinHTTP 12002 or deliver a state after the solver's 1600 ms reserve
is already unavailable. No reserve, submission floor, map, fuel or bot threshold
may be tuned from these matches.
