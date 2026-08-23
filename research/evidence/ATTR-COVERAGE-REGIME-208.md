# ATTR-COVERAGE-REGIME-208 — coverage-regime attribution (read-only)

Registered 2026-08-23, parent `690728a`. Frozen manifest
`research/holdouts/ATTR-COVERAGE-REGIME-208.csv` (registration-time SHA256
`81F47F1A2E1454404596036A04BDC1B48E5B1B2A7D4BD26910F563CF7DDF742D`; the
SYN-COLGEN-208 synthetic family definition and the two witness-replay hashes
were appended and frozen BEFORE the first synthetic measurement).

## Question

Consumed extreme-config BTC match m-3986 (32x32, 32 spots, 32 brands ==
spots, 200 steps, 8 agents, players=4, fuel 600) collapsed to 32/176/265
while three BTC bots held 320/318 tier-2. Day 1 achieved full 32-brand
coverage, days 2-10 fell to 10-23. Which pipeline stage loses the coverage:
generation, master, or selection?

## Replay-side attribution (consumed m-3986, client decision log)

Per-day audit extraction (decision lines carry full portfolio/master/
selection diagnostics):

| day | dailyDelta | colsByAgent | agentMs | agentQueries | cg deadline | pareto cacheHits |
|----|----|----|----|----|----|----|
| 1 | 32 | 12,21,22,24,25,23,24,23 | 0,0,0,17,107,126,47,136 | 24,54,52,38,42,34,24,34 | no | 178 |
| 2 | 23 | 12,23,22,12,12,3,3,3 | 28,218,205,155,95,0,0,0 | 24,54,54,42,29,0,0,0 | yes | 0 |
| 3 | 15 | 12,12,12,12,3,3,3,3 | 39,255,193,135,0,0,0,0 | 24,54,34,33,0,0,0,0 | yes | 0 |
| 4-10 | 10-16 | 12,12,12,12,3,3,3,3 | same shape | trailing zeros | yes | 0 |

Findings:

1. **Selection innocent**: the selected candidate equals the best audited
   candidate by after-today daily distinct on all 10 days.
2. **Master innocent (downstream)**: combinations 993 -> 119-158 because its
   input pool starves; capCuts only days 1-2; it consumes what exists.
3. **Generation is the earliest failing stage.** `planner.cpp:2820` runs a
   strictly agent-sequential column-generation loop under one shared phase
   deadline. From day 2 the Pareto route cache is cold every day
   (`cacheHits=0`; daily traffic churn invalidates it) and per-query cost
   rises ~10-20x, so the deadline lands mid-fleet: trailing agents receive
   **zero milliseconds and zero queries** and fall back to stub pools of 3
   columns (1 wait column priority -1 + 2 seed-plan columns,
   `planner.cpp:2794-2832`).
4. **Second non-scaling cap**: `decision.cpp:4124` fixes
   `maximumTargetSpots = 12` regardless of spot count, so even a
   fully-generated agent routes toward at most 12 of 32 spots (the constant
   "12 columns" plateau on completed agents).
5. The tanker occupies index 0 and is generated first every day
   (`agentMs[0]` always nonzero) while patrols at high indices starve —
   sequential order is agent-index order, blind to marginal value.

## Cross-replay prevalence (all 15 archived replays + m-3986)

Scan: starved day := column-generation deadlineReached AND >=1 non-first
agent with zero Pareto queries.

| match | config | starved days |
|----|----|----|
| m-3986 | 32 spots, brands==spots, players=4, steps=200, fuel 600 | **9/10** |
| m-3897-exp191 | 30 spots, 6 brands, players=2, steps=100, fuel 300 | **4/10** (days 5,6,8,10) |
| m-3908-exp195 | 30 spots, 6 brands, players=4, steps=100, fuel 300 | 0/10 |
| all 13 others (spots <= 24) | various | 0 (~90 days total) |

- Starvation is NOT exclusive to the extreme config: it fires in a
  standard-parameter match (m-3897, 30 spots).
- players=2 vs players=4 on otherwise similar 30-spot configs flips 4/10 vs
  0/10: traffic thresholds scale with player count, so fewer players means
  more busy/jammed flips, colder caches, costlier queries — traffic pressure
  is the causal driver of the per-query cost increase.

## Decisive-tier cost depends on brand redundancy

- m-3897 (6 brands over 30 spots, 5x redundancy): starvation cost **zero
  tier-2** (6/6 daily coverage held all 10 days) but **~40% of tier-3
  servings on stub days** (servDelta 29/27 on days 5-6 vs 44-48 healthy).
- m-3986 (brands == spots, zero redundancy): starvation directly destroys
  tier-2 (10-23 of 32).
