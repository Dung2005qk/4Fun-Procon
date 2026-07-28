# Frozen Strategy Evaluation

The final implementation and all parameters were frozen before the held-out
split was opened.

## Protocol

- train: seeds `0..119`;
- validation: seeds `10000..10119`;
- held-out: seeds `100000..100299`;
- six generated map families;
- four or five match days per map;
- seven strategies from wait-only through full UDON-SHIELD;
- each strategy feeds its own previous two road footprints into later traffic;
- all strategies share the same exogenous opponent-footprint stream per map;
- every submitted plan must agree under the exact simulator and independent
  validator.

The score comparison is the official lexicographic tuple. Pairwise confidence
intervals are Wilson 95% intervals over decisive maps, with a two-sided sign
test. Held-out results were not used for further tuning.

## Final Held-Out Result

At `1200 ms` per day over `300` held-out maps:

| Strategy | Strict best | Tied best | Pairwise vs full W/T/L | Invalid | p95 | p99 | Max |
|---|---:|---:|---:|---:|---:|---:|---:|
| wait-only | 0 | 0 | 0/0/300 | 0 | 0 ms | 0 ms | 0 ms |
| greedy one-day | 0 | 0 | 0/0/300 | 0 | 5 ms | 9 ms | 13 ms |
| weighted TOP-FC | 2 | 13 | 27/8/265 | 0 | 60 ms | 71 ms | 83 ms |
| rolling ALNS | 6 | 28 | 56/14/230 | 0 | 39 ms | 49 ms | 67 ms |
| rolling HALNS route-pool | 20 | 36 | 64/23/213 | 0 | 81 ms | 111 ms | 151 ms |
| UDON-SHIELD fixed roles | 64 | 104 | 81/124/95 | 0 | 241 ms | 330 ms | 571 ms |
| UDON-SHIELD | **82** | **101** | reference | **0** | **314 ms** | **434 ms** | **689 ms** |

Full UDON-SHIELD beat rolling HALNS on `213`, tied on `23`, and lost on `64`
held-out maps. The sign-test p-value is below the printed four-decimal
resolution. It won every generated family:

- high-stock: `38/4/8`;
- overnight: `36/5/9`;
- balanced: `37/3/10`;
- rare-brand: `33/5/12`;
- threshold-corridor: `41/2/7`;
- fuel-tight: `28/4/18`.

At the separate `500 ms` train profile, full UDON-SHIELD beat rolling HALNS
`41/4/15`, produced no invalid plans, and had a maximum measured day latency of
`255 ms`.

## Exact Master Oracle

`udonshield_master_oracle --seeds 1000` compared the native lexicographic
branch-and-bound master with exhaustive enumeration over the same complete
route portfolio:

- exact best-candidate matches: `1000/1000`;
- exhaustive combinations: `64000`;
- bounded combinations: `55217`;
- branches pruned: `5093`;
- combination reduction: `13.7234%`.

This proves exactness on each closed oracle portfolio. It does not claim global
optimality over all possible route columns, maps, or opponents.

## Reproduction

```powershell
./build-release/udonshield_master_oracle --seeds 1000
./build-release/udonshield_strategy_bench --split train --seeds 120 --budget-ms 1200 --summary-only
./build-release/udonshield_strategy_bench --split validation --seeds 120 --budget-ms 1200 --summary-only
./build-release/udonshield_strategy_bench --split heldout --seeds 300 --budget-ms 1200 --summary-only
ctest --test-dir build-release --output-on-failure
```

## Research Basis

The route-pool recombination follows the successful pattern of combining ALNS
route generation with a set-packing improvement stage:

- https://www.sciencedirect.com/science/article/pii/S0305054820301519
- https://www.sciencedirect.com/science/article/pii/S0305054820301568
- https://www.sciencedirect.com/science/article/pii/S0377221724007240
- https://www.sciencedirect.com/science/article/pii/S0377221724004636

