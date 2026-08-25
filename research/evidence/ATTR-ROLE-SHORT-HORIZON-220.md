# ATTR-ROLE-SHORT-HORIZON-220 — short-horizon role selection trusts truncated-rollout noise

Date: 2026-08-25. Parent: d80bf4e. Read-only attribution; binary unchanged
(SHA256 `96D538120BC9848DEA4EE1F904598260C06A36DC072F6DC97CB0F2FCBBED91FB`,
the binary that played the losses).

## Trigger

Target-host practice losses m-4195 (8/40/100, rank 4) and m-4196 (8/40/94,
rank 4) in the new 12x12 / 5-day / 60-step / fuel-180 / 8-brand / 12-spot
regime. All three opponents ran 3 patrols + 1 tanker; every team tied tier-1
(8) and tier-2 (40); winners scored 114/113 servings. Our live selection was
2 patrols + 2 tankers (`[0,0,1,1]` = PPTT).

## Root cause (instrument audit)

1. `select_roles_until` compares role assignments with
   `role_assignment_better_after_rollout` (decision.cpp:3530): lexicographic
   rollout L/D/Q first. Rollout evidence comes from
   `rollout_role_assignment` with per-day budgets of 40-60 ms and generation
   caps `maximumTargetSpots = 6` (decision.cpp:3473) — below
   `brand_count() = 8` in this regime. `rolloutComplete = 0` is the NORM
   (inner searches are budget-capped by design), not a rare overload.
2. On this regime the truncated rollout hands 2-tanker compositions a
   phantom daily-distinct edge (37-38 vs 36 for >=3-patrol rows) that
   JITTERS across identical reruns (rank 1 alternated PTPT/PPTT in two
   back-to-back `replay-roles` runs on m-4195). In reality every
   composition reaches 40/40 daily.
3. The accepted long-horizon protection
   `apply_incomplete_long_horizon_role_fallback` (8caea45) corrects exactly
   this phantom-D pattern on >5-day matches — the m-4043 table shows raw
   rank order PTTP D=38 above PPPT D=37 with the fallback rotating the
   single-tanker to front — but it is gated `day_count() <= 5`
   (decision.cpp:3583), and a tests/test_main.cpp case pins that gate
   ("protected 4/5-day horizons must preserve the parent role selection").
   Short matches were simply never in the practice pool until today
   (first ≤5-day multi-brand configs: m-4149/m-4155/m-4195/m-4196).

## Evidence A — replay-roles tables (`*-roles-tables.log`, SHA256 `C7AD8FED…340E`)

- m-4195/m-4196: 2-tanker rows lead on rollout D 37-38 vs 36/33-32 for
  >=3-patrol rows; PPPP rollout Q 79/58 far above the 2-tanker 54-56;
  upper bounds tie at L/D and favor more patrols on Q (135/150 vs 120).
  Rank 1 is unstable across reruns (noise decides the live match).
- m-4149/m-4155: rollout D saturates equal (16) for every composition, so
  the comparator correctly falls through to Q and picks PPPP (the live
  winner).
- m-3810/m-3907: `rollout_complete=1` — comparisons trustworthy, PPP wins.
- m-4043/m-4044/m-4045/m-4194 (>5 days): fallback already active; the
  m-4045 table shows what a REAL tanker D-effect looks like (PPPP D=39 vs
  single-tanker D=55 — large and structural, unlike the ±1-2 phantom).

## Evidence B — full-engine counterfactual matrix (`*-counterfactual-matrix.log`, SHA256 `1F3155E5…DF3C`)

`replay-counterfactual` (production engine, 5000 ms/day, recorded opponent
traffic), L/D/Q summaries:

| match | live | mask0 all-P | best 1-T | worst 1-T | winner |
|---|---|---|---|---|---|
| m-4195 | 8/40/100 (mask12, reproduced exactly) | 8/40/128 | 8/40/130 (m8,m2) | **8/37/117 (m4 — tier-2 LOSS)** | 114 |
| m-4196 | 8/40/94 (mask12, reproduced exactly) | 8/40/118 | 8/40/130 (m8,m4,m2,m1) | 8/40/130 | 113 |
| m-4149 | 4/16/64 live PPPP | 4/16/68 | 4/16/64 | 4/16/64 | won |
| m-4155 | 4/16/74 live PPPP | 4/16/71 | 4/16/70 | 4/16/63 | won |
| m-3810 | 1/4/24 live PPP | 1/4/24 | 1/4/20 | 1/4/19 | won |
| m-3907 | 1/4/16 live PPP | 1/4/16 | 1/4/16 | 1/4/15 | won |

Counterfactual run-to-run noise is ±3-4 servings (m-4149 mask0 68 vs live
64; m-4155 mask0 71 vs live 74 — same composition, wall-clock anytime
search). The m-4195 mask4 result shows single-tanker placement can lose
tier-2 daily distinct (37 < 40) — and mask4's composition (PPTP) is
rollout-Q-TIED for best single-tanker (64 = PTPP), so a pure single-tanker
floor would coin-flip into a tier-2 loss on exactly the regime it is meant
to fix.

## Conclusions

1. Both losses are fully attributed to the day-0 composition choice; the
   engine itself outscores every opponent (130 vs 114/113) given 3 patrols.
   Not machine speed (deadlineReached=false everywhere; checkpoint
   predictions matched realized exactly), not the protected-lane stack
   (0 acceptances, inert in this regime).
2. Rule shape identified (SCORE successor): extend the incomplete-rollout
   fallback to day_count()<=5 behind a default-off flag, with a
   short-horizon-only upgrade — when the best all-patrol row's rollout
   lexicographically dominates the best single-tanker row (L >=, D >=,
   Q strictly >), prefer all-patrol. On the frozen matrix this picks PPPP
   on all four multi-brand short matches (128 / 118 / 68 / 71 — rank 1
   everywhere, no tier ever lost, never below the live score) and stays
   inert on complete-rollout and >5-day matches. The double-tanker branch
   is structurally unreachable for day_count()<=5
   (`role_comparison_beam_width(config,1)` returns 1 there).
3. Residual oracle gap accepted for robustness: m-4196 all-patrol 118 vs
   single-tanker 130. Recovering it safely needs a tanker-placement
   D-safety certificate — reopen condition for a successor, not inline.

Frozen inputs: `ATTR-ROLE-SHORT-HORIZON-220-manifest-sha256.txt`
(SHA256 `2CF28E990AE34D90302EFD0FA5AFB81C4D383C00330688A5AE121FE1B635E2BA`).
