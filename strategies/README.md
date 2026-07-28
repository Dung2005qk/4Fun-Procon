# Strategy Research Lab

This directory isolates competitor strategies and evaluation tooling from the
production engine.

The comparison ladder is intentionally ordered from low complexity to the full
architecture:

1. wait-only safety baseline;
2. one-day greedy column choice;
3. weighted TOP-FC enumeration;
4. rolling ALNS without scenario shielding;
5. rolling HALNS with elite route-pool set-packing recombination;
6. rolling HALNS with an ALNS/recombination feedback cycle;
7. rolling HALNS feedback with diversity-preserving survivors;
8. full UDON-SHIELD with fixed roles as a role-selection ablation;
9. full UDON-SHIELD with the feedback-cycle challenger;
10. full UDON-SHIELD.

Seed splits are fixed before evaluation:

- train: seeds `[0, 9999]`;
- validation: seeds `[10000, 99999]`;
- held-out: seeds `[100000, 999999]`.

Tune only on train and validation. Open held-out once after selecting the final
logic. Every strategy receives the same generated map and opponent-footprint
stream, while its own prior road footprint feeds back into its later traffic
states. Plans are checked by both independent simulators.

The post-held-out research cycle uses disjoint, frozen windows:

- research-train starts at seed `1000`;
- research-validation starts at seed `20000`;
- research-confirm starts at seed `30000`;
- future-holdout starts at seed `200000`.

Only research-train, research-validation, and the disjoint confirmatory split
may be used while selecting a new challenger. Future-holdout is opened once
after the implementation and all parameters are frozen.

Use `--focus halns-feedback` or `--focus full-feedback` for large confirmatory
runs without spending the same budget on unrelated baselines.

Use `--focus proof-ablation` to compare deterministic fixed-operation ALNS,
proof-guided scales, and route-pool coordination without wall-clock search
cutoffs. The separately frozen `proof-holdout` split starts at seed `400000`.
Use `--focus proof-production` for the full-engine `8x` research challenger
against the production `2x/4x` profile. It failed the frozen high-stock family
gate and is not enabled by default.

`RoutePoolSearch::Feedback` is an opt-in research policy used only to reproduce
the challenger measurements. The production default remains
`RoutePoolSearch::SinglePass`.

The lab establishes statistical evidence over the registered generator family
and exactness on scoped tiny or portfolio oracles. It does not claim a universal
proof over every possible map or opponent.

Frozen commands and final held-out measurements are recorded in `RESULTS.md`.
The mapping from published champion methods to this architecture is recorded in
`CHAMPION_RESEARCH.md`.
