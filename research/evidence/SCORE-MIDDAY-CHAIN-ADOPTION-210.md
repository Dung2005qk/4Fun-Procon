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
sound by construction in the future-domain-DOMINANCE sense (correction
2026-08-24, external review): positions and road footprint are exactly
preserved while patrol fuel may be strictly HIGHER, so the successor state
is identical-or-dominating (a superset future domain), and no
opponent-traffic side channel exists (footprint equality, not subset). A
dominating state guarantees the reachable set only; a timed heuristic
policy is not monotone in its domain, so a bounded realized regression in
later timed solves remains possible. Main timed solve, column generation,
retention, master,
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
acc=21). Loss attribution (corrected wording): the 2 acc=0 losses have a
provably unchanged transition (pure wall-clock timed-search noise, 165
precedent); the 2 losses with acceptances have certified DOMINATING
transitions (fuel may be strictly higher), so they are bounded realized
regressions / timed-policy sensitivity, not provably pure noise. The
+336/39W-4L asymmetry bounds this downside far below the gain.
Logs: off `205EE168C985D197873D38A15AAA40FCFDEE0BC7CD20C4D5A1062CADB3DEC466`,
on `64E49EF9199780929308805AA522DBE9484C3EC8C1493C657674D522ED8C01BB`.

## Holdout gate (sealed, opened after dev-pass; same protocol)

paired=108 **W/T/L=63/42/3, servingsDelta=+465**, acceptance-conditional
**62W/0T/2L**, on-side acceptances=222 across 64/108 cases (off-side
control=0), zero invalid/emergency/lane-failure, runtime parity mean
2461/2458ms max 3020/3021ms. Every lane positive: fuel low 22/17/0,
default 22/13/2, high 19/12/1; easy 2/13/1, medium 9/7/0, hard 24/11/1,
very-hard 28/11/1; window short 28/24/2, long 35/18/1. All 3 losses within
the same channels (1 acc=0 pure noise; 2 with dominating-transition
acceptances = bounded realized regression / timed-policy sensitivity).
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

## BTC live gate — PASSED, debt paid 2026-08-24

The frozen production binary was used directly without rebuild; SHA256
remained
`B8D7DD216E91D0C921DC012905EE816B833BFAA6282A6D8B9011FA2E15990342`.
Three explicit advanced practice matches covered the requested gap regime
and two independent 32x32 controls:

- `m-4043`: hard, 3 bots, 7 days, 24x24, 100 steps/day, 5000 ms,
  4 agents, 18 spots, 6 brands, low fuel. 7/7 valid ACKs; exact
  replay-check `6/42/127`; max response 2693 ms, max solver 2102 ms.
  Mid-day lane active 6/6 nonterminal days, 608 routes, 22 generated and
  22 dual-valid plans, zero deadline/failure, reserve 1100 ms.
- `m-4044`: hard, 3 bots, 10 days, 32x32, 100 steps/day, 5000 ms,
  8 agents, 18 spots, 6 brands, default fuel. 10/10 valid ACKs; exact
  replay-check `6/60/364`; max response 4072 ms, max solver 2658 ms.
  Mid-day lane active 9/9 nonterminal days, 1280 routes, 80 generated and
  80 dual-valid plans, zero mid-day deadline/failure, reserve 1100 ms.
- `m-4045`: hard, 3 bots, 10 days, 32x32, 100 steps/day, 5000 ms,
  4 agents, 18 spots, 6 brands, low fuel. 10/10 valid ACKs; exact
  replay-check `6/60/144`; max response 3471 ms, max solver 2954 ms.
  Mid-day lane active 9/9 nonterminal days, 768 routes, 47 generated and
  47 dual-valid plans, zero deadline/failure, reserve 1100 ms.

Aggregate: 27/27 valid acknowledgements, zero emergency, 24/24 nonterminal
days with `middayChain=true`, 2656 enumerated routes and 149/149 generated
plans accepted by both the exact simulator and independent validator.
`middayChainAcceptances` was emitted on every protected-slack record and
was zero in all three matches. This does not claim a live score takeover:
these fixtures supplied no challenger that passed the strict certificate.
The gate closes the registered target-host wiring, validity, reserve and
telemetry debt; causal score authority remains the frozen development and
sealed-holdout A/B above.

Replay SHA256:

- `m-4043`: `5FBDD1D030045B26E5C162468CAE8CFD532B82C264B5A78B20E848FC50740810`
- `m-4044`: `117DCA82749300396ADF00372156C2D65F57F8119A3A4B672D077728463BFEC0`
- `m-4045`: `CB3C131120CEA9725D2AD9C4C607F9636E856809998853601EBA0BF031C63C90`

`m-4044` finished rank 2 at `6/60/364` versus the leading bot
`6/60/372`. Since lifetime and daily tiers were tied and the protected
mid-day lane accepted no replacement, this is a small tier-3 development
counterexample, not evidence of a 210 regression or promotion benefit.

## Venue

GCP VM udon-stream-185-0822 (c3-highmem-4, quiet, sequential sides — 208
rule); local machine used only for functional smoke on non-manifest seeds.
