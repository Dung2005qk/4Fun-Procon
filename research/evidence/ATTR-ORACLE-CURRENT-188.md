# ATTR-ORACLE-CURRENT-188

Date: 2026-08-22

Current production: `5c3aa7a`

Source oracle: completed experiment-185 slice `1721100 [0,88)`

Frozen consumed manifest:
`research/holdouts/ATTR-ORACLE-CURRENT-188.csv`

SHA256:
`B5210335257EB5D7A17A6457E102DB29A492DD88665C6FF8E9DFA7A21B931905`

## Question

Experiment 185 was registered against parent `828ea78`. Current production
`5c3aa7a` adds experiment 187 after the bounded decision: protected WAIT
refinement, acknowledgement-gated divergence and future planning from a virtual
parent. The 185 harness did not call this runtime layer. This attribution asks
whether the completed exact witness remains a counterexample of current
production.

## Method

The already-opened seed `1721100` was the only fixture used. The stale harness
was first rebuilt from the current working tree and required to reproduce its
registered baseline. An opt-in research-only mode then replayed the exact BTC
production state machine:

- the solver received the canonical 5000-ms budget and BTC HTTP calibration;
- protected refinement ended at the same absolute compute boundary minus the
  1600-ms network reserve;
- authoritative state was used until an improved plan was acknowledged;
- after divergence, the bounded decision continued from virtual-parent agents
  and ledger and was exact-revalidated on authoritative state;
- simulator, independent validator, transition dominance and ledger dominance
  had to agree after every day;
- the terminal day did not run protected WAIT refinement.

Local elapsed time has no performance authority. This probe establishes only
the score and state-machine relation on the consumed exact fixture.

## Reproduction control

The rebuilt stale-head path reproduced exactly:

- score `5/17/17` under maximum-dwell;
- plan-sequence hash `32dcfc4fa4a20fd6`.

This is identical to the completed 185 record.

## Current-production result

Two consecutive protected-production runs were field-identical:

| policy | exact 185 oracle | current `5c3aa7a` | first differing tier | current plan hash |
|---|---:|---:|---:|---:|
| maximum-dwell | `5/17/18` | `5/17/17` | servings `+1` oracle | `32dcfc4fa4a20fd6` |
| minimum-dwell | `5/17/18` | `5/16/17` | daily distinct `+1` oracle | `a9d910dc3b481ff7` |
| status-toggle | `5/17/18` | `5/17/17` | servings `+1` oracle | `32dcfc4fa4a20fd6` |

All three runs were exact-valid. Each reported:

- protected takeovers: `0`;
- generated protected plans: `0`;
- valid protected plans: `0`;
- liftable protected plans: `0`;
- protected deadline days: `0`;
- solver deadline days: `0`;
- actual score equal to virtual-parent score.

The accepted 187 mechanism is therefore inactive on this fixture and does not
close the exact gap. The three policy rows are three witnesses for one
underlying seed/state counterexample, not three independent counterexamples.

## Verdict

Closed `accepted-attribution-current-gap`. Experiment 185 has produced one
exact counterexample that survives revalidation against current competition
production `5c3aa7a`. It is not a regression caused by experiment 187; the
current plan is byte-identical to the stale parent because the protected
neighborhood contains no generated alternative.

This result authorizes causal attribution of the missing multi-day action under
a separate experiment. It does not authorize tuning seed `1721100`, weakening
the protected admission relation or modifying production before a general
mechanism is identified and tested on fresh stratified development.

## Functionality preservation

No production functionality or call path changed. The only source change is an
opt-in research-probe mode. Nothing was deleted, disabled, deferred or reduced.
