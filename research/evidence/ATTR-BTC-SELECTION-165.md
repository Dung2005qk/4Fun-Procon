# ATTR-BTC-SELECTION-165

- Parent: `cf7e4b4` plus accepted hard-cap wiring candidate 163.
- Scope: consumed BTC replay `m-3573` only.
- Source candidate: none.

The live match completed `6/59/296`, with one missing daily brand on day 10.
All ten submissions were valid, all nine transitions reconciled, and the
independent validator agreed. Bot rank is excluded from the quality verdict.

The first counterfactual used the wrong diagnostic policy because it omitted
the production current-score floor; its current/parent tie at `6/60/265` is not
a production A/B. With the production floor enabled, current ended `6/60/266`
and frozen parent `6/60/267`. Both kept `6/60` through day 9; the one-serving
tail difference is local timed-search noise and has no BTC performance authority.

The useful structural check is day 1. The plan selected by the corrected replay
already exists in the live candidate audit as a certified upside challenger.
It ties the live selection on certified lower bound `6/6/31` and valid upper
bound `6/60/445`, but its q50 is `6/37/221` versus `6/37/222` for the selected
floor leader. It therefore does not dominate under the complete registered
tuple available before the suffix is realized. Changing the comparator to pick
it solely because the realized replay later scores better would overfit this
match.

Verdict: closed negative attribution. No general source defect was found and no
source was changed. Reopen only after repeated diverse fresh BTC or human
counterexamples establish the same structural ordering failure, or when a
candidate dominates before the realized suffix is known.
