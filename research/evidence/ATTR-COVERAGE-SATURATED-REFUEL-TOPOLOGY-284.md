# ATTR-COVERAGE-SATURATED-REFUEL-TOPOLOGY-284

Date: 2026-09-02

Parent: canonical accepted production experiment 258 (`ab3d699`) on branch
HEAD `233a93c`.

This is read-only attribution over the already-consumed clean BTC row 22 replay
and the completed same-binary experiments 282/283. It changes no production
source, opens no holdout and has no promotion authority.

## Fresh counterexample

BTC match `m-8135` ended `3/15/182` for canonical production and
`3/15/320`, `3/15/311`, `3/15/309` for the three public bots. Protocol,
validity, transition reconciliation and deadline telemetry were clean. The
replay SHA256 is
`0EFC3363DBBCE1868A5195FE41677D358C250718D6FC91B11C602A8E33C5CBB3`.

All three bots publicly selected six Patrols plus one Tanker. At the start of
day 2, bot 1's Tanker was co-located with Patrols 0 and 2 at cell 384 and both
Patrols had full fuel 240; bot 2 co-located and fully refueled Patrol 0 at cell
174; bot 3 co-located and fully refueled Patrol 0 at cell 508. At the start of
day 3, the corresponding observations were bot 1 Patrol 1 at cell 508, bot 2
Patrol 2 at cell 13, and bot 3 Patrols 0 and 2 at cell 418, all at full fuel.
This is authoritative public-state evidence that the bots repeatedly use one
mobile Tanker to replenish bounded Patrol subsets across days.

The canonical one-Tanker counterfactual is active rather than unwired, but its
topology is inefficient. For mask 1 it scores `3/3/43` on day 1 and only adds
18 servings on day 2, versus all-Patrol `3/3/50` and +27. On day 1 the Tanker
and three Patrols terminate together at cell 252 with full fuel; on day 2 the
same four-agent cluster terminates together at cell 519. The complete five-day
score is `3/15/143`, below all-Patrol `3/15/160`/`165` and far below the public
bots.

## Runtime-path finding

The current canonical generator contains three real refuel mechanisms:

1. a co-located multi-Patrol lockstep escort to one Spot;
2. a co-located Patrol/Tanker lockstep escort to one Spot; and
3. a moving one-Patrol/one-Tanker rendezvous followed by one Patrol target,
   while the Tanker shares at most the first two departure directions.

Required/provided refuel events, exact timelines, the shared master, exact
simulator and independent validator are all wired. The missing topology is an
independent multi-waypoint Tanker provider that can serve different bounded
Patrol subsets at successive meetings. This is precisely the capability
experiment 158 previously attempted.

## Why 158--160 cannot simply be restored

Experiment 158 implemented canonical multi-waypoint provider columns and was
strongly positive on roadless lanes, but the full road-containing fixed matrix
was `23/68/29`, first-tier gain/loss `61/70`, with four of six families net
negative. Experiment 159's empty-footprint admission still lost five
road-containing holdout cases. Experiment 160 showed that snapshotting and
solving a parent master prefix did not reproduce the parent whole-pipeline
decision because candidate intake, budget/order and W1 profile state had
already diverged. A second full solver would violate the single-path and
5000-ms rules.

The current W1 machinery does not supply the missing exact suffix protection.
`FutureWitnessRepairer::repair_profile` constructs feasible suffixes only for
the finite frozen traffic scenarios and then assumes zero new opponent
footprint after the first modeled future transition. Its generated route
portfolio is also bounded. `certified_dominates` therefore proves dominance
over that frozen scenario support against an admissible incumbent upper; it is
not a universal proof over every legal future opponent footprint. Calling it
an exact future certificate would be unsound.

A genuinely sound nonterminal certificate could require a strict current
official-score gain, identical roles and terminal cells, no lower Patrol fuel,
and componentwise no-greater own road footprint than the immutable incumbent.
Under those conditions the incumbent future suffix remains reproducible. But
that certificate cannot compare the all-Patrol incumbent with a one-Tanker
role assignment, so it cannot by itself close row 22.

## Verdict

The refuel gap is real and not a missing wire: public bots repeatedly execute
bounded multi-Patrol mobile refueling, while the canonical one-Tanker planner
over-clusters Patrols and loses throughput. However, the only previously
implemented general caravan mechanism was rejected by broad road-containing
downside, and the present certificate is not the exact suffix proof required
to reopen it. No immediate SCORE successor is authorized from 284.

The next admissible work must first produce one of two genuinely new pieces of
evidence on fresh official-domain fixtures: either an order-stable role-level
full-horizon discriminator that selects one Tanker without the 158--160
regressions, or a state-transition dominance construction that remains useful
without changing the role class. It may not restore 158, widen caps, route by
map/seed/bot/match identity, weaken `may_submit`, or use this consumed replay as
promotion evidence.

Functionality-preservation answers: (1) no designed functionality is removed,
disabled, deferred or reduced; this is read-only attribution; (2) nothing is
deleted, so no replacement-equivalence proof is required.
