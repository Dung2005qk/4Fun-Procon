# ATTR-W1-TERMINAL-RANK-108

Parent: `f77c101`

On the already-consumed CEILING-CYCLE-PATROL-095 seed 1600000 day-2 state,
the unchanged complete per-agent outcome enumerator produced six nondominated
served outcomes ending at the missing patrol's oracle terminal cell 26. The
oracle outcome (spot mask 4, remaining fuel 6) occurred exactly once.

With the other active patrol fixed to its exact route, the oracle outcome was
rank 6 of 6 under current-official-score-first ordering, but unique rank 1 under
remaining-fuel-first ordering. It was also unique rank 1 under the unchanged
candidate-specific FastViability upper, with upper `6/24/30`.

This closes 108 as accepted attribution. A single arbitrary route per terminal
cell is not justified, but a route ending on each served spot selected first by
maximum remaining fuel is a public structural projection that retains this
counterexample. It is bounded by the public maximum of 16 spots per agent. Any
source candidate must preserve the existing maximal-mask bundles and add the
new routes only through separate, complete exact bundles. It must use a fresh
split and may not tune a cap, threshold, seed, family or replay.
