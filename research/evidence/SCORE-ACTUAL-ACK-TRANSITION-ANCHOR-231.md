# SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231

Date: 2026-08-27.

## Gap

The production HTTP path may replace the canonical decision with an
exact-validated protected refinement or public continuation before sending it.
After the server accepts those bytes, `MatchSession::acknowledge_submitted`
still records the original `pendingDecision_`. Consequently the response
profile remains attached to the correct protected checkpoint, but background
contingency/proof work starts from the checkpoint's score and final fuel rather
than from the richer transition actually accepted by BTC.

This is a real production anchor mismatch, but it is not the direct cause of
the 26/47 rejected proof witnesses in experiment 230. Protected nonterminal
refinement preserves the road footprint and terminal cells and may only
increase patrol fuel and official score. A route feasible from the poorer
checkpoint remains feasible from the richer accepted transition; the observed
next-day witness invalidation is therefore attributable to authoritative
traffic drift, not to the richer ACK state. The mismatch instead leaves
certified fuel/score gains unavailable to post-ACK generation and proof.

## Candidate

Keep the original response profile and checkpoint candidate as the submission
certificate. Under a default-off research switch, after exact validation and
the unchanged protected transition/score dominance certificate, construct a
separate acknowledged-transition anchor from the plan and simulation actually
accepted. Background precompute and strong-proof search use this exact anchor.
The original response comparator state is not rewritten. Any certified suffix
whose exact score was rooted at the poorer checkpoint is downgraded to an
ordinary seed and must revalidate before use.

Production integration, if accepted, must pass the exact sent plan and
simulation to the ACK overload. Rejected/fallback submissions continue through
the existing external-transition path. Main solve, protected checkpoint,
public continuation, server wire bytes, official comparator, simulator,
validator and deadline policy remain unchanged.

## Frozen gate

Parent: HEAD `e8bf766` plus accepted 227/228 production-source diff SHA256
`c7c173d456d905dfe7c09564a7eeb3aba50f88cc` after complete removal of rejected
230 source.

Frozen manifest:
`research/holdouts/SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231.csv`, SHA256
`F7E6DB5791C2FBEB7D9F60B29525004712CB77F9C6E64D5CB8A42D975A644B04`.

Frozen runner SHA256:
`5F982006F410D3AE888C74633BB5A7F0BDBD7D4ACB59B1E44120E8453E79894F`.

Frozen summarizer SHA256:
`92DAD659DB1818C260B4E942CD128F85AC78AECCCA726D12702DD90CD57CEDC7`.

Frozen VM source archive SHA256:
`CC5237AC45F11748E7691A173215A842B0F9B8D39B6E5FCAB0123438026A3E15`.

Frozen Linux tournament binary SHA256:
`27C72E463DFBA48E12212C4964805182A59D73AE5B8AD875415790423B41D4D5`.

Hash-verifying sequential VM launcher SHA256:
`3F0DEBF0F4F67DBC3FA2A7C369FADD17B671AC2C921B136B17CBA322EC592863`.
Windows unit tests passed before the frozen archive was created. Linux build
completed in `/home/LMC/udon231-0827`; development runs causal-off then
causal-on strictly sequentially on the Spot VM.

Development uses 60 fresh paired cases and the one-time sealed holdout uses
108 different fresh paired cases over easy/medium/hard/very-hard, low/default/
high fuel, fixed/native roles and six traffic families. Both sides run the same
3375-ms canonical solve and at most 1600 ms of post-ACK work in 100-ms slices.
The only causal switch is acknowledged-transition anchoring.

Development must show zero invalid/emergency/certificate failure, no tier-1 or
tier-2 loss, preserved proof coverage, at least one activated score gain and
clear global benefit with bounded nonsystematic tier-3 downside before holdout
may open. Mere anchor activation, larger caches or more proofs without a paired
score effect is insufficient. The sealed split may be opened once only.

## Result

