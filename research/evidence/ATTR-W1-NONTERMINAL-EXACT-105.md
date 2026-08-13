# ATTR-W1-NONTERMINAL-EXACT-105

Parent: `f77c101`

This read-only probe repeated the consumed seed 1600000 day-2 membership query
with the complete fuel-constrained exact resource enumerator, explicitly leaving
the bounded anytime enumerator disabled and retaining W1 cap3.

The oracle mask remained `011`, identical to the bounded anytime result: one
active patrol route was still absent. Portfolio widths were `5|5|3`.
Diagnostics reported three supported and three complete agents, 570 settled
states, eight terminal variants, two exact bundles and no deadline hit.

105 closes as accepted negative attribution. The entire nonterminal fuel-flag
axis is closed: neither bounded anytime nor complete exact restores the full
joint route set. No candidate or holdout is authorized, and no cap/node/deadline
tuning may reopen it. Source inspection shows complete resource enumeration
exports only inclusion-maximal spot masks; the next and final causal check is
whether the missing oracle route is individually eliminated by that projection.
