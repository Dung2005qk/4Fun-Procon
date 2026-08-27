# SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229

Date: 2026-08-26.

## Gap

Experiment 228 proved that post-ACK response artifacts survive into the next
day, but the only observed tail still had no valid full-daily cached plan. The
existing precompute repeats the same narrow search from the first scenario on
each idle slice and the BTC loop stops after the first zero-add slice. This can
spend the available background window recomputing duplicate narrow plans while
never reaching a broader, independently useful next-day candidate.

## Candidate

Preserve the existing narrow precompute as the exact first phase. Track its
scenario progress across idle slices instead of restarting from scenario zero.
After every narrow scenario has produced or exactly failed its parent attempt,
use only time that cannot delay the parent's background proof phase for a nested
production-width portfolio. Same-scenario cached plans are supplied in canonical
insertion order while the existing production-width `maximumSeedPlans = 2` cap
remains unchanged. Retain all prior cache entries and append only unique
exact-valid candidates. The canonical main solve, checkpoint, comparator,
response deadline, simulator and validator are unchanged.

The live idle loop continues while the stateful narrow phase reports unfinished
work. Revision v2 permits diversification in the residual of the slice that
finishes narrow work and, only if that slice added a new narrow plan, in the one
confirmation slice the parent would run before observing zero additions. It then
hands control to `prove_remaining_horizon` even if the diversified slice added
plans. An empty narrow result caused by an expired deadline does not advance the
scenario cursor. This preserves or advances the parent proof start instead of
spending its budget. Work still stops immediately when the engine budget is
exhausted or the next authoritative state arrives. Flag-off behavior is the
direct parent.

## Frozen gate

Frozen manifest:
`research/holdouts/SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229.csv`.
SHA256:
`9D28F7FE1088AC6D46CB11C075D26663A7387C4497E408A6E816946C7C65CE33`.

Parent production-source diff hash:
`d56b9d3c4cd8894137a328ee42765902792cc2b6` over HEAD `e8bf766`.

Development is 60 fresh paired cases and the one-time holdout is 108 fresh
paired cases across four map tiers, low/default/high fuel and fixed/native
roles. Both sides use a 3375-ms canonical solve plus 100-ms idle slices bounded
to 1600 ms; only the candidate enables diversification. Promotion requires zero
invalid/emergency result, zero tier-1/tier-2 regression, clear paired global
benefit at the first differing official tier, bounded and non-systematic tier-3
downside, target-conditioned evidence that diversified candidates survive exact
validation and affect the selected trajectory, and no loss of the parent proof
phase. The harness must run the same precompute-to-proof state machine as HTTP
and report complete/incomplete proof records on both sides. Any holdout result
is consumed once opened. m-4195 remains attribution-only and cannot promote or
tune this candidate.

## Result

Revision v1 was rejected before holdout by source audit: it could keep broad
precompute pending until the entire background budget was consumed, starving
the existing strong-proof phase, and its harness never called that phase. Its
partial VM development log stopped after 35 parent-only cases and is preserved
as invalid operational evidence only at
`SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229-v1-invalid-partial-off.log`,
SHA256 `A53D6BDC9F57B68C040DFF49D1FE85090EDC3DA52CC50A44582A2B96BDFE47A2`.
The candidate side never started and no sealed case was opened. Revision v2's
local structural smoke tied `6/30/154`; both sides made six cache
calls, completed two strong proofs and retained five proof records, while v2
added 127 diversified exact-valid contingencies versus zero. Local elapsed is
not performance evidence.

