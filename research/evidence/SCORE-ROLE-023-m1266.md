# SCORE-ROLE-023 BTC gate: m-1266

- Parent: `5599e76`.
- Configuration: hard, 3 bots, 10 days, 32x32, 100 steps/day, 5000 ms,
  8 agents, 12 spots, 6 brands, low fuel 1x.
- Live candidate assignment: mask `1`.
- Official result: rank 1, `6/60/228`; next bot `6/59/158`.
- Parent selector on the same replay: mask `4`.
- Exact parent-mask counterfactual: `6/60/220`; causal candidate gain `+8`.
- Role-selection wall time: `4641 ms`.
- Action response p95/p99/max: `3178/3178/3178 ms`.
- Maximum serialized decision time: `3105 ms` against a `4730 ms` internal
  decision envelope and the immutable `5000 ms` hard cap.
- Protocol: 10/10 action results HTTP 200 and valid; 9/9 state transitions
  reconciled; independent validator agreed on every day; emergency count 0.
- Replay SHA256:
  `11F037E67D344D14D0E9A8AA9230D4DC482D3A5272DA764732C340980F690649`.
