# SCORE-W1-CLOSED-LOOP-114 frozen holdout

Date: 2026-08-13
Parent: `f77c101`
Manifest SHA256: `EF74321329127E4B34B45C74386B4EC6464EAFE3EF0EADDF8F454FEE19D46DCB`

The immutable 54-fixture holdout was opened after the candidate and development
evidence were frozen. No source, rank, threshold, dispatcher or manifest field
was changed after opening.

The preregistered parent/candidate/candidate/parent execution completed, but its
PowerShell report failed after collection because a parser expression produced an
array subtraction error. With candidate source unchanged, one parent/candidate
pair was rerun solely to recover the lost score table. This rerun is the reported
table; it was not used to alter or select logic.

## Paired result

Candidate versus parent: `26/25/3`, invalid `0`.

- Tier 1: gain `0`, loss `0`.
- Tier 2: gain `11` across nine winning fixtures, loss `1` across one fixture;
  maximum gain `2`, maximum loss `1`.
- Tier 3: gain `42` across seventeen winning fixtures, loss `2` across two
  fixtures; maximum gain `5`, maximum loss `1`.

The candidate wins in low, default and high fuel. Gains cover diamond-balanced,
diamond-duplicate, rare-diamond, fuel-diamond and terminal-diamond. Rare/stock
families mostly tie; no fuel or family forms a systematic losing stratum.

The three losses are bounded: low/diamond-duplicate loses one serving; low/
terminal-diamond loses one daily distinct; default/fuel-diamond loses one serving.
All three reproduced exactly with candidate executed before parent, confirming
they are causal. There is no tier-1 loss and no tail loss greater than one at the
first differing tier.

## Verdict

Holdout passes. The benefit is global and materially larger than the bounded
downside: 26 wins against 3 small losses, tier-2 gain/loss `11/1`, tier-3
gain/loss `42/2`, with coverage across all fuel strata. This is not absolute
dominance, but it meets the preregistered global-improvement rule. Promotion is
still blocked on protected-matrix semantic/score gates and BTC target-host hard-
cap validation.
