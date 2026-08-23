# ORACLE-BOUNDARY-DOMINANCE-175

Date: 2026-08-20  
Parent: `828ea78`  
Status: accepted exact method and counterexample; attribution active  

## Question

Can the exact public-information minimax oracle from experiment 172 be made
finite without weakening any official rule by pruning only own day outcomes
that are provably dominated at a complete day boundary?

## Proof obligation

At one max node, own outcome `A` may remove own outcome `B` only when:

1. both end at the same own cell;
2. both produce the exact same saturated own road-footprint vector;
3. `A` retains at least as much patrol fuel;
4. `lifetime | brands(A)` is a superset of `lifetime | brands(B)`;
5. `A` has at least as many current-day distinct brands and servings;
6. at least one resource or score relation is strict.

For every legal opponent day outcome, conditions 1--2 make the next public
traffic state identical, opponent terminal state identical and future opponent
action set identical. Conditions 3--5 let `A` copy every future own policy of
`B` while starting with no less fuel or official accumulated score. Therefore
the worst legal opponent continuation after `A` cannot be worse than after
`B`, so `B` cannot maximize the node.

This proof does **not** permit componentwise-smaller own traffic as dominance:
lower congestion can enlarge the adversary's future legal action set. It also
does not merge arbitrary memo states, prune opponent outcomes in the wrong
direction, fix a future opponent footprint, truncate actions or shorten the
horizon.

## Gates

1. Exhaustively compare filtered and unfiltered max nodes on tiny complete
   legal-action states; any value mismatch rejects the relation.
2. Reconstruct the exact 172 solver and require identical result under a mode
   switch on every fixture that completes.
3. The consumed seed `1720000` has method-feasibility authority only. It cannot
   tune the relation or provide score/promotion evidence.
4. Holdout remains sealed until the proof relation closes the first development
   fixture without semantic reduction.

No production file or designed capability is changed, removed, disabled or
reduced. All work is research-only; local resource measurements have only
offline method-feasibility authority.

## Results

The reconstructed 172 oracle was run with a mode switch that either retained
all exact-deduplicated own outcomes or applied only the relation above.

- On the complete final-day subproblem, all nine development configurations
  matched exactly at every official score tier. The filtered maximum own
  frontier fell from `455--796` outcomes to `202--377`; representative pairs
  were `588 -> 320` and `796 -> 377`.
- On the complete two-day traffic subproblem for consumed seed `1720000`, both
  modes returned `6/9/9`. The relation reduced states from `2,328,298` to
  `1,430,951` and transitions from `40,956,276` to `14,893,534` (`-63.6%`).
  The filtered run removed `798,866` own outcomes across visited max nodes.
- An initial full four-day attempt was stopped prematurely at `126.84` local
  CPU seconds. After correcting that invalid time-based stop, the same command
  was allowed to complete under commit-headroom monitoring. It returned robust
  score `6/12/12` after visiting `8,016,487` states and `704,745,239`
  transitions. The boundary relation removed `45,710,194` own outcomes; maximum
  own frontier was `346` from a raw maximum of `984`.
- Against each of the three registered causal policies (`maximum-dwell`,
  `minimum-dwell`, `status-toggle`), the exact policy remained `6/12/12` while
  unchanged HEAD reached only `6/11/11`. Every policy comparison was an exact
  tier-2 oracle win of `+1`; oracle and HEAD plans were both independently
  valid. Oracle plan hash was `f8ce3cafc8c65c55`, HEAD plan hash was
  `21126e84e051b0e8`; HEAD reported zero deadline-limited days.
- The completed run used local resources only. Time and memory have method-
  feasibility authority, not BTC or competition-performance authority.
- No full-match fresh development result and no sealed holdout was opened. No
  HEAD score comparison or BTC run was justified because the exact oracle did
  not complete its first required fixture.

## Verdict

The corrected equal-footprint relation is mathematically useful and survived
the registered complete one- and two-day parity checks. The broader external
proposal is still unsound: componentwise-lower traffic, arbitrary memo-state
merging and opponent-independent state dominance remain forbidden.

Experiment 175 is accepted as an exact research method and as a real
public-information counterexample on the declared one-active-patrol subdomain.
It does not promote source: the fixture is consumed, the oracle is far outside
the competition compute cap, and one subdomain cannot establish a globally
beneficial production mechanism. Canonical production remains `828ea78` and no
commit is permitted.

The next gate is witness attribution: identify the first day and capability
boundary at which HEAD loses the twelfth daily distinct without importing the
oracle computation into production. Only a general mechanism that survives
fresh development and the protected matrix may become a source candidate.

Reopen only if a second independent exact quotient attacks a different state
dimension and composes with this proved relation strongly enough to give a
finite full-horizon bound before any new fixture is opened. Never weaken exact
footprint equality, truncate legal actions, shorten the official horizon or
tune against seed `1720000`.
