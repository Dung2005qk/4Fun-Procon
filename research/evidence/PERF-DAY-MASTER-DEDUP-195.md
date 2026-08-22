# PERF-DAY-MASTER-DEDUP-195

Verdict: rejected before holdout.

Parent: `f9c001967e597626303a8c22f47a46b2fbdaa04b`.

Frozen holdout manifest SHA256:
`6220B6E3AD298B182169432DC242301C79268C630CAB2772DDAE140701A3E28C`.
The holdout remained sealed.

## Semantic and replay gates

- The flat length-prefixed fixed-endian membership key induced exactly the
  canonical-plan equality classes over 134 generated plans.
- A complete no-deadline master run preserved candidate count, ordered
  canonical stable IDs, exact score, plan and master diagnostics.
- Exhaustive role selection was identical with the day-only flag disabled in
  the role path.
- Of 31 archived JSONL files, all 24 complete replays were valid and produced
  byte-identical replay-check output in parent and candidate modes. The seven
  incomplete decision/audit fragments failed identically. There were zero
  parent/candidate mismatches.

Archived replay log SHA256:
`D68B5FE5E66CD083152F2698773DA451A36B44E0AE78462901F20E7B39445844`.

## Fresh development falsification

The first timed development pair on seed `4790001`, fixed to the same role mask
selected once by the canonical selector, initially scored parent `6/30/131`
and candidate `6/30/129`. Two registered reversed-order repetitions produced
the same `6/30/129` score and plan hashes for both modes, so the first loss did
not follow the candidate and was classified as local cutoff noise.

Development partial-log SHA256:
`3764C355DEB1DD77E0BD40624D9990B600EA913AE9E96CF27EC30A3E7C641E46`.
Reproduction-log SHA256:
`730FD263F22C13CD0B0B8F638A10F4BD45F2A363B4E60F6AEF6D969B9B156F32`.

## BTC counterexample and historical attribution

Authenticated match `m-3908` used hard difficulty, three BTC bots, 10 days,
32x32, 100 steps/day, 5000-ms response, eight agents, 30 spots, six brands and
high fuel. Candidate 195 was valid on all 10 days, reconciled all nine
transitions and submitted `6/60/508`; the best bot scored `6/60/579`.
Rank is excluded from promotion, but the score deficit is retained as a
counterexample. Replay SHA256:
`618ED4AE3D96CFFCE0C9E7B139156876203F214ED94920B90B3341159C1B3CC9`.

On the same replay states and fixed role mask 2, two candidate counterfactual
runs both scored `6/60/495`. Parent runs scored `6/59/466` and `6/60/472`, so
the live deficit was not caused by the flat-key candidate. Historical
counterfactuals then exposed a stronger protected lane:

| commit | score |
| --- | --- |
| `02df79d` | `6/60/537`, repeated `6/60/533` |
| `fa21950` | `6/60/461` |
| `38c224b` | `6/60/495` |
| `831ca4d` | `6/60/461` |
| `7ee0c8f` | `6/60/491` |
| `8296cc6` | `6/60/461` |
| `6f84a06` | `6/60/496` |
| `7ef3694` | `6/60/491` |
| `828ea78` | `6/60/461` |

The first regression is `fa21950`. Its relevant solver-budget change raises
the BTC `networkFloor` from 1100 to 1600 ms, reducing the observed per-day
solver window on this replay from about 3875 to 3375 ms. Later score mechanisms
recover part of the loss but do not restore the `02df79d` lane. Local elapsed is
not performance authority; the causal budget change and exact scores justify a
new target-host reserve experiment, not an immediate reserve rollback.

## Decision

The candidate is rejected as a standalone production change because its
marginal benefit over direct parent `f9c0019` is not yet generality-qualified.
The consumed replay is positive (`495` versus `466/472`), but the fresh timed
development difference vanished under reversed-order reproduction and no
paired target-host evidence establishes a stable improvement. The older
`02df79d` score is retained only as attribution evidence for a separate runtime
reserve regression; failure to reach that historical score is not attributed
to experiment 195 and is not by itself a rejection gate for the flat key.

All experiment-195 source wiring is reverted and the sealed holdout is not
opened. The flat key may be reconsidered after a successor establishes a
target-host-safe protected reserve. Its next comparison must be that successor
parent versus successor plus the day-only flat key; `02df79d` remains a
historical reference rather than the marginal parent.
