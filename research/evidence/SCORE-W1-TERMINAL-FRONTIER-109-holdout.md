# SCORE-W1-TERMINAL-FRONTIER-109 holdout

Parent: `f77c101`

Frozen manifest SHA256:
`2864E9070E03DC031469CEA1309D035ED9BAB38148EF19EAA6F18459C713CBFB`.

The immutable 54-fixture holdout opened once after development passed. Direct
candidate-vs-parent result was `4/46/4`, invalid 0.

- First-differing tier-2 gain/loss sums: `+5/-2`; maximum `+3/-1`.
- First-differing tier-3 gain/loss sums: `+2/-2`; maximum `+2/-1`.
- branched-balanced/low: `1/1/1`.
- fuel-split/low: `0/1/2`.
- terminal-fork/low: `2/0/1`.
- terminal-fork/default: `1/2/0`.
- All other strata tied.

The four losses reproduced exactly in the reverse-order A/B/B/A attribution,
including identical score and plan hashes. They are binary-causal, not local
cutoff noise. Although aggregate gain magnitude exceeds aggregate loss, the
candidate is not a broad general improvement: low fuel is net-negative by case,
and fuel-split/low has a systematic two-of-three regression. This violates the
preregistered downside gate. No threshold, route rank, fuel condition or
dispatcher may be tuned on this opened split.

Verdict: rejected. BTC and protected matrices remain sealed. All production
source and tests are restored to `f77c101`; only research records and the probe
adapter remain.
