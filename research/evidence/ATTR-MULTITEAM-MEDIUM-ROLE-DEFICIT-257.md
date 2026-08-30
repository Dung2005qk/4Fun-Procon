# ATTR-MULTITEAM-MEDIUM-ROLE-DEFICIT-257

Date: 2026-08-30

## Frozen counterexample

Hard three-bot BTC match `m-5155`, seven days, 16x16, 60 steps/day, six
agents, sixteen spots, six brands and server-published 10000-ms windows.
Canonical `18ecdd3` selected `PPPPPT` and finished `6/42/192` behind
`226/204/204`. Replay SHA256 is
`03A494B4616CCA911C63E76CEEEC6751D6056C5BAE19470F3B4EA8B64389C7C3`.
Replay-check proves 7/7 exact-valid actions, validator agreement and 6/6
reconciled transitions.

## Attribution

Closed-loop forward/reverse role masks close role composition negative:
`PPPPPT` returned `6/41/151` twice; all-patrol returned `6/41/126` and
`6/39/119`. The live terminal continuation had already raised the checkpoint
trajectory to `6/42/192`.

The frozen sparse probe at 50,000 settled states found changed-terminal gains
on days 1--6 but no protected same-terminal gain, reproducing the closed
249--254 residual. On terminal day 7, patrol 1 mask `0xF230` is dual-valid and
strictly raises the fully refined incumbent from `6/42/192` to `6/42/193`.
Reapplying production `refine_terminal_sparse` evaluates 157 valid plans, finds
no gain and exits without deadline.

## Fixed cap ladder

Probe SHA256:
`05E2FEE7EB8777B41D208002D17AC98C8E17939E265E10C05785B50FC1752E87`.

| Cap | Emitted | Protected | Retained/valid | Best | Mask |
|---:|---:|---:|---:|---:|---:|
| 50,000 | 290 | 20 | 52/52 | 6/42/193 | 0xF230 |
| 100,000 | 453 | 37 | 64/64 | 6/42/193 | 0xF230 |
| 250,000 | 836 | 84 | 64/64 | 6/42/193 | 0xF230 |
| 500,000 | 1,278 | 182 | 64/64 | 6/42/193 | 0xF230 |
| 750,000 | 1,654 | 230 | 64/64 | 6/42/193 | 0x92F0 |
| 1,000,000 | 2,231 | 288 | 64/64 | 6/42/193 | 0x92F0 |
| 1,250,000 | 2,644 | 363 | 64/64 | 6/42/192 | none |

The candidate supply is non-monotonic under the final heuristic rank: more
search discovers more masks but evicts all strict terminal gains. This rules
out increasing the cap and forbids selecting a post-hoc 1,000,000-state cap.

## Next discriminator

At the unchanged 1,250,000-state cap, reconstruct and dual-validate the full
discovered frontier for consumed day 7 patrol 1. Production source remains
unchanged. A successor is allowed only if the strict route remains present and
can be retained by an additive terminal-only stock-aware marginal reservoir
that runs after the canonical parent path and fails closed to its exact plan.

## Full discovered-frontier result

The research-only probe was extended so only its retention limit could be set
above the number of emitted masks. Rebuilt probe SHA256 is
`6BBAE931F2C40A9BE52C1FE08937E245F3EF62789050A354B415D2A2217041B8`.
At the unchanged 1,250,000 settled-state cap it reconstructed and dual-validated
2,993 unique general-plus-protected routes. Exact-valid mask `0xF230` was still
present and restored `6/42/193`; the protected same-terminal best remained the
incumbent `6/42/192`.

Verdict: accepted positive attribution. The production-depth search discovers
the strict terminal gain; only bounded heuristic retention loses it. This
closes 257 into the independently frozen
`SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258`. No result from this consumed
replay may tune its rank, reservoir width, development split or holdout.

Functionality-preservation answers: (1) no designed production functionality
is removed, disabled, deferred or reduced; (2) nothing is deleted.
