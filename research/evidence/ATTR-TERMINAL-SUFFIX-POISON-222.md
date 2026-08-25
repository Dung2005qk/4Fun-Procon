# ATTR-TERMINAL-SUFFIX-POISON-222 — accepted-attribution

Date: 2026-08-26. Parent: f596bb9. Read-only; binary
`4EB926039A50D28F2202BFBE840866D770FD1928119183441C0034377BAA2FE4`.
Manifest: `ATTR-TERMINAL-SUFFIX-POISON-222-manifest-sha256.txt`.

## Findings

1. **Hazard reclassified: search variance, not infeasibility.** Across 12
   pre-declared m-4195 PPTP counterfactual runs the distribution is
   {117 ×4, 130 ×8}. Every 117 run shows the day-3 solve parking all four
   agents on cell 2 (day 3 unharmed at 8/26) and day 4 collapsing to 5
   brands. But run10 (`…-mask4-runs.log`) ends day 3 with the SAME
   all-at-cell-2 cluster and still finds an 8-brand day 4 → an 8-brand
   day-4 plan from the clustered start EXISTS and is findable. Clustered
   terminals therefore raise the next-day search-failure probability
   (~4 failures / 5 clustered starts observed; 0 / 7 spread starts) rather
   than capping the score.
2. **The safety oracle exists but is subordinated.** `TerminalSlack`
   (planner.hpp:215 — worstRemainingBrandSteps / totalRemainingBrandSteps /
   patrolFuelReserve / overnightSpotCount, compared at planner.hpp:224)
   measures exactly the cluster-vs-spread distinction, but sits at
   tie-break rank 5 in `better_evaluation` (decision.cpp:1035) — below the
   quantile ladder built from UNCERTIFIED scenario outcomes
   (profile_cert after the poisoned day stays 8/24/78 while q50 claims
   8/40/130). The future component of those outcomes rests on
   FastViability-style relaxations (per-agent fuel-aware reachability over
   ALL remaining steps, decision.cpp:1713+), which cannot separate a
   clustered terminal set from a spread one two days from the end. A
   candidate whose uncertified estimate is optimistic-by-noise therefore
   outranks a spread-terminal alternative before TerminalSlack is ever
   consulted.
3. **The counterfactual instrument overstates the live tail.**
   `run_replay_counterfactual` calls solve_day + record_submitted only —
   it never runs the live post-ACK `precompute_next_day_contingencies`
   path, so the cached-contingency safety net (a known-good next-day plan
   from the submitted terminals) is absent offline. Live evidence with the
   net armed: m-4208 (2-3 contingencies/day) held 8/8/8/8/8 daily at a flat
   24 servings/day; m-4209 (3-5/day) held 7s with servings rising
   (36,36,36,38,41). Zero live occurrences of the signature to date.

## Verdict

Accepted-attribution; **no SCORE successor opened now**. A main-comparator
reordering is the highest-risk change class in the program (198), and the
measured live harm is zero occurrences in two matches. The successor
design is recorded for when evidence justifies it: a protected-lane
adoption of same-exact-day-score plans with strictly dominant
TerminalSlack — this requires a NEW certificate class (terminal positions
differ, so the strict_protected_improvement equal-roadFootprint clause
cannot apply) with its own soundness argument against future-domain
interference.

## Reopen conditions

Reopen (as SCORE) if: (a) any live match shows a tier-2 daily dip with the
cluster signature (all patrols within one cell-neighborhood at a day
boundary preceding the dip); or (b) a contingency-faithful counterfactual
instrument (replay-counterfactual extended to run the post-ACK contingency
path) still shows a material failure tail on the m-4195 class.
