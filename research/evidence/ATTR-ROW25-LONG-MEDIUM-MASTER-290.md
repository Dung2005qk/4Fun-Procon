# ATTR-ROW25-LONG-MEDIUM-MASTER-290

Date: 2026-09-02

This is read-only attribution over the already-consumed qualifying BTC row 25
replay `m-9515`.  It does not change source, apply an alternate plan, open a
holdout, or authorize promotion.

## Frozen evidence

- Canonical production commit: `ab3d6999d60a7ca290a366bf235154178c6d721f`
- Branch HEAD at attribution: `233a93cb04675e959feb50a2e5e73df6d05a5e8b`
- Canonical Windows binary SHA256:
  `5FA10472D46E1136E3A2CFCD87FF26DA97C64575E42E2A385F1134CB44826F01`
- Row-25 replay SHA256:
  `7CAD9466EA007F7C9EBE595A0DC2847EA0461508FAFDC931FE314AD07BA1F243`
- Comparison replay `m-6134` SHA256:
  `9A0AADF8A2DCA35C3C3E6B77FD337A3352085D9880A81BE72010DDE9B26D6BB5`

Row 25 is hard difficulty with exactly three BTC bots, nine days, a 24x24
map, 96 steps/day, public 10000 ms, seven agents, 24 Spots, eight brands and
medium fuel 192.  Production selected one Tanker at agent 1 and finished
`8/72/389`, behind `8/72/424` and `8/72/392` and ahead of `8/72/351`.
All 9 actions were HTTP 200 and exact-valid, all 8 transitions reconciled,
stderr was empty, and maximum response time was 8735 ms.

## Existing production telemetry

The canonical checkpoint scores before accepted continuation were 41, 81, 123,
163, 206, 249, 291, 334 and 374 servings.  On days 1--9 respectively, the
same-day portfolio upper exceeded the selected lower by 1, 4, 3, 3, 5, 8, 2,
6 and 4 servings.  `portfolioSearchComplete=false`,
`masterDiagnostics.searchComplete=false`, `deadlineReached=true` and
`independentDeadlineReached=true` on every day.  Master cap cuts were
7, 5, 4, 5, 9, 13, 11, 14 and 8.

This signature matches the already-closed `m-6134` medium-fuel long-horizon
counterexample: its nine checkpoint days were also portfolio-incomplete,
master-incomplete and deadline-bound, with same-day upper gaps 3, 4, 5, 5, 2,
8, 7, 3 and 4 servings.  The role scan for row 25 enumerated seven distinct
one-Tanker identities plus all-Patrol; there was no duplicate mask.  The live
one-Tanker identity was in the top rollout tie, so neither role class nor an
enumeration bug explains the 35-serving deficit to the winner.

## Verdict

Closed as a fresh instance of the known long-horizon medium-fuel
candidate-supply/state-certificate boundary.  Experiments 269--281 and
274--277 already traced this class: unrestricted sparse supply exists, but
nonterminal gains change terminal/fuel/traffic state; exact-road and weaker
traffic relations do not certify the future; additive canonical-master and
protected-continuation successors did not pass their frozen gates.  Raising
caps, moving work before the 5000-ms checkpoint, repeating sparse enumeration,
or routing specifically for this replay would repeat closed research or break
the canonical contract.

No SCORE successor is authorized.  Reopen only with a genuinely new executable
whole-future public-state dominance proof, authoritative opponent trajectory,
or an operation-equivalent capability improvement that produces such a proof
on fresh evidence.

Functionality-preservation answers: (1) no designed functionality is removed,
disabled, deferred or reduced; (2) nothing is deleted, so no active-equivalent
claim is required.
