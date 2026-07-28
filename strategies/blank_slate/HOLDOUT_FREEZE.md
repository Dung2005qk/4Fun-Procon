# Blank-slate champion holdout freeze

> **Historical protocol.** This freeze predates production integration of the
> blank portfolio. It remains immutable evidence for the original challenge,
> not a description of the current production-vs-candidate boundary. See
> `../../AUDIT_KIEN_TRUC_TOAN_CUC.md` for current results.

Frozen on 2026-07-26 before opening any seed in the `blank-holdout` split.

## Candidate and baseline

- Candidate: `blank-portfolio`
- Baseline: `udon-shield`
- Roles: native end-to-end role selection for each strategy
- Match horizon: four stateful days
- Day budget: 1200 ms
- Traffic: endogenous own footprint plus identical common opponents, with the official two-day lookback
- Scoring: official lexicographic `(lifetime distinct, total daily distinct, total servings)`
- Validation: every submitted plan must agree between the exact simulator and the independent validator

## Unopened data

- Split: `blank-holdout`
- Seeds: `300000..300299`
- Fixtures: 300
- Families: six generators in the fixed benchmark cycle

## Frozen command

```powershell
.\build-release\udonshield_strategy_bench.exe --split blank-holdout --seeds 300 --budget-ms 1200 --focus blank-champion
```

## Predeclared decision rules

The candidate is the overall holdout winner only if all conditions hold:

1. Zero invalid plans and zero lifetime-tier drops versus `udon-shield`.
2. Two-sided paired sign-test `p < 0.01`.
3. The 95% Wilson lower bound of the decisive win rate is greater than `0.5`.
4. The measured p95 day-planning latency stays below the 1200 ms budget.

The candidate is general enough to replace `udon-shield` alone only if the
overall rules hold, it wins more than it loses in at least five of six
families, and no remaining family rejects equal decisive odds against the
candidate at `p < 0.05`.

If the overall rules pass but the family rule fails, the result supports
integration as an additional exact-evaluated portfolio component, not an
unconditional replacement.

## Frozen SHA-256

```text
6136fbe8dbf026708a395076f6261996c8f630424a35367e5cc30fce41b201ce  strategies\blank_slate\planners.hpp
3381d4b7204fe8620142d1b3eb8c60f34100398fb5db0a7fab78e60a7789a1bd  strategies\blank_slate\planners.cpp
a8c42ed4b5ca3ecc0eedf41beacef4ee341dfc20e2f380a7f02fd98f2ea05292  strategies\strategy_suite.cpp
e035ea2900c0d1a3f8f47452e9d70a1f393a16e125dac7316be8a31b625b86db  CMakeLists.txt
427e3f288eaa2221671d30b5e7b6c28f00f4327d5ca0ca748ac53ce8cbf10437  src\simulator.cpp
c77305fd1dc5e4066c410c51fba0f455b6042f4bedfe0d91a7387a7c37ed5b4a  src\validator.cpp
4a0dd7a24c1432a2b263f35ee6f91070152278a733a8cb22eae3a97556f41fbe  build-release\udonshield_strategy_bench.exe
```
