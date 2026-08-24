# SCORE-MIDDAY-CHAIN-ADOPTION-210 — protected mid-day deep-chain lane (ACCEPTED)

Registered 2026-08-24, parent `e9d3962`. Frozen manifest
`research/holdouts/SCORE-MIDDAY-CHAIN-ADOPTION-210.csv` SHA256
`33E1591047103CE10BFF4A35A7A488826B8CB9B321BDF596AF0DDECFE110D4FB`
(fresh seed blocks: dev 4920000/4921000/4922000/4923000, holdout
4930000/4931000/4932000/4933000; standard fuel rotation straddles the
3×daySteps regime boundary — low 10-12 and default 20-24 sit below it at
daySteps 16-18, high 8×horizon is the control).

## Gap (from closed 209 + live m-4039)

209 (3/3 witnesses) proved the oracle's suffix advantage is
position-then-sweep and that its decisive days are generation/adoption
blocked. Production's `prune_columns` runs its serving-coverage diversity
pass only when `fuelLimit >= 3*daySteps` (planner.cpp:4229/4384 → 1387) and
skips brand-marginal-zero columns (planner.cpp:1359); the accepted terminal
refiners (191/207) are guard-restricted to the terminal day. Live m-4039
lost rank 4 with tier-1/2 tied at cap and a structural ~3 servings/day
mid-day chain-depth deficit.

## Mechanism

New protected lane `ProtectedSlackRefiner::refine_midday_chains`
(src/slack_refiner.cpp), chained strictly after the wait-detour fixed point
inside the same protected refinement window, consuming only the remaining
budget: per patrol (one-agent, 191 pattern), enumerate deep routes with the
existing `enumerate_sparse_anytime_resource_routes` machinery (spots ≤ 32,
32 routes, 1.25M settled states, deadline-guarded, 4 workers), pre-filter to
the incumbent's terminal cell, substitute one agent plan, dual-engine
validate, hash-dedup, and accept ONLY via the unchanged production-accepted
`strict_protected_improvement` certificate (equal road footprint + same
terminal cells + patrol fuel ≥ + lifetime-brand monotone + strict
lexicographic day gain); iterate to a fixed point. Mid-day acceptance is
sound by construction: the day transition is state-identical, so the future
domain is preserved and no opponent-traffic side channel exists (equality,
not subset). Main timed solve, column generation, retention, master,
comparator, role selection and all existing refiners unchanged (194/195/198,
201, 164 honored). Research A/B flag `--midday-chain 0|1` in the historical
tournament harness; production (btc_main HTTP path) enables it, mirroring
187/191/197/207.

## Development gate (quiet VM, sequential off→on, same binary, both sides terminal-pair ON)

paired=60 **W/T/L=39/17/4, servingsDelta=+336**, acceptance-conditional
**38W/1T/2L**, on-side acceptances=145 across 41/60 cases (off-side
control=0), zero invalid/emergency/lane-failure, runtime parity mean
2470/2475ms max 3018/3015ms. Gap-regime lanes: hard 17/2/1, very-hard
16/6/2, fuel:low 17/6/2; max single-case gain +65 servings (4923019,
acc=21). All 4 losses in the 165 timed-search noise channel (2 with acc=0
where the transition is provably unchanged; 2 with certified
state-identical transitions diverging only in later timed solves).
Logs: off `205EE168C985D197873D38A15AAA40FCFDEE0BC7CD20C4D5A1062CADB3DEC466`,
on `64E49EF9199780929308805AA522DBE9484C3EC8C1493C657674D522ED8C01BB`.

## Holdout gate (sealed, opened after dev-pass; same protocol)

paired=108 **W/T/L=63/42/3, servingsDelta=+465**, acceptance-conditional
**62W/0T/2L**, on-side acceptances=222 across 64/108 cases (off-side
control=0), zero invalid/emergency/lane-failure, runtime parity mean
2461/2458ms max 3020/3021ms. Every lane positive: fuel low 22/17/0,
default 22/13/2, high 19/12/1; easy 2/13/1, medium 9/7/0, hard 24/11/1,
very-hard 28/11/1; window short 28/24/2, long 35/18/1. All 3 losses within
the same noise channel.
Logs: off `6C1926A088AD67F6846E686CD9AC02B3F37DE31685295C05DD622DBC7BC34C5E`,
on `D842212A0C71CDB179E6B6E76889389D81CBCA3D50BAD075437CF19A47669643`.

## Acceptance and production enablement

All pre-registered kill conditions clear (yield ≫ 0; zero
invalid/emergency; runtime parity). Production `udonshield_btc` HTTP path
enables the lane after the wait-detour phase and merges its diagnostics
into the protected_slack telemetry (middayRoutes/GeneratedPlans/ValidPlans/
ChainAcceptances/Rounds/Chain/Failure). Full build + unit/simulator/
validator suite passed. Production binary SHA256
`B8D7DD216E91D0C921DC012905EE816B833BFAA6282A6D8B9011FA2E15990342`.

**BTC DEBT**: operator plays live practice matches with the 210-enabled
binary on return — confirm valid submissions, reserve safety, and
middayChainAcceptances telemetry on the target host.

## Venue

GCP VM udon-stream-185-0822 (c3-highmem-4, quiet, sequential sides — 208
rule); local machine used only for functional smoke on non-manifest seeds.
