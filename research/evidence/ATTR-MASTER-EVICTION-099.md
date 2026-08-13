# ATTR-MASTER-EVICTION-099

Parent: `f77c101`

This read-only attribution used only the consumed CEILING-CYCLE-PATROL-095
cycle-balanced/low seed 1600000, day 1. The exact day plan is dual-valid, all
three agent routes are present in the canonical merged 16-column portfolio, its
day score is `6/6/9`, and its admissible full-match upper is joint-best at
`6/24/30`. No new score or promotion evidence was sought.

The existing cap sweep first separated branch-and-bound from retention. With the
same cap32 and 40000-combination limit, a quality-only population (`diversity=0`)
retained the exact outcome, while canonical 32/8 did not. Disabling
branch-and-bound reproduced both results: `no_bnb=0:1/8:0`. The canonical run
visited 366 combinations and returned a best current-day score of `6/6/12`.

Because the existing output could not prove whether canonical 32/8 had reached
the exact target before eviction, two temporary audit bits were added for one
replay and then removed. The canonical run reported
`target_evaluated=1,target_retained=0`. Thus the exact plan had already passed
master evaluation under the unchanged search and was subsequently removed by
population maintenance; it was not hidden behind a branch that required the
wider cap to explore.

This closes 099 as accepted causal attribution. A supplemental quality reservoir
can observe the same already-evaluated candidate stream while the canonical 32/8
reservoir remains the sole authority for branch pruning, ALNS and baseline F0.
It therefore does not require a second route search or a second solver. Any
successor must keep the baseline lane unchanged, consider only supplemental
candidates absent from it, and allow takeover only under strict certified
dominance after the baseline candidate has been selected. Seed 1600000 is
consumed attribution evidence and cannot tune or promote that successor.
