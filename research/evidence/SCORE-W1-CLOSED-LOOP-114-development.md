# SCORE-W1-CLOSED-LOOP-114 development gate

Date: 2026-08-13
Parent: `f77c101`
Manifest: `research/holdouts/SCORE-W1-CLOSED-LOOP-114.csv`
Manifest SHA256: `EF74321329127E4B34B45C74386B4EC6464EAFE3EF0EADDF8F454FEE19D46DCB`

The candidate was frozen before score evaluation. It composes the unchanged
mechanisms preregistered from 109 and 113: bounded minimum-fuel served-spot exact
routes and aligned bundles create an optional strictly-better whole-horizon W1
certificate; roadless cache then whole-suffix revalidates that certificate from
each authoritative next-day state. The legacy W1 and cache paths run first and
remain fallback. No plan is forced.

## Paired result

Direct source-frozen candidate versus parent at the 5000-ms solver cap produced
`10/8/0`, invalid `0`, in both first and reversed execution order. First differing
tier gain was tier 2 on two fixtures, total `+2`; eight further gains were tier 3,
total `+13`. There was no loss at any tier.

The ten gains span all fuel strata: three low, three default and four high. They
also span diamond-balanced, diamond-duplicate, fuel-diamond and terminal-diamond
families. This is a broad development signal rather than a single-seed or
single-fuel effect.

## Closed-loop attribution

On low-fuel seed `2100000`, the candidate finished `6/19/22` versus parent
`6/18/23`. Day 1 selected a complete certified witness with lower bound
`6/17/25`. At authoritative day 2 the cached three-plan certificate was eligible,
reused and retained; whole-suffix replay was dual-valid and reproduced
`6/17/25` exactly. At day 3 the remaining two-plan certificate again revalidated
and reproduced its source final score `6/19/22` exactly. The comparator was left
in control (`strict_takeover_days=0`).

This closes the intended causal chain: the representation creates a stronger
certificate and persistence preserves it without stale execution or forced
selection. The immutable 54-fixture holdout may therefore be opened once without
any candidate change.
