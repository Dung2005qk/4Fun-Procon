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

Log `ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209-1721200.log` SHA256
`A1614100DB507FE8E988D7A762EC94063FF7EC96410D185FFE1779765AEA0758`.
Walker exact:
final 5/20/21 under all three policies. Solved on the 32GB VM after the
local attempt thrashed (7.8GB machine, memo working set larger than free
RAM — venue note below).

| policy | day | oracle plan | prod_outcome | first_cap | note |
|----|----|----|----|----|----|
| max-dwell / status-toggle | 1 | 3.2.2.2.0.0.5.5.-2 | 1 | 32 | root available (200 confirmed) |
| max-dwell / status-toggle | 2 | -1.4.3.2.1.1.3.3.-2 | 1 | 32 | wait-prefixed yet reproducible here |
| max-dwell / status-toggle | 3 | **-1.0.1.5.5.5.4.3.2.-1 (mask 63)** | **0** | **-1** | wait-prefixed 6-spot sweep; route EXISTS in exact orienteering (maximal+terminal) but no production column reproduces the outcome |
| max-dwell / status-toggle | 4 | -1.5.0.1.2.2.3.3.-1 | 1 (prod_first=1) | 32 | |
| min-dwell | 2 | -1.4.3.2.2.2.0.1.5.-2 | **0** | **-1** | wait-prefixed |
| min-dwell | 3 | -1.2.4.3.5.5.5.-6 | **0** | **-1** | wait-prefixed |
| min-dwell | 4 | -1.2.2.2.0.1.5.-4 | **0** | **-1** | wait-prefixed |

## Cross-witness pattern (after witnesses 1-2)

`contains_outcome` matches on final state + score, so wait-prefixed oracle
plans with an outcome-equivalent unshifted twin in the portfolio still show
prod_outcome=1 (e.g. 1721200 day 2 max-dwell). The days that are truly
inexpressible at every cap (first_cap=-1 through 256, w1_outcome=0) fall
into exactly two shapes:

1. **Full-depth sweep days** (mask 63, all six spots in one day): the route
   IS present in `enumerate_exact_resource_routes` maximal routes
   (mask_maximal=1), but the exact-orienteering bundle selector never adopts
   it into a portfolio column — an adoption gap, not a reachability gap
   (1721100 day 4; 1721200 day 3 max-dwell/status-toggle).
2. **Restrained positioning days** (non-maximal spot set with a specific
   terminal cell and fuel, setting up the next day's sweep): dominance
   pruning removes sub-maximal routes, so nothing in reachability or the
   portfolio reproduces them (1721100 day 3; 1721200 min-dwell suffix).

Both shapes bind at **generation/adoption, not selection** — a frontier
selector over the existing pool would have nothing to select (076/081,
100/113/203 precedents). The oracle's suffix advantage is a
position-then-sweep couple: sacrifice a day for placement, then serve the
entire map in one deep chain.

## Witness 3: 1720100 (root 223, plan `3.2.-13`, robust 6/19/19)

Log `ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209-1720100.log` SHA256
`2C16917F64B334C215C204A03955BE7638FC90CB9B7E00FF2CBFAF4FC07B3A6D`.
Solved on the 32GB VM (subtree 44,483,280 states / 47.26G transitions,
~9h). Walker exact: final 6/19/19 under all three policies; production-like
head reaches only 6/18/18, so the oracle's entire +1 advantage on this seed
sits in the trajectory below.

The trajectory is identical across all three policies (days 1, 2, 4; day 3
differs only in min-dwell membership):

| day | oracle plan | day score | mask in maximal/terminal | prod_outcome | first_cap | note |
|----|----|----|----|----|----|----|
| 1 | **3.2.-13 (mask 17)** | 2br/2sv | **0/0** (1 strict superset, none same terminal+fuel) | **0** | **-1 (absent to cap 256)** | the WINNING ROOT is itself a restrained positioning day — dominance-pruned, inexpressible at every cap and by W1 |
| 2 | -1.5.0.1.2.2.2.4.3.-1 (mask 63) | 6br/6sv | 1/0-1 | 1 | 32 | wait-prefixed full sweep, outcome-equivalent twin exists |
| 3 | -1.0.1.5.5.5.4.3.2.-1 (mask 63) | 6br/6sv | 1/1 | 1 (max/status), **0 min-dwell (first_cap -1)** | 32 | second consecutive full sweep |
| 4 | -1.5.0.1.2.2.2.-5 (mask 61) | 5br/5sv | 1/0 | 1 | 32 | |

Reading: purest position-then-sweep instance yet. Day 1 serves only 2 spots
(`move 3, move 2, wait 13` — stops kept off the roads), buying three
consecutive deep sweeps (6/6/5 servings). The inexpressible day here is the
ROOT day itself: production cannot even enter this trajectory — its head
plays a different day 1 and finishes 6/18/18. Shape 2 (restrained
positioning, dominance-pruned) exactly as in witness 1 day 3.

## Verdict (3/3 witnesses, closed 2026-08-24)

Per the pre-registered decision rule this closes as **generation-blocked
with per-day attribution**. Scope (clarified 2026-08-24, external review):
the verdict is that the measured generators — witness-caps and
production-caps portfolios at every cap 32..256 — cannot express the
decisive days on these three witnesses; it is not a proof that every
conceivable generator fails. On every witness, the decisive advantage days
are absent from BOTH the witness-caps and production-caps candidate sets at
every cap 32..256 (`w1_outcome=0, prod_outcome=0, first_cap=-1`); the
remaining days are expressible (`prod_outcome=1` at cap 32). No
present-but-dropped-by-selection day was observed anywhere — a frontier
selector over the existing pool has nothing to select. The two blocked
shapes:

1. **Restrained positioning days** (1721100 d3; 1720100 d1 = the root):
   non-maximal spot sets with a specific terminal cell and fuel,
   dominance-pruned before any portfolio stage.
2. **Full-depth sweep days** (1721100 d4; 1721200 d3 max/status; min-dwell
   suffixes): route present in exact-orienteering reachability
   (`mask_maximal=1`) but never adopted into a portfolio column — an
   adoption gap, not a reachability gap.

Axis redirect: targeted suffix **generation** — a protected-lane mechanism
that (a) adopts deep maximal exact-orienteering routes into columns and
(b) admits restrained positioning columns when they enable a next-day
sweep, honoring 194/195/198 (no pool-growth redistribution) and 201
(parent-work protection), gated by runtime signals only. Converges with
the live m-4039 loss (~3 servings/day chain-depth gap at fuel=2x steps).
Never promote a SCORE candidate directly from these consumed roots.

## Venue

Witness 1 on the local idle machine; witnesses 2-3 solved on the 32GB GCP
VM after the local 7.8GB machine thrashed on the 1721200 memo working set.
Membership answers are structural (no deadlines set), so venue timing is
irrelevant to the verdict.
