# SEM-CANONICAL-LABELS-174

Date: 2026-08-19  
Parent: `828ea78`  
Verdict: `rejected-general-regression`  
Manifest SHA256: `4D7A79E2DACF1CB92AD2FE058075538C6E817F80464CA7EC41ADBBC554A6243D`

## Candidate

The parser assigned internal brand classes by structural signatures of each
brand's member spot positions and stocks, sorted spots by cell, and rebuilt
`spotAtCell`. Raw brand values, equality classes, map, stock, action protocol,
simulator, validator and planner logic were unchanged. Agent ordering was
explicitly outside scope.

## Development

The consumed `1730005` anchor became invariant under spot reversal and numeric
brand relabel at `6/24/39`, improving the parent canonical `6/24/35`. Agent
rotation remained different as expected.

Fresh fixed-role general development was candidate-vs-parent `3/8/1`: tier-2
gains `+1,+2,+2`, one tier-2 loss `-1`, invalid/emergency 0. All 24 spot/brand
metamorphic pairs were equal. Fresh deadline-role development was `1/3/2`: one
tier-3 gain `+1`, one tier-3 loss `-1`, and one tier-2 loss `-2`, with zero
invalid/emergency. Combined development was `4/11/3`, tier-2 gain/loss `5/3`
and tier-3 gain/loss `1/1`, enough to open only the first frozen holdout block.

## Holdout

The 36-fixture general fixed-role holdout was opened exactly once. Result was
candidate-vs-parent `6/19/11`, zero invalid/emergency. First-differing-tier
magnitudes were:

- tier 2: total gain 7, total loss 14;
- tier 3: total gain 1, total loss 4.

The largest losses were daily distinct `-3` on seeds `1741001` and `1741016`;
another loss was `-2` on `1741011`. The largest gain was daily distinct `+3`
on `1741025`. All 72 sealed spot-reverse/brand-relabel checks were score and
validity invariant, proving the mechanism worked while the selected canonical
heuristic representative regressed globally.

Native-role and every BTC-like holdout row remained sealed after this blocker.
No local timing is used in the verdict.

## Decision

Canonical representation does not create a monotonic solver improvement; it
merely chooses one member of a non-monotone bounded-search equivalence class.
Choosing a different canonical rule after seeing this holdout would be direct
overfit. Evaluating several representations would divide the same 5000-ms
budget or duplicate the solver and is not justified by a dominance certificate.

Candidate and metamorphic harness code were fully removed. `src/protocol.cpp`,
`old/harness/historical_tournament.cpp` and `research/probes/multi_patrol_oracle.cpp`
are content-identical to `HEAD`. No production commit was created.
