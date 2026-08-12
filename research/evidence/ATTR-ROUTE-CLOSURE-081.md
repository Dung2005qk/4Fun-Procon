# ATTR-ROUTE-CLOSURE-081

Date: 2026-08-12

Parent: champion `7ef3694` plus research diffs 072 and 073. Frozen holdout SHA256:
`6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`.

This read-only probe classified exact active-route membership for every oracle
day in all 18 opened CEILING-MATCH-071 development fixtures. Each oracle plan was
dual-valid and replayed before advancing state. One fixed ordered capability
lattice was used for every family, fuel profile and day: production W1, cap16,
cap32/paths4, cap128/paths8, generic harvest/orienteering, exact-highfuel,
fuel-constrained exact and anytime fuel-constrained exact. The order is
attribution precedence; stages that switch exact algorithms are not claimed to
be set-monotone. Structural deadlines were generous and local elapsed was not a
result.

First-membership counts over 81 transitions:

- W1: 25;
- cap16: 23;
- cap32/paths4: 4;
- cap128/paths8: 2;
- fuel-constrained exact: 6;
- absent from the full lattice: 21.

The six fuel-constrained exact routes occur in four independent fixtures:

- balanced-low seed `1300000`, days 2, 3 and 4;
- rare-late-default seed `1310100`, day 3;
- terminal-position-default seed `1310400`, day 5;
- fuel-allocation-high seed `1320300`, day 4.

For seed `1310400` day 5 the recovered active route was
`-1.2.2.2.3.2.2.-4`, producing daily score `3/4` and terminal state `31@0`.
It appeared as an exact-orienteering column after 139 settled exact states. The
same capability class therefore recurs across four independent fixture/fuel
contexts. Current-day production already enables it under a terminal-day,
fuel-constrained public condition, while future W1 does not.

Verdict: accepted attribution. Holdout stayed sealed. The 21 absent oracle
routes mostly belong to matches already tied in official score; they are not an
optimization target without independent exact-score counterexamples.
