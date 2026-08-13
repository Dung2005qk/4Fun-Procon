# SCORE-W1-TERMINAL-FRONTIER-109 development

Parent: `f77c101`

Frozen manifest SHA256:
`2864E9070E03DC031469CEA1309D035ED9BAB38148EF19EAA6F18459C713CBFB`.

The 18-fixture source-frozen A/B/B/A development comparison was candidate vs
parent `3/15/0`, invalid 0. All three gains reproduced identically in both
orders; there were no losses.

- branched-duplicate/low seed 1800100: `5/19/30 -> 5/20/30` (tier 2 +1).
- fuel-split/low seed 1800400: `6/22/26 -> 6/23/27` (tier 2 +1, plus serving).
- terminal-fork/low seed 1800500: `5/20/27 -> 5/20/28` (tier 3 +1).

Audit attribution is W1-causal. On day 1 the candidate selected an
`upside-challenger` in all three gains, while the parent selected the
`floor-leader`. The candidate's completed certified lower became `5/18/30`,
`6/22/27`, and `5/17/29`; the resulting exact-valid trajectories produced the
better final scores above. Unit tests and independent route simulations passed.
No local elapsed measurement is used as performance evidence.

Development passes. The immutable 54-fixture holdout may open once without
retuning. BTC remains sealed until holdout and protected score/semantic gates.
