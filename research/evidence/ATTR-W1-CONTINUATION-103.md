# ATTR-W1-CONTINUATION-103

Parent: `f77c101`

This read-only attribution used only consumed CEILING-CYCLE-PATROL-095
cycle-balanced/low seed 1600000. It replayed the exact oracle day-1 plan, then
reconstructed day-2 W1 generation and master selection with the production
repair settings: three columns per agent, six targets, harvest extensions,
one retained master candidate, one resolve round and 100 combinations. Exact
simulator and independent validator remained authoritative.

The first divergence occurred immediately on day 2. Portfolio widths were
`3|3|1`; the exact oracle plan route mask was `001`, so both active patrol routes
were absent and only the isolated control route survived. The master visited one
joint node. Its chosen cumulative current score was `6/12/20`; the exact
continuation deliberately took only `6/12/16` that day and later reached the
better full-match score. Neither the exact plan nor an equivalent terminal-state
outcome was retained.

103 closes as accepted route-availability attribution. Master ranking and W1
tie-breaking are exonerated at the first divergence because the required active
routes do not exist in their input. Prior experiments already reject blind cap
widening, dual rollouts and terminal-only fuel-exact wiring. The distinct open
question is whether the existing fuel-constrained anytime enumerator restores
these nonterminal low-fuel day-2 routes without changing the cap. Consumed seed
1600000 may answer membership only and cannot promote a candidate.