The first Linux v2 development launch was invalidated by a second pre-score
source audit after 16 causal-off results and before causal-on started. The
cursor advanced a narrow scenario even when viability/generation/master had
hit the slice deadline; this could retain an exact-valid partial plan yet skip
the complete narrow attempt that the parent could make in its confirmation
slice. The corrected implementation retains the partial plan but advances the
cursor only after a non-interrupted narrow attempt. It still grants no extra
slice: zero-add transfers immediately to strong proof, while a successful
parent-equivalent confirmation retries the unfinished scenario. The invalid
operational archive is
`SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229-v2-preaudit-invalid.tar.gz`,
SHA256 `ECA3A8BB3A6520D46EDAF04E1F800D7BC27FCD0E55D4FA98FF0D4394373F0F63`;
its scores have no decision authority. The corrected Linux binary is frozen at
SHA256 `5F1C818C22BECC3C29C1A9F8F264D8532F5F0688F648463215DC73D36829C654`,
with runner SHA256
`96E822D153211BE2A552A4305092AED156E5617537CC74FACB0CCD44060395BA`.
The resumable launcher verifies binary, runner and manifest hashes and is frozen
at SHA256 `5E2985179AFC8E8144D76EAD156E70D929FA3968403EB9F5EA88B9FEC1DCAEAD`.
Corrected 60+60 development completed from a fresh log directory with empty
stderr and the frozen binary, runner, manifest and launcher hashes unchanged.
The sealed holdout was never opened.
Before development aggregation, the official lexicographic summarizer and all
its score/proof/activation strata were frozen at SHA256
`35C79E33D26527252BAFDE54EB047022BBADC799B3092CFE2FD59BB4B04ABA59`.
The corrected Windows build passes the full unit executable and independent
replay checks on m-4043, m-4044 and m-4045 at exact scores `6/42/127`,
`6/60/364` and `6/60/144`, respectively.

Frozen-summarizer development result: candidate versus parent was `5/49/6`,
with all 11 first differences at tier 3 and aggregate delta `0/0/-68`. The
largest losses were `-44` and `-22`; low fuel was `1/20/4`, net `-73`, and
native roles were `2/16/3`, net `-64`. Both sides had zero invalid, emergency,
checkpoint-closed-loop, midday and terminal-sparse failures. Proof coverage was
exactly equal: completed proofs `38` versus `38`, strong-proof records `458`
versus `458`.

The causal gate failed more decisively than the aggregate score gate. The
candidate created `3021` diversified exact-valid contingencies across 22
activated cases, versus zero in the parent, but all 22 activated pairs tied.
Every one of the 11 score differences occurred in a pair with zero diversified
contingencies. Those differences therefore cannot be attributed to the proposed
mechanism and are timed-search variation, not evidence of benefit or regression
caused by diversification. Raw authoritative development logs:

- causal-off SHA256
  `B7369E35B51CCC722DAA896D7337B3F26A7F62F9680845D30396A997461A22CB`;
- causal-on SHA256
  `0D2DC5A58045F77040C9D41D072518805A8300BF21C2BA193B182B58E55B26E5`.

Verdict: rejected before holdout. Generating a much larger cache under the
existing virtual-parent anchor did not change any selected trajectory in an
activated development case, so there is no global benefit to justify production
complexity or consuming the sealed split. Revision-v2 production changes are
removed while accepted 227 diagnostics isolation and 228 lifecycle parity are
retained. Reopen only with a new invariant-derived consumer or generator whose
activation is linked to next-day adoption on fresh development seeds; the
unopened 229 holdout remains unused and cannot be repurposed automatically.

## Scheduling invariant

For flag-off, the precompute body is the parent body verbatim and the exported
pending predicate is exactly `added > 0`, matching the old HTTP loop. For
flag-on, a completed narrow scenario is never recomputed; therefore reaching an
unfinished scenario cannot occur later within a slice than the parent, which
must first replay every earlier scenario. A deadline-interrupted attempt retains
any exact-valid result but leaves the cursor on that scenario. A zero-add slice
stops precompute exactly like the parent. If the slice that completes narrow
work adds a plan, exactly one confirmation slice is permitted; that slice always
clears pending regardless of broad additions. Consequently diversification can
use only residual work plus that parent-equivalent confirmation slice, and the
first strong-proof call is not delayed. The remaining empirical risk is not
scheduling but proof complexity: extra exact-valid cache seeds may enlarge the
proof root portfolio, so completed proofs and strong-proof records are explicit
promotion gates rather than assumed monotonic.

## Scope boundary

Both the HTTP runtime and this harness precompute from the acknowledged
certificate's virtual-parent transition. Protected wait, midday and public
refinement may later submit a richer transition with a different terminal
state. Experiment 229 deliberately does not change that anchor, and every
cached plan is revalidated against the next authoritative state before reuse.
This keeps the 229 comparison faithful and safe, but a rejection cannot establish
a ceiling for the entire post-ACK axis. Only if development attribution shows
that diversification is generation-rich while next-day cache rejection is
systematically caused by virtual-parent versus acknowledged-transition drift
may a separate fresh-seed experiment anchor background generation to the actual
exact-validated ACKed transition. Such a successor must preserve the certified
virtual parent separately and cannot reuse 229's holdout.
