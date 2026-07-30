# High-fuel orienteering production gate

## Frozen implementation

- Challenger implementation: `4c6d250`.
- Production promotion: `53fd94d`.
- Current-day harvest mode: `6`.
- Future-witness harvest mode: `5`.
- Activation requires both `fuelLimit >= 3 * daySteps` and a search window of at least `1500 ms`.
- Ordinary-fuel generation has an action-key equality regression between mode 5 and an enabled orienteering flag.

## Exactness

- CTest: `3/3` passed.
- Master portfolio oracle: `2000/2000` exact matches.
- Every generated candidate still passes the exact simulator and independent validator before selection.

## BTC replay holdout

Fixed-role, ten-day, `5000 ms` counterfactual replay; comparison is mode 5 versus current mode 6 with future mode 5.

| Replay | Role mask | Mode 5 | Mode 6/5 | Delta |
|---|---:|---:|---:|---:|
| `m-0915` | 128 | 364 | 393 | +29 |
| `m-0910` | 64 | 381 | 394 | +13 |
| `m-0906` | 4 | 358 | 381 | +23 |
| `m-0912` | 8 | 358 | 379 | +21 |

All eight runs finished at `6/60`, with no invalid plan.

## Generated high-fuel holdout

Common roles, ten days, `2500 ms`, seeds not used for route design.

| Split | Fixtures | Win/tie/loss | Tier-3 delta | Invalid | Tier-1 drops |
|---|---:|---:|---:|---:|---:|
| offset 900 | 4 | 3/0/1 | +19 | 0 | 0 |
| offset 1200 | 6 | 5/0/1 | +134 | 0 | 0 |
| combined | 10 | 8/0/2 | +153 | 0 | 0 |

The losing fixtures were not patched with family- or seed-specific rules. Wider beam and compact-future-rollout experiments were rejected after horizon regressions.

## Remaining gate

Local latency is not a release claim because the workstation is concurrently loaded. Target-host response-time evidence must come from BTC training or a real match. Score, validity, exactness, and operation-count evidence remain valid local gates.
