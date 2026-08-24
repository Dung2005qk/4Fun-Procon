# ATTR-MIDDAY-LIVE-YIELD-212 — live-regime yield attribution (CLOSED, lane vindicated)

Registered 2026-08-25, parent `288d17f`. Frozen manifest
`research/holdouts/ATTR-MIDDAY-LIVE-YIELD-212.csv` SHA256
`BF9552E1D40BE3963540887FAD03238E882B7B8B53C29F58EFBB15272DCA354A`
(6 live replays; nonterminal days only). Probe
`research/probes/midday_live_yield_probe.cpp`: for every nonterminal live
day, reconstruct the exact wire state (`day_state` frame), ledger echo and
SUBMITTED plan (`actions` frame) as incumbent, dual-engine revalidate, then
run the 210 one-agent search structure offline with NO deadline at two cap
tiers — production (minimumSpots formula / 32 routes / 1.25M states) and
raised (1 / 128 / 8M) — decomposing every rejection. The certificate is
never relaxed. Log
`ATTR-MIDDAY-LIVE-YIELD-212.log` SHA256
`1DA9238FC40A4129F021EADD019CB21983E6BD1BC8FFFB2703F4987B11F5EF18`.

## Question

210 fired at 41/60 and 64/108 cases at daySteps 16-18 but produced zero
live acceptances at daySteps=100. Incumbent same-terminal optimality, cap
starvation, or a lane defect?

## Totals

| tier | routes | terminal mismatch | footprint | fuel | not-strict | certified |
|---|---|---|---|---|---|---|
| production | 8640 | 7978 (92%) | 528 | 5 | 16 | **11** |
| raised | 34779 | 32038 | 2254 | 21 | 54 | **30** |

## The decisive split

Every certified improvement lies in the **pre-210 matches**:

- m-4037 (32x32/12 spots): day 7, two agents, **+2 servings** total
  (production caps).
- m-4038 (32x32/18 spots): days 3/5/6/9, best-per-day **+6 servings**
  (production caps).
- m-4039 (24x24 loss): day 2 **+3**, day 4 **+2** (production caps; raised
  adds day 5 +2) — the 210 lane would have recovered roughly a third of
  that match's 18-serving tier-3 gap.

The three **210-enabled matches (m-4043/44/45) contain ZERO certified
improvements at BOTH tiers**, even with unbounded time and raised caps.

## Verdict (composite of pre-registered rules a and c)

1. **The live lane is not defective**: in its own matches its 0 acceptances
   were the correct answer — nothing certified existed to take.
2. **The lane's live-regime value class is real**: +13 servings at exact
   production caps on pre-210 states — the class of improvement 210 was
   built to harvest exists at daySteps=100.
3. **No cap-adaptation SCORE candidate is justified**: raised caps add
   nothing on 210-enabled states.
4. The residual live gap (m-4044, ~0.8 servings/day behind the best bot at
   8 agents) is **coordination-shaped** — outside every admissible
   one-agent certificate (and outside the two-agent pinned certificate,
   per closed 211).

Axis closed. The only remaining evidence-backed direction is the main-solve
retention gate (`prune_columns` spot-diversity pass, planner.cpp:1387,
active only at fuelLimit ≥ 3×daySteps) — registered separately as its own
experiment with 198-aware gates.

## Venue

Local machine, structural answers (no deadlines) — venue timing irrelevant
by design.
