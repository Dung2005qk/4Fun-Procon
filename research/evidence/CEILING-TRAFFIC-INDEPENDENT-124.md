# CEILING-TRAFFIC-INDEPENDENT-124

Parent: `5dccb0f`

The exact Markov quotient from 115 was run unchanged on fresh traffic seeds. The
first multi-case orchestration was terminated without usable partial output
after 20 minutes because stdout buffered across fixtures; no result from it was
used. Two distinct cases were then completed individually:

- traffic-duplicate/low seed2300100: oracle `5/20/22`, HEAD `5/13/21`, tier 2
  gain 7, both valid;
- threshold-loop/low seed2300200: oracle `6/22/22`, HEAD `6/15/20`, tier 2
  gain 7, both valid.

For threshold-loop day 1, the oracle plan scored only `6/6/6` versus HEAD's
`6/6/11`, but its valid upper was `6/24/28` versus `6/17/22`. All three routes
were present in legacy, expanded and merged portfolios. The unchanged merged
master retained the exact plan and equivalent outcome, and the upper ranked it
first; cap32/diversity8 retained it. Nevertheless it matched none of the 16
final audited decision candidates. The final exact result was seven daily
distinct higher.

Verdict: accepted attribution. The gap independently repeats and, in the fully
traced case, occurs after master retention rather than in route generation. The
remaining development and all holdout fixtures stay sealed. Research may now
trace F0/profile intake on this consumed development witness; it may not tune or
redesign from the sealed 124 data.