- Softening effect: a starved agent whose contingency cache still applies
  keeps 12-15 columns and loses nothing (m-3897 days 8/10, servDelta 48);
  the harmful case is starved-to-stub (<=3 columns).

## Synthetic reproduction (SYN-COLGEN-208, registered grid): NEGATIVE at idle

48-match grid (2 seeds x spots {24,30,32} x daySteps {100,200} x brandMode
{mod6,distinct} x players {2,4}, fuel = 3x daySteps, budget 3375ms/day,
fixed tanker=agent0), log
`ATTR-COVERAGE-REGIME-208-syn.log` SHA256
`C420F3666F35A4BD7E836F971BF86AD4EA4FA3AC274781D115BE0C0D6C680494`:

- **Zero starved days in 480 match-days** (6 stub-days from unrelated
  causes). colGen 330-640ms mean, caches warm every day.
- Tier-2 coverage in distinct-brand mode: **94-100% of the theoretical cap
  on every axis combination** (e.g. spots=32/steps=200: 98%).
- The mechanism does not fire on an idle machine even in the extreme regime.

## Throughput discriminator (replay logs, free)

labels/ms (throughput) vs labels/query (intrinsic hardness) per day:

- m-3908 (healthy): ~2200-2700 labels/ms constant, labels/query 1500-2500.
- m-3897: healthy days 2030-2618 labels/ms; starved days 5-6 collapse to
  976/763 labels/ms — a 2-3x throughput loss fluctuating WITHIN one match.
- m-3986: day 1 1283 labels/ms; days 2-10 631-840 (plus labels/query up
  ~1.7x from real traffic).
- Local idle baseline on the registered 32/200/distinct/players=2 cell:
  ~2600 labels/ms (`ATTR-COVERAGE-REGIME-208-load-baseline.log` SHA256
  `9EF5738279372A58597064BD893BDC8AF5C1C6D8E7ED9839B1385CE7C226A76B`).

## Controlled contention reproduction: POSITIVE

Same registered cell, same binary, same budget, with 8 verified spinner
processes saturating all 8 cores
(`ATTR-COVERAGE-REGIME-208-load-contended.log` SHA256
`FC29AE19C3D4758FDBB3EAE0A49D0931B35E77F888FCFD73ACEF82B815227166`):

- Score 32/312/868 (idle) -> **32/269/683** (-43 tier-2 daily, -185
  servings).
- colGenMs 1200-1450 (live m-3986: 1200-1340), labels/ms ~550-850 (live:
  631-840), master combos 0-129 (live: 119-158), cgDeadline/starved/stub
  days appear.
- The contended local run reproduces the live m-3986 signature; the idle run
  on the same fixture is healthy.

## Verdict

**accepted-attribution-venue-artifact.** The earliest failing stage is
column generation (sequential per-agent loop under a shared deadline), but
the dominant cause of the observed production starvation is EXTRINSIC CPU
contention on the operator machine: m-3986 provably ran inside the local
207-candidate development window (21:23:36-21:26:11 within 21:16-21:39);
m-3897's throughput collapse fluctuates within the match, consistent with
concurrent local experiment batches; the controlled contention experiment
reproduces the live signature on demand and the idle run removes it. Real
traffic contributes a ~1.7x intrinsic labels/query increase that an idle
machine absorbs comfortably within budget.

No algorithmic SCORE candidate is justified: on a clean venue the planner
already meets 94-100% of the tier-2 viability bound in every registered
regime. The sequential-generation shape remains a fragility amplifier under
slowdown (degradation is cliff-shaped, trailing agents get zero), so the
contingency is recorded as a reopen condition instead of a candidate.

Residual honesty note: BTC bots reached 100% coverage (320/320) in m-3986
versus our idle-machine ~94-98% synthetic coverage; a small real gap in the
extreme brands==spots regime may remain, but it is regime-scoped, low
competition value, and unmeasurable until clean-venue matches exist.

## Operational consequences (competition-critical)

1. **Never run experiment compute (local or otherwise CPU-visible) on the
   match machine during a live match.** Measured cost under full contention:
   -43 tier-2 / -185 servings on a 10-day extreme match. This also
   retroactively explains the user's "extreme-config collapse" benchmark
   losses (rank 4 at 32/176/265): those matches were self-contaminated.
2. Pre-competition check: after the first clean-venue match, scan the live
   replay's columnGeneration.agentParetoQueries for zeros — the starvation
   signal is logged for free. If zeros appear on a dedicated machine, the
   reopen condition below fires.
3. The 5000ms cap and all budgets stay unchanged.

## Reopen condition

Reopen a generation-rebalancing SCORE candidate only if a clean-venue
(dedicated machine, no concurrent compute) match on the BTC target host
shows columnGeneration starvation (zero-query patrol agents) or tier-2
coverage materially below the day viability bound; never justify it from
contended-venue evidence.
