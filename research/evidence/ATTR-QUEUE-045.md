# ATTR-QUEUE-045 evidence

Date: 2026-08-09
Parent: `afcd2da` source with a temporary same-binary research switch
Verdict: inconclusive for queue causality; accepted evaluator-wiring diagnosis

The probe generated one shallow-policy prefix through day 9 for opened
BTC-like low-fuel threshold-corridor seed 957002, then replayed the identical
submitted `DecisionResult` history into fresh engines. All four day-10 solves
used state hash `2130807422699737667` and prefix ledger `6/54/244`.

The A/B/A/B result was byte-identical:

| run | day-10 exact | cumulative | plan hash | invalid/emergency |
|---|---:|---:|---:|---:|
| forward shallow | `6/30` | `6/60/274` | `14218929452539799733` | `0/0` |
| forward cardinality | `6/30` | `6/60/274` | `14218929452539799733` | `0/0` |
| reverse cardinality | `6/30` | `6/60/274` | `14218929452539799733` | `0/0` |
| reverse shallow | `6/30` | `6/60/274` | `14218929452539799733` | `0/0` |

However, all four runs reported
`exact_supported=0, exact_settled=0, exact_bundles=0`. The policy equality is
therefore not evidence that the two queue orders are equivalent; the tested
harness did not enter the queue at all.

The wiring cause is exact. `old/harness/historical_tournament.cpp` constructs
`UdonShieldEngine(config)` and therefore uses the constructor default harvest
extension mode `6`. The BTC runtime explicitly defaults to mode `7`. The
terminal fuel-constrained anytime path requires `harvestExtensionMode_ > 6`, so
the prior `SCORE-QUEUE-043` generated protected harness never exercised its own
queue change. Its day-1 divergence and -72 tail are deadline/code-layout drift
on a disabled mechanism, not causal downside evidence.

This does not promote `SCORE-QUEUE-043`: its positive archived production-mode
fixture remains only one development example and it has no valid diverse
protected result. The next probe must repeat only the same-state terminal A/B
with mode `7` explicitly wired in both prefix and fresh engines. Any later logic
candidate still needs a new unopened holdout using production-parity mode `7`.
