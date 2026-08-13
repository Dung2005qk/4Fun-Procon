# ATTR-W1-UPPER-FRONTIER-107

Parent: `f77c101`

On consumed CEILING-CYCLE-PATROL-095 seed 1600000 exact day-2 state, the
unchanged complete per-agent outcome enumerator produced 53 outcomes for the
first patrol and 72 for the second. Exact joint stock composition yielded 3,324
unique terminal/score states. Every state was evaluated by the unchanged
FastViabilityAnalyzer from day 3.

The known oracle terminal class was present. Its candidate-specific upper was
`6/24/30`, equal to the maximum upper over the full frontier, so its upper rank
was 1. Thirty-nine unique terminal/score classes shared that maximum. Thus the
production upper is informative enough to keep the correct future-state class;
the failure is representation, not value ranking.

107 closes as accepted attribution. Blind retention of every reachable spot
mask is unnecessary and potentially exponential. A bounded structural frontier
is justified: retain the existing inclusion-maximal current-day routes and add
at most one served route per terminal cell chosen by an exact terminal-state
resource rank. Generate this as an additive exact bundle lane and preserve the
old W1 witness before accepting any strictly better complete witness. Any source
candidate requires a fresh split and may not route by fixture, fuel label or
seed.
