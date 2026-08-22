# RUNTIME-BTC-RESERVE-196

Verdict: rejected before holdout and live BTC.

Parent: `f9c001967e597626303a8c22f47a46b2fbdaa04b`.

Frozen manifest SHA256:
`0BFFCB1611443A1BDEC7EC4EE7610393BD158B4C5D135020FA2575A1EC150568`.
The holdout remained sealed.

## Archived BTC transport evidence

The available archive contains 124 accepted actions in 14 matches. Event-time
distributions are:

| measurement | p50 | p95 | p99 | max |
| --- | ---: | ---: | ---: | ---: |
| local action POST to accepted ACK | 13 ms | 34 ms | 61 ms | 132 ms |
| server `response_ms` minus local state-to-action time | not used | 249 ms | 259 ms | 274 ms |
| server `response_ms` | 2806 ms | 3599 ms | 3634 ms | 3646 ms |

There are zero recorded `action_transport_retry` events. This evidence shows
that the fixed 1600-ms reserve is conservative for the archived sample, but it
does not prove that a direct solver-budget increase is score-monotonic.

## Consumed causal score gate

On replay `m-3908`, fixed role mask 2 and otherwise current code, changing only
the day solver reserve from 1600 to 1100 ms produced `6/60/535`. The parent
replay runs were `6/59/466` and `6/60/472`; historical `02df79d` with the same
1100-ms reserve repeated `6/60/537` and `6/60/533`. Thus the 500-ms allocation
is causally valuable on this consumed large-map counterexample.

## Fresh development falsification

Direct parent/candidate timed runs exposed that more time is not a protected
continuation:

- seed `4820000`, fixed mask 4, produced candidate `123` versus parent `121`,
  then tied `121/121` on the registered restart;
- seed `4820001`, native role, initially produced candidate mask 2 score `130`
  versus parent mask 1 score `139`; the candidate immediately reproduced with
  mask 4 score `146`. After freezing the parent-selected mask 1, candidate
  scored `146` versus parent `139`;
- seed `4820003`, fixed mask 4, initially produced candidate `156` versus parent
  `160`; reversed order produced parent `150` and candidate `156`.

All plans were valid and no emergency occurred. The score and role reversals do
not follow the candidate and therefore cannot be used as local performance
evidence. More importantly, the direct 1100-ms path changes the complete search
trajectory and has no invariant protecting the 1600-ms incumbent. This repeats
the structural non-monotonicity already established by experiment 166, even
though the individual local losses are not stable enough to count as candidate
regressions.

## Decision

The direct fixed rollback is rejected. It recovers a large consumed score gap,
but it cannot guarantee that additional search preserves the existing role or
day incumbent, and local timed aggregation cannot establish that guarantee.
The production source and role path return to the 1600-ms parent.

A successor may use a smaller reserve only after the unchanged 1600-ms role and
day decision is frozen. Additional time may run only an exact monotonic refiner
that returns the byte-identical incumbent unless exact simulator plus
independent validator prove a strict official-score improvement under the
registered closed-loop state invariant.
