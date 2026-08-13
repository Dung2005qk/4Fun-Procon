# ATTR-LADDER-CURRENT-EXACT-132

Date: 2026-08-13  
Parent: `5dccb0f`  
Fixture: consumed ladder-balanced/low seed `1800000`, day 1

The research probe cloned unchanged current-day generation options, enabled the
existing complete fuel-constrained exact enumerator and left anytime enumeration
off. Enumeration completed for all three supported agents: 393 settled states,
10 terminal variants and 8 bundles. Portfolio widths were `24|24|10`.

Despite complete enumeration, oracle individual-route membership reached only
mask `011`; one route remained absent. The unchanged normal master visited 468
nodes and retained neither exact plan nor equivalent outcome. This agrees with
131's independent day-2 result: existing 114 minimum-fuel served-spot routes and
aligned bundles do not represent the required cross-terminal resource class.

Verdict: accepted negative attribution. Enabling exact/fuel flags alone is not a
candidate. Closing the ladder axis is preferable to adding the full 95-route
frontier plus a second upper-aware solver. Reopen only if a new independent
bounded representation naturally emits the missing class or a different
earliest causal stage appears on a fresh exact counterexample.
