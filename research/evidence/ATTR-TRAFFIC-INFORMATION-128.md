# ATTR-TRAFFIC-INFORMATION-128

Date: 2026-08-13  
Parent: `5dccb0f`  
Fixture: consumed `CEILING-TRAFFIC-INDEPENDENT-124` threshold-loop/low
seed `2300200`, day 1

## Information boundary

The exact Markov match DP freezes `fixture.opponentFootprints` for every future
day from the fixture seed. Its transition function directly adds those future
footprints when deriving later road status. Production cannot observe this
sequence on day 1. `TrafficScenarioGenerator` instead constructs its manifest
only from the public current opponent state, public endpoint/opponent history,
current road status and traffic belief: 5000 weight for the public likely path
bundle and 5000 total weight for public adversarial/fallback cases.

The exact DP is therefore a clairvoyant full-match upper bound on this fixture,
not an implementable online policy under the production information set.

## Consumed profile attribution

The already-computed unchanged production profiles were:

- exact-oracle action current `6/6/6`, upper `6/24/28`, support
  `6/19/21@6/24/28:1 | 6/6/6@6/24/28:1`;
- parent action current `6/6/11`, upper `6/17/22`, support
  `6/16/21@6/17/22:1 | 6/6/11@6/17/22:1`.

Each scenario has weight 5000. The exact action fails certified dominance at
threshold `6/17/22` (`5000 < 10000` possible incumbent weight). The parent
fails dominance at threshold `6/16/21` by the same `5000 < 10000`. Thus neither
action robustly dominates under information available to production. Selecting
the exact action solely because its frozen realized continuation ends
`6/22/22` instead of parent `6/15/20` would condition on future opponent data.

## Verdict

Accepted negative actionability attribution. The repeated 124 oracle gap is a
valid clairvoyant ceiling counterexample, but it does not prove an exploitable
planner/value mismatch under the online public-information contract. It cannot
authorize risk relaxation, scenario reweighting, F0 tuning or route injection.
Reopen traffic score research only with a causal public predictor/calibration
signal learned from independent historical opponent behavior and evaluated on
a frozen opponent holdout, or with a candidate that robustly improves the
unchanged public scenario profile.
