# ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209 — coupled-suffix pre-gate (read-only)

Registered 2026-08-23, parent `690728a`. Frozen manifest
`research/holdouts/ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209.csv` SHA256
`849C5162492219F6230EFA82B4C898072E0C12A7A08788433BD9EB5D98024B73`
(amended pre-measurement: scope string normalized; winning roots frozen in
the ledger row). Probe change: research-only `--trace-membership` mode in
`multi_patrol_oracle.cpp` — singleton root-stream re-solve of the winning
day-1 root, then a walk of the memoized argmax trajectory per causal policy
measuring, per day and from the coupled day state (both teams' traffic):
oracle spot-mask membership in exact-orienteering maximal/terminal routes,
witness-caps (W1) master contains-outcome, production-caps (16 cols / 12
targets / 4 paths, master 40000/32/8, no deadline so answers are structural)
contains-outcome, and the first candidate cap in {32..256} retaining the
oracle outcome. Every oracle day plan is dual-engine validated
(`evaluate_exact_plan`) before measurement; walker fidelity is checked by
the trajectory's final score equaling the memoized robust score.

## Witness 1: 1721100 (root 36, plan `3.2.2.2.0.1.5.-3`, robust 5/17/18)

Log `ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209-1721100.log` SHA256
`4017E64055BD0AF28F06A2A10FF3144D9FBB3CEDE322F2B5F17CA6E1BDEAF738`.
Subtree: 4,418,788 states / 3.74G transitions. Walker exact: final 5/17/18
under all three policies (head remains 5/17/17 max/status, 5/16/17 min).

| day | oracle plan | day score | mask in maximal/terminal | prod_outcome | first_cap | note |
|----|----|----|----|----|----|----|
| 1 | 3.2.2.2.0.1.5.-3 (mask 62) | 5br/5sv | 1/1 | **1** | 32 | root generated+retained (202 confirmed) |
| 2 | -1.2.4.3.5.5.5.-3 (mask 62) | 5br/5sv | 1/1 (max/status) | **1** | 32 | prod_first=1 under max-dwell/status-toggle |
| 3 | **-1.2.-14 (mask 34)** | 2br/2sv | **0/0** (3 strict supersets, none same terminal+fuel) | **0** | **-1 (absent to cap 256)** | deliberate restraint day |
| 4 | **-1.5.0.1.2.2.2.4.3 (mask 63)** | 5br/6sv | **1/0** | **0** | **-1** | wait-prefixed full sweep; route exists in exact orienteering but no portfolio column reproduces the outcome |

Reading: the oracle's suffix advantage on this witness is a **traffic-shaping
sacrifice**: day 3 deliberately serves only 2 spots with `wait 1, one move,
wait 14` (keeping stops off the roads), buying a 6-serving all-spot sweep on
day 4 that needs a wait-prefixed route. Neither piece is expressible by the
production generator: restrained (non-maximal) routes are dominance-pruned,
and wait-prefixed columns are not generated (the accepted 187 refiner
retrofits wait detours precisely because of this, but only as local detours
on the incumbent, not as full alternative-day plans). Selection is NOT the
binding stage on days 3-4 — there is nothing to select. Days 1-2 confirm
generation+retention are fine where the plan is a maximal route.

## Witness 2: 1721200 (root 7, plan `3.2.2.2.0.0.5.5.-2`, robust 5/20/21)

Pending.

## Witness 3: 1720100 (root 223, plan `3.2.-13`, robust 6/19/19)

Pending.

## Venue

Local idle machine (207 holdout owns the VM); membership answers are
structural (no deadlines set), so venue timing is irrelevant to the verdict.
