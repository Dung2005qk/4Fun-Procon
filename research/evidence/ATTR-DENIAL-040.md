# ATTR-DENIAL-040 canonical-frontier one-exchange

Date: 2026-08-09
Parent: `afcd2da`
Verdict: accepted read-only falsification

Starting from the frozen exact-valid `6/60/321` day-10 witness, the probe
enumerated the canonical anytime resource frontier for every patrol with the
production semantic parameters: minimum five spots, 32 retained routes,
1,250,000 settled states, preferred-brands zero and no wall deadline.

For agents `0,1,2,3,4,7`, all 32 one-agent substitutions were exact-valid and
the independent validator agreed, for 192 valid mutations total. The best score
for every agent remained `6/60/321`; no route in any retained frontier reached
the guidance value `6/60/322`.

This falsifies the hypothesis that the final serving gap can be closed by merely
running a deterministic one-exchange coordinator over the exact frontiers that
production already retains. It also explains why moving the existing proof
loop earlier could not improve. No exchange implementation is authorized from
this replay.

The remaining admissible question is exact feasibility outside the capped
frontier. Only agents 2, 4 and 7 claim the stock-two spot 0 in the 35 witness;
agents 4 and 7 share the same start/fuel state. A full fixed-state resource
oracle on the two unique claimant start states can establish whether any
resource-feasible route mask removes that excess claim and reaches 36. Such an
oracle has no production performance authority.
