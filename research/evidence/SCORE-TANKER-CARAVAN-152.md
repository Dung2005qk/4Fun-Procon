# SCORE-TANKER-CARAVAN-152

Date: 2026-08-14
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/SCORE-TANKER-CARAVAN-152.csv`
SHA256: `EB3BB794D53BE196AF63890266738DE3BD25620FAD9A659BE591ADC1ADA26E31`

151 proves the existing two-window mobile-hub set cannot express exact witness
3200100. Existing `event_routes` can already refuel repeatedly across multiple
rendezvous windows. Candidate 152 therefore adds no solver: it extends only
mobile hub generation with a bounded deterministic waypoint beam built from the
same public start/spot cells and `ParetoRouter`. A tanker trajectory may expose
up to four consecutive rendezvous windows; the existing EventConflict route
and exact evaluation layers remain unchanged.

The nonterminal admission guard from 151 remains mandatory: roadless map,
absolute all-brand/all-stock current-day score, all patrols at full terminal
fuel and one common terminal cell. The full static parent phase runs first and
the terminal-day behavior from `cf7e4b4` remains unchanged. Anchor 3200100 must
close before any fresh seed runs.

## Development result

The research adapter accepted the frozen manifest without changing oracle
semantics. Parent baseline was captured first: anchor `3200100` scored
`3/12/15`; the 12 fresh rows were valid and their parent scores were recorded
before candidate source existed.

The bounded four-waypoint tanker beam compiled and the full unit suite passed.
On the mandatory consumed anchor, candidate HEAD still scored `3/12/15`; day 1
remained `3/3/3` with the exact same parent plan and terminal state. The exact
target remains `3/12/16`, with day 1 `3/3/4`. Per the frozen gate, production
source was restored immediately. No fresh candidate score was run.

Verdict: rejected. More tanker windows do not express the missing joint route;
the earliest remaining gap is shared multi-patrol escort/lockstep generation.

## Holdout result

Sealed and unopened.
