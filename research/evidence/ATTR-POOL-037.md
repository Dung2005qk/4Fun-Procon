# ATTR-POOL-037 better-route provenance

Date: 2026-08-09
Parent: `afcd2da`
Verdict: inconclusive at the operator substage; pipeline narrowed

One frozen-parent `replay-solve` run at the unchanged 5000 ms logic cap
reproduced the frozen 35-serving plan byte for byte and independently validated
`6/60/321`. Local elapsed time was not interpreted.

Observed stage counters:

- exact seed servings: 34;
- exact local servings: 34;
- exact feasibility improvements: 0 over 504,176 nodes;
- final servings: 35;
- ALNS iterations: 41;
- ALNS improvements: 1;
- recombination improvements: 0.

This rules out the exact coordinator and late candidate-route recombination as
the direct source of the one-serving gain in this run. The selected plan has
exactly the one-agent `0x999 -> 0x9D8` substitution established by
`ATTR-MASTER-035`, matching the shape of `AdaptiveRouteImprover`, which mutates
one agent and exact-validates every result.

The existing BTC telemetry does not serialize the pre-ALNS best score,
`acceptedByOperator`, `synthesizedAccepted`, `proofGuidedAccepted` or
`proofGuidedImprovements`. Therefore it cannot distinguish whether the ALNS
improvement selected an existing portfolio column or a newly synthesized
Pareto repair. Source tracing limits it to those two general surfaces, but an
exact substage claim would be unsupported without exposing the already existing
diagnostic counters. No production change is authorized yet.
