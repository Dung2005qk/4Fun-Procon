# ATTR-TRAFFIC-TRIPLE-STRATA-120

Parent: `5dccb0f`

On the consumed seed1700000 day-1 witness, applying the frozen terminal selector
after unchanged generation produced these post-prune masks:

- width 16: `001` (`4|5|1` selected columns)
- width 24: `011` (`6|6|1`)
- width 32: `011` (`6|6|1`)
- width 48: `111` (`6|6|1`)
- width 64: `111` (`7|6|1`)

The actual preregistered 119 extraction occurred before final pruning and had
width16 mask `101`, so patrol0's route is constructed at 16 and only lost by
canonical pruning. Patrol1's route is absent there but appears by structural
width 24. In the width64 pool it has three servings and reports
`harvest_extension=0`, `exact_orienteering=0`; it is therefore a direct triple,
not a later extension. Patrol0's route has the same direct provenance.

Verdict: accepted attribution. The gap is the global budget on direct triple
construction plus final global pruning, not a need for a second generator or a
global canonical width increase. The next capability probe may continue the
same finite direct-triple loop into a terminal-stratified sidecar while leaving
the canonical prefix and pruning unchanged.
