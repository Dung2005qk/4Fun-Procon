# CEILING-TANKER-DURABLE-147

Date: 2026-08-13
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/CEILING-TANKER-DURABLE-147.csv`
SHA256: `552554C7C69F41EA5172E41A5A79F1D22FBCFFB90DF570BE18D8D6E50CF16E6C`

This is the fresh durable replacement for infrastructure-inconclusive sweep
146. It uses new seeds across the same six terminal tanker families and three
fuel strata. The complete joint step DP and official semantics are unchanged.
Only the research manifest allow-list may accept the new experiment ID.

Every seed runs in a separate bounded process and its output is captured before
the next seed starts. The sealed 54-case holdout may open only after all 18
development cases complete with zero invalid, incomplete or HEAD win. Local
elapsed time is excluded from every score and performance verdict.

## Development result

Low-fuel block completed `0/6/0` exact-vs-HEAD: all six cases tied, with zero
HEAD win, incomplete or invalid. Durable one-case results:

- rendezvous seed `2900000`: tie, hash `8fbd38d2d2acc8be`;
- split-frontier seed `2900100`: tie, hash `9688d0c33333cd5a`;
- stock-contention seed `2900200`: tie, hash `1969c14544549fd6`;
- rare-brand seed `2900300`: tie, hash `2bb2537ab42764b9`;
- mountain-bridge seed `2900400`: tie, hash `327deb6b14ae6955`;
- delayed-claim seed `2900500`: tie, hash `88d10f627f3912f0`.

Default block also completed `0/6/0` exact-vs-HEAD, all ties with zero HEAD
win, incomplete or invalid. Per-case hashes in family order rendezvous,
split-frontier, stock-contention, rare-brand, mountain-bridge, delayed-claim:
`4adb1ce681c62297`, `71f68c9957c7416b`, `9a99d02e33401eaf`,
`381e3e0a76d9dcba`, `e1998ee15cd9b976`, `1d69c75e10c43089`.

Combined low/default is 12/12 exact ties. High remains unopened.

High block completed `0/6/0` exact-vs-HEAD, all ties with zero HEAD win,
incomplete or invalid. Per-case hashes in family order rendezvous,
split-frontier, stock-contention, rare-brand, mountain-bridge, delayed-claim:
`b0196cce0ac89bf0`, `d93917ad9662bb4`, `a9dca0bf70a3c467`,
`92d940d8668cdc0b`, `f0536585352a6bcf`, `f376fcf3dada379a`.

Final development result is exact-vs-HEAD `0/18/0`: 18/18 ties, zero HEAD
win, incomplete or invalid across every family/fuel stratum. The sealed holdout
is authorized to open exactly once.

## Holdout result

Opened exactly once and completed exact-vs-HEAD `0/54/0`: all 54 cases tied,
with zero HEAD win, incomplete or invalid across every family/fuel stratum.
Result hash: `3a958862e744b7a6`.

Combined development plus holdout is 72/72 exact ties. This closes the frozen
terminal tanker domain against checkpoint `cf7e4b4`. It does not by itself
prove every architecture axis converged; reopening requires an independently
generalized exact domain or a fresh dual-valid exact counterexample.
