# Full-Engine Proof Scale Safety Holdout

This safety holdout is frozen before its first execution.

- Split name: `proof-production-holdout`.
- Seed window: `500000..500119`.
- Fixture count: `120`.
- Budget: `1200 ms` per day.
- Challenger: full UDON-SHIELD with `8x` proof-guided passes.
- Baseline: identical full engine with the previous normal/long `2x/4x` passes.
- Focus: `proof-production`.

The deterministic operation-budget holdout is the causal promotion test. This
full-engine run is an integration safety gate for W1, scenario shielding, and
deadline interaction. The challenger is retained only if:

1. invalid plans and lifetime-tier drops are both zero;
2. aggregate strict wins are not fewer than losses;
3. no fixture family loses by more than one match net;
4. p95 latency is no more than 5 percent above baseline.
