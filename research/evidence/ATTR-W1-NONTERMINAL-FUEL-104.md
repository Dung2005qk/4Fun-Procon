# ATTR-W1-NONTERMINAL-FUEL-104

Parent: `f77c101`

On consumed CEILING-CYCLE-PATROL-095 cycle-balanced/low seed 1600000 day 2,
this read-only probe kept the production W1 paths, targets, three-column cap,
harvest settings and state unchanged, then enabled the existing exact-harvest
and fuel-constrained anytime enumerator with its canonical 32-route and
1,250,000-node bounds.

The baseline oracle route mask was `001`. Anytime enumeration changed it only to
`011`: one active patrol oracle route reappeared, while the other remained
absent. The returned portfolio widths were `5|5|3`; diagnostics reported three
supported agents, zero complete agents, 863 settled states, nine terminal
variants, two exact bundles and no deadline hit. The anytime algorithm is
bounded-incomplete by design and did not restore the full joint route set.

104 closes as accepted negative attribution. No candidate or holdout is
authorized. Do not tune anytime route/node bounds on this consumed fixture. The
only remaining distinct membership check is the canonical complete
fuel-constrained exact enumerator at the same cap; if it does not restore mask
`111` with complete enumeration, the nonterminal fuel axis closes.
