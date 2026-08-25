# ATTR-NONTERMINAL-SPARSE-EXCHANGE-216

Parent: `177b588`

Frozen manifest: `research/holdouts/ATTR-NONTERMINAL-SPARSE-EXCHANGE-216.csv`

Manifest SHA256:
`9DE6557133B1649B557D4C30C210D72029D83E2407500965093575E49A02AE4E`

## Method

For every nonterminal day in the six frozen consumed BTC replays, the existing
`udonshield_claim_probe --recorded-sparse-exchange` reconstructs the submitted
plan and authoritative prefix. It performs the same sparse label relaxation,
dominance, minimum-spot rule, 32-route retention and 1,250,000-state cap used by
the production protected lane, without a wall-clock deadline. Each one-agent
substitution is checked by the exact simulator and independent validator.

`sparse_global_best` allows a changed terminal. `sparse_protected_best` also
requires `protected_slack_transition_dominates`, lifetime-brand monotonicity and
a strict official-score improvement; therefore road footprint and terminal
state are preserved and patrol fuel cannot decrease.

## Result

- Registered nonterminal days: 45.
- Global full-route strict improvements: 45/45, net `+136` servings; no tier-1
  or tier-2 change.
- Protected full-route strict improvements: 11/45, net `+17` servings; no
  tier-1 or tier-2 change and no simulator-validator disagreement.
- `m-4108` (32x32, 8 agents, 30 spots, default fuel): protected improvement on
  all 9 nonterminal days, per-day gains `+2,+2,+1,+1,+2,+2,+1,+1,+3`, net
  `+15`.
- `m-4044` (32x32, 8 agents, 18 spots, default fuel): protected `+1` on days 6
  and 8, reproducing the two target-terminal witnesses from experiment 214.
- The four 24x24/4-agent or low-fuel strata and the remaining 32x32/4-agent
  replay had no protected full-route gain, although every day had an
  unconstrained changed-terminal gain.

## Attribution

The capability is already mathematical, exact and future-safe. The remaining
gap is scheduling and duplicated search: accepted 215 first spends the entire
protected window enumerating a terminal-blind global pool and only then repeats
the same label search for the target terminal. In `m-4108`, the global prefix
reached the deadline every nonterminal day, so the target pass enumerated zero
routes even though the same label traversal contains 9 protected witnesses.

The next SCORE candidate must not replace or shorten the accepted global prefix.
It should collect the global and incumbent-terminal retained sets during one
shared sparse label traversal, evaluate the global pool in its existing order,
then evaluate the already-collected target pool with the unchanged protected
certificate. This preserves the designed global capability while removing the
duplicated traversal that starved 215.
