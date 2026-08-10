# SCORE-QUEUE-043 evidence

Date: 2026-08-09
Parent: `afcd2da`
Verdict: rejected on independent downside tail

- Frozen holdout: `research/holdouts/SCORE-QUEUE-043.csv`
- SHA256: `BFFB6312262CC28A21D8131968BF5A603CB37B12EB087472F1D0E2CBDE8AE48B`
- Candidate changed only the resource-search queue key from shallow
  steps/fuel to greater spot cardinality, then steps/fuel, under the unchanged
  1,250,000-state and 32-route caps.
- Unit suite passed.

The causal `m-1285` day-10 gate confirmed the mechanism. Representative agent 0
changed from 24 six-spot plus eight five-spot retained routes to 24 seven-spot
plus eight six-spot routes; proof mask `0x9D9` appeared at index 2. Exact
seed/local servings increased from 34 to 37, and selected official score
increased from frozen parent `6/60/321` to exact-valid `6/60/323`. ALNS and
recombination improvements were zero, so the gain came directly from the new
frontier and exact coordinator.

The separate high-fuel `m-1286` control tied exactly at `6/60/412`, with selected
score equal to guidance and zero invalid/emergency/overrun. Canonical plan bytes
differed because the two wall-bounded runs completed different counts of
terminal variants despite the high-fuel source path being unchanged; this was
tracked but caused no official-score downside on the terminal day.

The complete fresh general-fixed lane was `0/18/0`; every official score and
combination count matched, with zero invalid/emergency.

The first four frozen BTC-like low-fuel cases were decisive:

| seed/family | parent | candidate | delta |
|---|---:|---:|---:|
| 957000 balanced | 252 | 252 | 0 |
| 957001 rare-brand | 292 | 292 | 0 |
| 957002 threshold-corridor | 346 | 274 | -72 |
| 957003 fuel-tight | 372 | 372 | 0 |

All first two official tiers tied, so the loss is tier 3, but its magnitude 72
is an unacceptable downside tail relative to the development gain of two. This
is an official-score rejection, not a local performance conclusion. The
remaining low-fuel seeds, native/deadline/default/high/archive lanes and BTC
stayed unopened.

Production source was restored byte-identical to `afcd2da`; no commit was
created. Do not tune a cardinality/steps weight or alternate priority against
the opened `m-1285` and seed 957002 fixtures. Reopen anytime queue scheduling
only from a new diverse capability design that preserves both shallow route
diversity and deep-route discovery by construction, with a new holdout.

## Post-audit correction from ATTR-QUEUE-044/045

The `-72` binary observation is real but is not causal queue evidence. Day
details show action bytes diverge on day 1 and score diverges on day 2, while the
intended current-day anytime queue is terminal-only. More decisively, a
same-binary terminal A/B reported `exact_supported=0` under both policies.
`historical_tournament.cpp` used the `UdonShieldEngine(config)` default harvest
mode `6`; BTC production defaults to mode `7`, which is required to enable the
fuel-constrained anytime route path. Thus every generated protected lane in this
experiment ran with the candidate mechanism disabled. `SCORE-QUEUE-043` has no
valid global protected verdict and remains unpromotable/inconclusive, with source
reverted. Its opened seeds remain unavailable for tuning or future promotion.
