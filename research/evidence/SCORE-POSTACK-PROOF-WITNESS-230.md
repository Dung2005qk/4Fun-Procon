# SCORE-POSTACK-PROOF-WITNESS-230

Date: 2026-08-26.

## Gap

`prove_remaining_horizon` enumerates a persistent frozen-scenario route
portfolio, computes an exact best score inside that portfolio, records and
persists the proof, but discards the argmax route sequence. No decision consumer
uses the completed work. Experiment 229 showed that broad undirected cache
generation is not enough: 3021 added contingencies changed no activated pair.
The next intervention must use the proof objective itself rather than adding
more unranked routes.

## Candidate

Under a default-off experiment flag, carry the current exact plan path through
the existing proof DFS. When and only when a proof completes with a feasible
argmax, append its first plan to the existing next-day contingency cache, tagged
with the proof scenario. Deduplicate against every existing cache plan. On maps
without roads, retain the complete exact path as a certified suffix; on traffic
maps, retain only the first plan because later predicted road states are not
authoritative. W0 must still exact-evaluate the first plan against the next
authoritative state and all normal candidates remain available.

The proof's `bestScore` and `upperBound` remain scoped to
`remaining-horizon-persistent-frozen-scenario-route-portfolios-v1`. They must
not prune, dominate, or certify the global action space, and they must not be
used to replace a candidate without normal exact validation and official
lexicographic comparison.

## Frozen gate

Parent is HEAD `e8bf766` plus accepted 227/228 working source, production-source
diff SHA256
`97B9D096B1BFFA10432EB4EE476C651531D0BA389FE441DFDE2658C7D34720CD`.
The frozen manifest is
`research/holdouts/SCORE-POSTACK-PROOF-WITNESS-230.csv`, SHA256
`741944ED1C48FFF5BEBB3E3EF7D33BA82E77959136CC25D5592F97639544D8B3`.
The resumable runner is frozen at SHA256
`9A30964B3D76637584E23654EC1FC9332FB9859422D7595D7B81176FBEB7B1B0`;
the paired official-score summarizer is frozen at SHA256
`C89CFBBED7CC2A80D0D986BC8E2583D6B74D806C26577B23CC9E01F5355D711A`.
The VM source archive is frozen at SHA256
`240F9F6B1008E7916345318C95531BA56655C66750DF0FBF255203682FECB733`;
the Linux tournament binary built from it is frozen at SHA256
`DC1D79F7B80D50989E0155348D3C04E90BF7179AF2720DB4DA6CF94F536479BB`.
The Linux unit suite passed before development began.
The hash-verifying sequential VM launcher is frozen at SHA256
`04C9FDFF1BD7553719ED4D6F8DE9C7F5126064E73C9C10CF5CA8A58C68282FA6`.

Development is 60 fresh paired cases; holdout is 108 fresh paired cases across
four map tiers, low/default/high fuel and fixed/native roles. Both sides use a
3375-ms canonical solve plus at most 1600 ms post-ACK in 100-ms slices and differ
only in proof-witness adoption. Promotion requires zero invalid/emergency or
closed-loop/refinement failure, zero tier-1/tier-2 regression, no loss of proof
completion/records, actual proof-witness production and next-day reuse, clear
paired global benefit with bounded nonsystematic tier-3 downside, and unchanged
5000-ms canonical checkpoint behavior. The sealed holdout opens once only if all
development gates pass.

## Result

Development completed 60 causal-off plus 60 causal-on cases on the frozen Spot
VM binary. Both logs have `run_complete`, stderr remained empty and every
frozen hash matched. Log SHA256 values are:

- off: `3C6A66B0E8C1B155F2FCA87A1DFBCC612D204D123C2B4F8933F169978F3F1C17`;
- on: `1B9A2416DF9BF436FDAB0FB782DBDA630C72251E6A0F34B719E3C11FD10FBE2A`.

Candidate versus parent was `5/45/10`; all 15 first differences were tier 3
and aggregate score delta was `0/0/-17`. Both sides had zero invalid,
emergency, checkpoint closed-loop, midday and terminal-sparse failures. Proof
coverage was equal in aggregate at 49 completed proofs and 465 proof records,
although one case gained and one case lost proof coverage.

The mechanism was live: it produced 47 proof-witness contingencies in 20 cases;
47 became eligible on the next day, 21 were exact-valid and reused, and 26 were
rejected against the authoritative state. Fourteen cases reused at least one
witness. However, only two score differences coincided with witness production
and reuse, and both were losses: `-2` servings on seed `5321000` and `-22`
servings on seed `5323023`. There was no causal gain. The other 13 score
differences occurred without witness production and are not attributable to
the mechanism.

The preregistered global-benefit gate therefore fails. SCORE-230 is rejected
before holdout; the sealed 108-case split remains unopened. Its production and
harness changes must be removed without touching accepted experiments 227/228.
The result does not authorize deleting strong proof: it demonstrates that
injecting its frozen-scenario argmax directly into the next canonical W0 search
is not a safe consumer under authoritative-state drift.
