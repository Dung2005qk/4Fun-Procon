# SCORE-F0-UPPER-087 development

Date: 2026-08-12

Parent: `00711ba`.

Candidate mechanism: keep the existing 12/16 current-quality F0 slots and use
the remaining four existing diversity slots by candidate-specific admissible
future upper first, then the unchanged max-min plan distance and current-score
comparator. Bounds are computed once for the unchanged master output and reused
during W1 setup. Master output, F0/W1 count, operation caps and 5000-ms hard cap
are unchanged.

On the unchanged 18-case 085 development split, candidate-vs-parent W/T/L is
`4/14/0`, zero invalid. All four wins are tier 2 under low fuel:

| Family | Parent | Candidate | Exact | Paired gain |
|---|---:|---:|---:|---:|
| balanced | 5/17/24 | 5/19/24 | 5/20/26 | +2 tier 2 |
| duplicate-brand | 4/15/26 | 4/16/25 | 4/16/26 | +1 tier 2 |
| fuel-allocation | 5/19/21 | 5/20/22 | 5/20/24 | +1 tier 2 |
| terminal-separation | 4/15/24 | 4/16/23 | 4/16/26 | +1 tier 2 |

Three old tier-2 exact gaps are reduced to tier 3; balanced's tier-2 gap shrinks
from +3 to +1. The other 14 fixtures tie parent and exact. Candidate oracle gap
hash: `6e4072b927d9fa2a`; parent oracle gap hash: `c7324c01ff35e2e8`.

This passes the development gate. The frozen 54-case holdout may now be opened
once, paired against an independently built parent using the identical probe.
Local elapsed remains non-authoritative.

## Frozen holdout result

The holdout was opened exactly once with identical probe source. Frozen binary
SHA256 values were candidate
`8B184AC3B32E20309F98D3CF5A2CF1A71C5D15ECA374EE8A8B99063D7ACC762D`
and parent
`C8D68941D6317D207811D3C3C7BC60CC30420E3034055803E48BB0B4545A81E6`.

Candidate-vs-parent W/T/L was `8/42/4`, zero invalid. First-tier gain/loss
sums were `18/4`; maximum gain was `+4` and every loss was exactly `-1`.
By stratum, duplicate-brand low was `3/0/0`, fuel-allocation low `2/0/1`,
terminal-separation low `2/0/1`, and balanced low `1/0/2`; all default/high,
stock-contention and coverage-trap fixtures tied. Candidate and parent each
remained below the exact oracle on 12/54 cases, but candidate shifted two old
tier-2 gaps down to tier 3: candidate oracle gaps were 9 tier 2 and 3 tier 3,
versus parent 7 tier 2 and 5 tier 3. Candidate oracle-gap hash was
`dfdc999d65370d3a`; parent was `743edb2ffa2667da`.

The paired global effect is positive with bounded downside, so the unchanged
candidate advances to road-containing fixed/native protected lanes. The four
losses are immutable protected tails and may not be tuned. Performance remains
unjudged until BTC target-host telemetry.

## Protected rejection

The road-containing 36-case fixed all-patrol lane rejected the candidate:
candidate-vs-parent W/T/L `1/32/3`, zero invalid/emergency, first-tier gain/loss
sums `2/6`, tails `+2/-4`. Losses spanned rare-brand tier 3 -1, overnight tier
3 -4 and threshold-corridor tier 2 -1; the only gain was fuel-tight tier 3 +2.

Day-level attribution with both frozen binaries showed different plans from day
1 on rare-brand, fuel-tight, overnight and threshold-corridor. The first three
did not hit a search deadline; threshold-corridor did hit one on candidate day
1. Thus the protected losses are not dismissible as wholly inactive
compile-layout noise: upper-first intake causally changes road/traffic decisions,
and its extra viability passes also worsen deadline exposure. The production
diff was fully removed. Native and BTC were not run.

Reopen is forbidden as a universal intake rule. A successor may use the rule
only in the already-proven deterministic no-road domain, must keep road maps on
the parent path without the extra upper passes, and must pass an independent
roadless protected lane without tuning.
