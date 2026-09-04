# Literature and architecture audit for the remaining UDON-SHIELD gaps

Date: 2026-09-04
Scope: literature from 2003--2026, current canonical production 258, and the
remaining route-supply, synchronized-refuel, state-coupled and opponent-traffic
gaps.  This is a design audit, not a score experiment.  It changes no production
source and has no promotion authority.

## Executive verdict

There is no published drop-in algorithm that solves the full HEXUDON problem.
The game combines six hard structures that the literature usually studies
separately:

1. team orienteering and selective prize collection;
2. two resource dimensions (steps and Patrol fuel);
3. exact synchronization with a mobile, unlimited-fuel support vehicle;
4. a multi-day rolling state with stock, terminal positions and fuel carried
   across days;
5. endogenous two-day road traffic created by all 8--10 teams, with unknown
   opponent actions and threshold discontinuities; and
6. an exact three-tier lexicographic score under a 5000-ms main checkpoint.

The literature nevertheless contains one high-value architectural direction:
**incumbent-goal lexicographic bidirectional resource-constrained pricing**.
It combines:

- bidirectional label search and safe dominance from modern RCSP algorithms;
- the bounded bidirectional/DSSR pricing used by exact Team Orienteering
  branch-and-price;
- lexicographic goal pruning, using the already certified incumbent score as
  the goal; and
- the existing exact simulator, validator and protected-incumbent replacement
  rule.

This is materially different from the rejected experiments that appended more
columns to the same bounded portfolio.  Its purpose is to reach the same or a
strictly better route frontier with fewer settled states, not to add another
heuristic family or change the score semantics.  It is therefore the best
post-competition research direction, but it is not a safe six-day pre-contest
production change.

Until September 10, the correct operational choice is neither another blind BTC
bot batch nor passive waiting.  Freeze canonical 258, perform readiness drills
and artifact/protocol verification, and accept only genuinely diverse
human-opponent or official multi-team counterexamples.  BTC's three-bot proxy
should be used only when protocol freshness or a materially new public
configuration needs checking.

## What the current solver already implements

The current implementation is not a naive greedy solver:

- `src/orienteering.cpp:1001-1470` performs forward resource-label search over
  `(visited spot mask, cell, used steps, used fuel)`, keeps Pareto labels by
  step/fuel dominance, orders anytime work by cardinality/steps/fuel, and enforces
  state/deadline caps.
- `src/orienteering.cpp:1759-1879` retains and ranks maximal and supplemental
  routes, but the incomplete path is explicitly forward-only and finally
  truncates the returned route set.
- `src/planner.cpp:4450-5550` already handles synchronization metadata, bounded
  combinations, exact official lexicographic score, stock-aware ordering,
  suffix upper bounds and bundle-aware branch-and-bound.
- Production already preserves a complete 5000-ms incumbent and accepts a
  continuation replacement only after exact simulation, independent validation
  and certified strict non-regression.

Therefore generic A*, generic ALNS, another weighted score, or simply increasing
the cap would mostly duplicate existing concepts or break the research contract.
The unresolved opportunity is a stronger way to generate/prioritize exact
columns before the fixed checkpoint, plus a sound representation for coupled
Patrol/Tanker columns.

## Literature mapped to the actual gaps

### 1. Resource-constrained route generation: strongest direct transfer

Ahmadi, Tack, Harabor and Kilby introduce RC-EBBA*, a bidirectional A* method for
resource-constrained shortest paths.  Its main transferable ideas are strong
initial upper/lower bounds, safe resource-aware dominance, forward/backward
labels and exact joining.  On their benchmark suite it solved all 440 instances
and improved large-instance runtimes by as much as four orders of magnitude.
The result does not directly cover prize collection or multiple agents, but it
targets precisely the current forward label expansion bottleneck.