Revision v1 is invalid operational evidence. The causal-off side completed
60/60, then causal-on stopped after 9/60 while starting development seed
`5341001` with `acknowledged transition does not dominate the certified
checkpoint`. The launcher and child exited, runner stderr remained empty and
all frozen hashes matched. Preserved v1 log SHA256 values are:

- causal-off:
  `DD05F483504063AE0464A173AC9EA15F552C85247DE5C161CF42B58A69540866`;
- causal-on partial:
  `FE8637D86727571873B4E21791B9EAFB5A07145E397FA37545A1327AC87356B4`.

Source attribution found a domain error in v1 rather than an unsafe accepted
transition. The historical closed loop and production BTC path solve the
certified decision on `virtualState/virtualLedger`, may revalidate and refine
it on `checkpointState/checkpointLedger`, then may replay/continue it on the
authoritative `state/ledger`. V1 exact-evaluated the applied plan on the latter
domain but compared its raw `DayScore` against the certified simulation from
the former domain. `MatchSession` also retained the pending virtual state and
ledger for background work even when supplied the actual plan/simulation.
Finally, v1 incorrectly required terminal-transition equality on the final
day, where only exact validity and official lexicographic non-regression are
authoritative.

Revision v2 is registered before its source change. The ACK overload receives
both certified and applied domains. It exact-evaluates and dual-validates the
plan on the applied state/ledger, checks nonterminal transition dominance plus
post-day ledger dominance from certified to applied, and uses official
lexicographic non-regression on the final day. Background work stores the
applied state/ledger and exact anchored candidate while the response ledger
continues to retain the certified checkpoint. Any suffix certificate rooted
in a semantically different transition is downgraded to an ordinary plan seed.
The unchanged frozen holdout remains sealed. Both 60-case development sides
must restart under one new v2 binary; no v1 score prefix has authority.

Revision-v2 frozen artifacts:

- source archive SHA256:
  `8C207E1CE5CE30A5B63B8E6B18F1B2FBD69BA5D8F7EDC9B80A1EE04981510CA6`;
- Linux tournament binary SHA256:
  `4E1865493CE996CC5C52797F585680D4984C17F2C211E05061AB542A98464815`;
- hash-verifying sequential launcher SHA256:
  `98A0B32E175B3E2FEB63F22FCB7A0E374588B9CAA2CD5B9DD0D67A81439A3F0C`.

Windows unit tests pass. The exact v1 failure seed `5341001` completes on both
Windows and the frozen Linux binary under v2 with zero invalid/emergency or
certificate failure. These reruns are semantic falsification only and have no
score-promotion authority.

## Revision-v2 development verdict

The frozen VM run completed both sides `60/60 + 60/60`; runner stderr was empty
and every frozen binary, manifest, runner, summarizer and launcher hash matched.
The exact result was:

- paired official score: `7/47/6`;
- first differences: thirteen at tier 3, none at tier 1 or tier 2;
- aggregate score delta: `0/0/+1`;
- safety failures: zero on both sides;
- activated cases: `56/60`;
- differences with activation: `12/13`;
- high-fuel stratum: `2/11/3`, net `-13`, largest loss `-15`;
- completed proofs: `55 -> 54`;
- strong-proof records: `468 -> 467`.

The candidate therefore fails two preregistered requirements: proof coverage is
not preserved, and the nearly neutral aggregate does not establish clear global
benefit with bounded nonsystematic downside. The one-time 108-case holdout stays
sealed and unopened. Revision v2 is rejected and its source, runtime, harness and
test changes are removed.

Preserved complete development logs:

- causal-off SHA256:
  `6B1A833698CA929215600F3D36358C314FEEB0FF7DCF60FCEE23AB39AADA6826`;
- causal-on SHA256:
  `8DE0CDE21721C47192943CE77338A03D9C790A4C1EAC09AF6707A1A95E90A557`.

This result does not reopen experiment 230. A future post-ACK successor requires
a new sound consumer and fresh unopened seeds, must preserve proof coverage and
must prevent systematic fuel-regime downside; directly re-enabling the rejected
ACK anchor or proof-witness injection is not admissible.
