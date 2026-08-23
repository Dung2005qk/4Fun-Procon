# CEILING-TRAFFIC-MINIMAX-172

Date: 2026-08-19  
Parent: `828ea78`  
Verdict: `rejected-method-state-explosion`  
Frozen manifest SHA256: `6FD7914EE9AED02E0AC7CC7C0742D010A7D93D5365473031531CAF055B3F148A`

## Question

Can the clairvoyant opponent-footprint premise of experiment 124 be replaced by
an exact public-information four-day game in which our policy maximizes the
official lexicographic score and a legally moving opponent minimizes it?

The declared subdomain was a valid two-player match with one active patrol and
two isolated controls per team. The opponent was conservatively allowed to
choose after seeing the current own action. State retained both teams'
positions, fuel, previous traffic footprints, public road status and our
lifetime brand mask. Every action was to retain an exact simulator and
independent-validator witness. No future opponent footprint was frozen.

## Feasibility attempts

Only development seed `1720000` was touched. No holdout row was opened.

1. Full legal day outcomes plus exact memoization did not close after more than
   120 seconds. The process remained CPU-active and reached roughly 96 MiB
   resident memory before termination.
2. Adding an exact cache keyed by day, position, fuel and public road status did
   not close after more than 120 seconds and reached roughly 212 MiB.
3. Adding the last proved Markov quotient retained only maximum fuel for equal
   terminal position, road footprint and day score; opponent spot masks were
   removed because they cannot affect our objective or future state. This still
   did not close after 60 seconds and grew from roughly 251 MiB at 30 seconds to
   431 MiB at 60 seconds.

These numbers describe research-state growth on the local machine, not UDON
runtime or competition performance. The third attempt growing faster in memory
after exact quotienting shows that enumeration time had previously hidden the
underlying game-state expansion.

## Decision

The next available reductions are beam search, action truncation, shorter
horizon, synthetic footprints or a dominance assumption that is not proved in
the two-sided congestion game. Each would weaken the registered exact/public
semantics, so none is admissible. The probe source was removed completely;
`research/probes/multi_patrol_oracle.cpp` was restored content-identical to
`HEAD` (`712c00fa08a9f7e6ed608c5963ba645889745e0e`). Production source was never
changed.

Experiment 172 may reopen only with a new mathematically exact quotient/proof
representation or a suitable external proof host. It must not reopen by
reducing the declared rules or by tuning to seed `1720000`.
