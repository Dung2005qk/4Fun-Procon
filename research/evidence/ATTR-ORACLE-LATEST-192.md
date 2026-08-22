# ATTR-ORACLE-LATEST-192

Date: 2026-08-22

Parent: `d73a24a`

Status: closed `accepted-attribution-residual-gap`

Frozen consumed manifest:
`research/holdouts/ATTR-ORACLE-LATEST-192.csv`

SHA256:
`AC449977F5822B61645CC0475C09F004B345BEF20AF14D36ED86957BF635A42B`

## Question

Experiment 188 established an exact experiment-185 witness of `5/17/18`
against production `5c3aa7a`, but its production replay deliberately skipped
terminal-day protected refinement. Experiments 190 and 191 subsequently added
sparse coordinate ascent on the final day. This attribution checks whether that
new production boundary closes the already-opened exact gap.

## Method

Only consumed seed `1721100` was used. The experiment-188 protected-production
control was reproduced first under maximum-dwell, minimum-dwell and
status-toggle opponent policies. A separate opt-in mode then ran the identical
experiment-187 virtual-parent lifecycle and invoked the unchanged accepted-191
terminal refiner on day 4 inside the same absolute `5000 ms` compute boundary
and `1600 ms` reserve. Every plan passed the exact simulator and independent
validator path used by the probe. Local elapsed has no performance authority.

## Result

The control reproduced experiment 188 exactly:

- maximum-dwell: `5/17/17`, hash `32dcfc4fa4a20fd6`;
- minimum-dwell: `5/16/17`, hash `a9d910dc3b481ff7`;
- status-toggle: `5/17/17`, hash `32dcfc4fa4a20fd6`.

The latest-terminal mode produced the same three scores and hashes. All runs
were valid, had zero nonterminal protected takeover and zero solver/protected
deadline day. Terminal sparse routes, valid terminal alternatives, strict
terminal improvements and terminal rounds were all zero.

This inactivity is structural, not a timing failure. The consumed fixture has
six spots, for which the canonical dense exact state is supported. The accepted
190/191 sparse refiner intentionally returns without generating routes on that
domain, preserving the canonical dense path. Therefore 190/191 cannot close or
invalidate the experiment-185 counterexample.

## Verdict

Closed `accepted-attribution-residual-gap`. The exact `5/17/18` witness remains
strictly above current production `d73a24a` on the consumed state. This does not
authorize tuning the seed or copying the oracle into production. It confirms
that a later causal experiment must address multi-day closed-loop selection in
the dense-supported domain; final-day sparse coordinate ascent is an unrelated
capability.

## Functionality preservation

No production source or runtime behavior changed. The opt-in probe removes,
disables, defers or reduces no designed functionality and deletes nothing.