## Post-Held-Out Champion Challenger Cycle

The original held-out split remained frozen. A new research cycle used disjoint
seed windows and introduced no change to the official score, map generator,
traffic feedback, validity gate, or per-day budget.

The first challenger repeated the winning ALNS++ pattern:

1. ALNS generates exact-valid candidate routes;
2. the exact lexicographic master recombines the enlarged route pool;
3. a second ALNS pass searches from the recombined population;
4. a final exact master recombination closes the cycle.

At `1200 ms`, rolling HALNS feedback beat the one-pass rolling route-pool
baseline on both development splits:

- research-train, 60 maps: `16/40/4`, sign-test `p=0.0118`;
- research-validation, 120 maps: `24/85/11`, sign-test `p=0.0410`;
- invalid plans: `0`.

The cycle was then integrated behind an opt-in full-engine policy and compared
against unchanged full UDON-SHIELD:

- research-train, 60 maps: `5/52/3`;
- research-validation, 120 maps: `16/96/8`;
- research-confirm, 240 maps: `31/192/17`;
- combined development result: `52/340/28`, sign-test `p=0.0097`;
- research-confirm at `500 ms`, 120 maps: `18/93/9`;
- tier-one drops: `0`;
- invalid plans: `0`.

A diversity-preserving survivor archive inspired by HGS was also tested. It
reserved 25% of the ALNS population for candidates maximizing minimum plan
distance from the score elite. It did not improve full UDON-SHIELD on train
(`13/33/14`) and materially increased latency, so it was rejected without
parameter tuning.

Source and parameters were frozen in `FUTURE_HOLDOUT_FREEZE.md` before opening
the new future holdout exactly once.

### Future Holdout Verdict

At `1200 ms` over seeds `200000..200299`:

| Strategy | Strict wins | Ties | Losses | Invalid | p50 | p95 | p99 | Max |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| UDON-SHIELD feedback | 34 | 231 | 35 | 0 | 62 ms | 369 ms | 631 ms | 718 ms |
| UDON-SHIELD single-pass | 35 | 231 | 34 | 0 | 60 ms | 314 ms | 492 ms | 712 ms |

The feedback challenger has no held-out score advantage (`p=1.0000`) and costs
`55 ms` at p95. It is therefore rejected for production. The default remains
single-pass route-pool recombination. The feedback policy stays opt-in only so
the research result is reproducible; it is not part of the production runtime.

## Deterministic Proof-Guided Ablation

Wall-clock cutoffs were removed from this ablation. Every contender received
the same generated portfolio, exact master cap, base ALNS operations, roles,
traffic stream, simulator, and validator. Only the number of supplemental
proof-gap repair attempts changed. The holdout and promotion criteria were
frozen in `PROOF_ABLATION_FREEZE.md` before seeds `400000..400299` were opened.

On the 300-fixture proof holdout:

- production `2x` proof beat no-proof ALNS `26/262/12`, `p=0.0336`;
- `8x` proof beat `2x` proof `24/269/7`, `p=0.0033`;
- `8x` proof beat `4x` proof `10/290/0`, `p=0.0020`;
- all six fixture families were net-positive for `8x` versus `2x`;
- invalid plans and lifetime-tier drops were both `0`;
- deterministic p95 remained `16 ms` for `2x`, `4x`, and `8x`.

The deterministic result qualified `8x` for a full-engine safety gate, but did
not by itself change production. On the separately frozen 120-fixture full
engine holdout, `8x` versus the previous `2x/4x` profile finished `4/113/3`
with zero invalid plans, zero lifetime-tier drops, and lower p95 latency
(`560 ms` versus `564 ms`). However, the high-stock family was `0/18/2`, which
violated the pre-registered no-family-regression gate. Production therefore
remains `2x` for normal and `4x` for long deadlines; short still disables the
supplemental phase. The `8x` policy remains research-only and reproducible via
`--focus proof-production`.
