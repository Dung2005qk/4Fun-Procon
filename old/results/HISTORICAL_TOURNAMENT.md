# Historical UDON-SHIELD tournament

Generated 2026-07-31 from the frozen raw files in `old/results/raw`.

## Verdict

- The current checkpoint is not universally stronger at every deadline.
- At the production-like 2500 ms planner lane, every checkpoint from `02df79d` through HEAD is score-identical on all 120 fixed-role maps.
- Against the frozen BTC baseline, HEAD is directionally better at 2500 ms, but this 120-map sample is not statistically decisive.
- At 500 ms, the old baseline is decisively stronger because the modern engine deliberately suppresses expensive stages under short deadlines.
- No tested checkpoint produced an invalid plan or emergency day.

## Protocol

- One version-neutral C++ harness linked independently against each Git snapshot.
- Same map generator, seeds, opponent traffic, own two-day traffic feedback, official lexicographic scoring, exact simulator and independent validator.
- `fixed` lanes use the same one-tanker mask to isolate the day planner.
- `native` lanes use deterministic exhaustive role selection to remove local wall-clock noise from pre-match rollout.
- Local latency is diagnostic only; BTC remains authoritative for runtime gates.

## Primary: fixed roles, 2500 ms, 120 maps

| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |
|---|---:|---:|---:|---:|---:|---:|
| `baseline-btc` | 23/64/33 | 0.2288 | 697/2859/4734 | 0 | 0 | 558 ms |
| `adaptive-master` | 6/107/7 | 1.0000 | 697/2861/4748 | 0 | 0 | 870 ms |
| `abundant-fuel` | 6/107/7 | 1.0000 | 697/2861/4748 | 0 | 0 | 991 ms |
| `exact-highfuel` | 0/120/0 | 1.0000 | 697/2867/4746 | 0 | 0 | 1539 ms |
| `proven-threshold` | 0/120/0 | 1.0000 | 697/2867/4746 | 0 | 0 | 1537 ms |
| `final-lex` | 0/120/0 | 1.0000 | 697/2867/4746 | 0 | 0 | 1536 ms |
| `full-role-budget` | 0/120/0 | 1.0000 | 697/2867/4746 | 0 | 0 | 1538 ms |
| `current` | 0/120/0 | 1.0000 | 697/2867/4746 | 0 | 0 | 1536 ms |

### Family result: HEAD versus baseline

| Family | Current vs baseline W/T/L | Score delta current-baseline |
|---|---:|---:|
| balanced | 5/9/6 | +0/-1/-6 |
| fuel-tight | 9/7/4 | +0/+8/+7 |
| high-stock | 6/11/3 | +0/+2/+2 |
| overnight | 4/9/7 | +0/-2/-8 |
| rare-brand | 5/13/2 | +0/+0/+5 |
| threshold-corridor | 4/15/1 | +0/+1/+12 |

## Native roles, 2500 ms

Thirty maps include all eight checkpoints:

| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |
|---|---:|---:|---:|---:|---:|---:|
| `baseline-btc` | 5/14/11 | 0.2101 | 172/711/1202 | 0 | 0 | 660 ms |
| `adaptive-master` | 1/25/4 | 0.3750 | 172/709/1206 | 0 | 0 | 1244 ms |
| `abundant-fuel` | 1/26/3 | 0.6250 | 172/709/1208 | 0 | 0 | 1207 ms |
| `exact-highfuel` | 1/26/3 | 0.6250 | 172/711/1209 | 0 | 0 | 1542 ms |
| `proven-threshold` | 1/26/3 | 0.6250 | 172/711/1207 | 0 | 0 | 1540 ms |
| `final-lex` | 0/30/0 | 1.0000 | 172/711/1213 | 0 | 0 | 1543 ms |
| `full-role-budget` | 1/28/1 | 1.0000 | 172/711/1213 | 0 | 0 | 1542 ms |
| `current` | 0/30/0 | 1.0000 | 172/711/1213 | 0 | 0 | 1544 ms |

The independent 30-map future window was run for baseline and HEAD only. HEAD vs baseline: **20/28/12**, sign-test `p=0.2153`, aggregate delta `+0/+3/+10`.

## Stress: fixed roles, 500 ms, 60 maps

| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |
|---|---:|---:|---:|---:|---:|---:|
| `baseline-btc` | 57/2/1 | 0.0000 | 346/1422/2333 | 0 | 0 | 358 ms |
| `adaptive-master` | 0/59/1 | 1.0000 | 346/1379/1934 | 0 | 0 | 43 ms |
| `abundant-fuel` | 0/59/1 | 1.0000 | 346/1379/1934 | 0 | 0 | 47 ms |
| `exact-highfuel` | 0/60/0 | 1.0000 | 346/1379/1936 | 0 | 0 | 57 ms |
| `proven-threshold` | 0/60/0 | 1.0000 | 346/1379/1936 | 0 | 0 | 54 ms |
| `final-lex` | 0/60/0 | 1.0000 | 346/1379/1936 | 0 | 0 | 63 ms |
| `full-role-budget` | 0/60/0 | 1.0000 | 346/1379/1936 | 0 | 0 | 54 ms |
| `current` | 0/60/0 | 1.0000 | 346/1379/1936 | 0 | 0 | 49 ms |

## Stress: native roles, 500 ms, 60 maps

| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |
|---|---:|---:|---:|---:|---:|---:|
| `baseline-btc` | 55/3/2 | 0.0000 | 346/1418/2319 | 0 | 0 | 346 ms |
| `adaptive-master` | 2/54/4 | 0.6875 | 346/1380/1952 | 0 | 0 | 40 ms |
| `abundant-fuel` | 1/58/1 | 1.0000 | 346/1384/1958 | 0 | 0 | 30 ms |
| `exact-highfuel` | 1/59/0 | 1.0000 | 346/1384/1961 | 0 | 0 | 34 ms |
| `proven-threshold` | 1/55/4 | 0.3750 | 346/1381/1946 | 0 | 0 | 36 ms |
| `final-lex` | 1/56/3 | 0.6250 | 346/1382/1951 | 0 | 0 | 33 ms |
| `full-role-budget` | 1/57/2 | 1.0000 | 346/1383/1956 | 0 | 0 | 34 ms |
| `current` | 0/60/0 | 1.0000 | 346/1383/1955 | 0 | 0 | 35 ms |

## BTC-scale 32x32 probes

Default fuel, three ten-day maps:

| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |
|---|---:|---:|---:|---:|---:|---:|
| `baseline-btc` | 0/0/3 | 0.2500 | 18/180/979 | 0 | 0 | 2226 ms |
| `adaptive-master` | 2/1/0 | 0.5000 | 18/180/1210 | 0 | 0 | 2225 ms |
| `abundant-fuel` | 1/0/2 | 1.0000 | 18/180/1206 | 0 | 0 | 2226 ms |
| `exact-highfuel` | 1/2/0 | 1.0000 | 18/180/1205 | 0 | 0 | 2226 ms |
| `proven-threshold` | 1/2/0 | 1.0000 | 18/180/1205 | 0 | 0 | 2204 ms |
| `final-lex` | 1/2/0 | 1.0000 | 18/180/1205 | 0 | 0 | 2226 ms |
| `full-role-budget` | 1/1/1 | 1.0000 | 18/180/1204 | 0 | 0 | 2225 ms |
| `current` | 0/3/0 | 1.0000 | 18/180/1203 | 0 | 0 | 2225 ms |

High fuel, three ten-day maps:

| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |
|---|---:|---:|---:|---:|---:|---:|
| `baseline-btc` | 0/0/3 | 0.2500 | 18/180/1075 | 0 | 0 | 2227 ms |
| `adaptive-master` | 0/0/3 | 0.2500 | 18/180/1352 | 0 | 0 | 2225 ms |
| `abundant-fuel` | 0/0/3 | 0.2500 | 18/180/1353 | 0 | 0 | 2227 ms |
| `exact-highfuel` | 1/2/0 | 1.0000 | 18/180/1440 | 0 | 0 | 2226 ms |
| `proven-threshold` | 1/2/0 | 1.0000 | 18/180/1440 | 0 | 0 | 2225 ms |
| `final-lex` | 1/1/1 | 1.0000 | 18/180/1437 | 0 | 0 | 2225 ms |
| `full-role-budget` | 1/2/0 | 1.0000 | 18/180/1438 | 0 | 0 | 2226 ms |
| `current` | 0/3/0 | 1.0000 | 18/180/1432 | 0 | 0 | 2225 ms |

All BTC-scale checkpoints reached identical lifetime and daily tiers. Tier-3 servings near the 2500 ms cutoff varied across repeated local runs, so those small tail differences are not a promotion verdict.

## Interpretation

1. The large improvement happened by `02df79d`; later accepted commits mostly improve correctness, proof bounds, final-day edge cases, and live runtime wiring.
2. `6f84a06` is safer and more truthful about proof gaps, but it is not a general-score upgrade over `02df79d` on this closed tournament.
3. The frozen baseline remains the best short-deadline policy. A production fallback may reuse its scheduling profile only after a separate semantic and BTC deadline gate; this tournament does not authorize that change.
4. Further research must start from the measured open gap: preserve modern correctness while recovering short-deadline quality. No unrelated axis should be reopened first.
