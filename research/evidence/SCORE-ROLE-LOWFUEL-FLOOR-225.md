# SCORE-ROLE-LOWFUEL-FLOOR-225 — accepted-production

Date: 2026-08-26. Parent: baebad8. Gap: ATTR-ROLE-COLLAPSE-DISCRIMINATOR-224.
Production binary SHA256
`AAF73A3ADCD2E47B7C52A70E0A11B6767B37CB9C574BA7891E67399596FE08A7`
(also carries the 224 diagnostic `rolloutDailyTrace` — dead to decisions).

## Mechanism

Inside `apply_incomplete_long_horizon_role_fallback`, under the SAME
`shortHorizonRoleFallback` flag production already enables: for
`day_count() <= 5` with incomplete comparison, the early guard now keeps an
all-patrol front ONLY when `config.fuelLimit > maximumDaySteps` (the
existing low-fuel boundary from role_comparison_beam_width, 08771f1). An
all-patrol front at or below that boundary — a team whose entire match fuel
cannot cover one day of movement — takes the single-tanker floor exactly
like a tanker-heavy front. Single-tanker fronts are always kept; the
`>5-day` path is byte-identical; no new numeric constant exists.

## Why the structural boundary (224)

The truncated rollout is blind to multi-day fuel exhaustion in both
aggregate and per-day trace form (224 beams log). The boundary
`fuelLimit <= maximumDaySteps` classifies 10/10 observed cases: collapses
7300017 (100<=100), 8340005 (60<=60), 8400006 (100<=100), 7300008
(100<=100) vs healthy all-patrol 7300012 (200>100), 7340002/3 (180>60),
m-4195/96 (180>60), m-4149 (60>30), m-4209 (300>100).

## Evidence

- **Frozen-replay gates** (`…-225-gates.log`): replay-check exact on
  m-4043/44/45; beam[0] identical to accepted v3 on all eight replays
  (no archived live replay is short+low-fuel, so invariance is correct).
- **Collapse gates** (flag-on at dev budget): 7300017 6/26/83→6/30/128;
  8340005 8/27/69→8/40/115; 8400006 full daily (mask-same run); 7300008
  6/30/126 identical.
- **Dev** (fresh seeds, `…-dev-*`): easy 6/16/1 net +132 — all four tier-2
  differences are WINS (+2..+7 daily) via all-patrol→single-tanker on
  low-fuel seeds; brand8 0/13/2 net −4 single-tanker jitter only; medium
  control class-invariant; the single easy loss is same-mask
  (structurally not flag-attributable).
- **Sealed holdout** (`SCORE-ROLE-LOWFUEL-FLOOR-225.csv` SHA256
  `967509C68A76DE68…`, opened once): easy 4/10/1 net +131 — all four
  tier-2 differences WINS on fresh low-fuel seeds; brand8 2/5/0 net +35
  (its one mask change is a +23 win the floor cannot cause); general 0/7/0.
  Zero flag-attributable regression.

## BTC debt

Fresh short practice matches; if a short low-fuel config
(fuel <= daySteps) is creatable, expect >=1 tanker in the assignment frame.
