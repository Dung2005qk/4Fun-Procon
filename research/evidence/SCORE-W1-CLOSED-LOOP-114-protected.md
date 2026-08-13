# SCORE-W1-CLOSED-LOOP-114 protected matrix

Date: 2026-08-13
Parent: `f77c101`
Internal solver and role cap: `5000 ms`

The frozen candidate was compared directly with a separately built parent
snapshot. Local elapsed time is deliberately excluded from the verdict. All
reported plans passed the exact simulator and independent validator; invalid and
emergency counts were zero in every lane.

## General fixed, 120 fixtures

Candidate versus parent was `31/77/12`. Tier 1 had no differences. Tier-2
gain/loss was `14/1` (maximum gain 4, loss 1); tier-3 gain/loss was `43/24`
(maximum gain and loss both 9). Wins occurred in all six families. Overnight was
the weakest stratum at `5/8/7`, but its losses were tier 3 while one win was tier
2; it was not an across-tier or majority regression. All twelve original losses
were rerun candidate-first and reproduced, so they are retained as causal
downside rather than hidden as local noise.

## General exhaustive role, 60 fixtures

The conservative first pass was `13/38/9`. Tier 1 had no differences; tier-2
gain/loss was `4/1`, tier-3 `15/14`, and maximum tier-3 gain/loss was 3. Reverse
execution of the nine apparent losses converted two to exact ties and one to a
candidate win; six reproduced. The conservative first pass remains the recorded
gate. No fixed-lane tail repeated and overnight was positive at `4/4/2`.

## Production deadline role, 60 fixtures

The conservative first pass was `16/37/7`, zero invalid/emergency. Tier 1 had no
differences; tier-2 gain/loss was `3/4`, tier-3 `24/10`. Candidate-first reruns of
all seven apparent losses showed timed role-selection noise: seed 315033 changed
from a candidate tier-2 loss of 3 to a candidate tier-2 gain of 3 when the masks
crossed, and seed 315047 became an exact tie. The remaining causal tier-2 loss is
one unit. The order-attributed result is approximately `17/38/5`, tier-2
gain/loss `6/1`; the unadjusted first pass is retained above for transparency.

## BTC-like 32x32 screen

One low, default and high-fuel fixed-role fixture were screened. All were valid
with zero emergency. High fuel tied. The first low run showed candidate -2
servings and the reversed run showed candidate +1, proving local cutoff noise;
default showed candidate +2 in both orders. No local timing number has promotion
authority.

## Verdict

Protected score and semantic gates pass under the bounded-downside rule. Across
the immutable 54-case holdout plus fixed and conservative native protected lanes,
the candidate wins substantially more fixtures and higher-tier magnitude than it
loses, with no tier-1 regression, zero invalid/emergency and no family that
regresses across role modes. Production-deadline differences were explicitly
order-attributed rather than treated as deterministic. BTC target-host remains
the sole authority for 5000-ms hard-cap, lifecycle and production validity.
