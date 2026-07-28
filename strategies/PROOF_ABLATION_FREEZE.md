# Proof-Guided Search Ablation Freeze

This holdout is frozen before its first execution.

- Split name: `proof-holdout`.
- Seed window: `400000..400299`.
- Fixture count: `300`.
- Focus: `proof-ablation`.
- Reference: deterministic ALNS without proof-guided repair.
- Candidate scales: `2x`, `4x`, and `8x` agent-count proof iterations.
- Structural diagnostic: deterministic `2x` proof plus one exact route-pool recombination.
- All strategies use the same generator limits, exact simulator, exact validator,
  role policy, traffic stream, and fixed operation caps.

The production `2x` scale may be replaced only if a larger scale:

1. has zero invalid plans and zero lifetime-tier drops;
2. beats `2x` by a two-sided paired sign test with `p < 0.05`;
3. has no fixture-family with more losses than wins against `2x`;
4. increases deterministic p95 latency by no more than 5 percent.

The route-pool diagnostic is not a production challenger because production
already performs route-pool recombination before W1 certification. It measures
remaining same-day coordination headroom only.
