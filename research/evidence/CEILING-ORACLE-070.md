# CEILING-ORACLE-070 — complete terminal oracle audit

Date: 2026-08-12
Parent/champion: `7ef36949e1b166c862a30f52ecb3bd9c9fb210ea`
Manifest: `research/holdouts/CEILING-ORACLE-070.csv`
Manifest SHA256: `6C738A9E9B811C27A2AA7A25405BFFDBED971B0E929B330C330ABE99AC45B0B5`

## Question and frozen scope

Does unchanged HEAD leave any feasible official-score improvement on fresh
terminal states when the full fixed-role action space can be solved exactly?

The preregistered domain contains 144 official-valid 8x8 day-5 states: six
structural families (`balanced`, `rare-brand`, `threshold-corridor`,
`fuel-tight`, `high-stock`, `overnight`) x low/default/high fuel. Development
contains 36 fixtures and the one-time holdout contains 108. Every state has four
fixed patrols and eight spots. HEAD receives the production internal budget of
5000 ms. The server outer window is irrelevant to this local semantics audit.

## Why the oracle is exact in this domain

For every patrol, `enumerate_exact_resource_routes` completed without a
deadline and returned all inclusion-maximal resource-feasible spot masks. On a
terminal day, adding a reachable spot claim cannot reduce lifetime distinct,
daily distinct or stock-capped servings. Therefore an optimum exists among
those inclusion-maximal masks. The team DP takes their Cartesian product while
tracking every spot's claim count capped by official stock, then compares the
resulting `OfficialScore` lexicographically. It reconstructs the selected action
plan and accepts it only when the exact simulator and independent validator
agree. Fixed all-patrol roles remove tanker/refuel coupling from this proof.

The probe is `research/probes/ceiling_oracle.cpp`; it is research-only and does
not alter planner, decision, simulator, validator or BTC runtime logic.

## Harness preflight

The first development invocation stopped before solving a fixture because the
generator selected 14..18 steps while the published 8x8 bound starts at 16.
The generator was corrected to 16..20 before any evidence was accepted. No
holdout row was opened during this preflight failure.

Toolchain after the earlier build cleanup removed the old cache:

- CMake: `D:/VSBuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`
- C++: MSVC `19.44.35221.0`
- Ninja: `C:/mingw64/bin/ninja.exe`
- Build type: Release

## Results

| Split | Fixtures | Oracle W/T/L vs HEAD | Incomplete | Invalid | Result hash |
|---|---:|---:|---:|---:|---|
| Development | 36 | 0 / 36 / 0 | 0 | 0 | `151496ca6d63ee47` |
| One-time holdout | 108 | 0 / 108 / 0 | 0 | 0 | `6b4fecc3cc1eec37` |
| Combined | 144 | 0 / 144 / 0 | 0 | 0 | — |

All 18 family x fuel strata tied completely in both splits. No first differing
tier exists because every paired official score is identical.

Commands:

```text
build-release/udonshield_ceiling_oracle.exe --manifest research/holdouts/CEILING-ORACLE-070.csv --split development
build-release/udonshield_ceiling_oracle.exe --manifest research/holdouts/CEILING-ORACLE-070.csv --split holdout
```

Local elapsed time is intentionally omitted: it has no authority for BTC
performance or hard-cap readiness.

## Verdict

Accepted as evidence that the fixed-role terminal resource/team-allocation axis
has converged on this independently frozen domain. It is not evidence that
native role selection, tanker coupling, multi-day planning, traffic recourse or
the whole architecture has reached its practical ceiling. Those questions need
independent frozen experiments; these opened fixtures must not be tuned or
reused as a new holdout.