Primary source: [A Fast Exact Algorithm for the Resource Constrained Shortest
Path Problem, AAAI 2021](https://doi.org/10.1609/aaai.v35i14.17450).

PathWyse is a useful C++ research implementation containing bidirectional
labeling, DSSR, NG-path relaxations, dynamic halfway points and customizable
resources.  It is a reference for experimental design, not code to copy
wholesale: the open repository is GPLv3 and its built-in A*-based path is stated
to cover single-resource acyclic problems, whereas UDON needs cyclic,
multi-resource, elementary prize-collecting paths.

Implementation reference: [PathWyse](https://github.com/idea-idsia/pathwyse).

**Fit:** high for settled-state/candidate-supply gaps.  The backward extension
must exactly invert UDON's movement-cost convention, Spot auto-claim rule and
step/fuel feasibility.  Bidirectional joining must retain the full visited-Spot
mask and cannot merge labels using a scalar reward.

### 2. Team Orienteering: pricing instead of portfolio flooding

Keshtkaran, Ziarati, Bettinelli and Vigo solve Team Orienteering with
branch-and-price.  Their pricing subproblem uses bounded bidirectional dynamic
programming plus decremental state-space relaxation and a two-phase dominance
relaxation; their method closed 17 previously unsolved benchmark instances.

Primary source: [Enhanced exact solution methods for the Team Orienteering
Problem](https://doi.org/10.1080/00207543.2015.1058982).

This supplies a better architecture than repeatedly appending speculative
routes to a capped portfolio: keep the incumbent's columns in the restricted
master, price only columns that can improve the current lexicographic bound,
then re-solve.  Full LP branch-and-price is unlikely to fit a 5000-ms contest
checkpoint, but its **restricted-master/pricing separation** is valuable.  A
small exact or Lagrangian pricing layer could focus the existing enumerator
without importing a general-purpose MILP solver.

**Fit:** high conceptually, medium implementation feasibility.  Experiment 295
showed that useful new columns can still cause bounded-search interference.  A
pricing design must make the certified incumbent an immutable member of the
restricted master and must not evict it or consume its mandatory evaluation
budget.

### 3. Exact lexicographic goals: use the incumbent as a pruning target

Pulido, Mandow and Perez de la Cruz's LEXGO* is an exact multiobjective
label-setting method for lexicographic goal preferences.  It returns the Pareto
paths satisfying a set of lexicographic goals, or the closest subset when the
goals cannot all be met, and proves that it explores a subset of the labels of a
full Pareto search.  Their three-objective tests report improvements up to
several orders of magnitude as the goals become restrictive.

Primary source: [Multiobjective shortest path problems with lexicographic
goal-based preferences](https://doi.org/10.1016/j.ejor.2014.05.008).

The exact transfer is to use the protected incumbent's
`(lifetime distinct, daily distinct, servings)` as an aspiration vector.  A
route/master partial state whose admissible completion bound cannot reach or
strictly beat that vector can be pruned.  This is superior to a weighted sum,
which is forbidden and can hide a loss at the first differing tier.

**Fit:** high at the master/pricing boundary, not as a stand-alone path score.
The single-agent label bound must include brand overlap, remaining stock and the
other agents' possible claims; otherwise it is not admissible for the team
objective.  The current master already has relevant suffix bounds, so the new
work is to propagate a sound goal backward into route generation rather than
re-implement master pruning.

### 4. Patrol/Tanker rendezvous: the closest published problem family

Recent synchronized vehicle-routing work models primary and support vehicles,
selective service and replenishment at simultaneous rendezvous.  Sakarya et al.
use a time-expanded network, network reduction and branch-and-price for a
two-echelon prize-collecting VRP with exact vehicle synchronization and solve
reported instances up to 200 customers.

Primary source: [Two-echelon prize-collecting vehicle routing with time windows
and vehicle synchronization](https://doi.org/10.1016/j.trc.2024.104987).

Ha et al. use LP-based insertion feasibility inside adaptive large-neighborhood
search for regular/special vehicles with synchronization constraints.  Their LP
repair operators explicitly preserve time-window and rendezvous feasibility and
outperform their CP-based ALNS across their instance classes.

Primary source: [A new constraint programming model and a linear
programming-based adaptive large neighborhood search for the vehicle routing
problem with synchronization constraints](https://arxiv.org/abs/1910.13513).

Wittwer and Tamke separately show that support-vehicle flows and exact temporal
synchronization can be represented compactly, including switching support
vehicles between primary vehicles.

Primary source: [The vehicle routing problem with synchronization constraints
and support vehicle-dependent service times](https://arxiv.org/abs/2403.03355).

**Fit:** high for modeling, medium-low for a pre-contest implementation.  The
safe transferable mechanism is an atomic synchronized column/group with an
exact Patrol and Tanker timeline, not a post-selection mutation of one route.
The 286--295 experiments already established the danger of incomplete or
portfolio-perturbing service groups.  Literature supports a future
incumbent-centered destroy/repair neighborhood in which a complete rendezvous
group is inserted only after exact feasibility and strict lexicographic gain;
it does not rescue the rejected implementations.

### 5. Time-dependent and stochastic traffic: useful warning, no complete proof

Fast time-dependent orienteering heuristics can solve 100-node cases in seconds;
Verbeeck et al. report a 1.4% average gap and 0.5-second average time on their
test set using ant-colony construction plus time-dependent local search.

Primary source: [A fast solution method for the time-dependent orienteering
problem](https://doi.org/10.1016/j.ejor.2013.11.038).

Adaptive stochastic orienteering jointly chooses reward nodes and adapts paths
as uncertain travel times are observed; Dolinskaya, Shi and Smilowitz emphasize
that shared edges make path-time distributions correlated and the objective
non-additive.

Primary source: [Adaptive orienteering problem with stochastic travel
times](https://doi.org/10.1016/j.tre.2017.10.013).

These models assume exogenous time-dependent or stochastic travel times.  In
HEXUDON, future road state is partly generated by our own routes and by unknown
actions of 7--9 adversarial teams, then quantized through congestion thresholds.
That breaks the independence/additivity assumptions and makes a one-day route
gain an unsafe multi-day certificate.  This is why the position/traffic
residuals found in the local research are genuinely difficult rather than an
already solved textbook case.

**Fit:** medium for scenario generation and adaptive replanning, low for a
formal no-regression certificate.  Ant colony, VNS or expected-traffic planning
may improve average score but cannot replace the exact protected path without a
fresh paired gate and a state-coupled guarantee.

### 6. Safe baseline improvement: correct principle, unavailable assumptions

Petrik, Chow and Ghavamzadeh formalize safe improvement under an inaccurate
dynamics model by minimizing robust regret relative to a baseline.  Their
method improves in states where model accuracy is known and falls back to the
baseline elsewhere; the exact robust formulation is NP-hard and they provide
approximations.

Primary source: [Safe Policy Improvement by Minimizing Robust Baseline
Regret](https://arxiv.org/abs/1607.03842).

This validates the architecture already used by the protected continuation:
retain canonical 258 unless a candidate is certified to dominate it.  It does
not provide the missing certificate because UDON has no calibrated uncertainty
set for opponent traffic transitions.  Pretending the simulator's guessed
opponent trajectories have known error bounds would turn a theorem into an
unsupported heuristic.

**Fit:** high as a safety principle, low as an immediately implementable solver.

### 7. Anytime heuristic search: useful plumbing, not the missing idea

ARA* obtains a feasible solution quickly, tightens a provable suboptimality
bound as time remains, reuses earlier search work and converges to the optimum.

Primary source: [ARA*: Anytime A* Search with Provable Bounds on
Sub-Optimality](https://robots.stanford.edu/papers/Likhachev03b.html).

UDON already freezes a complete incumbent and performs monotone protected
replacement.  ARA* becomes valuable only after an admissible heuristic is
defined over the actual lexicographic team/master state.  Applying it merely to
grid distance would not solve stock overlap, synchronization or future-state
uncertainty.

**Fit:** medium-low independently; useful as the scheduling shell around the
bidirectional lexicographic pricing direction.

## Ranked research opportunities

| Rank | Direction | Expected value | Main risk | Pre-contest decision |
|---:|---|---|---|---|
| 1 | Lexicographic goal-directed bidirectional RCSP pricing | High: directly attacks forward-label/candidate-supply starvation while preserving exact semantics | Admissible team-level bounds and reverse transition semantics are subtle | Design only; do not integrate before Sept 10 |
| 2 | Restricted-master column generation with incumbent retention | High-medium: avoids the search interference observed in 295 | LP/dual machinery and synchronization make it a large architectural change | Defer |
| 3 | Atomic synchronization-aware incumbent LNS | Medium: literature is close to Patrol/Tanker rendezvous and it can isolate changes from the main portfolio | Historical rendezvous axes already failed; must prove it is a new neighborhood and not a renamed repeat | Defer pending fresh counterexample |
| 4 | Robust/scenario opponent-traffic planning | Medium long-term | No authoritative opponent model/error set; threshold discontinuities cause conservative or unsound decisions | Collect real multi-team data first |
| 5 | ARA*/generic anytime search | Low-medium by itself | Duplicates existing incumbent/checkpoint architecture without an admissible full-state heuristic | Do not open alone |
| 6 | Generic ACO, bee colony, GA, transformer or MARL | Low before contest | Weighted/scalar objectives, training distribution shift, no safety certificate, reproducibility and deadline risk | Reject for current production |

## A bounded future experiment, if research reopens after the contest

The first future experiment should test capability, not score tuning:

1. Freeze a fresh development/holdout split before source change.
2. Replace the research snapshot's forward resource enumerator with one
   canonical bidirectional label-and-join path; do not retain `v2`, legacy or
   fallback branches.
3. Use monotone step and fuel resources, exact visited-Spot elementarity, exact
   reverse movement/claim semantics and lexicographic incumbent goals.
4. On exhaustive small graphs, require equality of the complete nondominated
   route frontier, final state, ledger and validator result.
5. On frozen 8/9/10-team development lanes, measure settled labels,
   time-to-first strict incumbent improvement, frontier coverage by 5000 ms and
   final paired official W/T/L against canonical 258.
6. Open a sealed holdout only if semantic equivalence holds when both methods
   complete and the new method supplies strictly better frontier coverage under
   the same deadline without a systematic downside.

This experiment must not copy GPLv3 PathWyse code into the product.  Its papers
and implementation can be used to understand algorithms and construct
independent tests.

## Operational plan through September 10

1. Keep canonical production 258 and its known artifact hash frozen.
2. Do one clean-checkout Release build/test rehearsal and a BTC token/protocol
   lifecycle check only if the current session or platform contract may have
   changed.
3. Prefer a small number of genuinely diverse human-opponent, official-domain
   multi-team games.  Record complete replay/configuration and treat a loss as a
   counterexample, not an automatic reason to patch.
4. Do not start another blind three-bot BTC series.  It has diminishing
   information value and cannot establish strength against the official
   8--10-team field.
5. Reopen score research only for a fresh counterexample that escapes the closed
   role, high-fuel/long-horizon, terminal-sparse, rendezvous and state-coupled
   clusters, or for an authoritative rule/capability change.

## Functionality-preservation check

1. This audit removes, disables, defers or reduces no designed functionality.
2. Nothing is deleted, so no active-equivalent replacement proof is required.
3. The proposed future direction is explicitly an in-place algorithmic
   replacement only after equivalence proof; it does not authorize a second
   production solver, a heuristic dispatcher or a weakened score/safety gate.
